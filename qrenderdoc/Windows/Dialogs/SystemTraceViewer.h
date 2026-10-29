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

#pragma once

#include <functional>

#include <QFrame>
#include <QString>
#include <QVector>

#include "Code/Interface/QRDInterface.h"
#include "Code/SystemTrace.h"
#include "Code/SystemTraceCorrelation.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QToolButton;

// Navigation modes of the Trace view. These are the three modes Snapdragon Profiler documents for
// its Trace window ("Select, Pan, and Zoom ... switched via the toolbar buttons or the shortcut
// keys 1, 2, and 3"), kept with the same meaning and the same key bindings.
enum class TraceNavMode
{
  // Drag a lasso to select elements; the selection is described in the element pane.
  Select = 0,
  // Drag to shift the visible window. The default.
  Pan = 1,
  // Drag a region to zoom into it.
  Zoom = 2,
};

// Forward-declared: implemented entirely in the .cpp as a plain QWidget with a custom paintEvent.
// There is no QtCharts dependency in this project, so the lane rows, the shared ruler, the calipers
// and the frame overlay are all drawn by hand.
//
// The widget's data is declared in the .cpp rather than here: it carries the drawn state the viewer
// only ever hands over wholesale, so keeping it out of this header avoids a struct that no other
// translation unit can use.
class SystemTraceTimeline;

// P1-4: frame-interior counter x system-level trace correlation view.
//
// A dockable tool panel (the same shape as StatisticsViewer / TimelineBar) that overlays an
// imported system trace (bandwidth / occupancy curves) against a single RenderDoc frame's
// timeline, and lets the user click a draw / dispatch to both select it in RenderDoc and read
// the system counters at that instant.
//
// It is a panel rather than a floating dialog so it docks into the main window as a tab and
// participates in the saved layout. Ownership stays with the caller (MainWindow) for exactly
// that reason: QFrame panels are reused and hidden, never self-deleting, otherwise the dock
// would be left holding a dangling tab.
//
// It is a capture viewer: it follows the current frame and event, and clears itself when the
// capture closes. It never modifies the capture and never bundles a vendor binary.
class SystemTraceViewer : public QFrame, public ICaptureViewer
{
public:
  explicit SystemTraceViewer(ICaptureContext &ctx, QWidget *parent = 0);
  ~SystemTraceViewer();

  // The handle the tool window manager docks. Returns this, since the whole panel is one widget.
  QWidget *Widget() { return this; }

  // ICaptureViewer
  void OnCaptureLoaded() override;
  void OnCaptureClosed() override;
  void OnSelectedEventChanged(uint32_t eventId) override;
  void OnEventChanged(uint32_t eventId) override;

  // The replay context can change without loading or closing a capture. Refresh device-recording
  // availability at that point so a newly connected Android host enables Capture Trace immediately.
  void OnRemoteHostChanged();

private:
  void browseForTrace();
  void reloadTrace();
  // Two importers exist, so the format is chosen from the file itself: the JSON-lines interchange,
  // or the Chrome Trace Event Format that a device capture produces.
  void loadTraceFrom(const QString &path);

  // Record a system trace from the connected device. The capture is driven entirely through the
  // device protocol (ExecCommand / PushFile / PullFile), so it works on any device the protocol can
  // reach and does not involve Snapdragon Profiler itself.
  void captureTrace();
  // Capture is only offered while a protocol-backed host is connected and no capture is running.
  void updateCaptureControls();

  // Kick off the (async) event-timing fetch and, once it arrives, correlate + redraw.
  void rebuild();
  // A trace is usable when it carries anything the timeline can draw: counter samples, spans, or
  // frame boundaries. Requiring series alone would reject a slice-only trace, which is the shape a
  // plain systrace capture has.
  bool traceUsable() const
  {
    return m_Trace.valid && (!m_Trace.series.isEmpty() || m_Trace.SpanCount() > 0 ||
                             !m_Trace.frameBoundaries.isEmpty());
  }

  // Switch the timeline's navigation mode and keep the toolbar buttons in step, whichever drove
  // the change (a button or a 1/2/3 shortcut inside the timeline).
  void setNavMode(TraceNavMode mode);
  // Describe the span selected in the timeline in the element pane.
  void showElementFields(const TraceSpan &span, const QString &trackName);
  // Rebuild the per-thread scheduler-state summary from the imported lanes. This is deliberately
  // derived from spans rather than maintained by the parser, so JSON and systrace imports use the
  // exact same accounting and the table cannot drift from what the timeline draws.
  void updateThreadTime();
  // The trace-time extent of the frame currently correlated with the capture, for clipping the
  // thread-time table to it. False when there is no capture or the frame was not matched.
  bool frameWindow(double &beginSec, double &endSec) const;
  // Write the thread-time rows currently shown, percentages included, as CSV.
  void exportThreadTime();

  void updateHighlight();
  // Draw the imported series without any frame overlay (no capture open, so nothing to align to).
  void showTraceOnly();
  void selectEvent(uint32_t eventId);
  void updateStatusText();

  ICaptureContext &m_Ctx;

  SystemTrace m_Trace;
  bool m_Loaded = false;
  TraceCorrelation m_Correlation;

  // Cache so a redraw does not need to re-fetch counters.
  QHash<uint32_t, double> m_FrameEventTimes;
  double m_FrameDuration = 0.0;
  uint32_t m_CorrelatedFrame = 0;

  QLineEdit *m_TracePath = NULL;
  QListWidget *m_Series = NULL;
  QTableWidget *m_Frames = NULL;
  QTableWidget *m_ThreadTime = NULL;
  QCheckBox *m_ThreadTimeFrame = NULL;
  QLabel *m_ThreadTimeCaption = NULL;
  QPushButton *m_ThreadTimeExport = NULL;
  // The rows behind the table, kept for export so the file matches what is on screen.
  QVector<TraceThreadTime> m_ThreadTimeRows;
  // Seconds the percentages are shares of: the frame window when clipping, else the whole trace.
  double m_ThreadTimeWindowSec = 0.0;
  // The counter value under the cursor, as text. The timeline draws the same number next to the
  // curve, but a painter-drawn label is invisible to screen readers and to UI automation, so the
  // readable copy lives here.
  QLabel *m_HoverReadout = NULL;
  QLabel *m_Alignment = NULL;
  QLabel *m_Selection = NULL;
  QLabel *m_Status = NULL;
  QListWidget *m_Warnings = NULL;
  SystemTraceTimeline *m_Chart = NULL;

  // Element pane: the fields of whatever span was last selected on the timeline. This is the job
  // SDP's Inspector does for a selected trace element.
  QTableWidget *m_Element = NULL;
  QLabel *m_ElementCaption = NULL;

  QToolButton *m_ModeSelect = NULL;
  QToolButton *m_ModePan = NULL;
  QToolButton *m_ModeZoom = NULL;

  // Lane visibility controls. A phone trace has several hundred thread lanes; these are the means
  // of narrowing that to the handful a user is actually looking at, matching how Snapdragon
  // Profiler's flow view is driven from a metric selection instead of from every thread.
  QLineEdit *m_TrackFilter = NULL;
  QCheckBox *m_ActiveOnly = NULL;
  QPushButton *m_CollapseGroups = NULL;
  QPushButton *m_ExpandGroups = NULL;

  // Device capture controls. The log is kept visible after the capture finishes because every
  // command the driver issued and every tool result is recorded there: a failure is diagnosable
  // from what actually ran instead of from a single generic message.
  QPushButton *m_Capture = NULL;
  QSpinBox *m_CaptureSeconds = NULL;
  QPlainTextEdit *m_CaptureLog = NULL;
  bool m_Capturing = false;
};
