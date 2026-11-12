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

#include "MaliAnalysisDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QTabWidget>
#include <QTemporaryFile>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "Code/Interface/QRDInterface.h"
#include "Code/QRDUtils.h"
#include "Windows/Dialogs/SettingsDialog.h"

namespace
{
QHash<QString, MCP::MaliRunResult> &MaliResultCache()
{
  static QHash<QString, MCP::MaliRunResult> cache;
  return cache;
}

QString RdcToQString(const rdcstr &value)
{
  return QString::fromUtf8(value.c_str(), int(value.size()));
}

QString DisplayCommand(const QStringList &argv)
{
  QStringList quoted;
  for(const QString &argument : argv)
  {
    QString value = argument;
    value.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    if(value.contains(QRegularExpression(QStringLiteral("\\s"))))
      value = QLatin1Char('"') + value + QLatin1Char('"');
    quoted.push_back(value);
  }
  return quoted.join(QLatin1Char(' '));
}

// One row of the summary's metric grid. `missing` keeps an unreported value visible as an
// explicit hole instead of printing a zero the compiler never produced.
QLabel *CardLabel(const QString &text, const char *role)
{
  QLabel *label = new QLabel(text);
  label->setTextFormat(Qt::PlainText);
  label->setWordWrap(true);
  label->setMinimumWidth(0);
  label->setProperty("role", QString::fromLatin1(role));
  label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  return label;
}

// The extension renders these captions in upper case with extra tracking. Do it with a font so
// the string in code (and in the translation files) stays normal prose.
QLabel *MicroCaption(const QString &text)
{
  QLabel *label = CardLabel(text, "cardLabel");
  QFont font = label->font();
  font.setCapitalization(QFont::AllUppercase);
  font.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
  label->setFont(font);
  return label;
}

void SetStyleProperty(QWidget *widget, const char *name, const QVariant &value)
{
  if(widget->property(name) == value)
    return;

  widget->setProperty(name, value);
  widget->style()->unpolish(widget);
  widget->style()->polish(widget);
  widget->update();
}

void ClearLayout(QLayout *layout)
{
  while(QLayoutItem *item = layout->takeAt(0))
  {
    delete item->widget();
    delete item;
  }
}

// A wrapping row of pills, i.e. the flex-wrap pill list of the extension's summary card. Qt has no
// flow layout, and a fixed-column grid squeezes or clips the longer findings such as
// "High register pressure".
class FlowLayout : public QLayout
{
public:
  FlowLayout(QWidget *parent, int spacing)
      : QLayout(parent), m_HSpacing(spacing), m_VSpacing(spacing)
  {
    setContentsMargins(0, 0, 0, 0);
  }
  ~FlowLayout() { ClearLayout(this); }

  void addItem(QLayoutItem *item) override { m_Items.push_back(item); }
  int count() const override { return int(m_Items.size()); }
  QLayoutItem *itemAt(int index) const override { return m_Items.value(index); }

  QLayoutItem *takeAt(int index) override
  {
    if(index < 0 || index >= int(m_Items.size()))
      return NULL;
    return m_Items.takeAt(index);
  }

  Qt::Orientations expandingDirections() const override { return Qt::Orientations(); }
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int width) const override { return doLayout(QRect(0, 0, width, 0), true); }

  void setGeometry(const QRect &rect) override
  {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
  }

  QSize sizeHint() const override { return minimumSize(); }

  QSize minimumSize() const override
  {
    QSize size;
    for(QLayoutItem *item : m_Items)
      size = size.expandedTo(item->minimumSize());
    return size;
  }

private:
  int doLayout(const QRect &rect, bool testOnly) const
  {
    const QMargins margins = contentsMargins();
    const QRect area = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    int x = area.x();
    int y = area.y();
    int lineHeight = 0;

    for(QLayoutItem *item : m_Items)
    {
      const QSize hint = item->sizeHint();
      if(lineHeight > 0 && x + hint.width() > area.right() + 1)
      {
        x = area.x();
        y += lineHeight + m_VSpacing;
        lineHeight = 0;
      }

      if(!testOnly)
        item->setGeometry(QRect(QPoint(x, y), hint));

      x += hint.width() + m_HSpacing;
      lineHeight = qMax(lineHeight, hint.height());
    }

    return y + lineHeight - rect.y() + margins.bottom();
  }

  QVector<QLayoutItem *> m_Items;
  int m_HSpacing = 6;
  int m_VSpacing = 6;
};

QFrame *MetricTileWidget(MaliAnalysisDialog::MetricTile tile, QWidget *parent)
{
  QFrame *frame = new QFrame(parent);
  frame->setProperty("tile", true);
  frame->setProperty("missing", tile.missing);
  frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  QVBoxLayout *layout = new QVBoxLayout(frame);
  layout->setContentsMargins(10, 8, 10, 8);
  layout->setSpacing(2);

  QLabel *label = CardLabel(tile.label, "tileLabel");
  QLabel *value = CardLabel(tile.value, "tileValue");
  value->setWordWrap(false);
  // A long value (a shader file name, "Not reported") must not decide the column width: the grid
  // keeps both columns even and the value ellipsises inside its tile.
  value->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  value->setMinimumWidth(0);

  layout->addWidget(label);
  layout->addWidget(value);

  if(!tile.meta.isEmpty())
    layout->addWidget(CardLabel(tile.meta, "tileMeta"));

  return frame;
}

QString BreakdownMeta(const MCP::MaliCycleBreakdown &path)
{
  if(!path.available)
    return QObject::tr("malioc did not print an A/LS/V/T breakdown");

  QString meta = QObject::tr("A %1 · LS %2 · V %3 · T %4")
                     .arg(QString::number(path.arithmetic, 'g', 6))
                     .arg(QString::number(path.loadStore, 'g', 6))
                     .arg(QString::number(path.varying, 'g', 6))
                     .arg(QString::number(path.texture, 'g', 6));
  if(!path.bound.isEmpty())
    meta += QObject::tr(" · bound: %1").arg(path.bound);
  return meta;
}

// malioc marks each cycle row with the pipe that bounds it (A/LS/V/T). That letter is the only
// honest single-value summary of a row whose four columns are per-pipe cycle counts, so the tile
// shows the bottleneck and keeps the numbers in its caption.
QString BoundLabel(const QString &bound)
{
  const QString key = bound.trimmed().toUpper();
  if(key == lit("A"))
    return QObject::tr("Arithmetic-bound");
  if(key == lit("LS"))
    return QObject::tr("Load/store-bound");
  if(key == lit("V"))
    return QObject::tr("Varying-bound");
  if(key == lit("T"))
    return QObject::tr("Texture-bound");
  if(key.isEmpty())
    return QString();
  return QObject::tr("%1-bound").arg(bound.trimmed());
}

// malioc appends utilisation notes to register rows, e.g. "31 (96% used at 100% occupancy)".
// The parser keeps the number and the raw line, so pull the note back out for the tile caption.
QString UtilisationNote(const MCP::MaliReport &report, const QString &metricName)
{
  const MCP::MaliMetric *metric = MCP::FindMaliMetric(report, metricName);
  if(metric == NULL)
    return QString();

  const int open = metric->raw.indexOf(QLatin1Char('('));
  const int close = metric->raw.lastIndexOf(QLatin1Char(')'));
  if(open < 0 || close <= open)
    return QString();
  return metric->raw.mid(open + 1, close - open - 1).trimmed();
}

// The four per-pipe columns without the bound suffix, for tiles whose value already names the
// bounding pipe.
QString BreakdownNumbers(const MCP::MaliCycleBreakdown &path)
{
  if(!path.available)
    return QObject::tr("malioc did not print an A/LS/V/T breakdown");

  return QObject::tr("A %1 · LS %2 · V %3 · T %4")
      .arg(QString::number(path.arithmetic, 'g', 6))
      .arg(QString::number(path.loadStore, 'g', 6))
      .arg(QString::number(path.varying, 'g', 6))
      .arg(QString::number(path.texture, 'g', 6));
}

QString ConfidenceLabel(MCP::Confidence confidence)
{
  switch(confidence)
  {
    case MCP::Confidence::Confirmed: return QObject::tr("confirmed");
    case MCP::Confidence::Inferred: return QObject::tr("inferred");
    case MCP::Confidence::ToVerify: return QObject::tr("to verify");
    case MCP::Confidence::Low: return QObject::tr("low");
  }
  return QObject::tr("to verify");
}

// Findings map onto the extension's tone vocabulary: the two severity tiers are the same ones
// MCPExternalTools already derives from malioc's own numbers.
QString RiskTone(const QString &risk)
{
  if(risk.contains(QStringLiteral("High register"), Qt::CaseInsensitive) ||
     risk.contains(QStringLiteral("spilling"), Qt::CaseInsensitive) ||
     risk.contains(QStringLiteral("critical"), Qt::CaseInsensitive))
    return QStringLiteral("bad");
  return QStringLiteral("warn");
}

// Shared geometry and typography for the window. Surfaces are palette roles so the panel follows the
// application's UE theme; the Slate semantic accent colours (StyleColors.cpp defaults) only tint a
// card border or a pill, the way Slate marks a warning or error widget.
QString MaliStyleSheet()
{
  const QString dim = QStringLiteral("#808080");

  QString sheet = lit(
      "QFrame#maliHeader { background: palette(base); border-bottom: 1px solid palette(mid); }"
      "QLabel#maliTitle { font-size: 15px; font-weight: 700; }"
      "QLabel#maliSubtitle { color: %1; font-family: Consolas, \"Cascadia Mono\", \"Courier New\", "
      "monospace; font-size: 11px; }"
      "QFrame#maliDevicePicker { background: palette(window); border: 1px solid palette(mid); "
      "border-radius: 6px; }"
      "QLabel#maliDeviceCaption { color: %1; font-size: 10px; font-weight: 700; }"
      "QComboBox#maliDeviceSelect { background: transparent; border: none; padding: 1px 2px; "
      "min-width: 150px; }"
      "QComboBox#maliDeviceSelect:disabled { color: %1; }"
      "QPushButton { padding: 5px 10px; border-radius: 6px; }"
      "QPushButton#maliConfigure { background: palette(window); border: 1px solid palette(mid); }"
      "QPushButton#maliRun { background: palette(highlight); color: palette(highlighted-text); "
      "border: 1px solid palette(highlight); font-weight: 600; }"
      "QPushButton#maliRun:disabled { background: palette(window); color: %1; "
      "border: 1px solid palette(mid); }"
      "QPushButton#maliClose { background: transparent; border: 1px solid transparent; }"
      "QPushButton#maliClose:hover { background: palette(window); border: 1px solid palette(mid); }"
      "QPushButton#maliCopy { background: palette(window); border: 1px solid palette(mid); "
      "font-size: 11px; padding: 3px 8px; }"
      "QFrame[card=\"true\"] { background: palette(base); border: 1px solid palette(mid); "
      "border-radius: 14px; }"
      "QLabel { background: transparent; border: none; color: palette(window-text); }"
      "QLabel[role=\"cardLabel\"] { color: %1; font-size: 10px; font-weight: 700; }"
      "QLabel[role=\"cardTitle\"] { font-size: 15px; font-weight: 700; }"
      "QLabel[role=\"cardCopy\"] { color: %1; font-size: 12px; }"
      "QLabel[role=\"meta\"] { color: %1; font-family: Consolas, \"Cascadia Mono\", \"Courier New\", "
      "monospace; font-size: 11px; }"
      "QFrame[tile=\"true\"] { background: palette(window); border: 1px solid palette(mid); "
      "border-radius: 10px; }"
      "QFrame[tile=\"true\"][missing=\"true\"] { border: 1px dashed palette(mid); }"
      "QLabel[role=\"tileLabel\"] { color: %1; font-size: 10px; font-weight: 700; }"
      "QLabel[role=\"tileValue\"] { font-size: 15px; font-weight: 700; }"
      "QLabel[role=\"tileMeta\"] { color: %1; font-size: 11px; }"
      "QLabel[pill=\"true\"] { background: rgba(255, 255, 255, 14); border: 1px solid palette(mid); "
      "border-radius: 9px; padding: 2px 8px; font-size: 10px; font-weight: 700; }"
      "QLabel[role=\"line\"] { background: palette(window); border: 1px solid palette(mid); "
      "border-radius: 10px; padding: 7px 9px; font-size: 11px; }"
      "QLineEdit#maliCommand { background: transparent; border: none; color: %1; "
      "font-size: 11px; padding: 0; }"
      "QPlainTextEdit#maliOutput { background: palette(window); border: 1px solid palette(mid); "
      "border-radius: 10px; padding: 8px; }"
      "QTreeWidget#maliMetrics { background: palette(alternate-base); border: 1px solid "
      "palette(mid); border-radius: 10px; padding: 2px; }"
      "QTreeWidget#maliMetrics::item { padding: 3px 2px; }"
      "QTreeWidget#maliMetrics QHeaderView::section { background: transparent; border: none; "
      "border-bottom: 1px solid palette(mid); padding: 4px 2px; color: %1; font-size: 10px; "
      "font-weight: 700; }"
      "QScrollArea#maliLeftColumn { background: transparent; border: none; }"
      "QWidget#maliLeftInner { background: transparent; }"
      "QTabWidget#maliRightTabs::pane { border: none; background: transparent; }"
      "QTabWidget#maliRightTabs QTabBar { background: transparent; }"
      "QTabWidget#maliRightTabs QTabBar::tab { background: transparent; color: %1; "
      "border: 1px solid palette(mid); border-radius: 9px; padding: 5px 12px; margin-right: 6px; "
      "margin-bottom: 2px; font-size: 11px; }"
      "QTabWidget#maliRightTabs QTabBar::tab:selected { background: palette(base); "
      "color: palette(window-text); border: 1px solid palette(highlight); font-weight: 600; }"
      "QTabWidget#maliRightTabs QTabBar::tab:hover { color: palette(window-text); }");

  sheet = sheet.arg(dim);

  const QVector<QPair<QString, QString>> tones = {
      {QStringLiteral("good"), QStringLiteral("31, 228, 75")},
      {QStringLiteral("warn"), QStringLiteral("255, 184, 0")},
      {QStringLiteral("bad"), QStringLiteral("239, 53, 53")},
      {QStringLiteral("info"), QStringLiteral("38, 187, 255")},
  };
  for(const QPair<QString, QString> &tone : tones)
  {
    sheet += QStringLiteral("QFrame[card=\"true\"][tone=\"%1\"] { border: 1px solid rgba(%2, 120); }")
                 .arg(tone.first, tone.second);
    sheet += QStringLiteral(
                 "QLabel[pill=\"true\"][tone=\"%1\"] { background: rgba(%2, 34); "
                 "border: 1px solid rgba(%2, 104); }")
                 .arg(tone.first, tone.second);
  }

  return sheet;
}
}    // namespace

MaliAnalysisDialog::MaliAnalysisDialog(ICaptureContext &ctx, const MCP::MaliShaderExport &shader,
                                       QWidget *parent)
    : QDialog(parent), m_Ctx(ctx), m_Shader(shader), m_Cancelled(new std::atomic_bool(false))
{
  setAttribute(Qt::WA_DeleteOnClose);

  buildUi();

  const int configuredIndex = m_Device->findData(RdcToQString(m_Ctx.Config().MCP_MaliDevice));
  const int normalizedIndex =
      configuredIndex >= 0 ? configuredIndex
                           : m_Device->findData(
                                 MCP::NormalizeMaliDevice(RdcToQString(m_Ctx.Config().MCP_MaliDevice)));
  if(normalizedIndex >= 0)
    m_Device->setCurrentIndex(normalizedIndex);

  updateSubtitle();
  applyCompilerAvailability();
  restoreCachedResult();

  // Opening this window from the shader viewer is the request to analyse, so a first run starts by
  // itself - the same auto-run the extension performs for its Mali action. Decisions are limited to
  // what the dialog can prove: a shader malioc can read, a compiler that exists, and no cached
  // report for these exact bytes.
  if(canAnalyzeShader() && isCompilerAvailable())
    QTimer::singleShot(0, this, [this] {
      if(!m_Running && !m_ShowingCachedResult)
        startAnalysis();
    });
}

MaliAnalysisDialog::~MaliAnalysisDialog()
{
  // RunMaliAnalysis polls this while its child process is alive, so closing the
  // window terminates an in-flight invocation rather than leaving it orphaned.
  m_Cancelled->store(true);
}

void MaliAnalysisDialog::buildUi()
{
  setWindowTitle(tr("Mali Offline Compiler"));
  setStyleSheet(MaliStyleSheet());
  setMinimumSize(780, 560);
  resize(1000, 700);

  QVBoxLayout *root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  // ---- header: title, snapshot subtitle, profile picker, run actions -----------------------
  QFrame *header = new QFrame(this);
  header->setObjectName(lit("maliHeader"));
  QHBoxLayout *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(16, 12, 16, 12);
  headerLayout->setSpacing(10);

  QVBoxLayout *headerText = new QVBoxLayout();
  headerText->setSpacing(3);
  QLabel *title = new QLabel(tr("Mali Offline Compiler"), header);
  title->setObjectName(lit("maliTitle"));
  m_Subtitle = new QLabel(header);
  m_Subtitle->setObjectName(lit("maliSubtitle"));
  m_Subtitle->setTextFormat(Qt::PlainText);
  m_Subtitle->setWordWrap(true);
  m_Subtitle->setTextInteractionFlags(Qt::TextSelectableByMouse);
  // The header keeps the controls at their natural size and lets the snapshot line wrap instead
  // of squeezing the profile picker.
  m_Subtitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_Subtitle->setMinimumWidth(0);
  headerText->addWidget(title);
  headerText->addWidget(m_Subtitle);
  headerLayout->addLayout(headerText, 1);

  m_DevicePicker = new QFrame(header);
  m_DevicePicker->setObjectName(lit("maliDevicePicker"));
  QHBoxLayout *pickerLayout = new QHBoxLayout(m_DevicePicker);
  pickerLayout->setContentsMargins(10, 3, 6, 3);
  pickerLayout->setSpacing(8);
  QLabel *deviceCaption = new QLabel(tr("Target profile"), m_DevicePicker);
  deviceCaption->setObjectName(lit("maliDeviceCaption"));
  m_Device = new QComboBox(m_DevicePicker);
  m_Device->setObjectName(lit("maliDeviceSelect"));
  m_Device->setMinimumWidth(150);
  m_Device->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  m_Device->addItem(tr("Default profile"), QString());
  for(const QString &device : MCP::MaliDeviceNames())
    m_Device->addItem(device, device);
  pickerLayout->addWidget(deviceCaption);
  pickerLayout->addWidget(m_Device);

  m_Configure = new QPushButton(tr("Configure Path"), header);
  m_Configure->setObjectName(lit("maliConfigure"));
  m_Run = new QPushButton(tr("Run Analysis"), header);
  m_Run->setObjectName(lit("maliRun"));
  QPushButton *close = new QPushButton(tr("Close"), header);
  close->setObjectName(lit("maliClose"));

  headerLayout->addWidget(m_DevicePicker, 0, Qt::AlignVCenter);
  headerLayout->addWidget(m_Configure, 0, Qt::AlignVCenter);
  headerLayout->addWidget(m_Run, 0, Qt::AlignVCenter);
  headerLayout->addWidget(close, 0, Qt::AlignVCenter);
  root->addWidget(header);

  // ---- body: analysis summary + measured metrics beside the raw compiler output -------------
  QWidget *body = new QWidget(this);
  QHBoxLayout *columns = new QHBoxLayout(body);
  columns->setContentsMargins(16, 16, 16, 16);
  columns->setSpacing(16);

  m_LeftColumn = new QScrollArea(body);
  m_LeftColumn->setObjectName(lit("maliLeftColumn"));
  m_LeftColumn->setWidgetResizable(true);
  m_LeftColumn->setFrameShape(QFrame::NoFrame);
  m_LeftColumn->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_LeftColumn->setMinimumWidth(340);
  m_LeftColumn->setMaximumWidth(432);

  QWidget *leftInner = new QWidget(m_LeftColumn);
  leftInner->setObjectName(lit("maliLeftInner"));
  QVBoxLayout *leftLayout = new QVBoxLayout(leftInner);
  leftLayout->setContentsMargins(0, 0, 0, 0);
  leftLayout->setSpacing(16);

  m_SummaryCard = new QFrame(leftInner);
  m_SummaryCard->setProperty("card", true);
  m_SummaryCard->setProperty("tone", QStringLiteral("neutral"));
  QVBoxLayout *summaryLayout = new QVBoxLayout(m_SummaryCard);
  summaryLayout->setContentsMargins(16, 14, 16, 14);
  summaryLayout->setSpacing(8);

  m_SummaryTitle = CardLabel(QString(), "cardTitle");
  m_SummaryCopy = CardLabel(QString(), "cardCopy");

  QWidget *pillHost = new QWidget(m_SummaryCard);
  m_SummaryPills = new FlowLayout(pillHost, 6);

  QWidget *metricHost = new QWidget(m_SummaryCard);
  m_SummaryMetrics = new QGridLayout(metricHost);
  m_SummaryMetrics->setContentsMargins(0, 4, 0, 0);
  m_SummaryMetrics->setHorizontalSpacing(8);
  m_SummaryMetrics->setVerticalSpacing(8);
  m_SummaryMetrics->setColumnStretch(0, 1);
  m_SummaryMetrics->setColumnStretch(1, 1);

  QWidget *lineHost = new QWidget(m_SummaryCard);
  m_SummaryLines = new QVBoxLayout(lineHost);
  m_SummaryLines->setContentsMargins(0, 2, 0, 0);
  m_SummaryLines->setSpacing(6);

  summaryLayout->addWidget(MicroCaption(tr("Analysis Summary")));
  summaryLayout->addWidget(m_SummaryTitle);
  summaryLayout->addWidget(m_SummaryCopy);
  summaryLayout->addWidget(pillHost);
  summaryLayout->addWidget(metricHost);
  summaryLayout->addWidget(lineHost);

  leftLayout->addWidget(m_SummaryCard);
  leftLayout->addStretch(1);
  m_LeftColumn->setWidget(leftInner);
  columns->addWidget(m_LeftColumn, 0);

  // The detailed tables and the verbatim compiler output are two views of the same run, so they
  // share the right-hand side instead of pushing each other off the bottom of the window.
  m_RightTabs = new QTabWidget(body);
  m_RightTabs->setObjectName(lit("maliRightTabs"));
  m_RightTabs->setDocumentMode(true);

  QWidget *metricsPage = new QWidget(m_RightTabs);
  QVBoxLayout *metricsPageLayout = new QVBoxLayout(metricsPage);
  metricsPageLayout->setContentsMargins(0, 8, 0, 0);
  metricsPageLayout->setSpacing(0);

  QFrame *metricsCard = new QFrame(metricsPage);
  metricsCard->setProperty("card", true);
  QVBoxLayout *metricsLayout = new QVBoxLayout(metricsCard);
  metricsLayout->setContentsMargins(16, 14, 16, 14);
  metricsLayout->setSpacing(8);

  m_MetricsCaption = CardLabel(QString(), "cardCopy");

  m_Metrics = new QTreeWidget(metricsCard);
  m_Metrics->setObjectName(lit("maliMetrics"));
  m_Metrics->setColumnCount(2);
  m_Metrics->setHeaderLabels({tr("Metric"), tr("Value")});
  m_Metrics->setRootIsDecorated(true);
  m_Metrics->setAlternatingRowColors(true);
  m_Metrics->setUniformRowHeights(true);
  m_Metrics->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_Metrics->header()->setStretchLastSection(true);

  metricsLayout->addWidget(m_MetricsCaption);
  metricsLayout->addWidget(m_Metrics, 1);

  metricsPageLayout->addWidget(metricsCard);

  QWidget *outputPage = new QWidget(m_RightTabs);
  QVBoxLayout *outputPageLayout = new QVBoxLayout(outputPage);
  outputPageLayout->setContentsMargins(0, 8, 0, 0);
  outputPageLayout->setSpacing(0);

  m_OutputCard = new QFrame(outputPage);
  m_OutputCard->setProperty("card", true);
  m_OutputCard->setProperty("tone", QStringLiteral("neutral"));
  QVBoxLayout *outputLayout = new QVBoxLayout(m_OutputCard);
  outputLayout->setContentsMargins(16, 14, 16, 14);
  outputLayout->setSpacing(8);

  m_OutputTitle = CardLabel(QString(), "cardTitle");
  m_OutputCopy = CardLabel(QString(), "cardCopy");

  QHBoxLayout *outputHeader = new QHBoxLayout();
  outputHeader->setSpacing(8);
  outputHeader->addWidget(m_OutputTitle, 1);
  m_Copy = new QPushButton(tr("Copy"), m_OutputCard);
  m_Copy->setObjectName(lit("maliCopy"));
  outputHeader->addWidget(m_Copy, 0, Qt::AlignTop);

  QWidget *outputMetricHost = new QWidget(m_OutputCard);
  m_OutputMetrics = new QHBoxLayout(outputMetricHost);
  m_OutputMetrics->setContentsMargins(0, 4, 0, 0);
  m_OutputMetrics->setSpacing(8);

  // A compiler invocation is one unbreakable line (two absolute paths). A read-only line edit
  // shows its start, scrolls to the rest, and lets the command be selected, where a wrapping
  // label would either clip or stretch the card.
  m_Command = new QLineEdit(m_OutputCard);
  m_Command->setObjectName(lit("maliCommand"));
  m_Command->setReadOnly(true);
  m_Command->setFrame(false);
  m_Command->setCursorPosition(0);
  m_Command->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_Command->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  m_Command->setMinimumWidth(0);
  m_Command->setVisible(false);

  m_Output = new QPlainTextEdit(m_OutputCard);
  m_Output->setObjectName(lit("maliOutput"));
  m_Output->setReadOnly(true);
  m_Output->setLineWrapMode(QPlainTextEdit::NoWrap);
  m_Output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  m_Output->setMinimumHeight(220);
  m_Output->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  outputLayout->addLayout(outputHeader);
  outputLayout->addWidget(m_OutputCopy);
  outputLayout->addWidget(outputMetricHost);
  outputLayout->addWidget(m_Command);
  outputLayout->addWidget(m_Output, 1);

  outputPageLayout->addWidget(m_OutputCard);
  m_RightTabs->addTab(outputPage, tr("Compiler Output"));
  m_RightTabs->addTab(metricsPage, tr("Measured Metrics"));
  columns->addWidget(m_RightTabs, 1);
  root->addWidget(body, 1);

  connect(m_Configure, &QPushButton::clicked, this, [this] { configureCompiler(); });
  connect(m_Run, &QPushButton::clicked, this, [this] {
    if(m_Running)
      cancelAnalysis();
    else
      startAnalysis();
  });
  connect(m_Device, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
          [this](int) {
            updateSubtitle();
            if(!m_Running)
              restoreCachedResult();
          });
  connect(m_Copy, &QPushButton::clicked, this,
          [this] { QApplication::clipboard()->setText(m_Output->toPlainText()); });
  connect(close, &QPushButton::clicked, this, &QDialog::close);
}

QString MaliAnalysisDialog::currentDeviceLabel() const
{
  const QString device = m_Device->currentData().toString();
  return device.isEmpty() ? tr("Default profile") : device;
}

void MaliAnalysisDialog::updateSubtitle()
{
  QStringList parts;
  if(!m_Shader.stage.isEmpty())
    parts << tr("%1 stage").arg(m_Shader.stage);
  if(!m_Shader.entryPoint.isEmpty())
    parts << m_Shader.entryPoint;
  parts << MCP::MaliEncodingName(m_Shader.encoding);
  parts << tr("revision %1").arg(m_Shader.revision.left(12));
  parts << currentDeviceLabel();

  m_Subtitle->setText(parts.join(tr(" · ")));
}

bool MaliAnalysisDialog::canAnalyzeShader() const
{
  return MCP::MaliSupportsEncoding(m_Shader.encoding) && !m_Shader.contents.isEmpty();
}

bool MaliAnalysisDialog::isCompilerAvailable(QString *resolvedExecutable) const
{
  const QString resolved = MCP::ResolveMaliExecutablePath(RdcToQString(m_Ctx.Config().MCP_MaliCompilerPath));
  if(resolvedExecutable != NULL)
    *resolvedExecutable = resolved;

  return !resolved.isEmpty() && QFileInfo(resolved).isFile();
}

void MaliAnalysisDialog::applyCompilerAvailability()
{
  QString resolved;
  const bool available = isCompilerAvailable(&resolved);

  m_Configure->setText(available ? tr("Configure Path") : tr("Set Path"));
  m_Configure->setToolTip(available
                              ? tr("malioc: %1").arg(resolved)
                              : tr("Point RenderMax at malioc. It also looks in the bundled "
                                   "mali_offline_compiler folder and on PATH."));

  applyRunButton();
}

void MaliAnalysisDialog::applyRunButton()
{
  if(m_Running)
  {
    m_Run->setText(tr("Cancel Analysis"));
    m_Run->setEnabled(true);
    m_Run->setToolTip(tr("Stop the running Mali Offline Compiler process."));
    return;
  }

  const bool available = isCompilerAvailable();
  const bool analysable = canAnalyzeShader();

  if(m_ShowingCachedResult)
    m_Run->setText(tr("Re-run Analysis"));
  else
    m_Run->setText(tr("Run Analysis"));

  m_Run->setEnabled(available && analysable);

  if(!available)
    m_Run->setToolTip(tr("Mali Offline Compiler was not found. Use Set Path to point at malioc."));
  else if(!analysable)
    m_Run->setToolTip(tr("This shader cannot be read by malioc."));
  else if(m_ShowingCachedResult)
    m_Run->setToolTip(tr("Compile this shader revision again and replace the cached findings."));
  else
    m_Run->setToolTip(tr("Run malioc on this captured shader revision."));
}

void MaliAnalysisDialog::configureCompiler()
{
  SettingsDialog settings(m_Ctx, this);
  RDDialog::show(&settings);

  const QString configured = RdcToQString(m_Ctx.Config().MCP_MaliDevice);
  const int index = m_Device->findData(MCP::NormalizeMaliDevice(configured));
  if(index >= 0)
    m_Device->setCurrentIndex(index);

  updateSubtitle();
  applyCompilerAvailability();
  if(!m_Running)
    restoreCachedResult();
}

void MaliAnalysisDialog::setSummaryState(const QString &tone, const QString &title,
                                         const QString &copy)
{
  SetStyleProperty(m_SummaryCard, "tone", tone);
  m_SummaryTitle->setText(title);
  m_SummaryCopy->setText(copy);
  m_SummaryCopy->setVisible(!copy.isEmpty());
}

void MaliAnalysisDialog::setSummaryPills(const QVector<StatusPill> &pills)
{
  ClearLayout(m_SummaryPills);

  for(const StatusPill &pill : pills)
  {
    QLabel *label = new QLabel(pill.text);
    label->setTextFormat(Qt::PlainText);
    label->setProperty("pill", true);
    label->setProperty("tone", pill.tone);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_SummaryPills->addWidget(label);
  }

  m_SummaryPills->parentWidget()->setVisible(!pills.isEmpty());
}

void MaliAnalysisDialog::setSummaryMetrics(const QVector<MetricTile> &tiles)
{
  ClearLayout(m_SummaryMetrics);

  QWidget *host = m_SummaryMetrics->parentWidget();
  int index = 0;
  for(const MetricTile &tile : tiles)
  {
    m_SummaryMetrics->addWidget(MetricTileWidget(tile, host), index / 2, index % 2);
    index++;
  }

  host->setVisible(!tiles.isEmpty());
}

void MaliAnalysisDialog::setSummaryLines(const QStringList &lines)
{
  ClearLayout(m_SummaryLines);

  for(const QString &line : lines)
    m_SummaryLines->addWidget(CardLabel(line, "line"));

  m_SummaryLines->parentWidget()->setVisible(!lines.isEmpty());
}

void MaliAnalysisDialog::setOutputState(const QString &tone, const QString &title,
                                        const QString &copy)
{
  SetStyleProperty(m_OutputCard, "tone", tone);
  m_OutputTitle->setText(title);
  m_OutputCopy->setText(copy);
  m_OutputCopy->setVisible(!copy.isEmpty());
}

void MaliAnalysisDialog::setOutputMetrics(const QVector<MetricTile> &tiles)
{
  ClearLayout(m_OutputMetrics);

  QWidget *host = m_OutputMetrics->parentWidget();
  for(const MetricTile &tile : tiles)
    m_OutputMetrics->addWidget(MetricTileWidget(tile, host), 1);

  host->setVisible(!tiles.isEmpty());
}

void MaliAnalysisDialog::setRunning(bool running)
{
  m_Running = running;
  m_Device->setEnabled(!running);
  m_Configure->setEnabled(!running);

  if(!running)
  {
    applyRunButton();
    return;
  }

  m_ShowingCachedResult = false;
  applyRunButton();

  setSummaryState(lit("info"), tr("Analysis in progress"),
                  tr("malioc is compiling this shader revision for %1. The summary appears here "
                     "when the run finishes.")
                      .arg(currentDeviceLabel()));

  setSummaryPills({StatusPill{tr("Running"), lit("info")}});

  setSummaryMetrics(QVector<MetricTile>());

  QStringList lines;
  lines << tr("The compiler runs on a background thread; Cancel stops it.");
  setSummaryLines(lines);

  setOutputState(lit("info"), tr("Compiler output pending"),
                 tr("Verbatim malioc stdout and stderr appear here when the run completes."));
  setOutputMetrics(QVector<MetricTile>());

  m_Command->setVisible(false);
  m_Metrics->clear();
  m_MetricsCaption->setText(tr("Running…"));
  m_Output->setPlainText(tr("Waiting for Mali Offline Compiler output…"));
  m_Copy->setEnabled(false);
}

void MaliAnalysisDialog::cancelAnalysis()
{
  if(!m_Running)
    return;

  m_Cancelled->store(true);
  m_Run->setEnabled(false);
  m_Run->setText(tr("Cancelling…"));
  setSummaryState(lit("warn"), tr("Cancelling analysis"),
                  tr("Stopping the Mali Offline Compiler process…"));
}

QString MaliAnalysisDialog::cacheKey() const
{
  return m_Shader.revision + QLatin1Char('|') + m_Device->currentData().toString() +
         QLatin1Char('|') + RdcToQString(m_Ctx.Config().MCP_MaliCompilerPath) + QLatin1Char('|') +
         RdcToQString(m_Ctx.Config().MCP_MaliExtraArgs);
}

void MaliAnalysisDialog::restoreCachedResult()
{
  if(m_Running)
    return;

  const auto it = MaliResultCache().constFind(cacheKey());
  if(it == MaliResultCache().constEnd() || it->status != MCP::MaliRunStatus::Ran)
  {
    clearDisplayedResult();
    return;
  }

  showResult(*it, true);
}

void MaliAnalysisDialog::clearDisplayedResult()
{
  m_ShowingCachedResult = false;

  QString resolved;
  const bool available = isCompilerAvailable(&resolved);
  const bool analysable = canAnalyzeShader();

  m_Metrics->clear();
  m_MetricsCaption->setText(tr("No metrics parsed yet. Running the analysis splits malioc's report "
                               "into the sections below."));
  m_Command->setVisible(false);
  m_Output->setPlainText(available ? tr("No analysis output yet.")
                                   : tr("Mali Offline Compiler is not available."));
  m_Copy->setEnabled(false);

  QString statusValue = tr("Ready");
  QVector<StatusPill> pills;
  QStringList lines;

  if(!available)
  {
    statusValue = tr("Unavailable");
    pills.push_back(StatusPill{tr("Disabled"), QString()});
    setSummaryState(lit("warn"), tr("Compiler not configured"),
                    tr("Set the malioc path to run static analysis of the captured shader."));
    lines << tr("RenderMax looks for malioc next to the executable, in the bundled "
                "mali_offline_compiler folder, and on PATH.");
  }
  else if(!analysable)
  {
    statusValue = tr("Blocked");
    pills.push_back(StatusPill{tr("Blocked"), lit("warn")});
    setSummaryState(lit("warn"), tr("Shader cannot be analysed"),
                    m_Shader.limitation.isEmpty()
                        ? tr("malioc can only read captured SPIR-V or GLSL shader data.")
                        : m_Shader.limitation);
    if(!m_Shader.limitation.isEmpty())
      lines << m_Shader.limitation;
  }
  else
  {
    pills.push_back(StatusPill{tr("Not run"), QString()});
    setSummaryState(lit("neutral"), tr("Ready to analyze"),
                    tr("Run malioc on this captured shader revision. Cycle counts and register "
                       "pressure are static estimates for the selected profile, not measured frame "
                       "timings."));
  }
  setSummaryPills(pills);
  setSummaryLines(lines);

  // The card is about numbers: status, profile and snapshot are already stated by the header and
  // the pills, so the tiles start with what malioc measured.
  QVector<MetricTile> tiles;
  tiles.push_back(MetricTile{tr("Metrics"), tr("Not collected"),
                             tr("Run the analysis to collect malioc metrics"), true});
  setSummaryMetrics(tiles);

  if(available)
    setOutputState(lit("neutral"), tr("No compiler output yet"),
                   tr("Running the analysis captures malioc's stdout and stderr for this shader "
                      "revision."));
  else
    setOutputState(lit("neutral"), tr("No compiler output yet"),
                   tr("Configure the compiler path first."));

  QVector<MetricTile> outputTiles;
  outputTiles.push_back(MetricTile{tr("Output lines"), tr("Not collected"),
                                   tr("Verbatim compiler stdout/stderr"), true});
  outputTiles.push_back(MetricTile{tr("Exit code"), tr("Not run"), tr("malioc process status"), true});
  setOutputMetrics(outputTiles);

  applyRunButton();
}

void MaliAnalysisDialog::startAnalysis()
{
  if(m_Running)
    return;

  m_Cancelled->store(false);
  setRunning(true);

  MCP::MaliRunRequest request;
  request.configuredPath = RdcToQString(m_Ctx.Config().MCP_MaliCompilerPath);
  request.target.device = m_Device->currentData().toString();
  request.target.timeoutMs = 30000;

  const QString configuredArgs = RdcToQString(m_Ctx.Config().MCP_MaliExtraArgs);
  if(!configuredArgs.isEmpty())
    request.target.extraArgs.push_back(configuredArgs);

  const MCP::MaliShaderExport shader = m_Shader;
  const QSharedPointer<std::atomic_bool> cancelled = m_Cancelled;
  QPointer<MaliAnalysisDialog> dialog(this);

  LambdaThread *thread = new LambdaThread([request, shader, cancelled, dialog]() mutable {
    MCP::MaliRunResult result;

    if(!MCP::MaliSupportsEncoding(shader.encoding) || shader.contents.isEmpty())
    {
      result.status = MCP::MaliRunStatus::UnsupportedShader;
      result.hint = shader.limitation;
    }
    else if(cancelled->load())
    {
      result.status = MCP::MaliRunStatus::Cancelled;
      result.hint = QObject::tr("The analysis request was cancelled before export.");
    }
    else
    {
      QTemporaryFile temporary(QDir::tempPath() + QStringLiteral("/renderdoc_mali_XXXXXX") +
                               shader.extension);
      temporary.setAutoRemove(true);
      if(!temporary.open() || temporary.write(shader.contents) != shader.contents.size() ||
         !temporary.flush())
      {
        result.status = MCP::MaliRunStatus::FailedToStart;
        result.hint = QObject::tr("Could not create the temporary shader export file.");
        result.report.error = result.hint;
      }
      else
      {
        temporary.close();
        request.shaderPath = temporary.fileName();
        result = MCP::RunMaliAnalysis(request, [cancelled] { return cancelled->load(); });
      }
    }

    // The dialog can disappear as the capture/viewer closes. Queue through the
    // application object rather than the dialog so a destroyed dialog is never
    // used as GUIInvoke's target; QPointer guards the actual delivery.
    GUIInvoke::call(QCoreApplication::instance(), [dialog, result] {
      if(dialog)
        dialog->showResult(result);
    });
  });
  thread->setName(lit("Mali offline analysis"));
  thread->selfDelete(true);
  thread->start();
}

void MaliAnalysisDialog::showResult(const MCP::MaliRunResult &result, bool fromCache)
{
  m_Running = false;
  m_ShowingCachedResult = fromCache;
  m_Device->setEnabled(true);
  m_Configure->setEnabled(true);

  const MCP::MaliReport &report = result.report;

  if(result.status == MCP::MaliRunStatus::Ran && !fromCache)
    MaliResultCache().insert(cacheKey(), result);

  m_Command->setText(result.commandLine.isEmpty() ? tr("No compiler command was launched.")
                                                  : DisplayCommand(result.commandLine));
  m_Command->setCursorPosition(0);
  m_Command->setToolTip(DisplayCommand(result.commandLine));
  m_Command->setVisible(!result.commandLine.isEmpty());

  const bool producedReport = result.status == MCP::MaliRunStatus::Ran;
  const bool failed = !producedReport || !report.error.isEmpty();

  QString tone = lit("neutral");
  QString title;
  QString copy;
  QString statusValue;
  QStringList lines;

  switch(result.status)
  {
    case MCP::MaliRunStatus::Ran:
      if(report.error.isEmpty())
      {
        tone = lit("good");
        title = tr("Analysis available");
        copy = tr("malioc analysed this exact shader revision. Cycle counts are static estimates "
                  "for the selected profile, not measured frame timings.");
        statusValue = tr("Available");
      }
      else
      {
        tone = lit("bad");
        title = tr("Analysis failed");
        copy = report.error;
        statusValue = tr("Failed");
      }
      break;
    case MCP::MaliRunStatus::NotConfigured:
      tone = lit("warn");
      title = tr("Compiler not configured");
      copy = result.hint.isEmpty() ? tr("Set the Mali Offline Compiler path to analyse captured "
                                        "shaders.")
                                   : result.hint;
      statusValue = tr("Unavailable");
      break;
    case MCP::MaliRunStatus::ToolMissing:
      tone = lit("warn");
      title = tr("Compiler not found");
      copy = result.hint.isEmpty() ? tr("The configured Mali Offline Compiler executable does not "
                                        "exist.")
                                   : result.hint;
      statusValue = tr("Unavailable");
      break;
    case MCP::MaliRunStatus::UnsupportedShader:
      tone = lit("warn");
      title = tr("Shader cannot be analysed");
      copy = result.hint.isEmpty() ? tr("malioc can only read captured SPIR-V or GLSL shader data.")
                                   : result.hint;
      statusValue = tr("Blocked");
      break;
    case MCP::MaliRunStatus::Timeout:
      tone = lit("bad");
      title = tr("Analysis timed out");
      copy = result.hint.isEmpty() ? tr("malioc did not finish within the analysis timeout.")
                                   : result.hint;
      statusValue = tr("Timed out");
      break;
    case MCP::MaliRunStatus::Cancelled:
      tone = lit("warn");
      title = tr("Analysis cancelled");
      copy = result.hint.isEmpty() ? tr("The run stopped before malioc returned a report.")
                                   : result.hint;
      statusValue = tr("Cancelled");
      break;
    case MCP::MaliRunStatus::FailedToStart:
      tone = lit("bad");
      title = tr("Compiler did not start");
      copy = result.hint.isEmpty() ? tr("The Mali Offline Compiler process could not be launched.")
                                   : result.hint;
      statusValue = tr("Failed");
      break;
  }

  if(fromCache)
  {
    copy += QLatin1Char(' ');
    copy += tr("Showing the cached report for this shader revision and profile; re-run to refresh "
               "it.");
  }

  // Findings the compiler itself flagged, plus anything that limits how far the numbers can be
  // trusted. The extension shows these as bullets under the metric tiles.
  for(const QString &warning : report.warnings)
  {
    if(lines.size() >= 2)
      break;
    lines << warning;
  }
  // report.limitation is the parser's "no recognised total metrics" note. The confidence pill and
  // the dashed tiles already say exactly that, so it is not repeated as prose here.
  if(!m_Shader.limitation.isEmpty() && lines.size() < 3)
    lines << m_Shader.limitation;

  lines.push_front(tr("Snapshot: %1 · revision %2")
                       .arg(m_Shader.suggestedFileName.isEmpty() ? tr("selected shader")
                                                                 : m_Shader.suggestedFileName,
                            m_Shader.revision.left(12)));

  setSummaryState(tone, title, copy);
  setSummaryLines(lines);

  QVector<StatusPill> pills;
  pills.push_back(StatusPill{statusValue, tone == lit("neutral") ? QString() : tone});
  if(fromCache)
    pills.push_back(StatusPill{tr("Cached"), lit("info")});
  if(producedReport && report.error.isEmpty())
  {
    pills.push_back(StatusPill{tr("Confidence: %1").arg(ConfidenceLabel(report.confidence)),
                               QString()});
    for(const QString &risk : report.riskSignals)
      pills.push_back(StatusPill{risk, RiskTone(risk)});
  }
  setSummaryPills(pills);

  // ---- metric tiles: state, then every number malioc actually reported ----------------------
  // The card is about numbers: status, profile and snapshot are already stated by the header and
  // the pills, so the tiles start with what malioc measured.
  QVector<MetricTile> tiles;

  auto numberText = [](double value) { return QString::number(value, 'g', 12); };
  auto addTile = [&tiles](const QString &label, const QString &value, const QString &meta,
                          bool missing) {
    tiles.push_back(MetricTile{label, value, meta, missing});
  };
  auto metricValue = [&report](const char *name, double *out) -> bool {
    const MCP::MaliMetric *metric = MCP::FindMaliMetric(report, QString::fromLatin1(name));
    if(metric == NULL)
      return false;
    *out = metric->numeric ? metric->value : 0.0;
    return metric->numeric;
  };

  if(producedReport)
  {
    double value = 0.0;
    const bool hasWorkRegisters = report.hasWorkRegisters;
    const QString workNote = UtilisationNote(report, lit("Work registers"));
    addTile(tr("Work registers"),
            hasWorkRegisters ? numberText(report.workRegisters) : tr("Not reported"),
            hasWorkRegisters && !workNote.isEmpty() ? workNote : tr("Register pressure proxy"),
            !hasWorkRegisters);

    const QString uniformNote = UtilisationNote(report, lit("Uniform registers"));
    addTile(tr("Uniform registers"),
            report.hasUniformRegisters ? numberText(report.uniformRegisters) : tr("Not reported"),
            report.hasUniformRegisters && !uniformNote.isEmpty() ? uniformNote
                                                               : tr("Uniform file usage"),
            !report.hasUniformRegisters);

    // Cycle rows are reported per pipe with a bounding pipe. Show the bound as the value and the
    // A/LS/V/T numbers as the caption: picking one of the four numbers would misrepresent it.
    const bool hasLongest = report.longestPath.available;
    const QString longestBound = BoundLabel(report.longestPath.bound);
    addTile(tr("Longest path"),
            hasLongest ? (longestBound.isEmpty() ? tr("Reported") : longestBound)
                       : tr("Not reported"),
            BreakdownNumbers(report.longestPath), !hasLongest);

    const bool hasShortest = report.shortestPath.available;
    const QString shortestBound = BoundLabel(report.shortestPath.bound);
    addTile(tr("Shortest path"),
            hasShortest ? (shortestBound.isEmpty() ? tr("Reported") : shortestBound)
                        : tr("Not reported"),
            BreakdownNumbers(report.shortestPath), !hasShortest);

    const bool hasTotalPath = report.totalPath.available;
    const QString totalBound = BoundLabel(report.totalPath.bound);
    addTile(tr("Total cycles"),
            hasTotalPath ? (totalBound.isEmpty() ? tr("Reported") : totalBound)
                         : (report.hasTotals ? numberText(report.totalCycles) : tr("Not reported")),
            hasTotalPath ? BreakdownNumbers(report.totalPath)
                         : tr("Total instruction cycles reported by malioc"),
            !hasTotalPath && !report.hasTotals);

    // Only claim an instruction count when malioc printed one; older reports omit it entirely.
    if(metricValue("Total instructions", &value))
      addTile(tr("Total instructions"), numberText(value),
              tr("Instructions in the compiled program"), false);

    addTile(tr("Stack size"),
            report.hasStackSize ? tr("%1 bytes").arg(numberText(report.stackSize))
                                : tr("Not reported"),
            tr("Stack frame footprint"), !report.hasStackSize);

    addTile(tr("Stack spilling"),
            report.stackSpillingKnown ? (report.stackSpilling ? tr("Yes") : tr("No"))
                                      : tr("Not reported"),
            tr("Register overflow into the stack"), !report.stackSpillingKnown);

    const MCP::MaliMetric *half = MCP::FindMaliMetric(report, lit("16-bit arithmetic"));
    if(half != NULL && half->numeric)
      addTile(tr("16-bit arithmetic"), tr("%1%").arg(numberText(half->value)),
              tr("Share of operations in half precision"), false);

    const MCP::MaliMetric *varyings = MCP::FindMaliMetric(report, lit("Varyings"));
    if(varyings != NULL)
      addTile(tr("Varyings"), varyings->numeric ? numberText(varyings->value) : varyings->text,
              tr("Interpolator pressure"), false);

    const MCP::MaliMetric *samplers = MCP::FindMaliMetric(report, lit("Samplers"));
    if(samplers != NULL)
      addTile(tr("Samplers"), samplers->numeric ? numberText(samplers->value) : samplers->text,
              tr("Sampling descriptor pressure"), false);
  }
  else
  {
    addTile(tr("Metrics"), tr("Not collected"), tr("No compiler report was produced"), true);
  }
  setSummaryMetrics(tiles);

  // ---- measured metrics table --------------------------------------------------------------
  m_Metrics->clear();
  int metricCount = 0;
  for(const MCP::MaliSection &section : report.sections)
  {
    QTreeWidgetItem *parent = new QTreeWidgetItem(m_Metrics, QStringList(section.title));
    parent->setFirstColumnSpanned(true);
    parent->setExpanded(true);
    QFont sectionFont = parent->font(0);
    sectionFont.setBold(true);
    parent->setFont(0, sectionFont);

    for(const MCP::MaliMetric &metric : section.metrics)
    {
      QString value;
      if(metric.numeric)
      {
        value = numberText(metric.value);
        if(!metric.unit.isEmpty())
          value += QLatin1Char(' ') + metric.unit;
      }
      else
      {
        value = metric.text;
      }

      QTreeWidgetItem *item = new QTreeWidgetItem(parent, {metric.name, value});
      item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
      metricCount++;
    }
  }

  if(metricCount > 0)
  {
    m_Metrics->resizeColumnToContents(0);
    m_MetricsCaption->setText(tr("%1 parsed metric%2 from %3 section%4.")
                                  .arg(metricCount)
                                  .arg(metricCount == 1 ? QString() : QStringLiteral("s"))
                                  .arg(report.sections.size())
                                  .arg(report.sections.size() == 1 ? QString() : QStringLiteral("s")));
  }
  else
  {
    m_MetricsCaption->setText(tr("No metrics were parsed from the compiler output. The raw report "
                                 "on the right is the complete record."));
  }

  // ---- raw output card --------------------------------------------------------------------
  const bool hasOutput = !report.rawOutput.isEmpty();
  const QString outputTone = failed ? tone : lit("neutral");

  if(result.status == MCP::MaliRunStatus::Ran && report.error.isEmpty())
    setOutputState(outputTone, tr("Complete compiler report"),
                   tr("Verbatim malioc stdout and stderr for this shader revision. Nothing here is "
                      "reformatted."));
  else if(result.status == MCP::MaliRunStatus::Ran)
    setOutputState(outputTone, tr("Compiler error output"),
                   tr("malioc did not complete cleanly. The captured stdout and stderr are shown "
                      "unmodified."));
  else
    setOutputState(outputTone, tr("No compiler output captured"), copy);

  m_Output->setPlainText(hasOutput ? report.rawOutput : tr("No analysis output yet."));
  m_Copy->setEnabled(hasOutput);

  QVector<MetricTile> outputTiles;
  const int outputLineCount = hasOutput ? report.rawOutput.count(QLatin1Char('\n')) + 1 : 0;
  outputTiles.push_back(MetricTile{tr("Output lines"),
                                   hasOutput ? QString::number(outputLineCount) : tr("Not collected"),
                                   tr("Verbatim compiler stdout/stderr"), !hasOutput});
  outputTiles.push_back(MetricTile{tr("Exit code"),
                                   report.ran ? QString::number(report.exitCode) : tr("Not run"),
                                   tr("malioc process status"), !report.ran});
  setOutputMetrics(outputTiles);

  applyCompilerAvailability();
  applyRunButton();
}
