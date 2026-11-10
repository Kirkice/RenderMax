/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
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

#include "SystemTraceViewer.h"

#include <math.h>

#include <algorithm>
#include <functional>
#include <vector>

#include <QAbstractItemView>
#include <QBrush>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QImage>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPolygonF>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "Code/QRDUtils.h"
#include "Code/TraceCapture.h"
#include "Widgets/Extended/RDSplitter.h"

namespace
{
// The trace declares a time unit but the module works in seconds throughout; this is display-only.
QString FormatSeconds(double seconds)
{
  const double abs = fabs(seconds);

  if(abs < 1.0e-3)
    return QObject::tr("%1 us").arg(seconds * 1.0e6, 0, 'f', 1);
  if(abs < 1.0)
    return QObject::tr("%1 ms").arg(seconds * 1.0e3, 0, 'f', 2);
  return QObject::tr("%1 s").arg(seconds, 0, 'f', 3);
}

QString FormatValue(double value)
{
  return QString::number(value, 'g', 4);
}

// QTableWidget sorts text lexically, which puts "10 ms" before "2 ms". ThreadTime cells keep the
// pretty display text but compare their raw numeric value, so every duration/count column sorts as
// users expect. The value is stored in UserRole rather than parsed back out of translated text.
class NumericTableItem : public QTableWidgetItem
{
public:
  NumericTableItem(const QString &text, double value) : QTableWidgetItem(text)
  {
    setData(Qt::UserRole, value);
    setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  }

  bool operator<(const QTableWidgetItem &other) const override
  {
    return data(Qt::UserRole).toDouble() < other.data(Qt::UserRole).toDouble();
  }
};

QString ConfidenceName(ClockDomainConfidence c)
{
  switch(c)
  {
    case ClockDomainConfidence::Unknown:
      return QObject::tr("unknown");
    case ClockDomainConfidence::EstimatedFromFrameTiming:
      return QObject::tr("estimated");
    case ClockDomainConfidence::AssumedShared:
      return QObject::tr("shared");
  }

  return QObject::tr("unknown");
}

// Decides which importer a file needs. Extension alone is not enough: a `traceconv json` export is
// usually named `.json`, but a `.txt` systrace or a bare file can still be the same document. The
// format is therefore identified by looking for the Chrome event array first, and only then
// falling back to the JSON-lines interchange.
enum class TraceFileKind
{
  ChromeJson,
  JsonLines,
  SystraceText,
};

// Which importer a file needs is decided by its content, not its extension: `traceconv` writes a
// JSON document for `json`, a plain kernel trace for `systrace`, and the documented interchange is
// JSON-lines. Reading a bounded prefix is enough to tell them apart and keeps a multi-hundred-
// megabyte trace from being read twice.
TraceFileKind ClassifyTraceFile(const QString &path)
{
  QFile file(path);
  if(!file.open(QIODevice::ReadOnly))
    return TraceFileKind::ChromeJson;

  const QByteArray prefix = file.read(64 * 1024);
  const QByteArray trimmed = prefix.trimmed();

  // A bare top-level array of events is a Chrome trace; the JSON-lines interchange always starts
  // with a '{'.
  if(trimmed.startsWith('['))
    return TraceFileKind::ChromeJson;

  if(trimmed.startsWith('{'))
  {
    if(prefix.contains("\"traceEvents\"") || prefix.contains("\"systemTraceEvents\""))
      return TraceFileKind::ChromeJson;
    return TraceFileKind::JsonLines;
  }

  return TraceFileKind::SystraceText;
}

// Distinct, reasonably colour-blind friendly line colours for the overlay curves.
QColor SeriesColour(int index)
{
  static const QColor palette[] = {
      QColor(0x2e, 0x86, 0xde), QColor(0xd9, 0x53, 0x4f), QColor(0x3c, 0xa8, 0x6a),
      QColor(0xe6, 0x9f, 0x00), QColor(0x8e, 0x44, 0xad), QColor(0x00, 0x99, 0x99),
      QColor(0xb0, 0x6a, 0x00), QColor(0x66, 0x66, 0x66),
  };

  return palette[index % (sizeof(palette) / sizeof(palette[0]))];
}

// ---------------------------------------------------------------------------------------------
// Geometry. These follow the numbers Snapdragon Profiler's own theme declares so the panel keeps the
// density of the Trace view it is modelled on: `.lefttrackhbox` is at least 210px wide and
// `button.trackmetricbutton` is at least 30px tall.
//
// The colours are deliberately *not* declared here. This panel paints with QPainter instead of
// style sheets, so it has to read the theme RenderMax is actually running rather than imposing a
// scheme of its own: RDStyle installs the application palette and the project has no global
// stylesheet, so the widget's palette is the single authority. The accessors live on
// SystemTraceTimeline, mirroring how TimelineBar paints from palette().
// ---------------------------------------------------------------------------------------------
const int kHeaderWidth = 260;
const int kRulerHeight = 26;
const int kScrollBarWidth = 14;
// SDP draws its navigation chips at 24x24; keeping the button the same size means the artwork is
// never resampled and the highlight is exactly as large as the button.
const int kTraceChipSize = 24;
const int kTimeScrollBarHeight = 14;
const int kTimeScrollScale = 1000000;

// Row geometry is a view setting, not a constant: Snapdragon Profiler's Trace view carries a size
// control in its toolbar, and a wide window fits two lines of caption text where a narrow one fits
// one. These are the starting value and the bounds; the live value is SystemTraceTimeline's
// m_RowHeight, and every row - group header, lane, summary row - is exactly that tall. One height
// for every band is what makes a disclosure target easy to hit, and it is what stops the row grid
// from moving around as groups are opened and closed.
const int kRowHeightDefault = 30;
const int kRowHeightMin = 18;
const int kRowHeightMax = 72;
const int kMaxDrawDepth = 8;

// How many lanes of a group are laid out before the rest are summarised in a single "more" row.
// A phone capture produces one lane per thread; painting all of them on first open is exactly the
// wall of information this view has to avoid, so the busiest lanes of each process are shown and
// the remainder is one click away.
const int kLanesPerGroup = 12;

// Mixes two colours, `t` being the weight of `b`. Used for shades the palette has no role for; the
// result follows whichever scheme RDStyle installed instead of being fixed to one.
QColor Blend(const QColor &a, const QColor &b, double t)
{
  const double w = qBound(0.0, t, 1.0);

  return QColor(int(a.red() * (1.0 - w) + b.red() * w),
                int(a.green() * (1.0 - w) + b.green() * w),
                int(a.blue() * (1.0 - w) + b.blue() * w));
}

// A span's colour comes from its own category, so the same category is always the same colour
// within a session without inventing a category table the source never declared.
QColor SpanColour(const QString &category)
{
  if(category.isEmpty())
    return QColor(0x4f, 0x7f, 0xb8);

  const uint hash = qHash(category);
  const int hue = int(hash % 360u);
  const int value = 170 + int((hash >> 9) % 60u);

  return QColor::fromHsv(hue, 105, value);
}

// A round tick step (1, 2 or 5 x 10^n seconds) that keeps ruler labels about 90px apart.
double NiceStep(double span, double pixels)
{
  if(span <= 0.0 || pixels <= 0.0)
    return 1.0;

  const double target = span / qMax(1.0, pixels / 90.0);
  const double magnitude = pow(10.0, floor(log10(target)));
  const double norm = target / magnitude;

  double step = 10.0;
  if(norm <= 1.0)
    step = 1.0;
  else if(norm <= 2.0)
    step = 2.0;
  else if(norm <= 5.0)
    step = 5.0;

  return step * magnitude;
}
}    // namespace

namespace
{
// Everything the timeline draws for one rebuild. Passed by value so the widget never holds a
// reference into the viewer's mutable state; `trace` is non-owning and must outlive the data.
struct SystemTraceTimelineData
{
  const SystemTrace *trace = NULL;
  // Flattened actions for the shown frame, with timeSec relative to the frame origin.
  QVector<TraceActionRef> actions;
  ClockDomainAlignment alignment;
  // True when the two clock domains were related, so the action overlay is meaningful.
  bool aligned = false;
  // Trace-time window occupied by the shown frame (only meaningful when aligned).
  double frameStartSec = 0.0;
  double frameEndSec = 0.0;
  uint32_t frameNumber = 0;
  // Event to draw as the highlighted interval, or 0 when nothing is selected.
  uint32_t highlightEvent = 0;
  // False when no capture is open. The lanes are then drawn on their own time scale with no frame
  // overlay, instead of the panel claiming that nothing has been imported.
  bool captureLoaded = true;
};
}    // namespace

// ---------------------------------------------------------------------------------------------
// The navigation mode buttons. SDP's toolbar artwork is not a glyph on a transparent background:
// each state is a complete 24x24 chip with the glyph baked into it (pointers-active is the grey chip
// of an available tool, pointers-selection the blue chip of the current one). A chip therefore has
// to *be* the button. A plain QToolButton cannot do that - the style insets the icon by its button
// margin, so an 18px icon floats inside a ~30px frame and the blue highlight ends up visibly smaller
// than the thing the mouse is over. Painting is done here instead: the chip fills the whole button
// and hover/press brighten it, because any frame the style added would sit outside the highlight and
// read as a second, larger button.
class TraceModeButton : public QToolButton
{
public:
  explicit TraceModeButton(QWidget *parent) : QToolButton(parent)
  {
    setCheckable(true);
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    setFixedSize(kTraceChipSize, kTraceChipSize);
  }

  // `off` is the chip of an available tool, `on` the chip of the current one.
  void SetChips(const QString &off, const QString &on)
  {
    m_Off = QPixmap(off);
    m_On = QPixmap(on);
    update();
  }

protected:
  void paintEvent(QPaintEvent *) override
  {
    const QPixmap &chip = isChecked() ? m_On : m_Off;

    if(chip.isNull())
      return;

    QPainter p(this);
    p.drawPixmap(rect(), chip);

    if(!isChecked() && (isDown() || underMouse()))
    {
      // SourceAtop keeps the chip's rounded silhouette and only lightens what is already opaque.
      p.setCompositionMode(QPainter::CompositionMode_SourceAtop);
      p.fillRect(rect(), QColor(255, 255, 255, isDown() ? 44 : 24));
    }
  }

private:
  QPixmap m_Off;
  QPixmap m_On;
};

// The two bulk actions at the foot of the track-type list. They are drawn rather than loaded from
// an icon file: the theme's check/indeterminate SVGs only fill half of their 16px view box, so at a
// usable icon size the glyph shrinks to a faint speck. Drawing them keeps the stroke the same
// weight as the checked markers in the list and lets them use the same accent colour.
class TypeBulkButton : public QToolButton
{
public:
  enum Kind
  {
    CheckAll,
    ClearAll,
  };

  TypeBulkButton(Kind kind, QWidget *parent) : QToolButton(parent), m_Kind(kind)
  {
    setAutoRaise(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(24, 24);
  }

protected:
  void paintEvent(QPaintEvent *event) override
  {
    // The style still owns the background, so hover and press come from the theme.
    QToolButton::paintEvent(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPen pen(isEnabled() ? palette().color(QPalette::Highlight)
                         : palette().color(QPalette::Disabled, QPalette::ButtonText),
             2.0);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);

    const QRectF glyph = QRectF(rect()).adjusted(6.0, 7.0, -6.0, -7.0);
    if(m_Kind == CheckAll)
    {
      QPainterPath path;
      path.moveTo(glyph.left(), glyph.center().y());
      path.lineTo(glyph.center().x() - 0.5, glyph.bottom());
      path.lineTo(glyph.right(), glyph.top());
      p.drawPath(path);
    }
    else
    {
      p.drawLine(QPointF(glyph.left(), glyph.center().y()),
                 QPointF(glyph.right(), glyph.center().y()));
    }
  }

private:
  Kind m_Kind;
};

// SystemTraceTimeline — the trace view itself: one lane per process/thread (or per metric), a
// shared zoomable time ruler, and the frame overlay. There is no QtCharts module in this project,
// so every element is drawn directly with QPainter.
//
// The structure and the interactions follow Snapdragon Profiler's Trace view:
//   * a fixed-width caption column (SDP's `.lefttrackhbox`), one row per track, with a group
//     header for each owning process;
//   * counter metrics drawn as curves on their own lane and thread activity drawn as nested span
//     boxes, both against one shared time axis;
//   * three navigation modes — Select (lasso), Pan (drag) and Zoom (drag a region) — matching
//     SDP's "Select, Pan, and Zoom ... switched via the toolbar buttons or the shortcut keys 1, 2,
//     and 3";
//   * wheel zoom, Ctrl+W / Ctrl+S zoom, Ctrl+A / Ctrl+D pan, double-click calipers for measuring a
//     region, and right-click annotations on an element, which add an annotation lane to that
//     element's group.
// ---------------------------------------------------------------------------------------------
class SystemTraceTimeline : public QWidget
{
public:
  SystemTraceTimeline(QWidget *parent) : QWidget(parent)
  {
    setMinimumHeight(260);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_Scroll = new QScrollBar(Qt::Vertical, this);
    m_Scroll->setSingleStep(kRowHeightDefault / 2);

    connect(m_Scroll, &QScrollBar::valueChanged, this, [this](int value) {
      if(m_ScrollY == value)
        return;

      m_ScrollY = value;
      // The bands move under a stationary pointer, so a header that was hovered a moment ago is now
      // a different row.
      m_HoverHeaderRow = -1;
      ApplyCursor();
      update();
    });

    m_TimeScroll = new QScrollBar(Qt::Horizontal, this);
    connect(m_TimeScroll, &QScrollBar::valueChanged, this, [this](int value) {
      if(m_UpdatingTimeScroll || m_Data.trace == NULL)
        return;

      const double total = TotalSpan();
      if(total <= 0.0)
        return;

      m_ViewStartSec = TraceStart() + total * double(value) / double(kTimeScrollScale);
      ClampView();
      update();
    });

    SetNavMode(TraceNavMode::Pan);
  }

  // The parameter is deliberately not named `data`: QWidget declares a member called `data`, so
  // that name would shadow it (C4458).
  QWidget *CreateTypePanel(QWidget *parent)
  {
    QWidget *panel = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->addWidget(new QLabel(QObject::tr("Track types"), panel));
    m_TypeList = new QListWidget(panel);
    m_TypeList->setSpacing(3);
    m_TypeList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_TypeList->setToolTip(QObject::tr("Unchecked types are excluded from timeline layout and painting. Analysis still uses the complete trace."));
    layout->addWidget(m_TypeList);
    const QStringList names = TypeNames();
    for(int i = 0; i < names.size(); ++i)
    {
      QListWidgetItem *item = new QListWidgetItem(names[i], m_TypeList);
      item->setData(Qt::UserRole, i);
      item->setSizeHint(QSize(0, 30));
      item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
      item->setCheckState(Qt::Checked);
    }
    connect(m_TypeList, &QListWidget::itemChanged, this, [this](QListWidgetItem *item) {
      const int type = item->data(Qt::UserRole).toInt();
      if(item->checkState() == Qt::Checked) m_HiddenTypes.remove(type);
      else m_HiddenTypes.insert(type);
      m_Hover = SpanRef();
      m_HoverRow = -1;
      m_HoverSeries = -1;
      m_ScrollY = 0;
      RebuildRows();
      update();
    });
    // Snapdragon Profiler has no all/none pair for track types: its track selection is the
    // "Tracks" dialog, which is a checkbox tree plus a filter box (TrackSelector), and its i18n has
    // no such labels. This bulk shortcut is ours, so it is drawn the way SDP draws its own panel
    // controls: two compact icon-only buttons with tooltips, not full-width text buttons.
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->setSpacing(4);
    buttons->addStretch(1);
    const char *buttonTips[2] = {QT_TR_NOOP("Check every track type"),
                                 QT_TR_NOOP("Uncheck every track type")};
    for(int mode = 0; mode < 2; ++mode)
    {
      TypeBulkButton *button =
          new TypeBulkButton(mode == 0 ? TypeBulkButton::CheckAll : TypeBulkButton::ClearAll, panel);
      button->setToolTip(QObject::tr(buttonTips[mode]));
      buttons->addWidget(button);
      connect(button, &QToolButton::clicked, this, [this, mode]() {
        m_TypeList->blockSignals(true);
        m_HiddenTypes.clear();
        for(int i = 0; i < m_TypeList->count(); ++i)
        {
          m_TypeList->item(i)->setCheckState(mode == 0 ? Qt::Checked : Qt::Unchecked);
          if(mode != 0) m_HiddenTypes.insert(i);
        }
        m_TypeList->blockSignals(false);
        m_Hover = SpanRef();
        m_HoverRow = m_HoverSeries = -1;
        m_ScrollY = 0;
        RebuildRows();
        update();
      });
    }
    layout->addLayout(buttons);
    panel->setMinimumWidth(220);
    panel->setMaximumWidth(350);
    return panel;
  }

  void SetData(const SystemTraceTimelineData &newData)
  {
    const bool traceChanged = newData.trace != m_LastTrace;
    m_Data = newData;
    if(traceChanged)
    {
      m_RowsDirty = true;
      // Group collapse is remembered by name and survives a re-import, as does the filter text.
      m_KnownGroups.clear();
      m_CollapsedGroups.clear();
      m_ExpandedGroups.clear();

      // A different trace means a different time base: the selected window named two instants of the
      // trace that is being replaced, so it is dropped rather than re-meaning something else.
      m_RangeValid = false;
      m_RangeT0 = 0.0;
      m_RangeT1 = 0.0;

      // Measure the trace once, here. Everything else in this class treats the extent as a constant.
      double start = 0.0;
      double end = 0.0;
      if(m_Data.trace != NULL)
        m_Data.trace->TimeExtent(start, end);
      m_TraceStartSec = start;
      m_TraceSpanSec = end > start ? (end - start) : 0.0;
    }
    // The trace object is reassigned in the viewer, but the timeline receives the same member
    // address each time. Rebuilding rows on every selection/hover repaint therefore rescans every
    // span in the trace. Rows only change on a new import, Clear/ResetView, or a new annotation.
    if(m_RowsDirty)
    {
      RebuildRows();
      m_RowsDirty = false;
    }

    // The view window is only reset when the trace changed. A redraw caused by an event or
    // selection change must keep the user's zoom and pan.
    if(m_ResetPending || newData.trace != m_LastTrace)
    {
      m_LastTrace = newData.trace;
      m_ResetPending = false;

      // The window starts at the trace's own first timestamp: a Chrome trace from `traceconv`
      // carries device uptime, so anchoring the view at 0 s would show an empty window several
      // thousand seconds to the left of every lane.
      const double total = TotalSpan();
      m_ViewStartSec = TraceStart();
      m_ViewSpanSec = total > 0.0 ? total : 1.0;
    }

    ClampView();
    update();
  }

  // Drop everything that was drawn. Annotations are kept: they belong to the imported trace, not
  // to the capture being closed, and are cleared by ResetView() when a different trace is loaded.
  void Clear()
  {
    m_Data = SystemTraceTimelineData();
    m_TraceStartSec = 0.0;
    m_TraceSpanSec = 0.0;
    m_RowsDirty = true;
    m_Rows.clear();
    m_CounterSeriesInView.clear();
    m_ContentHeight = 0;
    m_ScrollY = 0;
    m_Hover = SpanRef();
    m_Selected = SpanRef();
    m_HoverRow = -1;
    m_HoverSeries = -1;
    m_HoverText.clear();
    // The window referred to the trace that is being dropped; it is not a window over nothing.
    m_RangeValid = false;
    m_RangeT0 = 0.0;
    m_RangeT1 = 0.0;
    if(OnSelectRange)
      OnSelectRange(false, 0.0, 0.0);
    UpdateScrollBar();
    UpdateTimeScrollBar();
    update();
  }

  // Discard the annotations, the selection and the calipers, and ask for the full trace on the next
  // data update. The window cannot be set here: the viewer reassigns its trace in place and calls
  // this before handing the new data over, so the extent is not yet known to the timeline.
  void ResetView()
  {
    m_Annotations.clear();
    m_Hover = SpanRef();
    m_Selected = SpanRef();

    m_CaliperASet = false;
    m_CaliperBSet = false;

    // A new trace means the window names times from the old one. ResetView has no data yet, so this
    // cannot notify: the viewer's own rebuild clears its panel.
    m_RangeValid = false;
    m_RangeT0 = 0.0;
    m_RangeT1 = 0.0;

    m_ResetPending = true;
    m_LastTrace = NULL;
    m_RowsDirty = true;

    update();
  }

  // The cursor states the mode, except over a row that is itself a control: there the hand says
  // "this band does something when clicked", which is the only affordance a painter-drawn header
  // can offer.
  void ApplyCursor()
  {
    if(m_HoverHeaderRow >= 0)
    {
      setCursor(Qt::PointingHandCursor);
      return;
    }

    switch(m_Mode)
    {
      case TraceNavMode::Pan: setCursor(Qt::OpenHandCursor); break;
      case TraceNavMode::Select:
      case TraceNavMode::Zoom: setCursor(Qt::CrossCursor); break;
    }
  }

  void SetNavMode(TraceNavMode mode)
  {
    m_Mode = mode;
    ApplyCursor();
    update();
  }

  TraceNavMode NavMode() const { return m_Mode; }

  // Height of every row in the lane area. Group headers, lanes and the summary rows all use it, so
  // the timeline is a uniform grid: opening or closing a group removes or adds bands but never
  // resizes the ones that stay, and the empty sliver between two rows that used to swallow clicks
  // cannot exist.
  int RowHeightSetting() const { return m_RowHeight; }

  void SetRowHeight(int height)
  {
    const int clamped = qBound(kRowHeightMin, height, kRowHeightMax);

    if(clamped == m_RowHeight)
      return;

    m_RowHeight = clamped;
    m_Scroll->setSingleStep(qMax(1, m_RowHeight / 2));
    RebuildRows();
    update();
  }

  // --- lane visibility -----------------------------------------------------------------------
  // The importer produces one lane per thread, which on a real phone capture is several hundred
  // lanes for a five second trace. Snapdragon Profiler never presents that set unfiltered: its flow
  // view is populated from a metric catalogue and the user narrows it down. These three controls are
  // the equivalent, applied to the lanes the importer did produce.
  void SetTrackFilter(const QString &text)
  {
    if(m_FilterText == text)
      return;

    m_FilterText = text.trimmed();
    m_ScrollY = 0;
    RebuildRows();
    update();
  }

  // "Active only" hides every lane that has nothing inside the current time window. It is the single
  // biggest reduction in visible information at frame-level zoom, and it costs one binary search per
  // lane when the window changes.
  void SetActiveOnly(bool active)
  {
    if(m_ActiveOnly == active)
      return;

    m_ActiveOnly = active;
    m_ScrollY = 0;
    RebuildRows();
    update();
  }

  bool ActiveOnly() const { return m_ActiveOnly; }

  void SetGroupsCollapsed(bool collapsed)
  {
    m_CollapsedGroups.clear();
    m_ExpandedGroups.clear();

    if(collapsed && m_Data.trace != NULL)
    {
      for(const TraceTrack &track : m_Data.trace->tracks)
        m_CollapsedGroups.insert(GroupOf(track));
    }
    if(m_Data.trace != NULL)
    {
      for(const TraceTrack &track : m_Data.trace->tracks)
        m_KnownGroups.insert(GroupOf(track));
    }

    m_ScrollY = 0;
    RebuildRows();
    update();
  }

  void SelectTrack(int trackIndex)
  {    ScrollToTrack(trackIndex);

    if(trackIndex < 0 || m_Data.trace == NULL || trackIndex >= m_Data.trace->tracks.size())
      return;

    const TraceTrack &track = m_Data.trace->tracks[trackIndex];
    if(!track.spans.isEmpty())
      SelectSpan(trackIndex, 0);
  }

  // --- time-range selection -------------------------------------------------------------------
  // SDP's Selection mode drags out a time window, and its Selection view then reports what the
  // window contains. A counter lane holds sampled measurements rather than elements, so a window is
  // the only thing there is to select on it: the pointer alone can only ever name one sample.
  bool HasRangeSelection() const { return m_RangeValid; }
  double RangeBeginSec() const { return m_RangeT0; }
  double RangeEndSec() const { return m_RangeT1; }

  void ClearRangeSelection(bool notify = true)
  {
    if(!m_RangeValid)
      return;

    m_RangeValid = false;
    m_RangeT0 = 0.0;
    m_RangeT1 = 0.0;

    if(notify && OnSelectRange)
      OnSelectRange(false, 0.0, 0.0);

    update();
  }

  // Series indices of the counter lanes the current layout actually shows, i.e. after the type
  // filter, group collapse and per-group lane cap. A lane the user has filtered out must not
  // reappear in the selection's statistics: the panel reports what is on screen.
  QVector<int> VisibleCounterSeries() const { return m_CounterSeriesInView; }

  // --- theme ---------------------------------------------------------------------------------
  // This widget draws its whole surface with QPainter, so it cannot inherit the theme the way a
  // styled child would. RenderMax installs its palette application-wide through RDStyle and has no
  // global stylesheet, so the widget's own palette is the authority: these accessors are the same
  // idiom TimelineBar uses (`palette().brush(QPalette::Window)` / `Base` / `Text`), which keeps the
  // panel consistent with the rest of the UI and correct in both the light and dark schemes.
  //
  // The roles are chosen to reproduce the layering the panel needs:
  //   * Base / AlternateBase — the lane surfaces, straight from Qt.
  //   * Window — the gutter and ruler chrome, one step off the data area.
  //   * Text / WindowText — labels.
  // A few shades have no palette role (grid lines, dimmed captions), so they are mixed from the
  // roles above rather than hard-coded; that way they follow the scheme too.
  QColor Surface() const { return palette().color(QPalette::Base); }
  QColor SurfaceAlt() const { return palette().color(QPalette::AlternateBase); }
  QColor Chrome() const { return palette().color(QPalette::Window); }
  QColor Grid() const { return Blend(Chrome(), palette().color(QPalette::WindowText), 0.22); }
  QColor Text() const { return palette().color(QPalette::WindowText); }
  QColor DimText() const { return Blend(Text(), Chrome(), 0.42); }
  // Accent is the interaction colour the rest of RenderMax uses for selection and focus.
  QColor Accent() const { return palette().color(QPalette::Highlight); }
  QColor AccentHover() const { return palette().color(QPalette::Highlight).darker(130); }
  // The emphasis colour for calipers, annotations and a selected span's outline: the accent lifted
  // far enough to read against the lane surface, instead of the flat yellow SDP used.
  QColor Highlight() const { return palette().color(QPalette::Highlight).lighter(135); }

  // Called when the user clicks a draw/dispatch marker, with the event id to select.
  std::function<void(uint32_t)> OnSelectEvent;
  // Called when the user selects a span, with the span and its lane caption.
  std::function<void(const TraceSpan &, const QString &)> OnSelectSpan;
  // Called when the mode changed from inside the timeline (the 1/2/3 shortcuts), so the toolbar
  // buttons can follow.
  std::function<void(TraceNavMode)> OnNavModeChanged;
  // Called while the cursor is over a counter lane, with a readable "series = value unit" line, and
  // with an empty string when it leaves. A counter lane is sampled measurement drawn as a curve: it
  // has no elements to click and no axis, so this text is the only way to read a number off it.
  std::function<void(const QString &)> OnHoverReadout;
  // Called when the time-range selection changes: first argument is whether a range is selected,
  // then the window in trace-clock seconds, ascending. Select mode drags out this window; the
  // viewer turns it into the per-counter aggregate SDP shows in its Selection view.
  std::function<void(bool, double, double)> OnSelectRange;

protected:
  void paintEvent(QPaintEvent *) override
  {
    QPainter p(this);
    // The timeline contains many small rectangles. Anti-aliasing makes every one substantially
    // more expensive while adding no information at this zoom level; text and the ruler remain
    // crisp with the raster painter's normal integer coverage.
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), Surface());

    const QRectF rows = RowsRect();

    if(m_Data.trace == NULL)
    {
      p.setPen(DimText());
      p.drawText(rect(), Qt::AlignCenter,
                 QObject::tr("No system trace imported.\nUse \"Open Trace...\" to load one."));
      return;
    }

    if(TotalSpan() <= 0.0)
    {
      p.setPen(DimText());
      p.drawText(rect(), Qt::AlignCenter,
                 QObject::tr("The imported trace contains no timed element to draw."));
      return;
    }

    PaintLanes(p, rows);
    PaintCaptions(p, rows);
    PaintRuler(p, rows);
    PaintOverlays(p, rows);

    if(m_Dragging != DragNone)
      PaintDrag(p, rows);
  }

  void resizeEvent(QResizeEvent *event) override
  {
    QWidget::resizeEvent(event);

    const QRectF rows = RowsRect();
    m_Scroll->setGeometry(int(rows.right()) + 4, int(rows.top()), kScrollBarWidth,
                          qMax(10, int(rows.height())));
    m_TimeScroll->setGeometry(int(rows.left()), int(rows.bottom()) + 4, qMax(10, int(rows.width())),
                              kTimeScrollBarHeight);

    UpdateScrollBar();
    UpdateTimeScrollBar();
  }

  void mousePressEvent(QMouseEvent *event) override
  {
    // Take focus so the 1/2/3 shortcuts work straight after a click on the timeline.
    setFocus(Qt::MouseFocusReason);

    if(event->button() == Qt::RightButton)
    {
      AddAnnotationAt(event->pos());
      event->accept();
      return;
    }

    if(event->button() != Qt::LeftButton || m_Data.trace == NULL)
    {
      QWidget::mousePressEvent(event);
      return;
    }

    const QRectF rows = RowsRect();

    // A group header is a control across its whole width, not just over its caption: the band is the
    // affordance, and requiring the pointer to be in the left column was why collapsing a group felt
    // like it only worked sometimes.
    if(event->pos().y() >= rows.top() && event->pos().y() <= rows.bottom())
    {
      const int rowIndex = RowAt(double(event->pos().y() - rows.top()), 3.0);

      if(IsControlRow(rowIndex))
      {
        HandleCaptionClick(event->pos());
        event->accept();
        return;
      }
    }

    if(!rows.contains(event->pos()))
    {
      QWidget::mousePressEvent(event);
      return;
    }

    // Shift-click closes the caliper pair: the first double-click sets one marker, shift-click the
    // other, and the measured interval is drawn with its duration.
    if(event->modifiers() & Qt::ShiftModifier)
    {
      const double t = TimeAtX(event->pos().x());

      if(!m_CaliperASet)
      {
        m_CaliperA = t;
        m_CaliperASet = true;
      }
      else
      {
        m_CaliperB = t;
        m_CaliperBSet = true;
      }

      update();
      event->accept();
      return;
    }

    m_DragStart = event->pos();
    m_DragCurrent = event->pos();
    m_DragSpanStartSec = m_ViewSpanSec;
    m_DragPanStartSec = m_ViewStartSec;

    switch(m_Mode)
    {
      case TraceNavMode::Select: m_Dragging = DragSelect; break;
      case TraceNavMode::Pan: m_Dragging = DragPan; break;
      case TraceNavMode::Zoom: m_Dragging = DragZoom; break;
    }

    event->accept();
  }

  void mouseMoveEvent(QMouseEvent *event) override
  {
    if(m_Dragging == DragPan)
    {
      const double dx = double(event->pos().x() - m_DragStart.x());
      const double range = qMax(1.0, RowsRect().width());

      m_ViewStartSec = m_DragPanStartSec - (dx / range) * m_DragSpanStartSec;
      ClampView();
      update();
      event->accept();
      return;
    }

    if(m_Dragging == DragSelect || m_Dragging == DragZoom)
    {
      m_DragCurrent = event->pos();
      update();
      event->accept();
      return;
    }

    const SpanRef hit = SpanAt(event->pos());
    if(hit.track != m_Hover.track || hit.span != m_Hover.span)
    {
      m_Hover = hit;
      update();
    }

    UpdateCounterHover(event->pos());

    // Point at the header under the cursor before the click rather than only reacting to it.
    const QRectF rows = RowsRect();
    int headerRow = -1;
    if(event->pos().y() >= rows.top() && event->pos().y() <= rows.bottom())
    {
      const int rowIndex = RowAt(double(event->pos().y() - rows.top()), 2.0);
      if(IsControlRow(rowIndex))
        headerRow = rowIndex;
    }

    if(headerRow != m_HoverHeaderRow)
    {
      m_HoverHeaderRow = headerRow;
      ApplyCursor();
      update();
    }

    QWidget::mouseMoveEvent(event);
  }

  void leaveEvent(QEvent *event) override
  {
    ClearCounterHover();

    if(m_HoverHeaderRow >= 0)
    {
      m_HoverHeaderRow = -1;
      ApplyCursor();
      update();
    }

    QWidget::leaveEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent *event) override
  {
    if(event->button() != Qt::LeftButton || m_Dragging == DragNone)
    {
      QWidget::mouseReleaseEvent(event);
      return;
    }

    const DragMode mode = m_Dragging;
    m_Dragging = DragNone;
    const QRect dragRect = QRect(m_DragStart, event->pos()).normalized();
    const bool moved = dragRect.width() > 3 || dragRect.height() > 3;

    if(mode == DragZoom && moved)
    {
      const double ta = TimeAtX(dragRect.left());
      const double tb = TimeAtX(dragRect.right());

      if(tb - ta > 0.0)
      {
        m_ViewStartSec = ta;
        m_ViewSpanSec = tb - ta;
        ClampView();
      }

      update();
    }
    else if(mode == DragSelect && moved)
    {
      // SDP's Selection mode drags out a time window over the lanes and its Selection view then
      // reports what that window contains. Elements are picked by clicking them; the drag is the
      // only way to ask about a counter lane, which holds measurements rather than elements.
      SetRangeSelection(TimeAtX(dragRect.left()), TimeAtX(dragRect.right()), true);
    }
    else if(!moved)
    {
      // A click that did not move is still a selection, whichever mode is active.
      SelectAt(event->pos());
    }
    else
    {
      update();
    }

    event->accept();
  }

  void mouseDoubleClickEvent(QMouseEvent *event) override
  {
    if(event->button() != Qt::LeftButton || m_Data.trace == NULL)
    {
      QWidget::mouseDoubleClickEvent(event);
      return;
    }

    if(!RowsRect().contains(event->pos()))
      return;

    // "Double click on the timeline ... to set markers at the beginning and end of the region you
    // wish to measure": the first double-click places the start marker, the next one the end.
    const double t = TimeAtX(event->pos().x());

    if(!m_CaliperASet || m_CaliperBSet)
    {
      m_CaliperA = t;
      m_CaliperASet = true;
      m_CaliperBSet = false;
    }
    else
    {
      m_CaliperB = t;
      m_CaliperBSet = true;
    }

    update();
    event->accept();
  }

  void wheelEvent(QWheelEvent *event) override
  {
    if(m_Data.trace == NULL)
    {
      event->ignore();
      return;
    }

    const QRectF rows = RowsRect();
    const int x = event->pos().x();

    // Over the caption column there is no time axis to zoom, so the wheel scrolls the lanes.
    if(x < rows.left() || x > rows.right())
    {
      m_Scroll->setValue(m_Scroll->value() - event->delta() / 4);
      event->accept();
      return;
    }

    const double anchor = TimeAtX(x);
    const double total = TotalSpan();

    // Same zoom-response as the rest of this project (see TimelineBar::wheelEvent) so the two
    // timelines feel identical.
    const double mod = 1.0 + event->delta() / 2500.0f;
    const double span = qMin(total, qMax(total * 1.0e-5, m_ViewSpanSec / mod));

    const double fraction = (x - rows.left()) / qMax(1.0, rows.width());
    m_ViewStartSec = anchor - fraction * span;
    m_ViewSpanSec = span;

    ClampView();
    update();
    event->accept();
  }

  void keyPressEvent(QKeyEvent *event) override
  {
    switch(event->key())
    {
      case Qt::Key_1:
        NotifyNavMode(TraceNavMode::Select);
        event->accept();
        return;
      case Qt::Key_2:
        NotifyNavMode(TraceNavMode::Pan);
        event->accept();
        return;
      case Qt::Key_3:
        NotifyNavMode(TraceNavMode::Zoom);
        event->accept();
        return;
      // SDP's trace panel: F and M zoom to the selection, 0 and Z show the whole trace, Esc clears
      // the selection. Both keys of each pair are accepted so a habit from either tool works; the
      // letters only reach here when the timeline itself has focus, never while typing in a filter.
      case Qt::Key_F:
      case Qt::Key_M:
        ZoomToSelection();
        event->accept();
        return;
      case Qt::Key_0:
      case Qt::Key_Z:
        ResetViewZoom();
        event->accept();
        return;
      case Qt::Key_Escape:
        m_CaliperASet = false;
        m_CaliperBSet = false;
        ClearRangeSelection();
        update();
        event->accept();
        return;
      default: break;
    }

    if(event->modifiers() & Qt::ControlModifier)
    {
      switch(event->key())
      {
        case Qt::Key_A:
          PanBy(-0.25);
          event->accept();
          return;
        case Qt::Key_D:
          PanBy(0.25);
          event->accept();
          return;
        case Qt::Key_W:
          ZoomBy(1.5);
          event->accept();
          return;
        case Qt::Key_S:
          ZoomBy(1.0 / 1.5);
          event->accept();
          return;
        default: break;
      }
    }

    QWidget::keyPressEvent(event);
  }

private:
  struct SpanRef
  {
    int track = -1;
    int span = -1;
    bool valid() const { return track >= 0 && span >= 0; }
  };

  struct Row
  {
    // Index into SystemTrace::tracks, -1 for a group header, an annotation row, or a counter row
    // synthesised from a trace that carried no lanes.
    int track = -1;
    // Prefix maximum of span end times. It lets hit-testing/rendering binary-search the first span
    // that can overlap the visible window, while preserving correctness for an early long span that
    // contains many later short spans.
    QVector<double> maxEndPrefix;
    // Counter value range, computed once when rows are rebuilt instead of rescanning all samples
    // on every paint. `counterRange` distinguishes a real [0,0] range from no counter.
    double counterMin = 0.0;
    double counterMax = 0.0;
    bool counterRange = false;
    // View-space geometry only: hover values/highlights remain live. Rows are recreated on data,
    // filter and layout changes, so cached vertices never outlive their source trace.
    mutable QPolygonF counterPolyline;
    mutable QRectF counterViewport;
    mutable double counterT0 = 0.0, counterT1 = 0.0;
    mutable double cachedMin = 0.0, cachedMax = 0.0;
    mutable bool counterGeometryValid = false;
    mutable QImage spanImage;
    mutable double spanT0 = 0.0, spanT1 = 0.0;
    mutable int spanSelected = -1, spanHovered = -1;
    mutable qint64 spanPaletteKey = 0;
    mutable QFont spanFont;
    // Index into SystemTrace::series for a synthesised counter row, otherwise -1.
    int series = -1;
    int top = 0;
    int height = 0;
    bool header = false;
    bool annotation = false;
    // A group header can be collapsed to hide its lanes. Collapsing removes bands from the grid; it
    // does not resize the bands that remain.
    bool collapsed = false;
    int trackCount = 0;
    int visibleTrackCount = 0;
    // Non-zero on the summary row that stands in for the lanes a group did not lay out.
    int moreCount = 0;
    // Per-nesting-level geometry, fitted into the row instead of the row growing with it.
    int spanStep = 0;
    int spanHeight = 0;
    QString group;
    QString annotationGroup;
  };

  struct Annotation
  {
    double beginSec = 0.0;
    QString text;
    // The owning process group, so the annotation lane lands in the group of the element that was
    // annotated instead of appearing somewhere unrelated.
    QString group;
  };

  enum DragMode
  {
    DragNone,
    DragSelect,
    DragPan,
    DragZoom,
  };

  // The trace's extent, computed once per SetData() rather than on demand. TimeExtent() walks every
  // span of every track and every sample of every series, and these accessors sit on the hottest path
  // in the widget: XForTime() is called twice per span drawn, and RowsRect() needs TotalSpan() to
  // decide the time scroll bar. Computing it on demand made a single repaint of a one-second phone
  // trace take minutes - the scan was repeated for every span on screen.
  double m_TraceStartSec = 0.0;
  double m_TraceSpanSec = 0.0;

  double TotalSpan() const { return m_TraceSpanSec; }

  // The trace's own time base. Only the extent is drawn: the absolute offset (device uptime) must
  // not be assumed to be zero, or the lanes fall outside the view window.
  double TraceStart() const { return m_TraceStartSec; }

  bool TimeScrollVisible() const
  {
    const double total = TotalSpan();
    return total > 0.0 && m_ViewSpanSec < total - qMax(1.0e-12, total * 1.0e-9);
  }

  QRectF RowsRect() const
  {
    const double left = kHeaderWidth;
    const double top = kRulerHeight;
    const double w = qMax(40.0, double(width() - kHeaderWidth - kScrollBarWidth - 6));
    const int timeScrollSpace = TimeScrollVisible() ? kTimeScrollBarHeight + 4 : 0;
    const double h = qMax(40.0, double(height() - kRulerHeight - 2 - timeScrollSpace));

    return QRectF(left, top, w, h);
  }

  double XForTime(double t) const
  {
    const QRectF rows = RowsRect();
    const double span = qMax(1.0e-9, m_ViewSpanSec);

    return rows.left() + ((t - m_ViewStartSec) / span) * rows.width();
  }

  double TimeAtX(double x) const
  {
    const QRectF rows = RowsRect();
    const double span = qMax(1.0e-9, m_ViewSpanSec);

    return m_ViewStartSec + ((x - rows.left()) / qMax(1.0, rows.width())) * span;
  }

  void ClampView()
  {
    const double total = TotalSpan();

    if(total <= 0.0)
    {
      m_ViewStartSec = 0.0;
      m_ViewSpanSec = 1.0;
      return;
    }

    // Never zoom past a ten-thousandth of the trace: below that the ruler step degenerates.
    m_ViewSpanSec = qMin(total, qMax(total * 1.0e-4, m_ViewSpanSec));

    const double start = TraceStart();
    m_ViewStartSec = qMin(qMax(start, m_ViewStartSec), start + total - m_ViewSpanSec);

    // With "active only" on, the set of lanes that have something to show depends on the window, so
    // every pan and zoom has to re-evaluate it. The test is one binary search per lane, not a span
    // scan, because the per-lane prefix index is built once per trace.
    if(m_ActiveOnly && m_Data.trace != NULL)
      RebuildRows();

    UpdateTimeScrollBar();
  }

  void PanBy(double fraction)
  {
    m_ViewStartSec += fraction * m_ViewSpanSec;
    ClampView();
    update();
  }

  void ZoomBy(double factor)
  {
    const double centre = m_ViewStartSec + m_ViewSpanSec * 0.5;

    m_ViewSpanSec = m_ViewSpanSec / factor;
    m_ViewStartSec = centre - m_ViewSpanSec * 0.5;

    ClampView();
    update();
  }

  void NotifyNavMode(TraceNavMode mode)
  {
    SetNavMode(mode);

    if(OnNavModeChanged)
      OnNavModeChanged(mode);
  }

  static QStringList TypeNames()
  {
    return {QObject::tr("Frames and markers"), QObject::tr("CPU scheduling"),
            QObject::tr("CPU metrics"), QObject::tr("GPU execution"),
            QObject::tr("GPU metrics"), QObject::tr("Memory and bandwidth"),
            QObject::tr("Power and temperature"), QObject::tr("Processes and threads"),
            QObject::tr("Other metrics")};
  }

  static int MetricType(const QString &name)
  {
    const QString n = name.toLower();
    if(n.contains(lit("bandwidth")) || n.contains(lit("ddr")) || n.contains(lit("memory"))) return 5;
    if(n.contains(lit("thermal")) || n.contains(lit("temperature")) ||
       n.contains(lit("power")) || n.contains(lit("voltage")) || n.contains(lit("battery"))) return 6;
    if(n.contains(lit("gpu")) || n.contains(lit("kgsl")) || n.contains(lit("adreno"))) return 4;
    if(n.contains(lit("cpu"))) return 2;
    if(n.contains(lit("frame"))) return 0;
    return 8;
  }

  static int TrackType(const TraceTrack &track)
  {
    if(track.kind == TraceTrackKind::Counter) return MetricType(track.group + lit(" ") + track.name);
    if(track.atrace) return 0;
    if(track.kind == TraceTrackKind::Cpu) return 1;
    const QString n = (track.group + lit(" ") + track.name).toLower();
    if(n.contains(lit("kgsl")) || n.contains(lit("gpu")) || n.contains(lit("adreno"))) return 3;
    return 7;
  }

  QString GroupOf(const TraceTrack &track) const
  {
    const QString owner = track.group.isEmpty() ? QObject::tr("Ungrouped") : track.group;
    return TypeNames()[TrackType(track)] + QStringLiteral(" / ") + owner;
  }

  QString CaptionOf(int trackIndex) const
  {
    if(m_Data.trace == NULL || trackIndex < 0 || trackIndex >= m_Data.trace->tracks.size())
      return QString();

    return m_Data.trace->tracks[trackIndex].name;
  }

  bool GroupHasAnnotations(const QString &group) const
  {
    for(const Annotation &a : m_Annotations)
    {
      if(a.group == group)
        return true;
    }

    return false;
  }

  void RebuildRows()
  {
    m_Rows.clear();
    m_CounterSeriesInView.clear();
    m_ContentHeight = 0;

    if(m_Data.trace == NULL)
    {
      UpdateScrollBar();
      return;
    }

    const SystemTrace &trace = *m_Data.trace;
    if(m_TypeList)
    {
      QVector<int> counts(TypeNames().size(), 0);
      if(!trace.tracks.isEmpty())
        for(const TraceTrack &track : trace.tracks) ++counts[TrackType(track)];
      else
        for(const TraceSeries &series : trace.series) ++counts[MetricType(series.name)];
      m_TypeList->blockSignals(true);
      for(int i = 0; i < counts.size(); ++i)
      {
        QListWidgetItem *item = m_TypeList->item(i);
        item->setText(TypeNames()[i] + QStringLiteral(" (%1)").arg(counts[i]));
        Qt::ItemFlags flags = item->flags();
        if(counts[i] > 0) flags |= Qt::ItemIsEnabled;
        else flags &= ~Qt::ItemIsEnabled;
        item->setFlags(flags);
      }
      m_TypeList->blockSignals(false);
    }
    EnsureTrackIndex();
    int y = 0;

    if(!trace.tracks.isEmpty())
    {
      // Group the lanes by their owning process, keeping the importer's order, so the captions
      // read as SDP's per-process track groups.
      QVector<QString> groupOrder;
      QHash<QString, QVector<int>> groupTracks;

      for(int i = 0; i < trace.tracks.size(); i++)
      {
        if(m_HiddenTypes.contains(TrackType(trace.tracks[i])))
          continue;
        const QString group = GroupOf(trace.tracks[i]);

        if(!groupTracks.contains(group))
          groupOrder.push_back(group);

        groupTracks[group].push_back(i);
      }

      const double t0 = m_ViewStartSec;
      const double t1 = t0 + m_ViewSpanSec;

      for(const QString &group : groupOrder)
      {
        // Lanes are ordered by weight, busiest first, before anything is filtered out: a group's
        // cap must not reshuffle as the time window moves, or the lanes would jump around while
        // panning. Weight is the lane's own element count, so the threads that actually did work in
        // the capture are the ones that survive the cap.
        QVector<int> ranked = groupTracks.value(group);
        std::stable_sort(ranked.begin(), ranked.end(), [&trace](int a, int b) {
          return TrackWeight(trace.tracks[a]) > TrackWeight(trace.tracks[b]);
        });

        // Filtering and the activity test happen per lane, before any row is emitted: a hidden
        // lane costs nothing to paint, and a group with nothing left disappears entirely instead
        // of leaving an empty header behind.
        QVector<int> visible;
        for(int trackIndex : ranked)
        {
          const TraceTrack &track = trace.tracks[trackIndex];

          if(!TrackMatchesFilter(track, group))
            continue;

          if(m_ActiveOnly && !TrackOverlapsView(trackIndex, t0, t1))
            continue;

          visible.push_back(trackIndex);
        }

        if(visible.isEmpty())
          continue;

        // Every counter lane the filters let through, whether or not the group is open and
        // whether or not the lane cap hides it: a time window summarises the counters the user has
        // asked to see, and folding a group away is a layout choice, not a data filter.
        for(int trackIndex : visible)
        {
          const TraceTrack &track = trace.tracks[trackIndex];
          if(track.kind == TraceTrackKind::Counter && track.counterSeries >= 0 &&
             track.counterSeries < trace.series.size() &&
             !m_CounterSeriesInView.contains(track.counterSeries))
            m_CounterSeriesInView.push_back(track.counterSeries);
        }

        if(!m_KnownGroups.contains(group))
        {
          // Only the first visible group starts expanded. Remember subsequent user toggles.
          if(!m_KnownGroups.isEmpty()) m_CollapsedGroups.insert(group);
          m_KnownGroups.insert(group);
        }
        const bool collapsed = m_CollapsedGroups.contains(group);

        Row header;
        header.header = true;
        header.group = group;
        header.collapsed = collapsed;
        header.trackCount = groupTracks.value(group).size();
        header.visibleTrackCount = visible.size();
        header.top = y;
        header.height = m_RowHeight;
        m_Rows.push_back(header);
        y += m_RowHeight;

        if(collapsed)
          continue;

        // The cap only applies until the user asks for the whole group; when it does apply, the
        // omitted lanes are reported in one row rather than silently dropped.
        int moreCount = 0;
        if(!m_ExpandedGroups.contains(group) && visible.size() > kLanesPerGroup)
        {
          moreCount = visible.size() - kLanesPerGroup;
          visible.resize(kLanesPerGroup);
        }

        for(int trackIndex : visible)
        {
          const TraceTrack &track = trace.tracks[trackIndex];
          const int maxDepth = m_TrackMaxDepth.value(trackIndex, 0);

          Row row;
          row.track = trackIndex;
          // The nesting is fitted into the row rather than the row growing with it. A lane that
          // changed height when it was opened moved every lane below it, so the next click landed
          // somewhere else; a lane that is one fixed band is also one fixed hit target. Levels too
          // deep to draw inside the band are drawn thinner and clipped to it - raise the row height
          // to see more of them.
          const int levels = qMin(kMaxDrawDepth, maxDepth) + 1;
          row.spanStep = qBound(5, (m_RowHeight - 2) / levels, qMax(7, m_RowHeight - 10));
          row.spanHeight = qMax(4, row.spanStep - 2);
          row.top = y;
          row.height = m_RowHeight;

          if(track.kind == TraceTrackKind::Counter && track.counterSeries >= 0 &&
             track.counterSeries < trace.series.size())
          {
            row.counterMin = trace.series[track.counterSeries].MinValue();
            row.counterMax = trace.series[track.counterSeries].MaxValue();
            row.counterRange = true;
          }
          else if(track.kind != TraceTrackKind::Counter && trackIndex < m_TrackMaxEnd.size())
          {
            row.maxEndPrefix = m_TrackMaxEnd[trackIndex];
          }

          m_Rows.push_back(row);
          y += row.height;
        }

        if(moreCount > 0)
        {
          Row row;
          row.moreCount = moreCount;
          row.group = group;
          row.top = y;
          row.height = m_RowHeight;
          m_Rows.push_back(row);
          y += m_RowHeight;
        }

        if(GroupHasAnnotations(group))
        {
          Row row;
          row.annotation = true;
          row.annotationGroup = group;
          row.top = y;
          row.height = m_RowHeight;
          m_Rows.push_back(row);
          y += m_RowHeight;
        }
      }
    }
    else
    {
      // A trace that carried only counters (the JSON-lines interchange) has no lanes, so one row
      // per series is synthesised rather than showing an empty timeline.
      for(int i = 0; i < trace.series.size(); i++)
      {
        const int type = MetricType(trace.series[i].name);
        if(m_HiddenTypes.contains(type)) continue;
        const QString group = TypeNames()[type] + QStringLiteral(" / ") + trace.series[i].name;
        if(!m_KnownGroups.contains(group))
        {
          if(!m_KnownGroups.isEmpty()) m_CollapsedGroups.insert(group);
          m_KnownGroups.insert(group);
        }
        Row header;
        header.header = true;
        header.group = group;
        header.collapsed = m_CollapsedGroups.contains(group);
        header.trackCount = header.visibleTrackCount = 1;
        header.top = y;
        header.height = m_RowHeight;
        m_Rows.push_back(header);
        y += m_RowHeight;
        if(header.collapsed) continue;
        Row row;
        row.series = i;
        row.top = y;
        row.height = m_RowHeight;
        row.counterMin = trace.series[i].MinValue();
        row.counterMax = trace.series[i].MaxValue();
        row.counterRange = true;
        m_Rows.push_back(row);
        y += m_RowHeight;
      }
    }

    m_ContentHeight = y;
    // Row indices change under the cursor, so a remembered hovered header would point at a
    // different band than the one the pointer is over.
    m_HoverHeaderRow = -1;
    UpdateScrollBar();
  }

  // Per-lane lookup tables, built once per imported trace rather than per layout pass: a prefix
  // maximum of span end times (for binary-searched overlap tests) and the deepest nesting level.
  void EnsureTrackIndex()
  {
    if(m_Data.trace == NULL)
      return;

    const SystemTrace &trace = *m_Data.trace;

    if(m_IndexedTrace == m_Data.trace && m_TrackMaxEnd.size() == trace.tracks.size())
      return;

    m_IndexedTrace = m_Data.trace;
    m_TrackMaxEnd.clear();
    m_TrackMaxDepth.clear();
    m_TrackMaxEnd.resize(trace.tracks.size());
    m_TrackMaxDepth.resize(trace.tracks.size());

    for(int i = 0; i < trace.tracks.size(); i++)
    {
      const TraceTrack &track = trace.tracks[i];
      int maxDepth = 0;

      if(track.kind != TraceTrackKind::Counter)
      {
        m_TrackMaxEnd[i].reserve(track.spans.size());
        double maxEnd = -HUGE_VAL;

        for(const TraceSpan &span : track.spans)
        {
          maxEnd = qMax(maxEnd, span.endSec);
          maxDepth = qMax(maxDepth, span.depth);
          m_TrackMaxEnd[i].push_back(maxEnd);
        }
      }

      m_TrackMaxDepth[i] = maxDepth;
    }
  }

  // Does this lane contribute anything to the visible time window? Spans are sorted by begin time
  // and the prefix index is monotone, so the first prefix entry that reaches the window's left edge
  // is the only candidate that has to be checked: if it starts after the right edge, nothing later
  // can overlap either.
  bool TrackOverlapsView(int trackIndex, double t0, double t1) const
  {
    if(m_Data.trace == NULL || trackIndex < 0 || trackIndex >= m_Data.trace->tracks.size())
      return false;

    const TraceTrack &track = m_Data.trace->tracks[trackIndex];

    if(track.kind == TraceTrackKind::Counter)
    {
      if(track.counterSeries < 0 || track.counterSeries >= m_Data.trace->series.size())
        return false;

      const TraceSeries &series = m_Data.trace->series[track.counterSeries];
      if(series.samples.isEmpty())
        return false;

      return series.samples.first().timeSec <= t1 && series.samples.last().timeSec >= t0;
    }

    if(trackIndex >= m_TrackMaxEnd.size() || m_TrackMaxEnd[trackIndex].isEmpty())
      return false;

    const QVector<double> &prefix = m_TrackMaxEnd[trackIndex];
    const auto it = std::lower_bound(prefix.constBegin(), prefix.constEnd(), t0);

    if(it == prefix.constEnd())
      return false;

    const int candidate = int(it - prefix.constBegin());

    // A sleeping span covers most of a thread's life, so counting it would make nearly every lane
    // look active in every window and defeat the whole point of the toggle. "Active" means the
    // thread did something in this window: ran, waited to run, or logged an atrace slice.
    for(int i = candidate; i < track.spans.size() && track.spans[i].beginSec <= t1; i++)
    {
      const TraceSpan &span = track.spans[i];

      if(span.endSec < t0)
        continue;

      if(span.category == QLatin1String("sched-sleeping"))
        continue;

      return true;
    }

    return false;
  }

  // The size of a lane, used to decide which lanes of a group are shown before the rest are folded
  // into a summary row. Counters weigh their sample count, span lanes their element count.
  // Busiest first, by element count. The idle lane counts as nothing: it is not work, it is the
  // absence of it, and ranking it by span count would push the threads that did work out of the
  // group's cap on every multi-core trace.
  static int TrackWeight(const TraceTrack &track) { return track.idle ? 0 : track.spans.size(); }

  // Substring match on the lane caption and its group, which is what a user types when looking for
  // one thread or one process. A leading '-' or '!' negates the term, so "-idle" drops the idle
  // lanes from an otherwise complete view.
  bool TrackMatchesFilter(const TraceTrack &track, const QString &group) const
  {
    if(m_FilterText.isEmpty())
      return true;

    const QStringList terms = m_FilterText.split(QLatin1Char(' '), QString::SkipEmptyParts);

    for(const QString &raw : terms)
    {
      const bool negated = raw.startsWith(QLatin1Char('-')) || raw.startsWith(QLatin1Char('!'));
      const QString term = negated ? raw.mid(1) : raw;

      if(term.isEmpty())
        continue;

      const bool match = track.name.contains(term, Qt::CaseInsensitive) ||
                         group.contains(term, Qt::CaseInsensitive);

      if(match == negated)
        return false;
    }

    return true;
  }

  // The row index of a lane's track, or -1 when the lane is currently hidden by a collapsed group
  // or the filter. Used by the Tracks list to jump to a lane the user picked there.
  int RowForTrack(int trackIndex) const
  {
    for(int i = 0; i < m_Rows.size(); i++)
    {
      if(m_Rows[i].track == trackIndex && !m_Rows[i].header)
        return i;
    }

    return -1;
  }

  void ScrollToTrack(int trackIndex)
  {
    const int row = RowForTrack(trackIndex);

    if(row < 0)
      return;

    const int viewport = qMax(0, int(RowsRect().height()));
    const int top = m_Rows[row].top;

    if(top < m_ScrollY || top + m_Rows[row].height > m_ScrollY + viewport)
    {
      m_ScrollY = qMax(0, top - viewport / 3);
      UpdateScrollBar();
    }

    update();
  }

  void UpdateScrollBar()
  {
    const int viewport = qMax(0, int(RowsRect().height()));
    const int maxValue = qMax(0, m_ContentHeight - viewport);

    m_Scroll->setRange(0, maxValue);
    m_Scroll->setPageStep(qMax(1, viewport));
    m_Scroll->setSingleStep(qMax(1, m_RowHeight / 2));
    m_Scroll->setVisible(maxValue > 0);

    m_ScrollY = qBound(0, m_ScrollY, maxValue);

    if(m_Scroll->value() != m_ScrollY)
      m_Scroll->setValue(m_ScrollY);
  }

  void UpdateTimeScrollBar()
  {
    const double total = TotalSpan();
    const bool visible = TimeScrollVisible();

    m_UpdatingTimeScroll = true;
    m_TimeScroll->setVisible(visible);

    if(visible)
    {
      const int page = qMax(1, int(double(kTimeScrollScale) * m_ViewSpanSec / total));
      const int maximum = qMax(0, kTimeScrollScale - page);
      const int value = qBound(
          0, int(double(kTimeScrollScale) * (m_ViewStartSec - TraceStart()) / total + 0.5), maximum);

      m_TimeScroll->setRange(0, maximum);
      m_TimeScroll->setPageStep(page);
      m_TimeScroll->setSingleStep(qMax(1, page / 20));
      m_TimeScroll->setValue(value);

      const QRectF rows = RowsRect();
      m_TimeScroll->setGeometry(int(rows.left()), int(rows.bottom()) + 4,
                                qMax(10, int(rows.width())), kTimeScrollBarHeight);
    }

    // Showing or hiding the time scrollbar changes the lane viewport height. Keep the vertical
    // scrollbar flush with that viewport immediately rather than waiting for a window resize.
    const QRectF rows = RowsRect();
    m_Scroll->setGeometry(int(rows.right()) + 4, int(rows.top()), kScrollBarWidth,
                          qMax(10, int(rows.height())));

    m_UpdatingTimeScroll = false;
    UpdateScrollBar();
  }

  // The value a series holds at `t`: its most recent sample at or before that instant, plus the
  // timestamp of that sample. A level series must never be interpolated - that would report a
  // frequency the device never ran at - and for a sampled series the enclosing sample is the
  // measurement that actually exists.
  bool SampleHeldValue(const TraceSeries &series, double t, double &value,
                       double &sampleTimeSec) const
  {
    if(series.samples.isEmpty())
      return false;

    if(t <= series.samples.first().timeSec)
    {
      value = series.samples.first().value;
      sampleTimeSec = series.samples.first().timeSec;
      return true;
    }

    int lo = 0;
    int hi = series.samples.size() - 1;
    int best = 0;

    while(lo <= hi)
    {
      const int mid = (lo + hi) / 2;

      if(series.samples[mid].timeSec <= t)
      {
        best = mid;
        lo = mid + 1;
      }
      else
      {
        hi = mid - 1;
      }
    }

    value = series.samples[best].value;
    sampleTimeSec = series.samples[best].timeSec;
    return true;
  }

  void ClearCounterHover()
  {
    if(m_HoverRow < 0 && m_HoverSeries < 0 && m_HoverText.isEmpty())
      return;

    m_HoverRow = -1;
    m_HoverSeries = -1;
    m_HoverText.clear();

    if(OnHoverReadout)
      OnHoverReadout(QString());

    update();
  }

  // Resolves the counter lane under the cursor into a number. Repainting happens only when the
  // held sample changes, not on every mouse move: the readout describes a sample, so a new pixel
  // inside the same held interval cannot change it and would only cost a full timeline repaint.
  void UpdateCounterHover(const QPoint &pos)
  {
    const QRectF rows = RowsRect();

    if(m_Data.trace == NULL || !rows.contains(pos))
    {
      ClearCounterHover();
      return;
    }

    const SystemTrace &trace = *m_Data.trace;
    const int rowIndex = RowAt(double(pos.y() - rows.top()));

    if(rowIndex < 0 || rowIndex >= m_Rows.size())
    {
      ClearCounterHover();
      return;
    }

    const Row &row = m_Rows[rowIndex];
    int seriesIndex = -1;

    if(row.series >= 0)
      seriesIndex = row.series;
    else if(row.track >= 0 && row.track < trace.tracks.size() &&
            trace.tracks[row.track].kind == TraceTrackKind::Counter)
      seriesIndex = trace.tracks[row.track].counterSeries;

    double value = 0.0;
    double sampleTimeSec = 0.0;

    if(seriesIndex < 0 || seriesIndex >= trace.series.size() ||
       !SampleHeldValue(trace.series[seriesIndex], TimeAtX(pos.x()), value, sampleTimeSec))
    {
      ClearCounterHover();
      return;
    }

    const TraceSeries &series = trace.series[seriesIndex];

    const QString text =
        series.unit.isEmpty()
            ? QObject::tr("%1 = %2   (sample at %3)")
                  .arg(series.name, FormatValue(value), FormatSeconds(sampleTimeSec))
            : QObject::tr("%1 = %2 %3   (sample at %4)")
                  .arg(series.name, FormatValue(value), series.unit,
                       FormatSeconds(sampleTimeSec));

    const bool rowChanged = rowIndex != m_HoverRow || seriesIndex != m_HoverSeries;
    const bool textChanged = text != m_HoverText;

    m_HoverRow = rowIndex;
    m_HoverSeries = seriesIndex;
    m_HoverTimeSec = sampleTimeSec;
    m_HoverValue = value;
    m_HoverText = text;

    if(textChanged && OnHoverReadout)
      OnHoverReadout(text);

    if(rowChanged || textChanged)
      update();
  }

  SpanRef SpanAt(const QPoint &pos) const
  {
    SpanRef ref;

    if(m_Data.trace == NULL)
      return ref;

    const QRectF rows = RowsRect();
    if(!rows.contains(pos))
      return ref;

    const double t = TimeAtX(pos.x());
    const double contentY = double(pos.y() - rows.top()) + m_ScrollY;

    for(const Row &row : m_Rows)
    {
      if(row.header || row.annotation || row.track < 0)
        continue;

      if(contentY < row.top || contentY >= row.top + row.height)
        continue;

      if(row.track >= m_Data.trace->tracks.size())
        continue;

      const TraceTrack &track = m_Data.trace->tracks[row.track];

      int first = 0;
      if(!row.maxEndPrefix.isEmpty())
      {
        const auto it = std::lower_bound(row.maxEndPrefix.constBegin(), row.maxEndPrefix.constEnd(), t);
        first = int(it - row.maxEndPrefix.constBegin());
      }

      for(int i = first; i < track.spans.size(); i++)
      {
        const TraceSpan &span = track.spans[i];

        // Spans are sorted by begin time, so nothing further right can contain the cursor.
        if(span.beginSec > t)
          break;

        if(t <= span.endSec)
        {
          // Prefer the innermost span under the cursor: that is the element the user pointed at.
          if(!ref.valid() || span.depth >= m_Data.trace->tracks[ref.track].spans[ref.span].depth)
          {
            ref.track = row.track;
            ref.span = i;
          }
        }
      }
    }

    return ref;
  }

  // Row index under a y position in the lane area, -1 when the point is past the last row. `slop`
  // widens each band by that many pixels on both sides, so a click that lands on the hairline
  // separator still belongs to the row the user was aiming at instead of to nothing.
  int RowAt(double y, double slop = 0.0) const
  {
    const double contentY = y + m_ScrollY;

    int nearest = -1;
    double nearestDistance = slop;

    for(int i = 0; i < m_Rows.size(); i++)
    {
      const double top = m_Rows[i].top;
      const double bottom = top + m_Rows[i].height;

      if(contentY >= top && contentY < bottom)
        return i;

      if(slop <= 0.0)
        continue;

      const double distance = qMin(qAbs(contentY - top), qAbs(contentY - bottom));
      if(distance <= nearestDistance)
      {
        nearestDistance = distance;
        nearest = i;
      }
    }

    return nearest;
  }

  // True when the row at this index is a control: a group header toggles its lanes and the summary
  // row of a capped group lays out the rest. Both answer to a click anywhere across the band.
  bool IsControlRow(int index) const
  {
    if(index < 0 || index >= m_Rows.size())
      return false;

    return m_Rows[index].header || m_Rows[index].moreCount > 0;
  }

  void HandleCaptionClick(const QPoint &pos)
  {
    const int index = RowAt(double(pos.y() - RowsRect().top()), 3.0);

    if(index < 0 || index >= m_Rows.size())
      return;

    const Row row = m_Rows[index];

    if(row.header)
    {
      // Anywhere on a group header collapses or expands it, and the whole width of the band is the
      // target: the header is a control, not a label.
      if(m_CollapsedGroups.contains(row.group))
        m_CollapsedGroups.remove(row.group);
      else
        m_CollapsedGroups.insert(row.group);
    }
    else if(row.moreCount > 0)
    {
      m_ExpandedGroups.insert(row.group);
    }
    else
    {
      return;
    }

    RebuildRows();

    // The rows moved under the pointer, so the band it is over is a different one now (and may not
    // be a control at all). Recompute rather than leaving the hand cursor on a lane.
    const int hovered = RowAt(double(pos.y() - RowsRect().top()), 2.0);
    m_HoverHeaderRow = IsControlRow(hovered) ? hovered : -1;
    ApplyCursor();
    update();
  }

  void SetRangeSelection(double ta, double tb, bool notify)
  {
    const double viewEnd = m_ViewStartSec + m_ViewSpanSec;
    const double lo = qBound(m_ViewStartSec, qMin(ta, tb), viewEnd);
    const double hi = qBound(m_ViewStartSec, qMax(ta, tb), viewEnd);

    // A window that spans no time cannot hold a sample, so an empty drag clears the selection rather
    // than leaving a zero-width band that reports "no samples in range".
    if(hi <= lo)
    {
      ClearRangeSelection(notify);
      return;
    }

    m_RangeValid = true;
    m_RangeT0 = lo;
    m_RangeT1 = hi;

    if(notify && OnSelectRange)
      OnSelectRange(true, lo, hi);

    update();
  }

  // Bring the selected window (or the selected element, when no window is selected) to fill the
  // plot. SDP binds this to F and M; keeping both means a user's habit from either tool works.
  void ZoomToSelection()
  {
    if(m_RangeValid)
    {
      m_ViewStartSec = m_RangeT0;
      m_ViewSpanSec = qMax(1.0e-6, m_RangeT1 - m_RangeT0);
      ClampView();
      update();
      return;
    }

    if(m_Data.trace == NULL || !m_Selected.valid())
      return;

    if(m_Selected.track >= m_Data.trace->tracks.size())
      return;

    const TraceTrack &track = m_Data.trace->tracks[m_Selected.track];
    if(m_Selected.span < 0 || m_Selected.span >= track.spans.size())
      return;

    const TraceSpan &element = track.spans[m_Selected.span];

    m_ViewStartSec = element.beginSec;
    m_ViewSpanSec = qMax(1.0e-6, element.endSec - element.beginSec);
    ClampView();
    update();
  }

  // Show the whole trace without discarding the annotations or the element selection, unlike
  // ResetView() which is the "different trace" path. Bound to 0 and Z in SDP.
  void ResetViewZoom()
  {
    const double total = TotalSpan();

    m_ViewStartSec = TraceStart();
    m_ViewSpanSec = total > 0.0 ? total : 1.0;

    ClampView();
    update();
  }

  void SelectAt(const QPoint &pos)
  {
    const SpanRef hit = SpanAt(pos);

    if(hit.valid())
    {
      SelectSpan(hit.track, hit.span);
      return;
    }

    SelectNearestAction(pos);
  }

  void SelectSpan(int trackIndex, int spanIndex)
  {
    m_Selected.track = trackIndex;
    m_Selected.span = spanIndex;
    update();

    if(m_Data.trace == NULL || trackIndex < 0 || trackIndex >= m_Data.trace->tracks.size())
      return;

    const TraceTrack &track = m_Data.trace->tracks[trackIndex];
    if(spanIndex < 0 || spanIndex >= track.spans.size())
      return;

    if(OnSelectSpan)
      OnSelectSpan(track.spans[spanIndex], track.name);
  }

  void SelectNearestAction(const QPoint &pos)
  {
    if(!m_Data.aligned || m_Data.frameEndSec <= m_Data.frameStartSec)
      return;

    const double clicked = TimeAtX(pos.x());

    // Only actions with real timing can be placed, so an action without a duration is never
    // selected here.
    double bestDistance = 0.0;
    uint32_t bestEvent = 0;

    for(const TraceActionRef &action : m_Data.actions)
    {
      if(!action.hasGpuDuration)
        continue;

      const double t = m_Data.frameStartSec + action.timeSec;
      const double distance = fabs(t - clicked);

      if(bestEvent == 0 || distance < bestDistance)
      {
        bestDistance = distance;
        bestEvent = action.eventId;
      }
    }

    if(bestEvent != 0 && OnSelectEvent)
      OnSelectEvent(bestEvent);
  }

  void AddAnnotationAt(const QPoint &pos)
  {
    if(m_Data.trace == NULL)
      return;

    if(!RowsRect().contains(pos))
      return;

    const SpanRef hit = SpanAt(pos);

    QString group = QObject::tr("Ungrouped");
    double t = TimeAtX(pos.x());

    if(hit.valid())
    {
      const TraceTrack &track = m_Data.trace->tracks[hit.track];
      group = GroupOf(track);

      if(hit.span < track.spans.size())
        t = track.spans[hit.span].beginSec;
    }
    else if(!m_Data.trace->tracks.isEmpty())
    {
      group = GroupOf(m_Data.trace->tracks[0]);
    }

    bool ok = false;
    const QString text = QInputDialog::getText(this, QObject::tr("Add annotation"),
                                               QObject::tr("Annotation:"), QLineEdit::Normal,
                                               QString(), &ok);

    if(!ok || text.isEmpty())
      return;

    Annotation annotation;
    annotation.beginSec = t;
    annotation.text = text;
    annotation.group = group;
    m_Annotations.push_back(annotation);

    m_RowsDirty = true;
    RebuildRows();
    m_RowsDirty = false;
    update();
  }

  // --- painting ---------------------------------------------------------------------------

  void PaintLanes(QPainter &p, const QRectF &rows)
  {
    const SystemTrace &trace = *m_Data.trace;
    const double rightEdge = rows.right();
    const double t0 = m_ViewStartSec;
    const double t1 = t0 + m_ViewSpanSec;
    m_LabelBudget = 1400;

    // Lane backgrounds and group headers. The clip is set before the scroll translation, so it
    // stays in widget space and the lanes never paint over the ruler. The translation includes the
    // ruler height because row coordinates are content coordinates starting at 0, while the lane
    // area starts below the ruler: without it every row is painted kRulerHeight above the band that
    // hit testing (SpanAt/RowAt, which do subtract rows.top()) attributes it to, and the pointer
    // lands on the row above.
    p.save();
    p.setClipRect(QRectF(0, rows.top(), rightEdge, rows.height()));
    p.translate(0, rows.top() - m_ScrollY);

    for(int i = 0; i < m_Rows.size(); i++)
    {
      const Row &row = m_Rows[i];
      if(row.top + row.height < m_ScrollY || row.top > m_ScrollY + rows.height())
        continue;
      const QRectF band(0, row.top, rightEdge, row.height);

      if(row.header)
      {
        const bool hovered = (i == m_HoverHeaderRow);

        p.fillRect(band, hovered ? Blend(SurfaceAlt(), Highlight(), 0.18) : SurfaceAlt());

        p.setPen(QPen(Grid(), 1));
        p.drawLine(QPointF(0, row.top), QPointF(rightEdge, row.top));

        // A disclosure triangle, as SDP's group headers have: the header is a control, not just a
        // label, and its state is what decides whether the group's lanes are laid out at all. It is
        // drawn rather than typed, because a font glyph changes size with the font and the row
        // height while the hit target behind it does not.
        const double centre = (double(row.height)) * 0.5;
        const double size = qBound(3.5, double(row.height) / 6.0, 6.0);
        QPainterPath triangle;
        if(row.collapsed)
        {
          triangle.moveTo(11.0 - size * 0.5, centre - size * 0.5);
          triangle.lineTo(11.0 + size * 0.5, centre);
          triangle.lineTo(11.0 - size * 0.5, centre + size * 0.5);
        }
        else
        {
          triangle.moveTo(11.0 - size * 0.5, centre - size * 0.4);
          triangle.lineTo(11.0 + size * 0.5, centre - size * 0.4);
          triangle.lineTo(11.0, centre + size * 0.5);
        }
        triangle.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(hovered ? Text() : (row.collapsed ? DimText() : Text()));
        p.save();
        p.translate(0.0, double(row.top));
        p.drawPath(triangle);
        p.restore();
        p.setBrush(Qt::NoBrush);

        QFont font = p.font();
        font.setBold(true);
        p.setFont(font);
        p.setPen(hovered ? Text() : (row.collapsed ? DimText() : Text()));
        const QString counts =
            row.visibleTrackCount == row.trackCount
                ? QObject::tr("%1").arg(row.trackCount)
                : QObject::tr("%1/%2").arg(row.visibleTrackCount).arg(row.trackCount);
        // The label is confined to the caption column. A group header spans the whole band, but
        // its text does not: the time grid is drawn after this pass and the column separator in
        // PaintCaptions, so a label reaching into the plot would have a vertical line through it.
        const QString label = QStringLiteral("%1  (%2)").arg(row.group, counts);
        const double labelWidth = double(kHeaderWidth) - 34.0;
        p.drawText(QRectF(24, row.top, labelWidth, row.height), Qt::AlignLeft | Qt::AlignVCenter,
                   p.fontMetrics().elidedText(label, Qt::ElideRight, int(labelWidth)));
        font.setBold(false);
        p.setFont(font);
        continue;
      }

      p.fillRect(band, (i % 2) == 0 ? Surface() : SurfaceAlt());

      if(row.moreCount > 0)
      {
        if(i == m_HoverHeaderRow)
          p.fillRect(band, Blend(SurfaceAlt(), Highlight(), 0.18));

        p.setPen(QPen(Grid(), 1));
        p.drawLine(QPointF(0, row.top + row.height - 1),
                   QPointF(rightEdge, row.top + row.height - 1));
        continue;
      }

      p.setPen(QPen(Grid(), 1));
      p.drawLine(QPointF(0, row.top + row.height - 1),
                 QPointF(rightEdge, row.top + row.height - 1));
    }

    p.restore();

    // Vertical time grid, identical in translated and untranslated space because it only depends
    // on the horizontal mapping.
    const double step = NiceStep(m_ViewSpanSec, rows.width());

    p.setPen(QPen(Grid(), 1));
    for(double t = ceil(t0 / step) * step; t <= t1; t += step)
    {
      const double x = XForTime(t);
      p.drawLine(QPointF(x, rows.top()), QPointF(x, rows.bottom()));
    }

    // Lane contents. Same content-to-widget mapping as the backgrounds above.
    p.save();
    p.setClipRect(QRectF(rows.left(), rows.top(), rows.width(), rows.height()));
    p.translate(0, rows.top() - m_ScrollY);

    for(int rowIndex = 0; rowIndex < m_Rows.size(); rowIndex++)
    {
      const Row &row = m_Rows[rowIndex];

      if(row.top + row.height < m_ScrollY || row.top > m_ScrollY + rows.height())
      {
        // Keep raster storage bounded to visible lanes, not the entire trace.
        row.spanImage = QImage();
        row.counterPolyline.clear();
        row.counterGeometryValid = false;
        continue;
      }
      if(row.header)
        continue;

      if(row.annotation)
      {
        PaintAnnotations(p, row, rows);
        continue;
      }

      if(row.track >= 0 && row.track < trace.tracks.size())
      {
        const TraceTrack &track = trace.tracks[row.track];

        if(track.kind == TraceTrackKind::Counter && track.counterSeries >= 0 &&
           track.counterSeries < trace.series.size())
        {
          PaintCounterLane(p, trace.series[track.counterSeries], row, track.counterSeries, t0, t1,
                           rowIndex);
        }
        else
        {
          PaintCachedSpanLane(p, track, row, t0, t1);
        }

        continue;
      }

      if(row.series >= 0 && row.series < trace.series.size())
        PaintCounterLane(p, trace.series[row.series], row, row.series, t0, t1, rowIndex);
    }

    p.restore();
  }

  void PaintCounterLane(QPainter &p, const TraceSeries &series, const Row &row, int colourIndex,
                        double t0, double t1, int rowIndex)
  {
    double vmin = row.counterRange ? row.counterMin : series.MinValue();
    double vmax = row.counterRange ? row.counterMax : series.MaxValue();

    if(vmax <= vmin)
      vmax = vmin + 1.0;

    const double top = double(row.top) + 4.0;
    const double bottom = double(row.top + row.height) - 5.0;

    const QRectF rows = RowsRect();
    const QString unit = series.unit.isEmpty() ? QString() : QStringLiteral(" ") + series.unit;

    // Without these two numbers the curve only says "higher than before". Per-lane rather than per-
    // shared-axis on purpose: each counter has its own range, so a shared axis would flatten the
    // small ones to a line.
    p.setPen(DimText());
    p.drawText(QRectF(rows.left() + 4.0, double(row.top) + 1.0, 140.0, 11.0),
               Qt::AlignLeft | Qt::AlignTop, FormatValue(vmax) + unit);
    p.drawText(QRectF(rows.left() + 4.0, double(row.top + row.height) - 12.0, 140.0, 11.0),
               Qt::AlignLeft | Qt::AlignTop, FormatValue(vmin) + unit);

    const auto yFor = [&](double value) {
      return bottom - ((value - vmin) / (vmax - vmin)) * (bottom - top);
    };

    const int count = series.samples.size();
    const bool reuseGeometry = row.counterGeometryValid && row.counterViewport == rows &&
                               row.counterT0 == t0 && row.counterT1 == t1 &&
                               row.cachedMin == vmin && row.cachedMax == vmax;
    if(!reuseGeometry)
    {
      QPolygonF poly;
      poly.reserve(qMax(8, int(rows.width()) * 4 + 8));

      // A counter lane is sampled far more often than it has pixel columns to draw on: a sysfs poller
      // running at kilohertz rates produces tens of thousands of samples across a few hundred pixels,
      // and the lane only ever shows one height per column. Emitting one vertex per sample therefore
      // spent seconds in the painter to draw the same picture, which made panning and zooming a large
      // trace unusable.
      //
      // Vertices are reduced to one column at a time, keeping the first, lowest, highest and last value
      // that falls in it. A spike narrower than a pixel survives, and so does the vertical riser of a
      // level series, while a flat stretch collapses to two points. Keeping every Nth sample instead
      // would drop exactly the excursions a counter lane exists to show.
      int columnX = 0;
      bool columnOpen = false;
      double columnFirstY = 0.0;
      double columnLastY = 0.0;
      double columnMinY = 0.0;
      double columnMaxY = 0.0;

      const auto flushColumn = [&]() {
        if(!columnOpen)
          return;

        const double x = double(columnX);
        poly << QPointF(x, columnFirstY);

        if(columnMinY < columnFirstY)
          poly << QPointF(x, columnMinY);
        if(columnMaxY > columnFirstY)
          poly << QPointF(x, columnMaxY);
        // The column's closing value is what draws a level series' riser. Emitting it even when one of
        // the extremes already ended there costs a repeated vertex, which a polyline draws as a
        // zero-length segment and which is cheaper than tracking whether it is needed.
        if(columnLastY != columnFirstY)
          poly << QPointF(x, columnLastY);

        columnOpen = false;
      };

      const auto push = [&](double x, double y) {
        const int column = int(x);

        if(!columnOpen || column != columnX)
        {
          flushColumn();

          columnOpen = true;
          columnX = column;
          columnFirstY = columnLastY = columnMinY = columnMaxY = y;
          return;
        }

        columnLastY = y;
        columnMinY = qMin(columnMinY, y);
        columnMaxY = qMax(columnMaxY, y);
      };

      // The lane has to be covered even where no sample of its own falls: a counter is sampled every
      // 20ms on a Pixel, while the timeline zooms down to a 200th of the trace, so a window narrower
      // than the sampling interval is the normal case, not the corner case. Stopping the drawing at
      // the last sample inside the window is what made the right-hand part of a lane go blank as soon
      // as the user zoomed in. Both edges therefore get the neighbour that they need.
      int first = 0;
      int afterT1 = count;
      if(count > 0)
      {
        const auto beginIt = std::lower_bound(
            series.samples.constBegin(), series.samples.constEnd(), t0,
            [](const TraceSample &sample, double time) { return sample.timeSec < time; });
        const auto endIt = std::upper_bound(
            series.samples.constBegin(), series.samples.constEnd(), t1,
            [](double time, const TraceSample &sample) { return time < sample.timeSec; });

        first = int(beginIt - series.samples.constBegin());
        afterT1 = int(endIt - series.samples.constBegin());

        // Keep one sample before the viewport so a step/curve remains connected at the left edge.
        if(first > 0)
          first--;
      }

      if(count > 0 && series.step)
      {
        // A level series (frequency, clock rate, idle state) holds each value until the next sample,
        // so the value in force at the left edge is the last sample at or before it. Interpolating
        // between two samples would draw a value that was never measured.
        int held = first;
        if(held < count && series.samples[held].timeSec > t0)
          held--;
        if(held < 0)
          held = 0;

        // The value in force at the right edge is the last sample at or before it, so the sample that
        // crosses the edge is not needed here: the hold is extended to the edge explicitly below.
        const int to = qMin(count - 1, afterT1 - 1);

        if(held <= to)
        {
          double heldY = yFor(series.samples[held].value);

          push(XForTime(qMax(t0, series.samples[held].timeSec)), heldY);

          for(int i = held + 1; i <= to; i++)
          {
            const double x = XForTime(series.samples[i].timeSec);
            push(x, heldY);
            heldY = yFor(series.samples[i].value);
            push(x, heldY);
          }

          // Still holding at the right edge, which is the whole point of drawing a level series as a
          // hold rather than as a sparse set of dots.
          push(XForTime(t1), heldY);
        }
      }
      else if(count > 0)
      {
        // A curve connects neighbouring samples, so the two segments that cross the window edges are
        // clipped in time and their values interpolated there. That keeps the drawing inside the
        // widget as well: an unclipped neighbour can be megabytes of pixels away when the sampling
        // interval is much wider than the window.
        const auto edge = [&](int i0, int i1, double at) {
          const TraceSample &a = series.samples[i0];
          const TraceSample &b = series.samples[i1];
          const double dt = b.timeSec - a.timeSec;
          const double f = dt > 0.0 ? (at - a.timeSec) / dt : 0.0;

          push(XForTime(at), yFor(a.value + f * (b.value - a.value)));
        };

        const int from = first;
        const int to = qMin(count - 1, afterT1);

        if(from == to)
        {
          // One known value cannot be connected to anything, but reporting it across the window is
          // still truer than an empty lane. Only done when the sample is actually inside the window:
          // a value that predates it must not be presented as current.
          const double t = series.samples[from].timeSec;

          if(t >= t0 && t <= t1)
          {
            const double y = yFor(series.samples[from].value);
            push(XForTime(t0), y);
            push(XForTime(t1), y);
          }
        }
        else
        {
          if(series.samples[from].timeSec < t0)
          {
            edge(from, from + 1, t0);
          }
          else
          {
            push(XForTime(series.samples[from].timeSec), yFor(series.samples[from].value));
          }

          for(int i = from + 1; i < to; i++)
            push(XForTime(series.samples[i].timeSec), yFor(series.samples[i].value));

          if(series.samples[to].timeSec > t1)
          {
            edge(to - 1, to, t1);
          }
          else
          {
            push(XForTime(series.samples[to].timeSec), yFor(series.samples[to].value));
          }
        }
      }

      flushColumn();
      row.counterPolyline.swap(poly);
      row.counterViewport = rows;
      row.counterT0 = t0;
      row.counterT1 = t1;
      row.cachedMin = vmin;
      row.cachedMax = vmax;
      row.counterGeometryValid = true;
    }
    const QPolygonF &poly = row.counterPolyline;

    if(poly.size() < 2)
      return;

    // One-pixel lanes would otherwise disappear entirely; a filled band also makes a level series
    // readable as "the CPU was at this frequency" rather than as a thin line.
    if(series.step)
    {
      QPolygonF band = poly;
      band << QPointF(poly.last().x(), bottom) << QPointF(poly.first().x(), bottom);

      QColor fill = SeriesColour(colourIndex);
      fill.setAlpha(48);
      p.setPen(Qt::NoPen);
      p.setBrush(fill);
      p.drawPolygon(band);
    }

    p.setPen(QPen(SeriesColour(colourIndex), 1.4));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(poly);

    if(rowIndex < 0 || rowIndex != m_HoverRow || m_HoverSeries < 0)
      return;

    // The value under the cursor, drawn where the eye already is. A tooltip would cover the curve
    // it describes and the caption column is a whole panel away from the point being read.
    const double x = XForTime(m_HoverTimeSec);
    const double y = bottom - ((m_HoverValue - vmin) / (vmax - vmin)) * (bottom - top);

    p.setPen(QPen(Highlight(), 1, Qt::DashLine));
    p.drawLine(QPointF(x, double(row.top) + 1.0), QPointF(x, double(row.top + row.height) - 1.0));

    p.setPen(Qt::NoPen);
    p.setBrush(SeriesColour(colourIndex));
    p.drawRect(QRectF(x - 2.5, y - 2.5, 5.0, 5.0));
    p.setBrush(Qt::NoBrush);

    const QString label = FormatValue(m_HoverValue) + unit;
    const double labelWidth = double(p.fontMetrics().width(label) + 8);
    double labelX = x + 6.0;

    if(labelX + labelWidth > rows.right() - 2.0)
      labelX = x - 6.0 - labelWidth;

    const QRectF box(labelX, qBound(rows.top() + 1.0, y - 7.0, rows.bottom() - 15.0), labelWidth,
                     14.0);

    p.setPen(Qt::NoPen);
    p.setBrush(SurfaceAlt());
    p.drawRect(box);
    p.setPen(Text());
    p.drawText(box, Qt::AlignCenter, label);
  }

  void PaintCachedSpanLane(QPainter &p, const TraceTrack &track, const Row &row, double t0,
                           double t1)
  {
    // Cache a lane in device pixels. Hover invalidates only the affected lane; counter hover,
    // selection overlays and repeated exposure no longer repaint thousands of unchanged spans.
    const qreal dpr = devicePixelRatioF();
    const QSize pixels(int(ceil(width() * dpr)), int(ceil(row.height * dpr)));
    if(pixels.width() <= 0 || pixels.height() <= 0 ||
       qint64(pixels.width()) * pixels.height() > 4 * 1024 * 1024)
    {
      PaintSpanLane(p, track, row, t0, t1);
      return;
    }
    const int selected = m_Selected.track == row.track ? m_Selected.span : -1;
    const int hovered = m_Hover.track == row.track ? m_Hover.span : -1;
    if(row.spanImage.isNull() || row.spanImage.size() != pixels ||
       row.spanImage.devicePixelRatio() != dpr || row.spanT0 != t0 || row.spanT1 != t1 ||
       row.spanSelected != selected || row.spanHovered != hovered ||
       row.spanPaletteKey != palette().cacheKey() || row.spanFont != p.font())
    {
      row.spanImage = QImage(pixels, QImage::Format_ARGB32_Premultiplied);
      row.spanImage.setDevicePixelRatio(dpr);
      row.spanImage.fill(Qt::transparent);
      QPainter lane(&row.spanImage);
      lane.setFont(p.font());
      lane.translate(0, -row.top);
      lane.setClipRect(QRectF(RowsRect().left(), row.top, RowsRect().width(), row.height));
      // Per-lane deterministic budget, independent of whether preceding lanes were cached.
      const int budget = m_LabelBudget;
      m_LabelBudget = 64;
      PaintSpanLane(lane, track, row, t0, t1);
      m_LabelBudget = budget;
      row.spanT0 = t0;
      row.spanT1 = t1;
      row.spanSelected = selected;
      row.spanHovered = hovered;
      row.spanPaletteKey = palette().cacheKey();
      row.spanFont = p.font();
    }
    p.drawImage(QPointF(0, row.top), row.spanImage);
  }

  void PaintSpanLane(QPainter &p, const TraceTrack &track, const Row &row, double t0, double t1)
  {
    // The nesting is compressed into the row, so the deepest levels can reach past it. Clipping to
    // the band keeps one lane from painting over the one below.
    p.save();
    p.setClipRect(QRectF(RowsRect().left(), double(row.top), RowsRect().width(), double(row.height)),
                  Qt::IntersectClip);

    int first = 0;
    if(!row.maxEndPrefix.isEmpty())
    {
      const auto it = std::lower_bound(row.maxEndPrefix.constBegin(), row.maxEndPrefix.constEnd(), t0);
      first = int(it - row.maxEndPrefix.constBegin());
    }

    for(int i = first; i < track.spans.size(); i++)
    {
      const TraceSpan &span = track.spans[i];

      if(span.beginSec > t1)
        break;

      if(span.endSec < t0)
        continue;

      const double xa = XForTime(span.beginSec);
      const double xb = XForTime(span.endSec);
      const double rawWidth = xb - xa;

      const bool selected = (m_Selected.track == row.track && m_Selected.span == i);
      const bool hovered = (m_Hover.track == row.track && m_Hover.span == i);

      // At the full 5-second view a Pixel trace can contain hundreds of thousands of scheduler
      // slices. Sub-pixel slices cannot be distinguished by the raster display, but drawing each
      // rounded rectangle still costs a painter operation. Keep selected/hovered elements visible;
      // dense unselected slices are omitted until zoom makes them at least one pixel wide.
      if(track.spans.size() > 2000 && rawWidth < 1.0 && !selected && !hovered)
        continue;

      const double w = qMax(1.5, rawWidth);
      const QRectF box(xa, double(row.top) + 3.0 + span.depth * row.spanStep, w,
                       double(row.spanHeight));

      QColor base = SpanColour(span.category);
      if(hovered)
        base = base.lighter(118);

      p.setPen(QPen(base.darker(190), 1));
      p.setBrush(selected ? base.lighter(150) : base);
      p.drawRect(box);

      if(selected || hovered)
      {
        p.setPen(QPen(selected ? Highlight() : AccentHover().lighter(160), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(box.adjusted(-1.0, -1.0, 1.0, 1.0));
      }

      // Labels remain readable on dense lanes and are clipped to the part of the span visible in
      // the viewport. The dark halo keeps the text legible on every category colour.
      if(w > 10.0 && row.spanHeight >= 10 && !span.name.isEmpty() && m_LabelBudget > 0)
      {
        QRectF area = box.adjusted(3.0, 0.0, -3.0, 0.0);
        area.setLeft(qMax(area.left(), RowsRect().left() + 3.0));
        area.setRight(qMin(area.right(), RowsRect().right() - 3.0));

        QFont font = p.font();
        font.setPixelSize(qBound(11, row.spanHeight - 3, 14));
        font.setBold(true);
        QFontMetrics metrics(font);
        if(area.width() >= metrics.width(QLatin1Char('W')))
        {
          const QString label = metrics.elidedText(span.name, Qt::ElideRight, int(area.width()));
          p.save();
          p.setClipRect(area);
          p.setFont(font);
          p.setPen(QPen(QColor(0x10, 0x12, 0x14, 230), 3.0));
          p.drawText(area, Qt::AlignHCenter | Qt::AlignVCenter, label);
          p.setPen(QColor(0xff, 0xff, 0xff));
          p.drawText(area, Qt::AlignHCenter | Qt::AlignVCenter, label);
          p.restore();
          m_LabelBudget--;
        }
      }
    }

    p.restore();
  }

  void PaintAnnotations(QPainter &p, const Row &row, const QRectF &rows)
  {
    const double t0 = m_ViewStartSec;
    const double t1 = t0 + m_ViewSpanSec;

    for(const Annotation &a : m_Annotations)
    {
      if(a.group != row.annotationGroup)
        continue;

      if(a.beginSec < t0 || a.beginSec > t1)
        continue;

      const double x = XForTime(a.beginSec);

      p.setPen(QPen(Highlight(), 1));
      p.setBrush(Highlight());

      QPolygonF flag;
      flag << QPointF(x, double(row.top) + 8.0) << QPointF(x + 6.0, double(row.top) + 11.0)
           << QPointF(x, double(row.top) + 14.0);
      p.drawPolygon(flag);

      p.setPen(Highlight());
      p.drawText(QRectF(x + 9.0, double(row.top), qMax(40.0, rows.right() - x - 12.0),
                        double(row.height)),
                 Qt::AlignLeft | Qt::AlignVCenter, a.text);
    }
  }

  void PaintCaptions(QPainter &p, const QRectF &rows)
  {
    const SystemTrace &trace = *m_Data.trace;

    p.save();
    p.setClipRect(QRectF(0, rows.top(), kHeaderWidth, rows.height()));
    p.translate(0, rows.top() - m_ScrollY);

    for(int i = 0; i < m_Rows.size(); i++)
    {
      const Row &row = m_Rows[i];
      if(row.top + row.height < m_ScrollY || row.top > m_ScrollY + rows.height())
        continue;

      // Group headers are painted by PaintLanes across the full width, label included, and that
      // label starts inside this column. They are left alone so the label survives.
      if(row.header)
        continue;

      // The caption column is its own surface, as SDP's left track header is: each lane's cell
      // gets the same colour PaintLanes used for that lane's band, so the column keeps the plot
      // area's banding instead of reading as one undifferentiated slab.
      p.fillRect(QRectF(0, row.top, double(kHeaderWidth), row.height),
                 (i % 2) == 0 ? Surface() : SurfaceAlt());

      p.setPen(QPen(Grid(), 1));
      p.drawLine(QPointF(0, row.top + row.height - 1),
                 QPointF(double(kHeaderWidth), row.top + row.height - 1));

      const double captionWidth = double(kHeaderWidth) - 40.0;
      QString caption;
      QString detail;
      QColor swatch(0x80, 0x80, 0x80);

      if(row.moreCount > 0)
      {
        // The summary row is a control: it says exactly how many lanes of this group are not on
        // screen and shows them when clicked.
        const QString label =
            QObject::tr("  %1 more lane(s) \u2014 click to show").arg(row.moreCount);
        p.setPen(Accent());
        p.drawText(QRectF(4.0, double(row.top), double(kHeaderWidth) - 8.0, double(row.height)),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   p.fontMetrics().elidedText(label, Qt::ElideRight, kHeaderWidth - 12));
        continue;
      }

      if(row.annotation)
      {
        caption = QObject::tr("Annotations");
        swatch = Highlight();
      }
      else if(row.track >= 0 && row.track < trace.tracks.size())
      {
        const TraceTrack &track = trace.tracks[row.track];
        caption = track.name;

        if(track.kind == TraceTrackKind::Counter)
        {
          swatch = SeriesColour(qMax(0, track.counterSeries));

          if(track.counterSeries >= 0 && track.counterSeries < trace.series.size())
          {
            const TraceSeries &series = trace.series[track.counterSeries];
            detail = series.unit.isEmpty()
                         ? QObject::tr("%1 samples").arg(series.samples.size())
                         : QObject::tr("%1 samples, %2").arg(series.samples.size()).arg(series.unit);
          }
        }
        else
        {
          swatch = SpanColour(track.spans.isEmpty() ? QString() : track.spans[0].category);

          // SDP's track header names the thread and then quantifies it; the id and the slice count
          // are what disambiguate two threads that share a caption.
          QStringList parts;
          if(track.tid >= 0)
            parts << QObject::tr("tid %1").arg(track.tid);

          parts << (track.kind == TraceTrackKind::Cpu ? QObject::tr("%1 slices").arg(track.spans.size())
                                                      : QObject::tr("%1 spans").arg(track.spans.size()));
          detail = parts.join(QStringLiteral("  \u00B7  "));
        }
      }
      else if(row.series >= 0 && row.series < trace.series.size())
      {
        const TraceSeries &series = trace.series[row.series];
        caption = series.name;
        swatch = SeriesColour(row.series);
        detail = series.unit.isEmpty() ? QObject::tr("%1 samples").arg(series.samples.size())
                                       : QObject::tr("%1 samples, %2")
                                             .arg(series.samples.size())
                                             .arg(series.unit);
      }

      if(caption.isEmpty())
        continue;

      // A lane whose nesting does not fit the band says so, rather than silently looking shallower
      // than it is. The lane never resizes to show the rest: the height is the user's setting, so
      // the marker is the only thing that may change.
      const int maxDepth = row.track >= 0 ? m_TrackMaxDepth.value(row.track, 0) : 0;
      const bool truncated = row.spanStep > 0 &&
                             (3 + maxDepth * row.spanStep + row.spanHeight) > row.height;

      p.fillRect(QRectF(8.0, double(row.top) + 6.0, 8.0, 8.0), swatch);

      if(truncated)
      {
        p.setPen(DimText());
        p.drawText(QRectF(double(kHeaderWidth) - 30.0, double(row.top), 24.0, double(row.height)),
                   Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("\u22EF"));
      }

      // Leave room for the truncation marker instead of drawing the two on top of each other.
      const double textWidth = truncated ? captionWidth - 16.0 : captionWidth;
      const QString elided = p.fontMetrics().elidedText(caption, Qt::ElideRight, int(textWidth));

      p.setPen(Text());
      p.drawText(QRectF(22.0, double(row.top) + 3.0, textWidth, 13.0),
                 Qt::AlignLeft | Qt::AlignVCenter, elided);

      if(!detail.isEmpty() && row.height >= 26)
      {
        p.setPen(DimText());
        p.drawText(QRectF(22.0, double(row.top) + 16.0, textWidth, 12.0),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   p.fontMetrics().elidedText(detail, Qt::ElideRight, int(textWidth)));
      }
    }

    p.restore();

    // A single continuous edge from the ruler down to the bottom of the lane area, so the caption
    // column reads as one column rather than as a per-row decoration.
    p.setPen(QPen(Grid(), 1));
    p.drawLine(QPointF(double(kHeaderWidth) - 1.0, rows.top()),
               QPointF(double(kHeaderWidth) - 1.0, rows.bottom()));
  }

  void PaintRuler(QPainter &p, const QRectF &rows)
  {
    const double t0 = m_ViewStartSec;
    const double t1 = t0 + m_ViewSpanSec;

    p.fillRect(QRectF(0.0, 0.0, double(width()), double(kRulerHeight)), SurfaceAlt());

    p.setPen(QPen(Grid(), 1));
    p.drawLine(QPointF(0.0, double(kRulerHeight) - 1.0), QPointF(double(width()), double(kRulerHeight) - 1.0));
    p.drawLine(QPointF(double(kHeaderWidth) - 1.0, 0.0),
               QPointF(double(kHeaderWidth) - 1.0, double(kRulerHeight)));

    p.setPen(DimText());
    if(m_CaliperASet && m_CaliperBSet)
    {
      p.drawText(QRectF(8.0, 0.0, double(kHeaderWidth) - 16.0, double(kRulerHeight)),
                 Qt::AlignLeft | Qt::AlignVCenter,
                 QObject::tr("dt %1").arg(FormatSeconds(fabs(m_CaliperB - m_CaliperA))));
    }
    else if(m_RangeValid)
    {
      // The selected window's own duration, next to the time axis it was dragged on, so the size of
      // the window is readable without leaving the plot for the Selection panel.
      p.drawText(QRectF(8.0, 0.0, double(kHeaderWidth) - 16.0, double(kRulerHeight)),
                 Qt::AlignLeft | Qt::AlignVCenter,
                 QObject::tr("sel %1").arg(FormatSeconds(m_RangeT1 - m_RangeT0)));
    }
    else
    {
      p.drawText(QRectF(8.0, 0.0, double(kHeaderWidth) - 16.0, double(kRulerHeight)),
                 Qt::AlignLeft | Qt::AlignVCenter, QObject::tr("Time"));
    }

    const double step = NiceStep(m_ViewSpanSec, rows.width());

    // Centred labels overhang their tick, and the first tick sits on the column edge, so the
    // labels are clipped to the plot area: a tick label must never be painted across the caption
    // column or through its separator.
    p.save();
    p.setClipRect(QRectF(rows.left(), 0.0, rows.width(), double(kRulerHeight)));

    for(double t = ceil(t0 / step) * step; t <= t1; t += step)
    {
      const double x = XForTime(t);

      p.setPen(QPen(DimText(), 1));
      p.drawLine(QPointF(x, double(kRulerHeight) - 7.0), QPointF(x, double(kRulerHeight) - 1.0));

      p.setPen(Text());
      p.drawText(QRectF(x - 50.0, 0.0, 100.0, double(kRulerHeight) - 8.0),
                 Qt::AlignHCenter | Qt::AlignVCenter, FormatSeconds(t));
    }

    p.restore();
  }

  void PaintOverlays(QPainter &p, const QRectF &rows)
  {
    p.save();
    p.setClipRect(rows);

    // --- frame window and action markers --------------------------------------------------
    if(m_Data.aligned && m_Data.frameEndSec > m_Data.frameStartSec)
    {
      const double t0 = m_ViewStartSec;
      const double t1 = t0 + m_ViewSpanSec;
      const double fa = qBound(t0, m_Data.frameStartSec, t1);
      const double fb = qBound(t0, m_Data.frameEndSec, t1);

      if(fb > fa)
      {
        const double x0 = XForTime(fa);
        const double x1 = XForTime(fb);

        // The frame window is drawn in the theme's accent so it reads as a selection over the
        // lanes rather than as a fourth unrelated colour on top of them.
        QColor frameTint = Accent();
        frameTint.setAlpha(32);

        p.setPen(Qt::NoPen);
        p.setBrush(frameTint);
        p.drawRect(QRectF(x0, rows.top(), x1 - x0, rows.height()));
        p.setBrush(Qt::NoBrush);

        p.setPen(QPen(Accent(), 1, Qt::DashLine));
        p.drawLine(QPointF(x0, rows.top()), QPointF(x0, rows.bottom()));
        p.drawLine(QPointF(x1, rows.top()), QPointF(x1, rows.bottom()));

        p.setPen(Accent().lighter(145));
        p.drawText(QRectF(x0 + 3.0, rows.top() + 2.0, qMax(80.0, x1 - x0), 14.0),
                   Qt::AlignLeft | Qt::AlignTop,
                   QObject::tr("Frame %1").arg(m_Data.frameNumber));
      }
    }

    const bool overlay = m_Data.aligned && m_Data.frameEndSec > m_Data.frameStartSec;

    if(overlay)
    {
      // Each action is a tick at its own start time inside the frame window.
      const double frameSeconds = m_Data.frameEndSec - m_Data.frameStartSec;

      for(const TraceActionRef &action : m_Data.actions)
      {
        if(!action.hasGpuDuration)
          continue;

        const double fraction = action.timeSec / frameSeconds;

        if(fraction < 0.0 || fraction > 1.0)
          continue;

        const double x = XForTime(m_Data.frameStartSec + action.timeSec);

        if(action.eventId == m_Data.highlightEvent)
        {
          p.setPen(QPen(Highlight(), 2));
          p.drawLine(QPointF(x, rows.top()), QPointF(x, rows.bottom()));

          p.setPen(Highlight());
          p.drawText(QRectF(x + 3.0, rows.bottom() - 18.0, 240.0, 14.0),
                     Qt::AlignLeft | Qt::AlignBottom,
                     QObject::tr("EID %1 %2").arg(action.eventId).arg(action.name));
        }
        else
        {
          p.setPen(QPen(QColor(0xff, 0xff, 0xff, 90), 1));
          p.drawLine(QPointF(x, rows.top() + 4.0), QPointF(x, rows.bottom() - 4.0));
        }
      }
    }
    else if(!m_Data.captureLoaded)
    {
      // No capture open: the lanes are still meaningful on their own, but there is nothing to
      // place them against, so say that instead of implying a failed clock alignment.
      p.setPen(QColor(0xe0, 0xb0, 0x6c));
      p.drawText(QRectF(rows.left() + 6.0, rows.top() + 4.0, rows.width() - 12.0, 16.0),
                 Qt::AlignLeft | Qt::AlignTop,
                 QObject::tr("No capture loaded: trace shown on its own time base"));
    }
    else
    {
      // Unaligned: the trace is shown from time zero, clearly labelled so the lanes are never
      // mistaken for being placed against the capture.
      p.setPen(QColor(0xe0, 0x6c, 0x6c));
      p.drawText(QRectF(rows.left() + 6.0, rows.top() + 4.0, rows.width() - 12.0, 16.0),
                 Qt::AlignLeft | Qt::AlignTop,
                 QObject::tr("Trace not aligned to the frame (clock domains unrelated)"));
    }

    // --- selected time window --------------------------------------------------------------
    // Drawn before the calipers and after the frame tint, so a selection over a frame reads as a
    // selection rather than as a brighter frame. The fill is translucent and the edges solid: the
    // window has to be visible over curves without hiding the samples inside it.
    if(m_RangeValid)
    {
      const double xa = XForTime(m_RangeT0);
      const double xb = XForTime(m_RangeT1);

      p.setPen(Qt::NoPen);
      p.setBrush(QColor(Highlight().red(), Highlight().green(), Highlight().blue(), 40));
      p.drawRect(QRectF(xa, rows.top(), xb - xa, rows.height()));
      p.setBrush(Qt::NoBrush);

      p.setPen(QPen(Highlight(), 1));
      p.drawLine(QPointF(xa, rows.top()), QPointF(xa, rows.bottom()));
      p.drawLine(QPointF(xb, rows.top()), QPointF(xb, rows.bottom()));
    }

    // --- calipers -------------------------------------------------------------------------
    if(m_CaliperASet && m_CaliperBSet)
    {
      const double xa = XForTime(qMin(m_CaliperA, m_CaliperB));
      const double xb = XForTime(qMax(m_CaliperA, m_CaliperB));

      p.setPen(Qt::NoPen);
      p.setBrush(QColor(Highlight().red(), Highlight().green(), Highlight().blue(), 28));
      p.drawRect(QRectF(xa, rows.top(), xb - xa, rows.height()));
      p.setBrush(Qt::NoBrush);
    }

    if(m_CaliperASet)
    {
      const double x = XForTime(m_CaliperA);
      p.setPen(QPen(Highlight(), 1, Qt::DashLine));
      p.drawLine(QPointF(x, rows.top()), QPointF(x, rows.bottom()));
    }

    if(m_CaliperBSet)
    {
      const double x = XForTime(m_CaliperB);
      p.setPen(QPen(Highlight(), 1, Qt::DashLine));
      p.drawLine(QPointF(x, rows.top()), QPointF(x, rows.bottom()));
    }

    p.restore();
  }

  void PaintDrag(QPainter &p, const QRectF &rows)
  {
    const QRect dragRect = QRect(m_DragStart, m_DragCurrent).normalized();
    const QRectF clipped = QRectF(dragRect).intersected(rows);

    if(clipped.isEmpty())
      return;

    if(m_Dragging == DragZoom)
    {
      p.setPen(Qt::NoPen);
      p.setBrush(QColor(Accent().red(), Accent().green(), Accent().blue(), 60));
      p.drawRect(clipped);
      p.setBrush(Qt::NoBrush);
    }

    p.setPen(QPen(m_Dragging == DragSelect ? Highlight() : Accent(), 1, Qt::DashLine));
    p.drawRect(clipped);
  }

  SystemTraceTimelineData m_Data;
  QVector<Row> m_Rows;
  QVector<int> m_CounterSeriesInView;
  QVector<Annotation> m_Annotations;
  QScrollBar *m_Scroll = NULL;
  QScrollBar *m_TimeScroll = NULL;
  bool m_UpdatingTimeScroll = false;

  // Which trace the current view window belongs to. When the viewer hands over a different trace
  // the window is reset to its full extent; an event-driven redraw leaves the zoom and pan alone.
  const SystemTrace *m_LastTrace = NULL;
  bool m_ResetPending = false;

  TraceNavMode m_Mode = TraceNavMode::Pan;
  DragMode m_Dragging = DragNone;

  QPoint m_DragStart;
  QPoint m_DragCurrent;
  double m_DragSpanStartSec = 0.0;
  double m_DragPanStartSec = 0.0;

  double m_ViewStartSec = 0.0;
  double m_ViewSpanSec = 1.0;

  int m_ScrollY = 0;
  int m_ContentHeight = 0;

  SpanRef m_Hover;
  SpanRef m_Selected;

  // Row under the cursor that is a control rather than a lane (a group header, or the summary row
  // standing in for the lanes a group did not lay out). Highlighted while hovered and pointed at
  // with the hand cursor, so the target is visible before the click rather than after it.
  int m_HoverHeaderRow = -1;

  // Live row height, in logical pixels. Changed through SetRowHeight() only.
  int m_RowHeight = kRowHeightDefault;

  // Counter readout state: the row under the cursor, the series it draws, the value that series
  // holds at the cursor, and the sample that value came from. Kept so a repaint draws the same
  // number the caption was told about, instead of resolving it twice and risking a mismatch.
  int m_HoverRow = -1;
  int m_HoverSeries = -1;
  double m_HoverTimeSec = 0.0;
  double m_HoverValue = 0.0;
  QString m_HoverText;
  int m_LabelBudget = 1400;

  bool m_RowsDirty = true;

  // Lane visibility state. Kept separate from m_Data because it describes the user's view of the
  // trace, not the trace itself, and must survive a data refresh that keeps the same trace.
  QString m_FilterText;
  bool m_ActiveOnly = true;
  QListWidget *m_TypeList = NULL;
  QSet<int> m_HiddenTypes;
  QSet<QString> m_KnownGroups;
  QSet<QString> m_CollapsedGroups;
  // Groups whose lane cap the user has lifted, by name so it survives a re-import.
  QSet<QString> m_ExpandedGroups;

  const SystemTrace *m_IndexedTrace = NULL;
  QVector<QVector<double>> m_TrackMaxEnd;
  QVector<int> m_TrackMaxDepth;

  bool m_CaliperASet = false;
  bool m_CaliperBSet = false;
  double m_CaliperA = 0.0;
  double m_CaliperB = 0.0;

  // The Select-mode drag window, in trace-clock seconds, always ascending. Selecting a window is how
  // a counter lane is inspected as a whole: it is a series of measurements, not a list of elements.
  bool m_RangeValid = false;
  double m_RangeT0 = 0.0;
  double m_RangeT1 = 0.0;
};

SystemTraceViewer::SystemTraceViewer(ICaptureContext &ctx, QWidget *parent)
    : QFrame(parent), m_Ctx(ctx)
{
  // The title is the docked tab's caption, not a window caption: the tool window manager reads
  // windowTitle() when it builds the tab. The panel is explicitly *not* delete-on-close, because
  // MainWindow keeps the single instance and re-shows it; deleting it here would leave that
  // pointer dangling.
  setWindowTitle(tr("System Trace"));
  setObjectName(lit("systemTraceViewer"));

  m_TracePath = new QLineEdit(this);
  m_TracePath->setReadOnly(true);
  m_TracePath->setPlaceholderText(tr("No trace loaded"));

  QPushButton *browse = new QPushButton(tr("Open Trace..."), this);
  QPushButton *reload = new QPushButton(tr("Reload"), this);
  QPushButton *saveTrace = new QPushButton(tr("Save Trace As..."), this);
  connect(saveTrace, &QPushButton::clicked, this, [this]() {
    if(m_Importing || m_TracePath->text().isEmpty())
    { RDDialog::critical(this, tr("Save System Trace"), tr("Open or finish recording a trace first.")); return; }
    QString path = RDDialog::getSaveFileName(this, tr("Save System Trace"),
        QFileInfo(m_TracePath->text()).absolutePath() + QStringLiteral("/capture.rdt"),
        tr("RenderMax System Trace (*.rdt)"));
    if(path.isEmpty()) return;
    if(!path.endsWith(QStringLiteral(".rdt"), Qt::CaseInsensitive)) path += QStringLiteral(".rdt");
    QString error;
    if(!SaveSystemTraceArchive(m_TracePath->text(), path, error))
    { RDDialog::critical(this, tr("Save System Trace failed"), error); return; }
    loadTraceFrom(path);
  });

  // Recording controls. The duration is chosen before starting because a device protocol
  // serialises its commands onto one thread: a running `perfetto` cannot be interrupted, so the
  // recording must be bounded up front rather than stopped on demand.
  //
  // The ceiling is 10 seconds because that is the limit Snapdragon Profiler documents for its own
  // trace captures; offering more would only produce a duration the device tooling caps anyway. The
  // default is the shortest duration that still contains a few frames of work: the recorded text is
  // roughly twenty megabytes per second on a busy device, and every one of those lines is parsed on
  // this machine afterwards.
  m_CaptureSeconds = new QSpinBox(this);
  m_CaptureSeconds->setRange(1, 10);
  m_CaptureSeconds->setValue(3);
  m_CaptureSeconds->setSuffix(tr(" s"));
  m_CaptureSeconds->setToolTip(tr("How long to record on the device. Longer recordings produce a "
                                  "larger trace: roughly 20 MB of text per second on a busy "
                                  "device, all of which has to be converted and imported here."));

  // Which GPU events to ask for. The device's own capabilities decide what is actually available; this
  // only decides how much of what is available is worth recording, because the per-command events
  // dominate the size of the result.
  m_CaptureDetail = new QComboBox(this);
  m_CaptureDetail->addItem(tr("GPU: metrics"), int(GpuTraceDetail::Metrics));
  m_CaptureDetail->addItem(tr("GPU: metrics + command lanes"), int(GpuTraceDetail::Commands));
  m_CaptureDetail->addItem(tr("GPU: off"), int(GpuTraceDetail::Off));
  m_CaptureDetail->setCurrentIndex(0);
  m_CaptureDetail->setToolTip(
      tr("Which GPU tracepoints to record. `metrics` collects frequency, utilisation, temperature "
         "and throttling. `metrics + command lanes` adds the per-cmdbatch and per-fence intervals "
         "that the GPU Commands and GPU Waits lanes are drawn from, which is the largest part of a "
         "GPU trace. `off` records the smallest trace, with CPU and scheduling lanes only."));

  m_Capture = new QPushButton(tr("Capture Trace..."), this);

  connect(browse, &QPushButton::clicked, this, [this] { browseForTrace(); });
  connect(reload, &QPushButton::clicked, this, [this] { reloadTrace(); });
  connect(m_Capture, &QPushButton::clicked, this, [this] { captureTrace(); });

  QHBoxLayout *fileRow = new QHBoxLayout();
  fileRow->addWidget(new QLabel(tr("Trace:"), this));
  fileRow->addWidget(m_TracePath, 1);
  fileRow->addWidget(browse);
  fileRow->addWidget(reload);
  fileRow->addWidget(saveTrace);
  fileRow->addWidget(new QLabel(tr("Record:"), this));
  fileRow->addWidget(m_CaptureSeconds);
  fileRow->addWidget(m_CaptureDetail);
  fileRow->addWidget(m_Capture);

  // Navigation modes, matching SDP's Trace window: Select, Pan and Zoom, also reachable with the
  // 1, 2 and 3 keys while the timeline has focus. The artwork is SDP's own toolbar icon set: the
  // grey chip for a mode that is not current and the blue selection chip for the one that is,
  // exactly the two states its toolbar cycles through. The buttons carry no text, because the
  // icon says the same thing in a quarter of the width and the tooltip still spells it out.
  auto modeButton = [this](const QString &tip, const QString &chipOff, const QString &chipOn) {
    TraceModeButton *button = new TraceModeButton(this);
    button->SetChips(chipOff, chipOn);
    button->setToolTip(tip);
    return button;
  };

  m_ModeSelect = modeButton(tr("Select (1): drag out a time window to summarise the counters inside "
                                "it, or click an element to show its fields. F/M zoom to the "
                                "selection, Esc clears it."),
                            QStringLiteral(":/SDP/trace-select.png"),
                            QStringLiteral(":/SDP/trace-select-on.png"));

  m_ModePan = modeButton(tr("Pan (2): drag to move the visible time window. Ctrl+A / Ctrl+D also "
                            "pan. This is the default mode."),
                         QStringLiteral(":/SDP/trace-pan.png"),
                         QStringLiteral(":/SDP/trace-pan-on.png"));

  m_ModeZoom = modeButton(tr("Zoom (3): drag a region to zoom into it. The wheel and Ctrl+W / "
                             "Ctrl+S zoom as well."),
                          QStringLiteral(":/SDP/trace-zoom.png"),
                          QStringLiteral(":/SDP/trace-zoom-on.png"));

  connect(m_ModeSelect, &QToolButton::clicked, this, [this] { setNavMode(TraceNavMode::Select); });
  connect(m_ModePan, &QToolButton::clicked, this, [this] { setNavMode(TraceNavMode::Pan); });
  connect(m_ModeZoom, &QToolButton::clicked, this, [this] { setNavMode(TraceNavMode::Zoom); });

  // Row height is adjustable because the caption column is fixed at kHeaderWidth while the window
  // is not: at 30 px a two-line caption fits, at a small window it does not, and a trace with deep
  // atrace nesting wants more room per lane. Every row uses this height, so nothing moves when a
  // group is opened or closed.
  m_RowHeight = new QSpinBox(this);
  m_RowHeight->setRange(kRowHeightMin, kRowHeightMax);
  m_RowHeight->setSingleStep(2);
  m_RowHeight->setValue(kRowHeightDefault);
  m_RowHeight->setSuffix(tr(" px"));
  m_RowHeight->setToolTip(tr("Height of every row, from %1 to %2 pixels. Group headers and lanes "
                             "share it, so the grid does not shift when a group opens.")
                              .arg(kRowHeightMin)
                              .arg(kRowHeightMax));

  QLabel *modeHint = new QLabel(tr("Select-drag summarises counters (F/M zoom, Esc clears). "
                                  "Calipers: double-click, then shift-click. Right-click annotates."),
                                this);

  QHBoxLayout *modeRow = new QHBoxLayout();
  modeRow->setSpacing(6);
  modeRow->addWidget(new QLabel(tr("Navigation:"), this));
  modeRow->addWidget(m_ModeSelect);
  modeRow->addWidget(m_ModePan);
  modeRow->addWidget(m_ModeZoom);
  modeRow->addSpacing(8);
  modeRow->addWidget(new QLabel(tr("Rows:"), this));
  modeRow->addWidget(m_RowHeight);
  modeRow->addWidget(modeHint, 1);

  // Lane visibility. The importer cannot know which of the several hundred threads matters for the
  // question being asked, so the panel exposes the same three narrowing tools SDP's flow view has:
  // a text filter, "only what happens in this window", and collapsing by group.
  m_TrackFilter = new QLineEdit(this);
  m_TrackFilter->setClearButtonEnabled(true);
  m_TrackFilter->setPlaceholderText(tr("Filter tracks, e.g. Unity -idle"));
  m_TrackFilter->setToolTip(
      tr("Show only tracks whose name or process contains every word. A leading '-' or '!' excludes "
         "a word instead, so \"-idle\" hides the idle lanes. Press / to focus this box."));

  m_ActiveOnly = new QCheckBox(tr("Active in view"), this);
  m_ActiveOnly->setChecked(true);
  m_ActiveOnly->setToolTip(
      tr("Hide tracks that have nothing inside the visible time range. Frame-level zoom then shows "
         "only the threads that ran in that frame."));

  m_CollapseGroups = new QPushButton(tr("Collapse all"), this);
  m_CollapseGroups->setToolTip(tr("Reduce the timeline to the list of processes."));

  m_ExpandGroups = new QPushButton(tr("Show every lane"), this);
  m_ExpandGroups->setToolTip(
      tr("Lay out every track of every process. A large capture can produce hundreds of lanes; this "
         "is the command that brings them all back."));

  connect(m_TrackFilter, &QLineEdit::textChanged, this,
          [this](const QString &text) { m_Chart->SetTrackFilter(text); });
  connect(m_ActiveOnly, &QCheckBox::toggled, this,
          [this](bool checked) { m_Chart->SetActiveOnly(checked); });
  connect(m_CollapseGroups, &QPushButton::clicked, this,
          [this] { m_Chart->SetGroupsCollapsed(true); });
  connect(m_ExpandGroups, &QPushButton::clicked, this,
          [this] { m_Chart->SetGroupsCollapsed(false); });

  QHBoxLayout *filterRow = new QHBoxLayout();
  filterRow->addWidget(new QLabel(tr("Tracks:"), this));
  filterRow->addWidget(m_TrackFilter, 1);
  filterRow->addWidget(m_ActiveOnly);
  filterRow->addWidget(m_CollapseGroups);
  filterRow->addWidget(m_ExpandGroups);

  m_Chart = new SystemTraceTimeline(this);
  m_Chart->OnSelectEvent = [this](uint32_t eventId) { selectEvent(eventId); };
  m_Chart->OnSelectSpan = [this](const TraceSpan &span, const QString &trackName) {
    showElementFields(span, trackName);
  };
  m_Chart->OnNavModeChanged = [this](TraceNavMode mode) { setNavMode(mode); };
  m_Chart->OnHoverReadout = [this](const QString &text) { m_HoverReadout->setText(text); };
  m_Chart->OnSelectRange = [this](bool valid, double beginSec, double endSec) {
    updateRangeSelection(valid, beginSec, endSec);
  };

  connect(m_RowHeight, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          [this](int value) {
            if(m_Chart)
              m_Chart->SetRowHeight(value);
          });
  m_Chart->SetRowHeight(m_RowHeight->value());

  // '/' focuses the lane filter while the timeline has focus, the way SDP's filter entries are one
  // keystroke away. The shortcut is scoped to the timeline widget so that typing '/' inside the
  // filter box itself still inserts a character.
  QShortcut *focusFilter = new QShortcut(QKeySequence(Qt::Key_Slash), m_Chart);
  focusFilter->setContext(Qt::WidgetShortcut);
  connect(focusFilter, &QShortcut::activated, this, [this]() {
    m_TrackFilter->setFocus(Qt::ShortcutFocusReason);
    m_TrackFilter->selectAll();
  });

  m_Series = new QListWidget(this);
  m_Series->setToolTip(tr("Double-click a track to scroll the timeline to it and select its first "
                          "span."));
  connect(m_Series, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
    if(item == NULL || m_Chart == NULL)
      return;

    const QVariant index = item->data(Qt::UserRole);
    if(index.isValid())
      m_Chart->SelectTrack(index.toInt());
  });

  m_Frames = new QTableWidget(this);
  m_Frames->setColumnCount(3);
  m_Frames->setHorizontalHeaderLabels({tr("Frame"), tr("Trace start"), tr("GPU busy")});
  m_Frames->horizontalHeader()->setStretchLastSection(true);
  m_Frames->verticalHeader()->setVisible(false);
  m_Frames->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_Frames->horizontalHeaderItem(1)->setToolTip(
      tr("When the marker that starts this frame was written, in the trace's own clock (seconds since "
         "device boot). It is not a frame duration."));
  m_Frames->horizontalHeaderItem(2)->setToolTip(
      tr("GPU execution time inside this frame, added up from the kernel's own GPU counters and "
         "counted once however many contexts were in flight. Not the interval to the next frame."));

  // The Frames tab said "GPU busy" with nothing anywhere explaining where the number comes from, and
  // the obvious reading of a table of frame boundaries - subtract two rows and call it the frame time
  // - is exactly the inference this panel must not invite. The distance between two derived
  // boundaries is the spacing of the markers, which is a CPU and vsync artefact; the GPU may have
  // finished in a fraction of it or still be running when the next marker fires. So both facts are
  // stated here rather than left to be rediscovered.
  QWidget *framesPane = new QWidget(this);
  m_FramesCaption = new QLabel(
      tr("Trace start is the frame's marker time; the interval between two rows is the marker "
         "spacing, not GPU time. GPU busy is measured only from the kernel's GPU counters, so a dash "
         "means the trace could not measure it, and a frame that ran no GPU work at all shows 0."),
      framesPane);
  m_FramesCaption->setWordWrap(true);

  QVBoxLayout *framesLayout = new QVBoxLayout(framesPane);
  framesLayout->setContentsMargins(0, 0, 0, 0);
  framesLayout->addWidget(m_FramesCaption);
  framesLayout->addWidget(m_Frames, 1);

  // Per-thread scheduler-state totals, the equivalent of SDP's ThreadTime view. These values are
  // computed from exactly the running/runnable/sleeping spans that the timeline draws, so selecting
  // either JSON or systrace input cannot produce a different summary.
  m_ThreadTime = new QTableWidget(this);
  m_ThreadTime->setColumnCount(9);
  m_ThreadTime->setHorizontalHeaderLabels(
      {tr("Thread"), tr("TID"), tr("Process"), tr("Running"), tr("Runnable"), tr("Sleeping"),
       tr("CPU %"), tr("Wait %"), tr("Switches")});
  m_ThreadTime->horizontalHeader()->setStretchLastSection(false);
  m_ThreadTime->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_ThreadTime->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
  m_ThreadTime->verticalHeader()->setVisible(false);
  m_ThreadTime->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_ThreadTime->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_ThreadTime->setSortingEnabled(true);

  // The table answers two different questions: what a thread did over the whole capture, and what it
  // did inside the frame being inspected. The second is the one that explains a stalled frame, so the
  // window is a control rather than a re-import, and the result can be taken away as CSV.
  QWidget *threadTimePane = new QWidget(this);

  m_ThreadTimeFrame = new QCheckBox(tr("Selected frame only"), threadTimePane);
  m_ThreadTimeFrame->setToolTip(
      tr("Clip every running/runnable/sleeping interval to the frame correlated with the capture, so "
         "the totals describe that frame instead of the whole trace."));

  m_ThreadTimeCaption = new QLabel(tr("Whole trace"), threadTimePane);

  m_ThreadTimeExport = new QPushButton(tr("Export CSV..."), threadTimePane);
  m_ThreadTimeExport->setToolTip(tr("Write the rows shown here, with whatever sorting is applied, to "
                                    "a CSV file."));

  connect(m_ThreadTimeFrame, &QCheckBox::toggled, this, [this](bool) { updateThreadTime(); });
  connect(m_ThreadTimeExport, &QPushButton::clicked, this, [this]() { exportThreadTime(); });

  QHBoxLayout *threadTimeRow = new QHBoxLayout();
  threadTimeRow->addWidget(m_ThreadTimeFrame);
  threadTimeRow->addWidget(m_ThreadTimeCaption, 1);
  threadTimeRow->addWidget(m_ThreadTimeExport);

  QVBoxLayout *threadTimeLayout = new QVBoxLayout(threadTimePane);
  threadTimeLayout->setContentsMargins(0, 0, 0, 0);
  threadTimeLayout->addLayout(threadTimeRow);
  threadTimeLayout->addWidget(m_ThreadTime, 1);

  m_Warnings = new QListWidget(this);

  // The capture log is kept as a tab rather than a transient popup: it holds every device command
  // and every tool result in order, which is the only way a failed conversion is diagnosable.
  m_CaptureLog = new QPlainTextEdit(this);
  m_CaptureLog->setReadOnly(true);
  m_CaptureLog->setPlaceholderText(tr("No capture has been recorded in this window yet."));

  // Element pane. This is the job SDP's Inspector does for a selected trace element: the fields
  // the trace recorded for it, verbatim.
  QWidget *elementPane = new QWidget(this);

  m_ElementCaption = new QLabel(tr("No element selected"), elementPane);
  m_ElementCaption->setWordWrap(true);

  m_Element = new QTableWidget(elementPane);
  m_Element->setColumnCount(2);
  m_Element->setHorizontalHeaderLabels({tr("Field"), tr("Value")});
  m_Element->horizontalHeader()->setStretchLastSection(true);
  m_Element->verticalHeader()->setVisible(false);
  m_Element->setEditTriggers(QAbstractItemView::NoEditTriggers);

  QVBoxLayout *elementLayout = new QVBoxLayout(elementPane);
  elementLayout->addWidget(m_ElementCaption);
  elementLayout->addWidget(m_Element, 1);

  // Selection pane. SDP opens a Selection view for the window dragged out in Selection mode; on a
  // counter lane what that window contains is a set of samples, so the summary it can honestly give
  // is how many were recorded and their min/max/mean. The caption states the counting rules, because
  // a min/max/avg over a sparse sampled curve is exactly the place where an interpolated "average"
  // would look plausible and be wrong.
  m_RangePane = new QWidget(this);

  m_RangeCaption = new QLabel(tr("No time window selected. Drag in Select mode (1)."), m_RangePane);
  m_RangeCaption->setWordWrap(true);

  m_RangeTable = new QTableWidget(m_RangePane);
  m_RangeTable->setColumnCount(5);
  m_RangeTable->setHorizontalHeaderLabels({tr("Counter"), tr("Samples"), tr("Min"), tr("Max"),
                                           tr("Avg")});
  m_RangeTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_RangeTable->verticalHeader()->setVisible(false);
  m_RangeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_RangeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_RangeTable->setToolTip(
      tr("Only the samples recorded inside the selected window are counted: nothing is interpolated "
         "and a level that was measured before the window opened is not folded in. An empty window "
         "therefore reports no samples rather than the nearest value."));

  QVBoxLayout *rangeLayout = new QVBoxLayout(m_RangePane);
  rangeLayout->setContentsMargins(0, 0, 0, 0);
  rangeLayout->addWidget(m_RangeCaption);
  rangeLayout->addWidget(m_RangeTable, 1);

  QTabWidget *sideTabs = new QTabWidget(this);
  m_SideTabs = sideTabs;
  sideTabs->addTab(m_Series, tr("Tracks"));
  sideTabs->addTab(m_RangePane, tr("Selection"));
  sideTabs->addTab(framesPane, tr("Frames"));
  sideTabs->addTab(threadTimePane, tr("Thread Time"));
  sideTabs->addTab(elementPane, tr("Element"));
  sideTabs->addTab(m_Warnings, tr("Warnings"));
  sideTabs->addTab(m_CaptureLog, tr("Capture Log"));

  // The timeline and the side tabs are separated by a real splitter rather than a fixed layout, so
  // the boundary between the two is draggable: the trace plot and the field/log panes compete for
  // the same width and which one needs more depends on what the user is reading. The side pane is
  // still capped (a 2000px-wide "Capture Log" tab would be useless) but the cap is now well above
  // the initial 320px share, so dragging can widen it instead of the cap silently refusing.
  //
  // RDSplitter, not QSplitter: the project's splitter handle paints its own grab dots from the
  // palette's WindowText, so it lands in whatever scheme RDStyle installed without a stylesheet --
  // a bare handle would be an untinted platform default in the middle of a themed panel. The 12px
  // width is the same one BufferViewer and BufferFormatSpecifier use.
  //
  // Stretch factors give the timeline all the slack: widening the window grows the plot, not the
  // side tabs, because the side pane's content is mostly fixed-width tables. Children are not
  // collapsible, so the handle never reduces either pane to nothing and no collapse arrows are drawn.
  RDSplitter *middle = new RDSplitter(Qt::Horizontal, this);
  middle->setHandleWidth(12);
  middle->addWidget(m_Chart->CreateTypePanel(middle));
  middle->addWidget(m_Chart);
  middle->addWidget(sideTabs);
  middle->setStretchFactor(0, 0);
  middle->setStretchFactor(1, 1);
  middle->setStretchFactor(2, 0);
  middle->setChildrenCollapsible(false);
  middle->setSizes({240, 700, 300});

  sideTabs->setMinimumWidth(180);
  sideTabs->setMaximumWidth(900);

  // The counter value under the cursor. Placed directly above the alignment text because it is
  // the same kind of readout, and because a painter-drawn number inside the timeline is invisible
  // to assistive tooling.
  m_HoverReadout = new QLabel(this);
  m_HoverReadout->setWordWrap(true);

  m_Alignment = new QLabel(this);
  m_Alignment->setWordWrap(true);

  m_Selection = new QLabel(this);
  m_Selection->setWordWrap(true);

  m_Status = new QLabel(this);
  m_Status->setWordWrap(true);

  // No close button: the panel is closed through its dock tab, and hiding it via the tab keeps
  // the imported trace and the capture log alive for when it is re-opened.
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->addLayout(fileRow);
  layout->addLayout(modeRow);
  layout->addLayout(filterRow);
  layout->addWidget(middle, 1);
  layout->addWidget(m_HoverReadout);
  layout->addWidget(m_Alignment);
  layout->addWidget(m_Selection);
  layout->addWidget(m_Status);

  m_Ctx.AddCaptureViewer(this);

  setNavMode(TraceNavMode::Pan);
  updateCaptureControls();
  updateStatusText();
  updateHighlight();
}

SystemTraceViewer::~SystemTraceViewer()
{
  // Only the viewer registration is released here. This panel is not owned by CaptureContext - the
  // MainWindow creates and retains it, and the dock area owns it once added - so it must not call
  // BuiltinWindowClosed(): that would hit the unrecognised-window branch and log a critical for a
  // panel CaptureContext never handed out. Detaching from the capture viewer list is still required,
  // because the context notifies every registered viewer on capture load / close / event change.
  m_Ctx.RemoveCaptureViewer(this);
}

void SystemTraceViewer::OnCaptureLoaded()
{
  updateCaptureControls();
  rebuild();
}

void SystemTraceViewer::OnRemoteHostChanged()
{
  updateCaptureControls();
  updateStatusText();
}

void SystemTraceViewer::OnCaptureClosed()
{
  m_Correlation = TraceCorrelation();
  m_FrameEventTimes.clear();
  m_FrameDuration = 0.0;
  m_CorrelatedFrame = 0;

  updateCaptureControls();

  if(m_Chart)
    m_Chart->Clear();

  updateStatusText();
  updateHighlight();
}

void SystemTraceViewer::OnSelectedEventChanged(uint32_t)
{
  updateHighlight();
}

void SystemTraceViewer::OnEventChanged(uint32_t)
{
  updateHighlight();
}

void SystemTraceViewer::browseForTrace()
{
  const QString path = RDDialog::getOpenFileName(
      this, tr("Select a system trace to correlate"), m_Ctx.Config().LastCaptureFilePath,
      tr("RenderMax System Trace (*.rdt);;Trace files (*.jsonl *.json *.txt);;All Files (*)"));

  if(path.isEmpty())
    return;

  loadTraceFrom(path);
}

void SystemTraceViewer::reloadTrace()
{
  if(m_TracePath->text().isEmpty())
  {
    browseForTrace();
    return;
  }

  loadTraceFrom(m_TracePath->text());
}

// Runs the importer for `path` and returns the trace. This is the half of the load that has no Qt
// widget involvement at all, so it is also the half that moves to a worker thread -- a phone trace
// is a hundred-megabyte text file and the unoptimised development build needs minutes to parse it.
SystemTrace ImportTraceFile(const QString &path)
{
  // Three inputs are accepted - a Chrome Trace Event Format document, the documented JSON-lines
  // interchange, and a raw systrace/kernel trace text file - and the file's own content decides
  // which importer runs. Selecting on the extension alone would mis-route a `traceconv json` output
  // named `.txt`, and `traceconv systrace` produces a file that starts with a kernel trace header.
  if(QFileInfo(path).suffix().compare(QStringLiteral("rdt"), Qt::CaseInsensitive) == 0)
    return ImportSystemTraceFile(path);
  switch(ClassifyTraceFile(path))
  {
    case TraceFileKind::ChromeJson: return ImportChromeTraceEventFormat(path);
    case TraceFileKind::JsonLines: return ImportSystemTraceJsonLines(path);
    case TraceFileKind::SystraceText: return ImportSystraceText(path);
  }

  return SystemTrace();
}

void SystemTraceViewer::loadTraceFrom(const QString &path, const QStringList &captureWarnings)
{
  const uint64_t serial = ++m_ImportSerial;
  m_Importing = true;
  m_ImportPath = path;

  if(m_TracePath)
    m_TracePath->setText(path);

  updateImportControls();
  updateStatusText();

  // Do not start several hundred-megabyte parses concurrently. Keep only the newest request and
  // start it when the current worker reports back; the warnings are part of that request as well.
  if(m_ImportWorkerRunning)
  {
    m_PendingImportPath = path;
    m_PendingImportWarnings = captureWarnings;
    return;
  }

  startImport(path, serial, captureWarnings);
}

void SystemTraceViewer::startImport(const QString &path, uint64_t serial,
                                    const QStringList &captureWarnings)
{
  m_ImportWorkerRunning = true;
  QPointer<QObject> guard(this);

  LambdaThread *thread = new LambdaThread([path, serial, captureWarnings, guard]() {
    SystemTrace trace = ImportTraceFile(path);
    if(!guard)
      return;

    GUIInvoke::call(guard, [trace, path, serial, captureWarnings, guard]() {
      if(guard.isNull())
        return;
      static_cast<SystemTraceViewer *>(guard.data())
          ->finishImport(trace, path, serial, captureWarnings);
    });
  });

  thread->setName(lit("ImportSystemTrace"));
  thread->selfDelete(true);
  thread->start();
}

void SystemTraceViewer::finishImport(const SystemTrace &trace, const QString &path,
                                     uint64_t serial, const QStringList &captureWarnings)
{
  m_ImportWorkerRunning = false;

  // The result is stale, but a newer request is waiting. Start exactly that newest request; do not
  // apply the old trace and do not let its capture warnings leak into the newer document.
  if(serial != m_ImportSerial)
  {
    if(!m_PendingImportPath.isEmpty())
    {
      const QString nextPath = m_PendingImportPath;
      const QStringList nextWarnings = m_PendingImportWarnings;
      m_PendingImportPath.clear();
      m_PendingImportWarnings.clear();
      startImport(nextPath, m_ImportSerial, nextWarnings);
    }
    return;
  }

  m_Importing = false;
  m_ImportPath.clear();
  m_Trace = trace;
  m_Loaded = true;

  updateImportControls();
  applyImportedTrace(path, captureWarnings);
}

void SystemTraceViewer::applyImportedTrace(const QString &path,
                                           const QStringList &captureWarnings)
{
  m_TracePath->setText(path);
  m_Loaded = true;

  // Fill the informational side panels. These are static for a given import, so they are not
  // touched on every frame change.
  m_Series->clear();
  for(int i = 0; i < m_Trace.tracks.size(); i++)
  {
    const TraceTrack &track = m_Trace.tracks[i];
    QListWidgetItem *item = NULL;

    if(track.kind == TraceTrackKind::Counter)
    {
      QString unit;
      if(track.counterSeries >= 0 && track.counterSeries < m_Trace.series.size())
        unit = m_Trace.series[track.counterSeries].unit;

      item = new QListWidgetItem(unit.isEmpty()
                                     ? tr("Metric: %1").arg(track.name)
                                     : tr("Metric: %1 [%2]").arg(track.name, unit));
    }
    else
    {
      item = new QListWidgetItem(tr("Track: %1 (%2 spans)%3")
                                     .arg(track.name)
                                     .arg(track.spans.size())
                                     .arg(track.group.isEmpty()
                                              ? QString()
                                              : tr(" in %1").arg(track.group)));
    }

    item->setData(Qt::UserRole, i);
    m_Series->addItem(item);
  }

  // A trace that carried only counters has no lanes, so the list falls back to the series it does
  // have rather than appearing empty.
  if(m_Trace.tracks.isEmpty())
  {
    for(const TraceSeries &series : m_Trace.series)
    {
      m_Series->addItem(series.unit.isEmpty()
                            ? tr("%1 (%2 samples)").arg(series.name).arg(series.samples.size())
                            : tr("%1 [%2] (%3 samples)")
                                  .arg(series.name, series.unit)
                                  .arg(series.samples.size()));
    }
  }

  m_Frames->setRowCount(m_Trace.frameBoundaries.size());
  for(int i = 0; i < m_Trace.frameBoundaries.size(); i++)
  {
    const TraceFrameBoundary &b = m_Trace.frameBoundaries[i];

    QTableWidgetItem *frameItem = new QTableWidgetItem(QString::number(b.frameNumber));
    QTableWidgetItem *timeItem = new QTableWidgetItem(FormatSeconds(b.timeSec));

    // How much of the frame the GPU spent executing, measured from the kernel's own GPU counters.
    // Shown as a duration and not as a percentage: the frame's length is the interval to the next
    // boundary, but a frame usually ends when the CPU presents rather than when the GPU finishes, so
    // a percentage of it would compare the GPU against something it is not bounded by. A dash means
    // the trace carried no usable GPU counters, which is a different statement from zero.
    QTableWidgetItem *gpuItem =
        new QTableWidgetItem(b.gpuBusySec < 0.0 ? tr("-")
                                               : FormatSeconds(b.gpuBusySec));

    m_Frames->setItem(i, 0, frameItem);
    m_Frames->setItem(i, 1, timeItem);
    m_Frames->setItem(i, 2, gpuItem);
  }

  updateThreadTime();

  m_Warnings->clear();
  for(const QString &warning : m_Trace.warnings)
    m_Warnings->addItem(warning);

  // Warnings travel with the import request, so they cannot be attached to a newer manually opened
  // trace if the capture result was superseded while parsing.
  for(const QString &warning : captureWarnings)
    m_Warnings->addItem(tr("Capture: %1").arg(warning));

  // A newly imported trace starts from its full extent, and any previous annotations and
  // calipers belong to the trace that is gone.
  if(m_Chart)
    m_Chart->ResetView();

  if(m_Element)
    m_Element->setRowCount(0);

  if(m_ElementCaption)
    m_ElementCaption->setText(tr("No element selected"));

  // The selection pane names a window of the trace that has just been replaced.
  updateRangeSelection(false, 0.0, 0.0);

  if(m_HoverReadout)
    m_HoverReadout->clear();

  rebuild();
  updateStatusText();
}

void SystemTraceViewer::updateImportControls()
{
  // Only the record controls need to know: opening a different file during an import is allowed and
  // supersedes the parse in flight, so Open Trace... and Reload stay usable.
  updateCaptureControls();
}

void SystemTraceViewer::captureTrace()
{
  if(m_Capturing)
    return;

  // The device is reached through the protocol of the host this session is connected to. When no
  // protocol is active there is nothing this path can talk to, so the button is refused rather
  // than starting a pipeline that would fail at the first command.
  const RemoteHost remote = m_Ctx.Replay().CurrentRemote();
  IDeviceProtocolController *protocol = remote.Protocol();

  if(protocol == NULL)
  {
    RDDialog::warning(this, tr("Cannot capture a trace"),
                      tr("No device protocol is connected. Connect to an Android device (or "
                         "another protocol that supports file transfer) and try again."));
    return;
  }

  TraceCaptureOptions opts;
  opts.durationMs = m_CaptureSeconds->value() * 1000;
  opts.gpuDetail = GpuTraceDetail(m_CaptureDetail->itemData(m_CaptureDetail->currentIndex()).toInt());

  // The only place a `traceconv` binary may be found is the Snapdragon Profiler installation the
  // user already pointed at in Settings; the driver falls back to PATH discovery when this is
  // empty. Nothing is bundled or redistributed with RenderMax.
  const QString profilerPath =
      QString::fromUtf8(m_Ctx.Config().ExternalTool_SnapdragonProfiler.c_str());
  if(!profilerPath.isEmpty())
    opts.snapdragonProfilerPath = QFileInfo(profilerPath).absolutePath();

  // The hostname IS the device URL the caller selected this device by, so it is passed through
  // unchanged: the driver must never re-derive the device identity.
  const QString url = QString::fromUtf8(remote.Hostname().c_str());

  m_Capturing = true;
  updateCaptureControls();

  m_Status->setText(tr("Recording up to %1 seconds of system trace on %2. The recording is "
                       "bounded by that duration because a device command cannot be interrupted "
                       "once it is running.")
                        .arg(m_CaptureSeconds->value())
                        .arg(url));

  if(m_CaptureLog)
  {
    m_CaptureLog->clear();
    m_CaptureLog->appendPlainText(tr("Starting capture on %1 (duration %2 ms, %3)")
                                      .arg(url)
                                      .arg(opts.durationMs)
                                      .arg(m_CaptureDetail->currentText()));
  }

  // The driver blocks on every device command, so it runs on a worker thread; TraceCaptureJob
  // delivers the result back on this object's thread and guards against the dialog being closed
  // while the pipeline is still running.
  TraceCaptureJob::Start(this, protocol, url, opts, [this](const TraceCaptureResult &result) {
    m_Capturing = false;
    updateCaptureControls();

    if(m_CaptureLog)
    {
      m_CaptureLog->clear();
      for(const QString &line : result.log)
        m_CaptureLog->appendPlainText(line);

      if(!result.error.isEmpty())
        m_CaptureLog->appendPlainText(tr("\nFAILED: %1").arg(result.error));

      if(!result.localTracePath.isEmpty())
        m_CaptureLog->appendPlainText(tr("\nPerfetto trace: %1").arg(result.localTracePath));
      if(!result.localTimelinePath.isEmpty())
        m_CaptureLog->appendPlainText(tr("Importable timeline: %1").arg(result.localTimelinePath));
    }

    if(!result.success)
    {
      updateStatusText();
      const QString persistentLog =
          result.localLogPath.isEmpty()
              ? QString()
              : tr("\n\nPersistent Capture Log: %1").arg(result.localLogPath);
      RDDialog::warning(this, tr("Trace capture failed"),
                        tr("The trace could not be recorded or converted.\n\n%1\n\nEvery command "
                           "that ran is listed in the Capture Log tab.%2")
                            .arg(result.error.isEmpty() ? tr("unknown error") : result.error,
                                 persistentLog));
      return;
    }

    // The capture log is filled before the import starts, because the import now runs on a worker
    // thread and would otherwise clear the log from underneath it.
    if(m_CaptureLog && !result.warnings.isEmpty())
    {
      m_CaptureLog->appendPlainText(QStringLiteral("\nWarnings:"));
      for(const QString &warning : result.warnings)
        m_CaptureLog->appendPlainText(tr("  %1").arg(warning));
    }

    // Anything the capture could not collect is listed beside the trace's own warnings, so a lane
    // that is missing because the device refused to report it reads as a device limitation rather
    // than as an importer that failed to parse something. The import is asynchronous, so these are
    // parked and added by applyImportedTrace() once the trace is actually in place.
    // The captured timeline is a document the importer accepts as-is, so it is loaded through the
    // normal path: one code path for both captured and manually opened traces. Warnings are attached
    // to this request rather than stored globally while another import may supersede it.
    loadTraceFrom(result.localTimelinePath, result.warnings);
  });
}

void SystemTraceViewer::updateCaptureControls()
{
  if(m_Capture == NULL || m_CaptureSeconds == NULL)
    return;

  const bool haveProtocol = m_Ctx.Replay().CurrentRemote().Protocol() != NULL;

  // A capture cannot start while a trace is being parsed: its result would arrive at the panel out
  // of order, and the panel would show the recording's trace only after the file being opened.
  m_Capture->setEnabled(haveProtocol && !m_Capturing && !m_Importing);
  m_CaptureSeconds->setEnabled(!m_Capturing);

  if(m_Capturing)
  {
    m_Capture->setText(tr("Recording..."));
    m_Capture->setToolTip(
        tr("A recording cannot be stopped early: the device protocol serialises commands, so the "
           "run finishes at the duration chosen above."));
  }
  else
  {
    m_Capture->setText(tr("Capture Trace..."));
    m_Capture->setToolTip(haveProtocol ? tr("Record a system trace directly from the connected "
                                            "device with its own perfetto, then import it.")
                                       : tr("Connect to a device that implements file transfer "
                                            "commands to record a trace."));
  }
}

void SystemTraceViewer::setNavMode(TraceNavMode mode)
{
  if(m_Chart)
    m_Chart->SetNavMode(mode);

  // The buttons are kept in step with the timeline, whichever drove the change (a button, or the
  // 1/2/3 shortcut handled inside the timeline).
  if(m_ModeSelect)
    m_ModeSelect->setChecked(mode == TraceNavMode::Select);
  if(m_ModePan)
    m_ModePan->setChecked(mode == TraceNavMode::Pan);
  if(m_ModeZoom)
    m_ModeZoom->setChecked(mode == TraceNavMode::Zoom);
}

// The trace-time extent of the frame currently correlated with the capture. False when there is no
// capture, no correlation, or the frame was not matched, in which case the table has to fall back to
// the whole trace rather than pretend to a window it does not have.
bool SystemTraceViewer::frameWindow(double &beginSec, double &endSec) const
{
  if(m_FrameEventTimes.isEmpty() || !m_Ctx.IsCaptureLoaded())
    return false;

  const uint32_t frame =
      m_CorrelatedFrame != 0 ? m_CorrelatedFrame : m_Ctx.FrameInfo().frameNumber;

  const auto it = m_FrameEventTimes.constFind(frame);
  if(it == m_FrameEventTimes.constEnd())
    return false;

  beginSec = it.value();
  endSec = beginSec + m_FrameDuration;

  return endSec > beginSec;
}

void SystemTraceViewer::exportThreadTime()
{
  if(m_ThreadTimeRows.isEmpty())
    return;

  const QString path = RDDialog::getSaveFileName(
      this, tr("Export thread time"), m_Ctx.Config().LastCaptureFilePath,
      tr("CSV files (*.csv);;All Files (*)"));

  if(path.isEmpty())
    return;

  // Export in the order the table shows, not the order the aggregates were computed in.
  QVector<TraceThreadTime> ordered;
  ordered.reserve(m_ThreadTime->rowCount());
  for(int row = 0; row < m_ThreadTime->rowCount(); row++)
  {
    const QTableWidgetItem *nameItem = m_ThreadTime->item(row, 0);
    const int index = nameItem != NULL ? nameItem->data(Qt::UserRole).toInt() : row;

    if(index >= 0 && index < m_ThreadTimeRows.size())
      ordered.push_back(m_ThreadTimeRows[index]);
  }

  if(ordered.isEmpty())
    ordered = m_ThreadTimeRows;

  QFile file(path);
  if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    RDDialog::critical(this, tr("Export failed"),
                       tr("Could not open %1 for writing.").arg(path));
    return;
  }

  // The formatting lives in the importer so the exported text is covered by the trace tests; the
  // dialog only decides where it goes.
  QTextStream out(&file);
  out << FormatThreadTimeCsv(ordered, m_ThreadTimeWindowSec);

  file.close();
}

void SystemTraceViewer::updateThreadTime()
{
  if(m_ThreadTime == NULL)
    return;

  // Disable sorting while inserting rows: otherwise setItem() can move a half-populated row and the
  // remaining cells land on a different thread. Restore it only after every row is complete.
  m_ThreadTime->setSortingEnabled(false);
  m_ThreadTime->setRowCount(0);

  double windowBegin = 0.0;
  double windowEnd = 0.0;
  const bool frameOnly =
      m_ThreadTimeFrame != NULL && m_ThreadTimeFrame->isChecked() && frameWindow(windowBegin, windowEnd);

  if(frameOnly)
  {
    m_ThreadTimeWindowSec = windowEnd - windowBegin;
    m_ThreadTimeCaption->setText(tr("Frame %1: %2 of trace time")
                                     .arg(m_CorrelatedFrame != 0 ? m_CorrelatedFrame
                                                                 : m_Ctx.FrameInfo().frameNumber)
                                     .arg(FormatSeconds(m_ThreadTimeWindowSec)));
  }
  else
  {
    m_ThreadTimeWindowSec = m_Trace.SpanSec();
    if(m_ThreadTimeCaption != NULL)
    {
      m_ThreadTimeCaption->setText(
          m_ThreadTimeFrame != NULL && m_ThreadTimeFrame->isChecked()
              ? tr("Whole trace (no correlated frame to clip to)")
              : tr("Whole trace"));
    }
  }

  const QVector<TraceThreadTime> rows =
      frameOnly ? m_Trace.ThreadTimes(windowBegin, windowEnd) : m_Trace.ThreadTimes();
  m_ThreadTimeRows = rows;

  // Percentages are shares of the window the numbers were measured over, so a frame-clipped table
  // reads "how much of this frame did the thread run" instead of "how much of the capture".
  const double window = m_ThreadTimeWindowSec;

  m_ThreadTime->setRowCount(rows.size());
  for(int row = 0; row < rows.size(); row++)
  {
    const TraceThreadTime &total = rows[row];
    const double cpuPercent = window > 0.0 ? total.runningSec * 100.0 / window : 0.0;
    const double waitPercent = window > 0.0 ? total.runnableSec * 100.0 / window : 0.0;

    // The row remembers which aggregate it shows so that exporting follows the table's own order:
    // the user sorted it, and a file in insertion order would not be the table they were reading.
    QTableWidgetItem *nameItem = new QTableWidgetItem(total.name);
    nameItem->setData(Qt::UserRole, row);
    m_ThreadTime->setItem(row, 0, nameItem);
    m_ThreadTime->setItem(row, 1, new NumericTableItem(QString::number(total.tid), total.tid));
    m_ThreadTime->setItem(row, 2, new QTableWidgetItem(total.process));
    m_ThreadTime->setItem(
        row, 3, new NumericTableItem(FormatSeconds(total.runningSec), total.runningSec));
    m_ThreadTime->setItem(
        row, 4, new NumericTableItem(FormatSeconds(total.runnableSec), total.runnableSec));
    m_ThreadTime->setItem(
        row, 5, new NumericTableItem(FormatSeconds(total.sleepingSec), total.sleepingSec));
    m_ThreadTime->setItem(row, 6,
                          new NumericTableItem(QString::number(cpuPercent, 'f', 2), cpuPercent));
    m_ThreadTime->setItem(row, 7,
                          new NumericTableItem(QString::number(waitPercent, 'f', 2), waitPercent));
    m_ThreadTime->setItem(row, 8,
                          new NumericTableItem(QString::number(total.switches), total.switches));
  }

  m_ThreadTime->setSortingEnabled(true);
  m_ThreadTime->sortItems(3, Qt::DescendingOrder);
  m_ThreadTime->resizeColumnToContents(1);
  for(int column = 3; column < m_ThreadTime->columnCount(); column++)
    m_ThreadTime->resizeColumnToContents(column);
}

void SystemTraceViewer::showElementFields(const TraceSpan &span, const QString &trackName)
{
  if(m_Element == NULL)
    return;

  m_Element->setRowCount(0);

  const auto addField = [this](const QString &name, const QString &value) {
    const int row = m_Element->rowCount();
    m_Element->insertRow(row);
    m_Element->setItem(row, 0, new QTableWidgetItem(name));
    m_Element->setItem(row, 1, new QTableWidgetItem(value));
  };

  if(!trackName.isEmpty())
    addField(tr("Track"), trackName);

  addField(tr("Name"), span.name);

  if(!span.category.isEmpty())
    addField(tr("Category"), span.category);

  addField(tr("Begin"), FormatSeconds(span.beginSec));
  addField(tr("End"), FormatSeconds(span.endSec));
  addField(tr("Duration"), FormatSeconds(span.endSec - span.beginSec));

  if(span.depth > 0)
    addField(tr("Nesting depth"), QString::number(span.depth));

  // The trace's own argument fields, verbatim. Nothing here is reinterpreted: the importer kept
  // them as the source rendered them.
  for(const QString &field : span.fields)
  {
    const int split = field.indexOf(QStringLiteral(" = "));

    if(split > 0)
      addField(field.left(split), field.mid(split + 3));
    else
      addField(tr("Argument"), field);
  }

  m_Element->resizeColumnToContents(0);

  if(m_ElementCaption)
    m_ElementCaption->setText(trackName.isEmpty()
                                  ? tr("Element: %1").arg(span.name)
                                  : tr("Element: %1 on %2").arg(span.name, trackName));
}

void SystemTraceViewer::updateRangeSelection(bool valid, double beginSec, double endSec)
{
  if(m_RangeTable == NULL)
    return;

  m_RangeTable->setRowCount(0);

  const double window = endSec - beginSec;

  if(!valid || !traceUsable())
  {
    if(m_RangeCaption)
      m_RangeCaption->setText(tr("No time window selected. Drag in Select mode (1)."));
    return;
  }

  // Only the lanes the timeline actually shows are summarised: a type the user has filtered out is
  // not part of what they are looking at, and reporting it here would contradict the plot.
  const QVector<int> series = m_Chart != NULL ? m_Chart->VisibleCounterSeries() : QVector<int>();

  // Raise the pane the selection belongs to, the way SDP opens its Selection view for a window; a
  // summary the user has to go looking for is a summary they will not read.
  if(m_SideTabs != NULL && m_RangePane != NULL)
    m_SideTabs->setCurrentWidget(m_RangePane);

  int withSamples = 0;
  int withoutSamples = 0;

  for(int seriesIndex : series)
  {
    if(seriesIndex < 0 || seriesIndex >= m_Trace.series.size())
      continue;

    const TraceSeries &s = m_Trace.series[seriesIndex];

    TraceSeriesRangeStats stats;
    const bool hasValue = s.RangeStats(beginSec, endSec, stats);

    if(!hasValue)
    {
      withoutSamples++;
      continue;
    }

    withSamples++;

    const int row = m_RangeTable->rowCount();
    m_RangeTable->insertRow(row);

    m_RangeTable->setItem(row, 0, new QTableWidgetItem(s.name));
    m_RangeTable->setItem(row, 1, new NumericTableItem(QString::number(stats.count), double(stats.count)));
    m_RangeTable->setItem(row, 2, new NumericTableItem(FormatValue(stats.min), stats.min));
    m_RangeTable->setItem(row, 3, new NumericTableItem(FormatValue(stats.max), stats.max));
    m_RangeTable->setItem(row, 4, new NumericTableItem(FormatValue(stats.avg), stats.avg));

    // The unit is per series, so it travels on the row rather than in a shared column header.
    QString detail = s.unit.isEmpty()
                         ? tr("%1 samples inside the window; first %2, last %3")
                               .arg(stats.count)
                               .arg(FormatValue(stats.firstValue), FormatValue(stats.lastValue))
                         : tr("%1 samples inside the window; first %2 %3, last %4 %5")
                               .arg(stats.count)
                               .arg(FormatValue(stats.firstValue), s.unit, FormatValue(stats.lastValue),
                                    s.unit);

    if(s.step && stats.hasHeldBegin && stats.hasHeldEnd)
    {
      // A level series holds a value that was measured earlier; state the held levels separately so
      // they are never read as samples taken inside the window.
      detail += tr("\nLevel held %1 at the start of the window, %2 "
                   "at the end (measured earlier, not counted)")
                    .arg(s.unit.isEmpty() ? FormatValue(stats.heldBegin)
                                          : QStringLiteral("%1 %2").arg(FormatValue(stats.heldBegin),
                                                                         s.unit),
                         s.unit.isEmpty() ? FormatValue(stats.heldEnd)
                                          : QStringLiteral("%1 %2").arg(FormatValue(stats.heldEnd),
                                                                         s.unit));
    }

    for(int column = 0; column < m_RangeTable->columnCount(); column++)
    {
      QTableWidgetItem *item = m_RangeTable->item(row, column);
      if(item != NULL)
        item->setToolTip(detail);
    }
  }

  if(m_RangeCaption)
  {
    QString caption = tr("Window %1 to %2 (%3). ")
                          .arg(FormatSeconds(beginSec), FormatSeconds(endSec), FormatSeconds(window));

    if(withSamples == 0)
    {
      caption += tr("No samples in range: this window contains no measurement, so there is nothing "
                    "to average. Widen it or pick a window the trace sampled.");
    }
    else
    {
      // State the counting rule next to the numbers, and what was left out.
      caption += tr("%1 counter%2 with samples, counted only where the trace recorded them "
                    "(no interpolation; averages are of the samples, not of time).")
                     .arg(withSamples)
                     .arg(withSamples == 1 ? QString() : QStringLiteral("s"));

      if(withoutSamples > 0)
        caption += QStringLiteral(" ") + tr("%1 shown counter%2 had none.")
                                               .arg(withoutSamples)
                                               .arg(withoutSamples == 1 ? QString()
                                                                        : QStringLiteral("s"));
    }

    m_RangeCaption->setText(caption);
  }

  m_RangeTable->resizeColumnToContents(0);
}

void SystemTraceViewer::rebuild()
{
  // Any cached correlation belongs to the capture it was computed for, so drop it unconditionally
  // before deciding what can be drawn.
  m_FrameEventTimes.clear();
  m_FrameDuration = 0.0;
  m_CorrelatedFrame = 0;
  m_Correlation = TraceCorrelation();

  if(!traceUsable())
  {
    if(m_Chart)
      m_Chart->Clear();

    updateHighlight();
    updateStatusText();
    return;
  }

  // The imported lanes do not depend on RenderMax: keep showing them (on their own time base)
  // even with no capture open. Only the frame overlay and the per-event readout need a capture.
  if(!m_Ctx.IsCaptureLoaded())
  {
    showTraceOnly();
    updateStatusText();
    return;
  }

  const uint32_t frame = m_Ctx.FrameInfo().frameNumber;

  // The per-event timing is a RenderMax replay query, so it must run on the replay thread. The
  // anchor's timeSec is 0 because RenderMax exposes no absolute frame-origin timestamp: action
  // times are already relative to the frame origin (see ClockDomainAlignment).
  m_Ctx.Replay().AsyncInvoke([this, frame](IReplayController *r) {
    const rdcarray<CounterResult> results = r->FetchCounters({GPUCounter::EventGPUDuration});

    // Prefix-sum the per-event durations in event-id order to obtain each event's start time
    // relative to the frame origin. Event ids increase in submission order, so this ordering is
    // the same one RenderMax's own event browser uses.
    std::vector<CounterResult> sorted;
    sorted.reserve((size_t)results.size());
    for(const CounterResult &cr : results)
      sorted.push_back(cr);
    std::sort(sorted.begin(), sorted.end(),
              [](const CounterResult &a, const CounterResult &b) { return a.eventId < b.eventId; });

    QHash<uint32_t, double> starts;
    starts.reserve(int(sorted.size()));

    double total = 0.0;
    for(const CounterResult &cr : sorted)
    {
      starts.insert(cr.eventId, total);
      total += cr.value.d;
    }

    // GUIInvoke::call guards against the dialog being closed before the result arrives.
    GUIInvoke::call(this, [this, frame, starts, total]() {
      m_FrameEventTimes = starts;
      m_FrameDuration = total;
      m_CorrelatedFrame = frame;

      updateHighlight();
      updateStatusText();
    });
  });
}

void SystemTraceViewer::updateHighlight()
{
  if(m_Chart == NULL)
    return;

  if(!traceUsable())
  {
    m_Chart->Clear();
    m_Alignment->setText(QString());
    m_Selection->setText(QString());
    return;
  }

  if(!m_Ctx.IsCaptureLoaded())
  {
    // The lanes stay visible; the frame overlay and the per-event readout cannot be produced
    // without a capture, so they are cleared instead of showing stale data.
    showTraceOnly();
    return;
  }

  const uint32_t frame =
      m_CorrelatedFrame != 0 ? m_CorrelatedFrame : m_Ctx.FrameInfo().frameNumber;

  QVector<TraceFrameAnchor> anchors;
  TraceFrameAnchor anchor;
  anchor.frameNumber = frame;
  anchor.timeSec = 0.0;
  anchors.push_back(anchor);

  const QVector<TraceActionRef> actions =
      FlattenActionsForCorrelation(m_Ctx.CurRootActions(), frame, m_FrameEventTimes);

  m_Correlation = CorrelateSystemTrace(m_Trace, actions, anchors);

  SystemTraceTimelineData chartData;
  chartData.trace = &m_Trace;
  chartData.actions = m_Correlation.actions;
  chartData.alignment = m_Correlation.alignment;
  chartData.aligned = m_Correlation.aligned();
  chartData.frameStartSec = m_Correlation.alignment.offsetSec;
  chartData.frameEndSec = chartData.frameStartSec + m_FrameDuration;
  chartData.frameNumber = frame;
  chartData.highlightEvent = m_Ctx.CurSelectedEvent();

  m_Chart->SetData(chartData);

  m_Alignment->setText(tr("Clock alignment: %1%2  |  %3")
                           .arg(ConfidenceName(m_Correlation.alignment.confidence))
                           .arg(m_Correlation.alignment.toleranceSec > 0.0
                                    ? tr(" (tolerance %1)")
                                          .arg(FormatSeconds(m_Correlation.alignment.toleranceSec))
                                    : QString())
                           .arg(m_Correlation.alignment.description));

  // Describe the selected event in terms of the system counters, if we could place it.
  QString selectionText;
  const uint32_t selected = m_Ctx.CurSelectedEvent();
  for(const TraceActionRef &action : m_Correlation.actions)
  {
    if(action.eventId != selected)
      continue;

    selectionText = tr("Selected EID %1: %2").arg(action.eventId).arg(action.name);

    if(m_Correlation.aligned() && m_FrameDuration > 0.0)
    {
      const double traceTime = m_Correlation.alignment.offsetSec + action.timeSec;

      QStringList readings;
      for(const TraceSeries &series : m_Trace.series)
      {
        double value = 0.0;
        if(series.SampleAt(traceTime, value))
        {
          readings << (series.unit.isEmpty() ? tr("%1=%2").arg(series.name, FormatValue(value))
                                             : tr("%1=%2 %3").arg(series.name,
                                                                  FormatValue(value), series.unit));
        }
      }

      if(!readings.isEmpty())
        selectionText += QStringLiteral("  |  ") + readings.join(QStringLiteral(", "));
      else
        selectionText += QStringLiteral("  |  ") + tr("no samples near this instant");
    }
    else if(!action.hasGpuDuration)
    {
      selectionText += QStringLiteral("  |  ") + tr("no GPU timing: cannot place on the trace");
    }

    break;
  }

  m_Selection->setText(selectionText);

  // A frame-clipped thread-time table is a view of the frame that is now selected, so it has to be
  // recomputed when the capture moves to another one. It is only rebuilt when the window is in use,
  // because the whole-trace totals cannot change.
  if(m_ThreadTimeFrame != NULL && m_ThreadTimeFrame->isChecked())
    updateThreadTime();
}

void SystemTraceViewer::showTraceOnly()
{
  // Trace-only presentation: the lanes are drawn on their own time base and both correlation
  // readouts are emptied, because neither has a meaning without a frame to align to.
  if(m_Chart)
  {
    SystemTraceTimelineData chartData;
    chartData.trace = &m_Trace;
    chartData.aligned = false;
    chartData.captureLoaded = false;

    m_Chart->SetData(chartData);
  }

  if(m_Alignment)
  {
    m_Alignment->setText(
        tr("Clock alignment: not applicable — no capture is loaded, so the trace is shown on its "
           "own time base."));
  }

  if(m_Selection)
    m_Selection->setText(QString());
}

void SystemTraceViewer::selectEvent(uint32_t eventId)
{
  if(eventId == 0)
    return;

  // Exclude ourselves so the selection change does not bounce straight back into updateHighlight.
  m_Ctx.SetEventID({this}, eventId, eventId);
}

void SystemTraceViewer::updateStatusText()
{
  if(m_Status == NULL)
    return;

  // Parsing happens on a worker thread and a phone trace can take a while, so the state has to be
  // visible: without it the panel looks idle and the only sign of activity is a file name in the
  // path box. This takes precedence over everything else, including a previously loaded trace.
  if(m_Importing)
  {
    m_Status->setText(
        tr("Importing %1 ... The file is parsed on a worker thread, so the panel stays usable; "
           "opening another trace supersedes this one.")
            .arg(QFileInfo(m_ImportPath).fileName()));
    return;
  }

  if(!m_Loaded)
  {
    if(m_Capturing)
    {
      m_Status->setText(tr("Recording a system trace on the connected device..."));
      return;
    }

    const bool haveProtocol = m_Ctx.Replay().CurrentRemote().Protocol() != NULL;

    if(haveProtocol)
    {
      m_Status->setText(tr("No trace loaded. Use \"Capture Trace...\" to record one directly from "
                           "the connected device, or \"Open Trace...\" to import one. Then click a "
                           "lane element to read its fields, or a draw/dispatch marker to select it "
                           "in RenderMax."));
    }
    else
    {
      m_Status->setText(tr("No trace loaded. Use \"Open Trace...\" to import one, then click a lane "
                           "element to read its fields, or a draw/dispatch marker to select it in "
                           "RenderMax. Recording a trace needs a connected device, which this "
                           "session does not have."));
    }
    return;
  }

  if(!m_Trace.valid)
  {
    m_Status->setText(tr("Could not import the trace: %1")
                          .arg(m_Trace.warnings.isEmpty() ? tr("unknown error")
                                                          : m_Trace.warnings.join(QLatin1Char(' '))));
    return;
  }

  const QString source = m_Trace.source.isEmpty() ? tr("(unnamed)") : m_Trace.source;

  if(!m_Ctx.IsCaptureLoaded())
  {
    m_Status->setText(tr("Trace: %1  |  %2 tracks, %3 spans, %4 metrics, %5 frame boundaries. No "
                         "capture is loaded, so there is nothing to correlate against yet; open a "
                         "capture and the frame overlay appears here.")
                          .arg(source)
                          .arg(m_Trace.tracks.size())
                          .arg(m_Trace.SpanCount())
                          .arg(m_Trace.series.size())
                          .arg(m_Trace.frameBoundaries.size()));
    return;
  }

  m_Status->setText(tr("Trace: %1  |  %2 tracks, %3 spans, %4 metrics, %5 frame boundaries, %6 "
                       "matched to this capture")
                        .arg(source)
                        .arg(m_Trace.tracks.size())
                        .arg(m_Trace.SpanCount())
                        .arg(m_Trace.series.size())
                        .arg(m_Trace.frameBoundaries.size())
                        .arg(m_Correlation.matchedFrames));
}
