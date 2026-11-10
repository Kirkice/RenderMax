// Offline CLI backend. Shares tool schemas and algorithms with the UI/MCP implementation.
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "Code/SystemTrace.h"
#include "Code/AI/MCPSystemTraceAnalysis.h"
#include "Code/AI/SystemTraceLegacyAnalysis.h"
#include "Code/AI/SystemTraceToolContracts.h"

static int Output(const QJsonObject &v, int code = 0)
{
  const QByteArray bytes = QJsonDocument(v).toJson(QJsonDocument::Compact);
  fwrite(bytes.constData(), 1, size_t(bytes.size()), stdout);
  fputc('\n', stdout);
  return code;
}
static int Fail(const QString &message)
{ return Output({{"error", QJsonObject{{"code", -32004}, {"message", message}}}}, 2); }
int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  QJsonArray definitions;
  MCP::RegisterTraceFileTools([&](const char *name, const char *description,
      const QJsonObject &props, const QJsonArray &required, bool readOnly) {
    definitions.append(QJsonObject{{"name", name}, {"description", description},
      {"readOnly", readOnly}, {"inputSchema", QJsonObject{{"type", "object"},
      {"properties", props}, {"required", required}, {"additionalProperties", false}}}});
  });
  if(argc == 2 && QString::fromLocal8Bit(argv[1]) == "--tools")
    return Output({{"tools", definitions}});
  if(argc != 2) return Fail("Expected a JSON request file or --tools");
  QFile request(QString::fromLocal8Bit(argv[1]));
  if(!request.open(QIODevice::ReadOnly)) return Fail(request.errorString());
  QJsonParseError error;
  const QJsonDocument doc = QJsonDocument::fromJson(request.readAll(), &error);
  if(error.error != QJsonParseError::NoError || !doc.isObject()) return Fail("Invalid JSON request");
  const QString tool = doc.object().value("name").toString();
  bool supported = false;
  for(const auto &v : definitions) if(v.toObject().value("name").toString() == tool) supported = true;
  if(!supported) return Fail("Not an offline Trace tool: " + tool);
  const QJsonObject args = doc.object().value("arguments").toObject();
  const QString path = args.value("filePath").toString();
  if(!QFileInfo(path).isAbsolute() || !QFileInfo(path).isFile())
    return Fail("filePath must name an existing absolute file");
  if(tool == "renderdoc_saveSystemTrace")
  {
    const QString destination = args.value("destinationPath").toString();
    if(QFileInfo::exists(destination) && !args.value("overwrite").toBool())
      return Fail("Destination exists; set overwrite true");
    QString why;
    if(!SaveSystemTraceArchive(path, destination, why)) return Fail(why);
    return Output({{"ok", true}, {"filePath", destination}});
  }
  const SystemTrace trace = ImportSystemTraceFile(QFileInfo(path).absoluteFilePath());
  if(!trace.valid) return Fail("Cannot import trace: " + trace.warnings.join("; "));
  double extentBegin, extentEnd;
  trace.TimeExtent(extentBegin, extentEnd);
  const double begin = qMax(extentBegin, args.value("beginSec").toDouble(extentBegin));
  const double end = qMin(extentEnd, args.value("endSec").toDouble(extentEnd));
  if(!std::isfinite(begin) || !std::isfinite(end) || end <= begin)
    return Fail("A non-empty window overlapping the trace is required");
  if(args.contains("compareBeginSec") || args.contains("compareEndSec"))
  {
    const double a = args.value("compareBeginSec").toDouble(begin);
    const double b = args.value("compareEndSec").toDouble(end);
    if(!std::isfinite(a) || !std::isfinite(b) || b <= a || a < extentBegin || b > extentEnd)
      return Fail("Comparison window must be inside the trace extent");
  }
  return Output(tool == "renderdoc_analyzeSystemTrace" ?
    TraceAnalysis::LegacyAnalyze(trace, args, begin, end, extentBegin, extentEnd) :
    TraceAnalysis::Analyze(tool, trace, args, begin, end));
}
