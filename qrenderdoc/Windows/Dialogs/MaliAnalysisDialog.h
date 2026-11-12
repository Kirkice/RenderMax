/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2017-2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#pragma once

#include <atomic>

#include <QDialog>
#include <QSharedPointer>
#include <QStringList>
#include <QVector>

#include "Code/AI/MCPExternalTools.h"

class QComboBox;
class QFrame;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QLayout;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class QTabWidget;
class QTreeWidget;
class QVBoxLayout;

struct ICaptureContext;

// Native presentation for a single captured shader analysed with Arm's Mali
// Offline Compiler. The compiler invocation and output parsing stay in
// MCPExternalTools, shared with the MCP tool, so the UI never builds a shell
// command or has its own parser.
//
// The layout mirrors the VS Code extension's Mali Offline Compiler window: a
// header with the target-profile picker and the run/configure actions, then a
// two-column body of an "Analysis Summary" card (status pills, metric tiles and
// highlighted findings) beside a "Raw Mali Output" card that keeps the verbatim
// compiler output. All colours come from the application palette plus Slate's
// semantic accent colours, so the panel stays inside the UE theme.
class MaliAnalysisDialog : public QDialog
{
public:
  // One metric tile in the summary or output card.
  struct MetricTile
  {
    QString label;
    QString value;
    QString meta;
    // True when the compiler did not report this value. The tile is drawn as a
    // dashed placeholder instead of showing a fabricated zero.
    bool missing;
  };

  // One status pill. `tone` is empty ("neutral"), "good", "warn", "bad" or "info".
  struct StatusPill
  {
    QString text;
    QString tone;
  };

  explicit MaliAnalysisDialog(ICaptureContext &ctx, const MCP::MaliShaderExport &shader,
                              QWidget *parent = 0);
  ~MaliAnalysisDialog();

private:
  void buildUi();
  void applyCompilerAvailability();
  void applyRunButton();

  void startAnalysis();
  void cancelAnalysis();
  void configureCompiler();
  void showResult(const MCP::MaliRunResult &result, bool fromCache = false);
  void setRunning(bool running);
  QString cacheKey() const;
  bool isCompilerAvailable(QString *resolvedExecutable = NULL) const;
  bool canAnalyzeShader() const;
  QString currentDeviceLabel() const;
  void updateSubtitle();
  void restoreCachedResult();
  void clearDisplayedResult();

  void setSummaryState(const QString &tone, const QString &title, const QString &copy);
  void setSummaryPills(const QVector<StatusPill> &pills);
  void setSummaryMetrics(const QVector<MetricTile> &tiles);
  void setSummaryLines(const QStringList &lines);
  void setOutputState(const QString &tone, const QString &title, const QString &copy);
  void setOutputMetrics(const QVector<MetricTile> &tiles);

  ICaptureContext &m_Ctx;
  MCP::MaliShaderExport m_Shader;
  QSharedPointer<std::atomic_bool> m_Cancelled;

  QLabel *m_Subtitle = NULL;
  QComboBox *m_Device = NULL;
  QFrame *m_DevicePicker = NULL;
  QScrollArea *m_LeftColumn = NULL;

  QFrame *m_SummaryCard = NULL;
  QLabel *m_SummaryTitle = NULL;
  QLabel *m_SummaryCopy = NULL;
  QLayout *m_SummaryPills = NULL;
  QGridLayout *m_SummaryMetrics = NULL;
  QVBoxLayout *m_SummaryLines = NULL;

  QTreeWidget *m_Metrics = NULL;
  QLabel *m_MetricsCaption = NULL;

  QFrame *m_OutputCard = NULL;
  QLabel *m_OutputTitle = NULL;
  QLabel *m_OutputCopy = NULL;
  QLineEdit *m_Command = NULL;
  QHBoxLayout *m_OutputMetrics = NULL;
  QTabWidget *m_RightTabs = NULL;
  QPlainTextEdit *m_Output = NULL;

  QPushButton *m_Configure = NULL;
  QPushButton *m_Run = NULL;
  QPushButton *m_Copy = NULL;

  bool m_Running = false;
  bool m_ShowingCachedResult = false;
};
