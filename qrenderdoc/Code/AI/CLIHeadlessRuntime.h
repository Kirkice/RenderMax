// Private, file-based CLI transport. No HTTP listener or MCP session/configuration.
// Included only by qrenderdoc.cpp so existing build targets share the same runtime.
#pragma once
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSaveFile>
#include <QTimer>
#include <QDateTime>
#include <thread>
#include "MCPTools.h"
#include "MCPExternalTools.h"
#include "MCPSessionTools.h"
#include "MCPShaderTools.h"
#include "MCPComparisonTools.h"
#include "MCPCompatibilityTools.h"
#include "MCPResourceTools.h"
#include "MCPAssetExport.h"

static int RunHeadlessCLI(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  if(argc != 4)
    return 2;
  const QString requestPath = QString::fromLocal8Bit(argv[2]);
  const QString resultPath = QString::fromLocal8Bit(argv[3]);
  QFile input(requestPath);
  if(!input.open(QIODevice::ReadOnly))
    return 2;
  QJsonParseError parse;
  const QJsonDocument document = QJsonDocument::fromJson(input.readAll(), &parse);
  if(parse.error != QJsonParseError::NoError || !document.isObject())
    return 2;
  const QJsonObject request = document.object();
  GlobalEnvironment environment;
  rdcarray<rdcstr> arguments;
  RENDERDOC_InitialiseReplay(environment, arguments);
  GUIInvoke::init();
  int status = 0;
  {
    PersistentConfig config;
    // Do not load/save UI.config. Sensitive access is an explicit CLI opt-in.
    config.MCP_AllowSensitiveData = request.value(QStringLiteral("allowSensitiveData")).toBool(false);
    CaptureContext context(config, true);
    MCP::QueryAdapter adapter(&context);
    MCP::ToolRegistry registry;
    MCP::RegisterCoreTools(registry, adapter);
    MCP::RegisterExternalToolTools(registry, adapter);
    MCP::RegisterSessionTools(registry, adapter);
    MCP::RegisterShaderTools(registry, adapter);
    MCP::RegisterComparisonTools(registry, adapter);
    MCP::RegisterCompatibilityTools(registry, adapter);
    MCP::RegisterResourceTools(registry, adapter);
    MCP::RegisterAssetExportTools(registry, adapter);
    QJsonObject output;
    std::thread worker([&]() {
      if(request.value(QStringLiteral("operation")).toString() == QStringLiteral("tools"))
      {
        QJsonArray tools;
        for(const MCP::ToolDefinition &definition : registry.Definitions())
          tools.append(QJsonObject{{QStringLiteral("name"), definition.name},
            {QStringLiteral("description"), definition.description},
            {QStringLiteral("inputSchema"), definition.inputSchema},
            {QStringLiteral("annotations"), QJsonObject{{QStringLiteral("readOnlyHint"),
              definition.effects == MCP::ToolEffects::ReadOnly}}}});
        output[QStringLiteral("tools")] = tools;
      }
      else
      {
        QJsonArray results;
        const QJsonArray steps = request.value(QStringLiteral("steps")).toArray();
        bool failed = false;
        for(const QJsonValue &step : steps)
        {
          const QJsonObject command = step.toObject();
          MCP::CallContext call;
          call.ctx = &context;
          const int timeout = request.value(QStringLiteral("timeoutMs")).toInt(120000);
          call.deadlineMs = QDateTime::currentMSecsSinceEpoch() + qMax(1, timeout);
          const QString name = command.value(QStringLiteral("name")).toString();
          const MCP::ToolResult result = registry.Call(name,
              command.value(QStringLiteral("arguments")).toObject(), call);
          QJsonObject record{{QStringLiteral("name"), name}, {QStringLiteral("ok"), result.ok}};
          if(result.ok)
            record[QStringLiteral("result")] = result.structured;
          else
            record[QStringLiteral("error")] = result.error.toJson();
          results.append(record);
          const bool businessFailure = result.structured.value(QStringLiteral("ok")) == QJsonValue(false) ||
              result.structured.value(QStringLiteral("success")) == QJsonValue(false);
          if(!result.ok || businessFailure)
          {
            failed = true;
            break; // No subsequent side effects after a failed prerequisite.
          }
        }
        output[QStringLiteral("results")] = results;
        output[QStringLiteral("ok")] = !failed && !steps.isEmpty();
      }
      QMetaObject::invokeMethod(&application, "quit", Qt::QueuedConnection);
    });
    application.exec();
    worker.join();
    context.CloseCapture();
    QSaveFile result(resultPath);
    const QByteArray bytes = QJsonDocument(output).toJson(QJsonDocument::Compact);
    if(!result.open(QIODevice::WriteOnly) || result.write(bytes) != bytes.size() || !result.commit())
      status = 1;
  }
  RENDERDOC_ShutdownReplay();
  return status;
}
