#!/usr/bin/env python3
"""RenderMax CLI: independent Trace/Capture runtime; optional instance connection. Python stdlib only."""
import argparse
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import urllib.error
import urllib.parse
import urllib.request

EXIT_CODES = {-32600: 2, -32601: 2, -32602: 2, -32004: 2,
              -32001: 3, -32003: 4, -32008: 5, -32007: 6, -32009: 7}
TRACE = {
    'info': 'getSystemTraceCapabilities', 'analyze': 'analyzeSystemTrace',
    'events': 'getSystemTraceEventInventory', 'series': 'querySystemTraceSeries',
    'tracks': 'querySystemTraceTracks', 'cpu': 'analyzeSystemTraceCpu',
    'cpu-utilization': 'getSystemTraceCpuUtilization', 'thread-times': 'getSystemTraceThreadTimes',
    'scheduling': 'analyzeSystemTraceScheduling', 'wakeups': 'analyzeSystemTraceWakeups',
    'cpu-frequency': 'analyzeSystemTraceCpuFrequency', 'cpu-idle': 'analyzeSystemTraceCpuIdle',
    'gpu': 'analyzeSystemTraceGpu', 'gpu-utilization': 'getSystemTraceGpuUtilization',
    'gpu-batches': 'getSystemTraceGpuBatches', 'gpu-queue': 'analyzeSystemTraceGpuQueue',
    'gpu-waits': 'analyzeSystemTraceGpuWaits', 'gpu-frequency': 'analyzeSystemTraceGpuFrequency',
    'bandwidth': 'analyzeSystemTraceBandwidth', 'power': 'analyzeSystemTracePower',
    'thermal': 'analyzeSystemTraceThermal', 'power-states': 'analyzeSystemTracePowerStates',
    'frames': 'getSystemTraceFrames', 'frame': 'analyzeSystemTraceFrame',
    'jank': 'analyzeSystemTraceJank', 'compare': 'compareSystemTraceWindows',
    'report': 'buildSystemTraceReport', 'save': 'saveSystemTrace',
    'record': 'captureSystemTrace', 'correlate': 'correlateSystemTrace',
}
SESSION = {'targets': 'listCaptureTargets', 'check-android': 'checkAndroidLaunchReadiness',
           'launch': 'launchApplication', 'launch-windows': 'launchWindowsApplication',
           'launch-android': 'launchRemoteApplication', 'connect-replay': 'connectRemoteReplay',
           'wait-target': 'waitForLiveTarget', 'wait-capture': 'waitForCapture', 'close': 'closeSession'}
CAPTURE = {'open': 'openCapture', 'frame': 'captureFrame', 'remote-frame': 'triggerRemoteCapture'}


class CliError(Exception):
    def __init__(self, message, code=1, data=None):
        super().__init__(message)
        self.code, self.data = code, data


def strict_json(text):
    def reject(value):
        raise ValueError('Non-finite JSON number: ' + value)
    return json.loads(text, parse_constant=reject)


def validate(schema, value, path='$'):
    """Validate the tool schema subset before crossing either execution boundary."""
    kinds = {'object': dict, 'array': list, 'string': str, 'boolean': bool}
    kind = schema.get('type')
    valid = True
    if kind in kinds:
        valid = isinstance(value, kinds[kind])
    elif kind in ('integer', 'number'):
        valid = (isinstance(value, (int, float)) and not isinstance(value, bool)
                 and math.isfinite(value) and (kind != 'integer' or int(value) == value))
    elif kind == 'null':
        valid = value is None
    if not valid:
        raise CliError(path + ': expected ' + kind, 2)
    if 'enum' in schema and value not in schema['enum']:
        raise CliError(path + ': not in enum', 2)
    if isinstance(value, dict):
        props = schema.get('properties', {})
        for key in schema.get('required', []):
            if key not in value:
                raise CliError(path + '.' + key + ': required', 2)
        for key, child in value.items():
            if key in props:
                validate(props[key], child, path + '.' + key)
            elif schema.get('additionalProperties') is False:
                raise CliError(path + '.' + key + ': unknown argument', 2)
    elif isinstance(value, list):
        for index, child in enumerate(value):
            validate(schema.get('items', {}), child, path + '[' + str(index) + ']')
    elif isinstance(value, str):
        if len(value) < schema.get('minLength', 0):
            raise CliError(path + ': string too short', 2)
        if 'pattern' in schema:
            import re
            if not re.search(schema['pattern'], value):
                raise CliError(path + ': pattern mismatch', 2)
    elif isinstance(value, (int, float)) and not isinstance(value, bool):
        if value < schema.get('minimum', -math.inf) or value > schema.get('maximum', math.inf):
            raise CliError(path + ': out of range', 2)


class Client:
    def __init__(self, endpoint, timeout):
        parsed = urllib.parse.urlparse(endpoint)
        if parsed.scheme not in ('http', 'https') or not parsed.hostname or parsed.username or parsed.password:
            raise CliError('Expected an http(s) endpoint without embedded credentials', 2)
        self.endpoint, self.timeout = endpoint, timeout
        self.session, self.ident, self.protocol = None, 0, '2025-11-25'
        # Do not leak local MCP calls through environment proxy settings.
        self.opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))

    def request(self, method, params=None, notification=False):
        self.ident += 1
        payload = {'jsonrpc': '2.0', 'method': method}
        if not notification:
            payload['id'] = self.ident
        if params is not None:
            payload['params'] = params
        headers = {'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream'}
        if self.session:
            headers['Mcp-Session-Id'] = self.session
            headers['MCP-Protocol-Version'] = self.protocol
        req = urllib.request.Request(self.endpoint, json.dumps(payload).encode('utf-8'), headers)
        try:
            with self.opener.open(req, timeout=self.timeout) as response:
                self.session = response.headers.get('Mcp-Session-Id', self.session)
                body = response.read().decode('utf-8-sig')
                content_type = response.headers.get('Content-Type', '')
        except (TimeoutError, __import__('socket').timeout) as exc:
            raise CliError('Request timed out; a submitted side effect may still complete', 5) from exc
        except urllib.error.HTTPError as exc:
            raise CliError('HTTP %s: %s (no automatic retry)' % (exc.code, exc.read().decode('utf-8', 'replace'))) from exc
        except urllib.error.URLError as exc:
            raise CliError('Cannot connect to RenderMax instance: ' + str(exc.reason), 4) from exc
        if notification:
            return {}
        if 'text/event-stream' in content_type:
            messages = []
            data = []
            for line in body.splitlines() + ['']:
                if line.startswith('data:'):
                    data.append(line[5:].lstrip())
                elif not line and data:
                    messages.append(strict_json('\n'.join(data)))
                    data = []
            result = next((v for v in messages if v.get('id') == self.ident), None)
        else:
            result = strict_json(body)
        if not isinstance(result, dict) or result.get('id') != self.ident:
            raise CliError('Invalid JSON-RPC response or mismatched request id')
        if 'error' in result:
            err = result['error']
            raise CliError(err.get('message', 'RPC error'), EXIT_CODES.get(err.get('code'), 1), err)
        return result.get('result', {})

    def initialize(self):
        result = self.request('initialize', {'protocolVersion': self.protocol,
            'capabilities': {}, 'clientInfo': {'name': 'rendermax-cli', 'version': '1.0'}})
        self.protocol = result['protocolVersion']
        self.request('notifications/initialized', notification=True)

    def close(self):
        if not self.session:
            return
        req = urllib.request.Request(self.endpoint, method='DELETE', headers={
            'Mcp-Session-Id': self.session, 'MCP-Protocol-Version': self.protocol})
        try:
            with self.opener.open(req, timeout=min(self.timeout, 2)):
                pass
        except (OSError, urllib.error.URLError):
            pass  # Transport session cleanup must not mask the operation result.
        self.session = None

    def tools(self):
        tools, cursor, seen = [], None, set()
        while True:
            result = self.request('tools/list', {'cursor': cursor} if cursor else {})
            tools.extend(result.get('tools', []))
            cursor = result.get('nextCursor')
            if not cursor:
                return tools
            if cursor in seen:
                raise CliError('Repeated tools/list pagination cursor')
            seen.add(cursor)

    def call(self, name, args):
        result = self.request('tools/call', {'name': name, 'arguments': args})
        content = result.get('structuredContent')
        if content is None:
            content = {'content': result.get('content', [])}
        if result.get('isError'):
            raise CliError(content.get('message', 'Tool failed'), EXIT_CODES.get(content.get('code'), 1), content)
        return content


class Headless:
    """Owns an independent replay/target runtime. Private files, no network listener."""
    def __init__(self, executable, timeout, allow_sensitive=False):
        self.executable, self.timeout = str(executable), timeout
        self.allow_sensitive = allow_sensitive
        if not Path(executable).is_file():
            raise CliError('Headless runtime not found: ' + str(executable), 7)

    def run(self, request):
        request['timeoutMs'] = int(self.timeout * 1000)
        request['allowSensitiveData'] = self.allow_sensitive
        with tempfile.TemporaryDirectory(prefix='rendermax-headless-') as directory:
            source, target = Path(directory) / 'request.json', Path(directory) / 'result.json'
            source.write_text(json.dumps(request, ensure_ascii=False), encoding='utf-8')
            try:
                result = subprocess.run([self.executable, '--cli-headless', str(source), str(target)],
                    capture_output=True, timeout=self.timeout, encoding='utf-8', errors='replace')
            except subprocess.TimeoutExpired as exc:
                raise CliError('Headless runtime timed out and was terminated; submitted device operations may still complete', 5) from exc
            if result.stderr:
                print(result.stderr.rstrip(), file=sys.stderr)
            if result.returncode or not target.is_file():
                raise CliError('Headless runtime failed (exit %s): %s' % (result.returncode, result.stdout[-1000:]), 4)
            return strict_json(target.read_text(encoding='utf-8'))

    def tools(self):
        return self.run({'operation': 'tools'})['tools']

    def batch(self, steps):
        return self.run({'steps': steps})

    def call(self, name, args):
        result = self.batch([{'name': name, 'arguments': args}])
        if not result.get('results'):
            raise CliError('Headless runtime returned no operation result')
        record = result['results'][-1]
        if not record['ok']:
            err = record['error']
            raise CliError(err.get('message', 'Tool failed'), EXIT_CODES.get(err.get('code'), 1), err)
        return record['result']


class Offline:
    def __init__(self, executable, timeout):
        self.executable, self.timeout = str(executable), timeout
        if not Path(executable).is_file():
            raise CliError('Offline backend not found: %s; build util/cli or use --backend instance' % executable, 7)

    def run(self, arg):
        try:
            result = subprocess.run([self.executable, arg], capture_output=True,
                                    timeout=self.timeout, encoding='utf-8', errors='replace')
        except subprocess.TimeoutExpired as exc:
            raise CliError('Offline analysis timed out; backend terminated', 5) from exc
        if result.stderr:
            print(result.stderr.rstrip(), file=sys.stderr)
        try:
            content = strict_json(result.stdout)
        except ValueError as exc:
            raise CliError('Backend returned invalid JSON: ' + result.stdout[:500]) from exc
        if result.returncode:
            err = content.get('error', {})
            raise CliError(err.get('message', 'Offline backend failed'), result.returncode, err)
        return content

    def tools(self):
        return self.run('--tools')['tools']

    def call(self, name, args):
        with tempfile.TemporaryDirectory(prefix='rendermax-cli-') as directory:
            request = Path(directory) / 'request.json'
            request.write_text(json.dumps({'name': name, 'arguments': args}, ensure_ascii=False), encoding='utf-8')
            return self.run(str(request))


def parser():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--endpoint', default=os.environ.get('RENDERMAX_ENDPOINT', 'http://127.0.0.1:38967/mcp'))
    p.add_argument('--backend', choices=['auto', 'local', 'headless', 'instance'], default='auto')
    p.add_argument('--trace-backend', default=os.environ.get('RENDERMAX_TRACE_BACKEND'))
    p.add_argument('--runtime', default=os.environ.get('RENDERMAX_RUNTIME'), help='Native headless runtime executable')
    p.add_argument('--capture', help='Open this local .rdc before invoking the tool in the independent runtime')
    p.add_argument('--allow-sensitive-data', action='store_true', help='Explicitly permit source/buffer data in the independent runtime')
    p.add_argument('--timeout-ms', type=int, default=120000)
    p.add_argument('--output', help='Write JSON to a file, leaving stdout empty')
    sub = p.add_subparsers(dest='command', required=True)
    sub.add_parser('batch', help='Run ordered tool steps in one independent session').add_argument('file')
    tools = sub.add_parser('tools').add_subparsers(dest='action', required=True)
    tools.add_parser('list')
    tools.add_parser('describe').add_argument('tool')
    def options(cmd, file=False):
        if file:
            cmd.add_argument('file', nargs='?')
        source = cmd.add_mutually_exclusive_group()
        source.add_argument('--args', help='JSON object (tool schema uses camelCase keys)')
        source.add_argument('--args-file', help='UTF-8 JSON object; - reads stdin')
        cmd.add_argument('--set', action='append', default=[], metavar='KEY=JSON',
                         help='Override a tool argument; quote JSON strings, e.g. filePath="C:/x.rdt"')
    call = sub.add_parser('call')
    call.add_argument('tool')
    options(call)
    for domain, aliases in [('trace', TRACE), ('session', SESSION), ('capture', CAPTURE)]:
        commands = sub.add_parser(domain).add_subparsers(dest='action', required=True)
        for alias in aliases:
            options(commands.add_parser(alias), domain == 'trace' and alias != 'record')
    return p


def arguments(ns):
    value = {}
    if ns.args is not None:
        value = strict_json(ns.args)
    elif ns.args_file:
        value = strict_json(sys.stdin.read() if ns.args_file == '-' else Path(ns.args_file).read_text(encoding='utf-8-sig'))
    if not isinstance(value, dict):
        raise CliError('Tool arguments must be a JSON object', 2)
    for entry in ns.set:
        if '=' not in entry:
            raise CliError('--set expects KEY=JSON', 2)
        key, raw = entry.split('=', 1)
        if not key:
            raise CliError('--set key cannot be empty', 2)
        value[key] = strict_json(raw)
    if getattr(ns, 'file', None):
        if 'filePath' in value:
            raise CliError('filePath specified twice', 2)
        value['filePath'] = str(Path(ns.file).resolve())
    return value


def execute(ns):
    if ns.timeout_ms <= 0:
        raise CliError('--timeout-ms must be positive', 2)
    aliases = {'trace': TRACE, 'session': SESSION, 'capture': CAPTURE}
    tool = getattr(ns, 'tool', None)
    if ns.command in aliases:
        tool = aliases[ns.command][ns.action]
    if tool and not tool.startswith('renderdoc_'):
        tool = 'renderdoc_' + tool
    offline_names = {'renderdoc_' + v for k, v in TRACE.items() if k not in ('record', 'correlate')}
    local = ns.backend == 'local' or (ns.backend == 'auto' and tool in offline_names)
    native = ns.backend == 'headless' or (ns.backend == 'auto' and not local)
    if ns.command == 'batch' and ns.backend in ('local', 'instance'):
        raise CliError('batch requires auto or headless backend', 2)
    if native:
        root = Path(__file__).resolve().parents[2]
        executable = ns.runtime or root / 'x64/Development' / ('qrenderdoc.exe' if os.name == 'nt' else 'qrenderdoc')
        runner = Headless(executable, ns.timeout_ms / 1000, ns.allow_sensitive_data)
    elif local:
        executable = ns.trace_backend
        if not executable:
            script = Path(__file__).resolve()
            binary = 'rendermax-trace.exe' if os.name == 'nt' else 'rendermax-trace'
            beside = script.parent / binary
            executable = beside if beside.is_file() else script.parents[2] / 'x64' / 'Development' / binary
        runner = Offline(executable, ns.timeout_ms / 1000)
    else:
        runner = Client(ns.endpoint, ns.timeout_ms / 1000)
        runner.initialize()
    try:
        definitions = runner.tools()
        if ns.command == 'batch':
            if ns.capture:
                raise CliError('Use an explicit openCapture step in batch instead of --capture', 2)
            document = strict_json(Path(ns.file).read_text(encoding='utf-8-sig'))
            steps = document.get('steps') if isinstance(document, dict) else document
            if not isinstance(steps, list) or not steps:
                raise CliError('Batch must be a non-empty array of {name, arguments} steps', 2)
            for step in steps:
                if not isinstance(step, dict) or not isinstance(step.get('name'), str):
                    raise CliError('Invalid batch step', 2)
                if not step['name'].startswith('renderdoc_'):
                    step['name'] = 'renderdoc_' + step['name']
                definition = next((d for d in definitions if d['name'] == step['name']), None)
                if definition is None:
                    raise CliError('Unknown batch tool: ' + step['name'], 2)
                step.setdefault('arguments', {})
                validate(definition.get('inputSchema', {}), step['arguments'])
            result = runner.batch(steps)
            if not result.get('ok'):
                raise CliError('Batch stopped at a failed operation', 1, result)
            return result
        if ns.command == 'tools' and ns.action == 'list':
            return {'backend': 'headless' if native else ('local' if local else 'instance'), 'tools': definitions}
        definition = next((d for d in definitions if d['name'] == tool), None)
        if definition is None:
            raise CliError('Tool not available on selected backend: ' + str(tool), 7)
        if ns.command == 'tools':
            return definition
        args = arguments(ns)
        validate(definition.get('inputSchema', {}), args)
        if ns.capture:
            if not isinstance(runner, Headless):
                raise CliError('--capture requires the independent headless backend', 2)
            loaded = runner.batch([
                {'name': 'renderdoc_openCapture', 'arguments': {'filePath': str(Path(ns.capture).resolve())}},
                {'name': tool, 'arguments': args}])
            records = loaded.get('results', [])
            if not loaded.get('ok'):
                err = records[-1].get('error', {}) if records else {}
                raise CliError(err.get('message', 'Capture open/query failed'),
                               EXIT_CODES.get(err.get('code'), 1), loaded)
            result = records[-1]['result']
        else:
            result = runner.call(tool, args)
        # Native tools can successfully execute but report a business failure in their payload.
        if isinstance(result, dict) and (result.get('success') is False or result.get('ok') is False):
            raise CliError(result.get('error') or result.get('message') or 'Operation did not succeed', 1, result)
        return result
    finally:
        if isinstance(runner, Client):
            runner.close()


def main(argv=None):
    ns = parser().parse_args(argv)
    try:
        value = execute(ns)
        data = json.dumps(value, ensure_ascii=False, indent=2, allow_nan=False) + '\n'
        if ns.output:
            Path(ns.output).write_text(data, encoding='utf-8')
        else:
            sys.stdout.write(data)
        return 0
    except KeyboardInterrupt:
        # No cancellation guarantee for a tool already submitted to the instance.
        print('Interrupted; an instance request already submitted may still complete.', file=sys.stderr)
        return 6
    except (CliError, OSError, ValueError) as exc:
        error = {'message': str(exc), 'exitCode': getattr(exc, 'code', 2)}
        if getattr(exc, 'data', None) is not None:
            error['details'] = exc.data
        print(json.dumps({'error': error}, ensure_ascii=False), file=sys.stderr)
        return error['exitCode']


if __name__ == '__main__':
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    sys.exit(main())
