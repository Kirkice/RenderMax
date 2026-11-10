import contextlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import io
import json
import os
from pathlib import Path
import tempfile
import threading
import unittest

import rendermax_cli as cli


class CLI(unittest.TestCase):
    def test_validation(self):
        schema = {'type': 'object', 'properties': {'n': {'type': 'integer', 'minimum': 1}},
                  'required': ['n'], 'additionalProperties': False}
        cli.validate(schema, {'n': 2})
        for args in ({}, {'n': True}, {'n': 0}, {'n': 1.5}, {'n': 1, 'bad': 2}):
            with self.assertRaises(cli.CliError):
                cli.validate(schema, args)
        with self.assertRaises(ValueError):
            cli.strict_json('{"n":NaN}')

    def test_arguments(self):
        ns = cli.parser().parse_args(['trace', 'gpu', 'x.rdt', '--set', 'beginSec=1.5'])
        args = cli.arguments(ns)
        self.assertEqual(args['beginSec'], 1.5)
        self.assertTrue(Path(args['filePath']).is_absolute())

    def test_local_all_tools(self):
        exe = Path(__file__).resolve().parents[2] / 'x64/Development/rendermax-trace.exe'
        if not exe.is_file():
            self.skipTest('Build offline backend to run integration tests')
        offline = cli.Offline(exe, 30)
        definitions = offline.tools()
        self.assertEqual(len(definitions), 28)
        self.assertEqual({d['name'] for d in definitions},
                         {'renderdoc_' + v for k, v in cli.TRACE.items() if k not in ('record', 'correlate')})
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / '输入 trace.json'
            source.write_text(json.dumps({'traceEvents': [
                {'ph': 'C', 'name': 'GPU Busy', 'pid': 1, 'ts': 0, 'args': {'value': 10}},
                {'ph': 'C', 'name': 'GPU Busy', 'pid': 1, 'ts': 1000000, 'args': {'value': 20}}]}), encoding='utf-8')
            archive = Path(directory) / 'saved.rdt'
            for definition in definitions:
                name = definition['name']
                args = {'filePath': str(source)}
                if name.endswith('analyzeSystemTraceFrame'):
                    args['frameIndex'] = 0
                if name.endswith('saveSystemTrace'):
                    args['destinationPath'] = str(archive)
                cli.validate(definition['inputSchema'], args)
                result = offline.call(name, args)
                self.assertTrue(result.get('ok', result.get('valid')), name)
            source.unlink()
            self.assertTrue(offline.call('renderdoc_getSystemTraceCapabilities', {'filePath': str(archive)})['ok'])
            with self.assertRaises(cli.CliError):
                offline.call('renderdoc_saveSystemTrace', {'filePath': str(archive), 'destinationPath': str(archive)})
            archive.write_bytes(b'not a trace')
            with self.assertRaises(cli.CliError):
                offline.call('renderdoc_analyzeSystemTraceGpu', {'filePath': str(archive)})

    @unittest.skipUnless(os.environ.get('RENDERMAX_TEST_ENDPOINT'), 'Set RENDERMAX_TEST_ENDPOINT for live parity')
    def test_live_parity(self):
        root = Path(__file__).resolve().parents[2]
        exe = os.environ.get('RENDERMAX_TRACE_BACKEND', str(root / 'x64/Development/rendermax-trace.exe'))
        offline = cli.Offline(exe, 30)
        client = cli.Client(os.environ['RENDERMAX_TEST_ENDPOINT'], 30)
        client.initialize()
        try:
            live = {d['name']: d for d in client.tools()}
            with tempfile.TemporaryDirectory() as directory:
                source = Path(directory) / 'parity.json'
                source.write_text(json.dumps({'traceEvents': [
                    {'ph': 'C', 'name': 'Metric', 'pid': 1, 'ts': 0, 'args': {'value': 10}},
                    {'ph': 'C', 'name': 'Metric', 'pid': 1, 'ts': 1000000, 'args': {'value': 20}}]}))
                for definition in offline.tools():
                    name = definition['name']
                    self.assertEqual(definition['inputSchema'], live[name]['inputSchema'])
                    if name.endswith('saveSystemTrace'):
                        continue
                    args = {'filePath': str(source)}
                    if name.endswith('analyzeSystemTraceFrame'):
                        args['frameIndex'] = 0
                    self.assertEqual(offline.call(name, args), client.call(name, args), name)
        finally:
            client.close()

    def test_headless(self):
        root = Path(__file__).resolve().parents[2]
        exe = root / 'x64/Development/qrenderdoc.exe'
        if not exe.is_file():
            self.skipTest('Build native runtime for independent integration tests')
        runtime = cli.Headless(exe, 60)
        definitions = runtime.tools()
        self.assertEqual(len(definitions), 111)
        names = {d['name'] for d in definitions}
        self.assertTrue({'renderdoc_exportFrameAssets', 'renderdoc_exportDrawAssets'} <= names)
        result = runtime.batch([
            {'name': 'renderdoc_getReplayStatus', 'arguments': {}},
            {'name': 'renderdoc_openCapture', 'arguments': {'filePath': 'missing.rdc'}}])
        self.assertFalse(result['ok'])
        self.assertEqual(len(result['results']), 1)  # no follow-up work after failure
        self.assertEqual(result['results'][0]['error']['code'], -32001)
        capture = os.environ.get('RENDERMAX_TEST_CAPTURE')
        if capture:
            result = runtime.batch([
                {'name': 'renderdoc_openCapture', 'arguments': {'filePath': capture}},
                {'name': 'renderdoc_getReplayStatus', 'arguments': {}},
                {'name': 'renderdoc_enumerateCounters', 'arguments': {}},
                {'name': 'renderdoc_getFrameSummary', 'arguments': {}}])
            self.assertTrue(result['ok'], result)
            self.assertTrue(result['results'][1]['result']['capture']['replayAvailable'])

    def test_transport(self):
        observed = []
        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass
            def do_DELETE(self):
                observed.append('delete')
                self.send_response(200)
                self.end_headers()
            def do_POST(self):
                request = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
                observed.append(request['method'])
                if request['method'] != 'initialize':
                    self.assert_session()
                if request['method'] == 'notifications/initialized':
                    self.send_response(202)
                    self.end_headers()
                    return
                if request['method'] == 'initialize':
                    value = {'protocolVersion': '2025-11-25'}
                elif request['method'] == 'tools/list':
                    value = {'tools': [{'name': 'renderdoc_dummy', 'inputSchema': {'type': 'object'}}]}
                else:
                    value = {'structuredContent': {'ok': True, 'null': None}}
                self.send_response(200)
                self.send_header('Mcp-Session-Id', 'test-session')
                self.send_header('Content-Type', 'text/event-stream')
                self.end_headers()
                envelope = json.dumps({'jsonrpc': '2.0', 'id': request['id'], 'result': value})
                self.wfile.write(('event: message\ndata: ' + envelope + '\n\n').encode())
            def assert_session(self):
                if self.headers.get('Mcp-Session-Id') != 'test-session':
                    raise AssertionError('Missing session header')
        server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            client = cli.Client('http://127.0.0.1:%d/mcp' % server.server_port, 5)
            client.initialize()
            self.assertEqual(client.tools()[0]['name'], 'renderdoc_dummy')
            self.assertIsNone(client.call('renderdoc_dummy', {})['null'])
            client.close()
            self.assertEqual(observed, ['initialize', 'notifications/initialized', 'tools/list', 'tools/call', 'delete'])
        finally:
            server.shutdown()
            server.server_close()
            worker.join()

    def test_error_output(self):
        with contextlib.redirect_stderr(io.StringIO()) as err, contextlib.redirect_stdout(io.StringIO()) as out:
            code = cli.main(['--trace-backend', 'missing-backend', 'trace', 'gpu', 'missing.rdt'])
        self.assertEqual(code, 7)
        self.assertEqual(out.getvalue(), '')
        self.assertIn('error', json.loads(err.getvalue()))


if __name__ == '__main__':
    unittest.main()
