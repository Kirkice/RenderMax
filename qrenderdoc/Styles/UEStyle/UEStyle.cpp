/******************************************************************************
 * The MIT License (MIT)
 ******************************************************************************/

#include "UEStyle.h"

#include <QApplication>

UEStyle::UEStyle() : RDStyle(RDStyle::Dark)
{
}

void UEStyle::applyPalette(QPalette &palette)
{
  // Exact sRGB defaults from UnrealEngine/Engine/Source/Runtime/SlateCore/Private/
  // Styling/StyleColors.cpp, USlateThemeManager::InitializeDefaults(). Qt consumes
  // sRGB colours directly; do not use Slate's linear-light values here.
  const QColor window("#242424");      // Panel
  const QColor base("#0F0F0F");        // Input
  const QColor input("#1A1A1A");       // Recessed (alternating rows)
  const QColor button("#383838");      // Secondary
  const QColor text("#C0C0C0");        // Foreground
  const QColor muted("#808080");       // AccentGray
  const QColor border("#383838");      // InputOutline
  const QColor accent("#0070E0");      // Primary / Select / Highlight

  palette = QPalette(text, window, QColor("#575757"), QColor("#0F0F0F"), border, text, base);
  palette.setColor(QPalette::Window, window);
  palette.setColor(QPalette::Base, base);
  palette.setColor(QPalette::AlternateBase, input);
  palette.setColor(QPalette::Button, button);
  palette.setColor(QPalette::ButtonText, text);
  palette.setColor(QPalette::Text, text);
  palette.setColor(QPalette::WindowText, text);
  palette.setColor(QPalette::Mid, border);
  palette.setColor(QPalette::Dark, QColor("#0F0F0F"));
  palette.setColor(QPalette::Light, QColor("#575757"));
  palette.setColor(QPalette::Midlight, QColor("#4C4C4C"));
  palette.setColor(QPalette::Shadow, QColor("#000000"));
  palette.setColor(QPalette::Highlight, accent);
  palette.setColor(QPalette::HighlightedText, Qt::white);
  palette.setColor(QPalette::Link, QColor("#0070E0"));
  palette.setColor(QPalette::LinkVisited, QColor("#A139BF"));
  palette.setColor(QPalette::BrightText, Qt::white);
  palette.setColor(QPalette::ToolTipBase, QColor("#383838"));
  palette.setColor(QPalette::ToolTipText, text);

  palette.setColor(QPalette::Inactive, QPalette::Highlight, QColor("#40576F"));
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText, Qt::white);
  palette.setColor(QPalette::Disabled, QPalette::WindowText, muted);
  palette.setColor(QPalette::Disabled, QPalette::Text, muted);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
  palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor("#40576F"));
  palette.setColor(QPalette::Disabled, QPalette::Base, QColor("#1A1A1A"));
  palette.setColor(QPalette::Disabled, QPalette::Button, window);
}

void UEStyle::polish(QPalette &palette)
{
  applyPalette(palette);
}

QPalette UEStyle::standardPalette() const
{
  QPalette palette = RDStyle::standardPalette();
  applyPalette(palette);
  return palette;
}

void UEStyle::polish(QApplication *app)
{
  RDStyle::polish(app);
  app->setPalette(standardPalette());

  // A restrained Starship-like surface treatment. This is intentionally applied at
  // the application level so existing .ui files gain the new visual language without
  // changing their geometry or behaviour.
  QString sheet = QStringLiteral(R"qss(
    /* Product-style foundation: clear surface hierarchy, restrained contrast and
       visible focus/selection states. No widget is removed or functionally altered. */
    QMainWindow, QDialog {
      background: #1c1f21;
    }
    /* Keep the native RenderMax geometry. The theme changes surfaces and states, not
       control dimensions; this preserves the compact editor density and font metrics. */
    #systemTraceViewer {
      background: #1c1f21;
    }
    #systemTraceViewer QLabel {
      color: #aeb6bd;
    }
    #systemTraceViewer QLineEdit#tracePath {
      color: #9fa8af;
      background: #141617;
      border-color: #343b41;
    }
    #systemTraceViewer QPushButton#primaryCaptureButton {
      color: #ffffff;
      background: #3976a8;
      border: 1px solid #67a9d7;
      border-radius: 3px;
      padding: 5px 14px;
      font-weight: bold;
    }
    #systemTraceViewer QPushButton#primaryCaptureButton:hover {
      background: #4c8fbe;
      border-color: #9bd0f0;
    }
    #systemTraceViewer QPushButton#primaryCaptureButton:pressed {
      background: #28577d;
    }
    #systemTraceViewer QLabel#traceReadout {
      color: #dbe7ee;
      background: #20282e;
      border-left: 3px solid #4e9bd1;
      padding: 4px 8px;
    }
    #systemTraceViewer QLabel#traceStatus {
      color: #87939c;
      padding: 2px 4px;
    }
    #systemTraceViewer QTabWidget::pane {
      border: 1px solid #343c43;
      background: #17191b;
      top: -1px;
    }
    #systemTraceViewer QTabBar::tab {
      padding: 6px 11px;
    }
    QMenuBar {
      color: #c9ced2;
      background: #202326;
      border-bottom: 1px solid #101214;
      padding: 2px 4px;
    }
    QMenuBar::item {
      background: transparent;
      padding: 5px 9px;
      border-radius: 2px;
    }
    QMenuBar::item:selected, QMenuBar::item:pressed {
      color: #ffffff;
      background: #35414b;
    }
    QMenu {
      color: #d9dde0;
      background: #25292c;
      border: 1px solid #4b535b;
      padding: 4px;
    }
    QMenu::item {
      padding: 5px 28px 5px 24px;
      border-radius: 2px;
    }
    QMenu::item:selected {
      color: #ffffff;
      background: #3976a8;
    }
    QMenu::separator {
      height: 1px;
      background: #3a4147;
      margin: 4px 7px;
    }
    QToolBar {
      background: #24282b;
      border: 0;
      border-bottom: 1px solid #111315;
      spacing: 3px;
      padding: 3px 5px;
    }
    QToolBar::separator {
      width: 1px;
      background: #464e55;
      margin: 4px 5px;
    }
    QToolBar QToolButton {
      padding: 2px;
      border: 1px solid transparent;
      background: transparent;
    }
    QToolBar QToolButton:hover {
      background: #38434c;
      border-color: #596b78;
    }
    QToolBar QToolButton:checked {
      background: #28577d;
      border-color: #4e9bd1;
    }
    QStatusBar {
      color: #9da6ad;
      background: #202326;
      border-top: 1px solid #111315;
    }
    QStatusBar::item {
      border: 0;
      padding: 2px 6px;
    }
    QSplitter::handle {
      background: #17191b;
    }
    QSplitter::handle:hover {
      background: #4e9bd1;
    }
    QCheckBox, QRadioButton {
      color: #cdd2d6;
      spacing: 6px;
    }
    QCheckBox:hover, QRadioButton:hover {
      color: #ffffff;
    }
    QCheckBox::indicator, QRadioButton::indicator {
      width: 12px;
      height: 12px;
    }
    QCheckBox::indicator {
      background: #17191b;
      border: 1px solid #5a646c;
      border-radius: 2px;
    }
    QCheckBox::indicator:hover { border-color: #73add5; }
    QCheckBox::indicator:checked {
      background: #3976a8;
      border-color: #73add5;
    }
    QRadioButton::indicator {
      background: #17191b;
      border: 1px solid #5a646c;
      border-radius: 7px;
    }
    QRadioButton::indicator:checked {
      background: #3976a8;
      border: 4px solid #17191b;
    }
    QProgressBar {
      color: #e5e8ea;
      background: #17191b;
      border: 1px solid #3d444a;
      border-radius: 2px;
      text-align: center;
    }
    QProgressBar::chunk {
      background: #3976a8;
      border-radius: 1px;
    }
    QSlider::groove:horizontal {
      height: 4px;
      background: #3b4248;
      border-radius: 2px;
    }
    QSlider::sub-page:horizontal { background: #3976a8; border-radius: 2px; }
    QSlider::handle:horizontal {
      width: 12px;
      margin: -5px 0;
      background: #c9d0d5;
      border: 1px solid #6d7b85;
      border-radius: 6px;
    }
    QAbstractScrollArea::corner { background: #17191b; }

    QToolTip {
      color: #f2f2f2;
      background: #25282b;
      border: 1px solid #4b535b;
      padding: 5px 7px;
    }

    QMainWindow, QDialog, QDockWidget, QWidget {
      selection-background-color: #3976a8;
      selection-color: #ffffff;
    }

    QDockWidget {
      titlebar-close-icon: url(:/qt-project.org/styles/commonstyle/images/standardbutton-close-16.png);
      titlebar-normal-icon: url(:/qt-project.org/styles/commonstyle/images/standardbutton-restore-16.png);
    }
    QDockWidget::title {
      color: #c8cdd2;
      background: #24272a;
      border-bottom: 1px solid #111315;
      padding: 5px 7px;
      text-align: left;
    }

    QTabBar::tab {
      color: #aeb5bb;
      background: #202326;
      border: 1px solid #141618;
      border-bottom: 2px solid transparent;
      padding: 5px 10px;
      margin-right: 1px;
    }
    QTabBar::tab:hover {
      color: #e5e8ea;
      background: #30353a;
    }
    QTabBar::tab:selected {
      color: #ffffff;
      background: #2b3035;
      border-bottom: 2px solid #4e9bd1;
    }
    QTabBar::tab:disabled { color: #62686d; }

    QLineEdit, QPlainTextEdit, QTextEdit, QSpinBox, QDoubleSpinBox,
    QComboBox, QDateEdit, QTimeEdit, QDateTimeEdit {
      color: #e2e5e8;
      background: #17191b;
      border: 1px solid #3d444a;
      border-radius: 2px;
      padding: 2px 5px;
      selection-background-color: #3976a8;
    }
    QLineEdit:hover, QPlainTextEdit:hover, QTextEdit:hover,
    QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover {
      border-color: #5a6873;
    }
    QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus,
    QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {
      border: 1px solid #4e9bd1;
    }
    QLineEdit:disabled, QComboBox:disabled, QSpinBox:disabled {
      color: #73797e;
      background: #202224;
      border-color: #303438;
    }
    QComboBox QAbstractItemView {
      color: #e2e5e8;
      background: #24272a;
      border: 1px solid #4b535b;
      selection-background-color: #3976a8;
      selection-color: #ffffff;
      padding: 2px;
      outline: 0;
    }
    QComboBox QAbstractItemView::item {
      min-height: 24px;
      padding: 4px 8px;
    }

    QPushButton, QToolButton {
      color: #d9dde0;
      background: #303438;
      border: 1px solid #4a5157;
      border-radius: 2px;
      padding: 4px 9px;
    }
    QPushButton:hover, QToolButton:hover {
      color: #ffffff;
      background: #3a4249;
      border-color: #6b7882;
    }
    QPushButton:pressed, QToolButton:pressed,
    QPushButton:checked, QToolButton:checked {
      background: #28577d;
      border-color: #4e9bd1;
    }
    QPushButton:disabled, QToolButton:disabled {
      color: #70767b;
      background: #25282a;
      border-color: #34383b;
    }

    QHeaderView::section {
      color: #bfc5ca;
      background: #292d31;
      border: 0;
      border-right: 1px solid #17191b;
      border-bottom: 1px solid #111315;
      padding: 4px 6px;
    }
    QTreeView, QTableView, QListView {
      background: #151719;
      alternate-background-color: #1b1e20;
      border: 1px solid #30353a;
      gridline-color: #292d31;
      outline: 0;
    }
    QTreeView::item, QTableView::item, QListView::item {
      padding: 2px;
    }
    QTreeView::item:hover, QTableView::item:hover, QListView::item:hover {
      background: #2a333b;
    }
    QTreeView::item:selected, QTableView::item:selected, QListView::item:selected {
      background: #3976a8;
      color: #ffffff;
    }

    QGroupBox {
      color: #c8cdd2;
      border: 1px solid #343a40;
      border-radius: 2px;
      margin-top: 9px;
      padding-top: 5px;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 8px;
      padding: 0 4px;
      color: #9fa8b0;
      background: #1c1f21;
    }

    QScrollBar:vertical, QScrollBar:horizontal {
      background: #17191b;
      border: 0;
      margin: 0;
    }
    QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
      background: #454d54;
      border-radius: 3px;
      min-height: 24px;
      min-width: 24px;
    }
    QScrollBar::handle:hover { background: #60707b; }
    QScrollBar::add-line, QScrollBar::sub-line,
    QScrollBar::add-page, QScrollBar::sub-page { background: none; border: 0; }
  )qss");

  // Map each surface/state in the existing stylesheet to the exact UE default token.
  // Keeping this colour-only mapping separate leaves the approved widget geometry intact.
  const struct
  {
    const char *source;
    const char *ue;
  } colors[] = {
      {"#1c1f21", "#242424"}, // Panel
      {"#aeb6bd", "#C0C0C0"}, {"#9fa8af", "#808080"},
      {"#141617", "#0F0F0F"}, {"#343b41", "#383838"},
      {"#3976a8", "#0070E0"}, {"#67a9d7", "#0070E0"},
      {"#4c8fbe", "#0E86FF"}, {"#9bd0f0", "#0E86FF"},
      {"#28577d", "#0050A0"}, {"#dbe7ee", "#C0C0C0"},
      {"#20282e", "#1A1A1A"}, {"#4e9bd1", "#0070E0"},
      {"#87939c", "#808080"}, {"#343c43", "#383838"},
      {"#17191b", "#0F0F0F"}, {"#c9ced2", "#C0C0C0"},
      {"#202326", "#151515"}, {"#101214", "#0F0F0F"},
      {"#35414b", "#575757"}, {"#d9dde0", "#C0C0C0"},
      {"#25292c", "#383838"}, {"#4b535b", "#4C4C4C"},
      {"#3a4147", "#4C4C4C"}, {"#24282b", "#242424"},
      {"#111315", "#0F0F0F"}, {"#464e55", "#383838"},
      {"#38434c", "#575757"}, {"#596b78", "#808080"},
      {"#9da6ad", "#C0C0C0"}, {"#cdd2d6", "#C0C0C0"},
      {"#5a646c", "#383838"}, {"#73add5", "#0E86FF"},
      {"#e5e8ea", "#FFFFFF"}, {"#3d444a", "#383838"},
      {"#3b4248", "#383838"}, {"#c9d0d5", "#C0C0C0"},
      {"#6d7b85", "#808080"}, {"#f2f2f2", "#C0C0C0"},
      {"#25282b", "#383838"}, {"#c8cdd2", "#C8C8C8"},
      {"#24272a", "#383838"}, {"#aeb5bb", "#C0C0C0"},
      {"#141618", "#0F0F0F"}, {"#30353a", "#575757"},
      {"#2b3035", "#242424"}, {"#62686d", "#808080"},
      {"#e2e5e8", "#C0C0C0"}, {"#5a6873", "#575757"},
      {"#73797e", "#808080"}, {"#202224", "#1A1A1A"},
      {"#303438", "#383838"}, {"#4a5157", "#4C4C4C"},
      {"#3a4249", "#575757"}, {"#6b7882", "#808080"},
      {"#70767b", "#808080"}, {"#25282a", "#242424"},
      {"#34383b", "#383838"}, {"#bfc5ca", "#C8C8C8"},
      {"#292d31", "#2F2F2F"}, {"#151719", "#1A1A1A"},
      {"#1b1e20", "#242424"}, {"#2a333b", "#242424"},
      {"#343a40", "#383838"}, {"#9fa8b0", "#C8C8C8"},
      {"#454d54", "#383838"}, {"#60707b", "#575757"},
  };
  for(const auto &color : colors)
    sheet.replace(QLatin1String(color.source), QLatin1String(color.ue), Qt::CaseSensitive);
  app->setStyleSheet(sheet);
}

int UEStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                         const QWidget *widget) const
{
  // Preserve the existing layout. Only use the same compact spacing philosophy as Slate;
  // do not change control dimensions here, since .ui files and dock layouts rely on them.
  return RDStyle::pixelMetric(metric, option, widget);
}
