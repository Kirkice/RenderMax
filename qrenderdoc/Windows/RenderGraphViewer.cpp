#include "RenderGraphViewer.h"
#include <QApplication>
#include <QCoreApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QFileDialog>
#include <QImage>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QShortcut>
#include <QSvgGenerator>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QWheelEvent>
#include <QTimer>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <functional>
#include <cmath>
#include <QPainterPath>
#include <set>
#include "Code/QRDUtils.h"

namespace
{
QString HierarchicalTitle(const RenderGraph::Pass &pass);
QColor Accent(const QString &kind)
{
  if(kind == QStringLiteral("Compute")) return QColor(98,199,199);
  if(kind == QStringLiteral("Transfer")) return QColor(209,178,92);
  if(kind == QStringLiteral("Clear")) return QColor(185,185,185);
  return QColor(78,161,255);
}
// Qt's default path selection decoration is a dashed bounding rectangle spanning the
// entire connection. Use the stroke itself as the selection indicator instead.
class DependencyPath : public QGraphicsPathItem
{
public:
  DependencyPath(const QPainterPath &path, const QPen &pen) : QGraphicsPathItem(path)
  { setPen(pen); }
  void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override
  {
    QStyleOptionGraphicsItem clean(*option);
    clean.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
    p->save();
    if(isSelected())
    {
      QPen highlight(QColor(255,184,0),4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
      highlight.setCosmetic(true); p->setPen(highlight); p->drawPath(path());
    }
    else QGraphicsPathItem::paint(p,&clean,widget);
    p->restore();
  }
};
class TexturePreviewItem : public QGraphicsItem
{
public:
  QImage image;
  QString name;
  QPalette colors;
  std::function<void()> open;
  QPointF press;
  QRectF boundingRect() const override { return QRectF(0,0,112,98); }
  void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override
  {
    const qreal zoom = QStyleOptionGraphicsItem::levelOfDetailFromTransform(p->worldTransform());
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    p->setBrush(colors.color(QPalette::Base));
    QPen border(QColor(38,187,255),1); border.setCosmetic(true); p->setPen(border);
    p->drawRoundedRect(boundingRect(),5,5);
    QSizeF size = image.size(); size.scale(QSizeF(104,70),Qt::KeepAspectRatio);
    const QRectF target(QPointF(56-size.width()/2,39-size.height()/2),size);
    p->drawImage(target,image);
    // At overview scale use the image only; reveal text when there are enough screen pixels.
    if(zoom >= 0.65)
    {
      QFont font = QApplication::font(); font.setPointSizeF(8); p->setFont(font);
      p->setPen(colors.color(QPalette::Text));
      p->drawText(QRectF(4,78,104,18),p->fontMetrics().elidedText(name,Qt::ElideRight,104));
    }
  }
  void mousePressEvent(QGraphicsSceneMouseEvent *event) override
  { press = event->screenPos(); event->accept(); }
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
  {
    if((QPointF(event->screenPos())-press).manhattanLength() < QApplication::startDragDistance() && open) open();
    event->accept();
  }
};
class GraphView : public QGraphicsView
{
public:
  GraphView(QGraphicsScene *s) : QGraphicsView(s)
  {
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setFrameShape(QFrame::NoFrame);
  }
private:
  QPoint m_Press, m_Last;
  bool m_Left = false, m_Middle = false, m_Panning = false;
protected:
  void mousePressEvent(QMouseEvent *e) override
  {
    m_Press = m_Last = e->pos(); m_Panning = false;
    m_Left = e->button() == Qt::LeftButton;
    m_Middle = e->button() == Qt::MiddleButton;
    if(m_Middle) { viewport()->setCursor(Qt::ClosedHandCursor); e->accept(); return; }
    QGraphicsView::mousePressEvent(e);
  }
  void mouseMoveEvent(QMouseEvent *e) override
  {
    if(m_Middle || (m_Left && (m_Panning ||
        (e->pos()-m_Press).manhattanLength() >= QApplication::startDragDistance())))
    {
      m_Panning = true;
      const QPoint delta = e->pos()-m_Last; m_Last = e->pos();
      horizontalScrollBar()->setValue(horizontalScrollBar()->value()-delta.x());
      verticalScrollBar()->setValue(verticalScrollBar()->value()-delta.y());
      viewport()->setCursor(Qt::ClosedHandCursor); e->accept(); return;
    }
    if(m_Left) { e->accept(); return; }
    QGraphicsView::mouseMoveEvent(e);
  }
  void mouseReleaseEvent(QMouseEvent *e) override
  {
    // Deliver the left release to clear Qt's item/hand grab even after custom panning.
    if(!m_Middle) QGraphicsView::mouseReleaseEvent(e);
    else e->accept();
    m_Left = m_Middle = m_Panning = false; viewport()->unsetCursor();
  }
  void wheelEvent(QWheelEvent *e) override
  {
    const double factor = e->angleDelta().y() > 0 ? 1.12 : 1.0/1.12;
    const double next = transform().m11() * factor;
    if(next >= 0.05 && next <= 2.5) scale(factor, factor);
    e->accept();
  }
  void drawBackground(QPainter *p, const QRectF &r) override
  {
    p->fillRect(r, palette().color(QPalette::Window));
    if(transform().m11() < 0.35) return;
    QColor dot = palette().color(QPalette::Mid); dot.setAlpha(75);
    p->setPen(QPen(dot, 1));
    const int step = 28;
    for(int x = int(std::floor(r.left()/step))*step; x < r.right(); x += step)
      for(int y = int(std::floor(r.top()/step))*step; y < r.bottom(); y += step)
        p->drawPoint(QPointF(x,y));
  }
};
class PassItem : public QGraphicsItem
{
public:
  RenderGraph::Pass pass;
  QPalette palette;
  bool current = false, hot = false;
  QString timing;
  std::function<void()> openFocus;
  qreal cardHeight = 156.0;
  QRectF titleRect, outputRect;
  PassItem(const RenderGraph::Pass &p, const QPalette &pal) : pass(p), palette(pal)
  {
    setFlag(ItemIsSelectable); setCursor(Qt::PointingHandCursor);
    QFont titleFont = QApplication::font(); titleFont.setPointSizeF(9); titleFont.setBold(true);
    QFont outputFont = QApplication::font(); outputFont.setPointSizeF(9); outputFont.setBold(true);
    const QFontMetrics titleMetrics(titleFont), outputMetrics(outputFont);
    titleRect = titleMetrics.boundingRect(QRect(0,0,298,10000),Qt::AlignLeft|Qt::TextWordWrap,
                                          HierarchicalTitle(pass));
    const QString outputs = pass.outputNames.isEmpty() ? QStringLiteral("—") : pass.outputNames.join(lit("\n"));
    outputRect = outputMetrics.boundingRect(QRect(0,0,245,10000),Qt::AlignLeft|Qt::TextWordWrap,outputs);
    titleRect.setHeight(qMax<qreal>(20,titleRect.height()));
    outputRect.setHeight(qMax<qreal>(18,outputRect.height()));
    cardHeight = 33 + titleRect.height() + 29 + 13 + outputRect.height() + 33;
  }
  QRectF boundingRect() const override { return QRectF(-4,-4,338,cardHeight+8); }
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
  {
    if(openFocus) openFocus();
    event->accept();
  }
  void paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *) override
  {
    const QColor accent = Accent(pass.kind);
    QColor surface = palette.color(QPalette::Base);
    if(isSelected()) surface = palette.color(QPalette::AlternateBase);
    p->setBrush(surface);
    p->setPen(QPen(isSelected() ? palette.color(QPalette::Highlight) :
                  current ? accent : palette.color(QPalette::Mid), isSelected() ? 2 : 1));
    p->drawRoundedRect(QRectF(0,0,330,cardHeight),8,8);
    p->setPen(QPen(accent,3)); p->drawLine(QPointF(1,12),QPointF(1,cardHeight-12));
    p->setPen(accent);
    QFont f = p->font(); f.setPointSizeF(8); f.setBold(true); p->setFont(f);
    p->drawText(QRectF(16,11,118,18), pass.kind.toUpper());
    if(hot) p->setPen(QColor(255,184,0));
    p->drawText(QRectF(168,11,146,18), Qt::AlignRight, timing);
    f.setPointSizeF(9); p->setFont(f); p->setPen(palette.color(QPalette::Text));
    qreal y = 33;
    p->drawText(QRectF(16,y,298,titleRect.height()),Qt::AlignLeft|Qt::AlignVCenter|Qt::TextWordWrap,
                HierarchicalTitle(pass));
    y += titleRect.height()+5;
    f.setPointSizeF(9); f.setBold(false); p->setFont(f);
    p->setPen(palette.color(QPalette::Disabled, QPalette::Text));
    p->drawText(QRectF(16,y,298,18), QStringLiteral("EID %1–%2  ·  %3 draws  ·  %4 clears")
                .arg(pass.first).arg(pass.last).arg(pass.draws).arg(pass.clears));
    y += 28; p->setPen(palette.color(QPalette::Mid)); p->drawLine(QPointF(16,y),QPointF(314,y)); y += 8;
    f.setBold(true); p->setFont(f); p->setPen(palette.color(QPalette::Text));
    p->drawText(QRectF(16,y,57,18),QStringLiteral("OUTPUT"));
    const QString outputs = pass.outputNames.isEmpty() ? QStringLiteral("—") : pass.outputNames.join(lit("\n"));
    p->drawText(QRectF(73,y,241,outputRect.height()),Qt::AlignLeft|Qt::TextWordWrap,outputs);
    y += outputRect.height()+8;
    f.setBold(false); p->setFont(f); p->setPen(palette.color(QPalette::Disabled,QPalette::Text));
    p->drawText(QRectF(16,y,298,18), QStringLiteral("%1 inputs   ·   %2 outputs%3").arg(pass.inputs).arg(pass.outputs)
                .arg(pass.inferred ? QStringLiteral("   ·   inferred") : QString()));
  }
};
QString Html(const QString &s) { return s.toHtmlEscaped(); }
QStringList Hierarchy(const QString &path)
{
  QStringList result;
  for(const QString &part : path.split(lit(" / "),QString::SkipEmptyParts))
  {
    if(result.isEmpty() || result.last() != part) result.push_back(part);
  }
  return result;
}
QString HierarchicalTitle(const RenderGraph::Pass &pass)
{
  QStringList parts = pass.hierarchy;
  if(parts.isEmpty() || parts.last() != pass.name) parts.push_back(pass.name);
  return parts.join(lit(" › "));
}
}

RenderGraphViewer::RenderGraphViewer(ICaptureContext &ctx, QWidget *parent)
    : QFrame(parent), m_Ctx(ctx)
{
  setWindowTitle(tr("RenderGraph Viewer"));
  setStyleSheet(lit("QFrame#renderGraphHeader{border-bottom:1px solid palette(mid);background:palette(window);}" 
                    "QLabel#renderGraphEyebrow{font-size:9px;font-weight:700;color:palette(highlight);}" 
                    "QLabel#renderGraphStats{padding:7px 12px;border-bottom:1px solid palette(mid);font-weight:600;}" 
                    "QWidget#renderGraphInspector{border-left:1px solid palette(mid);background:palette(base);}"));
  auto *root = new QVBoxLayout(this); root->setContentsMargins(0,0,0,0); root->setSpacing(0);
  auto *header = new QFrame(); header->setObjectName(lit("renderGraphHeader"));
  auto *headerLayout = new QVBoxLayout(header); headerLayout->setContentsMargins(14,9,14,9); headerLayout->setSpacing(7);
  auto *primary = new QHBoxLayout(); primary->setSpacing(8);
  auto *identity = new QVBoxLayout(); identity->setSpacing(0);
  auto *eyebrow = new QLabel(tr("FRAME ANALYSIS")); eyebrow->setObjectName(lit("renderGraphEyebrow")); identity->addWidget(eyebrow);
  auto *title = new QLabel(tr("RenderGraph"));
  QFont font = title->font(); font.setBold(true); font.setPointSize(12); title->setFont(font); identity->addWidget(title);
  primary->addLayout(identity); primary->addSpacing(12);
  m_Mode = new QComboBox(); m_Mode->addItems({tr("Frame Overview"),tr("Pass Focus"),tr("Resource History")});
  m_Mode->setMinimumWidth(158); primary->addWidget(m_Mode);
  m_Search = new QLineEdit(); m_Search->setPlaceholderText(tr("Find pass, resource, or EID"));
  m_Search->setClearButtonEnabled(true); m_Search->setMinimumWidth(220); primary->addWidget(m_Search,1);
  m_OpenFocus = new QPushButton(tr("Focus selected")); m_OpenFocus->setEnabled(false); primary->addWidget(m_OpenFocus);
  QObject::connect(m_OpenFocus,&QPushButton::clicked,this,[this] {
    if(m_Selected >= 0) { m_Mode->setCurrentIndex(1); Display(); Fit(); }
  });
  m_Overview = new QPushButton(tr("← Overview")); m_Overview->setEnabled(false); m_Overview->setVisible(false); primary->addWidget(m_Overview);
  QObject::connect(m_Overview,&QPushButton::clicked,this,[this] { m_Mode->setCurrentIndex(0); Display(); Fit(); });
  auto *fit = new QPushButton(tr("Fit")); primary->addWidget(fit);
  headerLayout->addLayout(primary);
  auto *secondary = new QHBoxLayout(); secondary->setSpacing(8);
  m_Bindings = new QCheckBox(tr("Include noisy bindings")); m_Bindings->setChecked(false);
  m_Bindings->setToolTip(tr("Include constants, vertex buffers and index buffers. Disabled by default to reduce shared-resource noise."));
  secondary->addWidget(m_Bindings); secondary->addStretch();
  m_Rebuild = new QPushButton(tr("Rebuild")); secondary->addWidget(m_Rebuild);
  m_Timing = new QPushButton(tr("Measure GPU")); secondary->addWidget(m_Timing);
  QObject::connect(m_Timing,&QPushButton::clicked,this,[this] { Measure(); });
  m_Export = new QPushButton(tr("Export")); m_Export->setEnabled(false); secondary->addWidget(m_Export);
  QObject::connect(m_Export,&QPushButton::clicked,this,[this] { ExportImage(); });
  m_Cancel = new QPushButton(tr("Cancel")); m_Cancel->setEnabled(false); secondary->addWidget(m_Cancel);
  headerLayout->addLayout(secondary); root->addWidget(header);
  m_Stats = new QLabel(tr("PASS / RESOURCE DEPENDENCIES")); m_Stats->setObjectName(lit("renderGraphStats")); root->addWidget(m_Stats);
  auto *split = new QSplitter(); root->addWidget(split,1);
  m_Scene = new QGraphicsScene(this); m_View = new GraphView(m_Scene); split->addWidget(m_View);
  auto *inspector = new QWidget(); inspector->setObjectName(lit("renderGraphInspector"));
  inspector->setMinimumWidth(310); inspector->setMaximumWidth(440);
  auto *side = new QVBoxLayout(inspector); side->setContentsMargins(16,14,16,14); side->setSpacing(9);
  auto *caption = new QLabel(tr("SELECTION DETAILS")); caption->setObjectName(lit("renderGraphEyebrow")); side->addWidget(caption);
  m_Details = new QTextBrowser(); m_Details->setFrameShape(QFrame::NoFrame); m_Details->setMaximumHeight(250); side->addWidget(m_Details);
  m_Jump = new QPushButton(tr("Go to first event")); m_Jump->setEnabled(false); side->addWidget(m_Jump);
  m_Resources = new QTreeWidget(); m_Resources->setHeaderLabels({tr("Resource"),tr("Access / version")});
  m_Resources->setRootIsDecorated(false); m_Resources->setAlternatingRowColors(true); side->addWidget(m_Resources,1);
  m_Resource = new QPushButton(tr("Inspect resource")); m_Resource->setEnabled(false); side->addWidget(m_Resource);
  m_History = new QPushButton(tr("Resource history")); m_History->setEnabled(false); side->addWidget(m_History);
  QObject::connect(m_History,&QPushButton::clicked,this,[this] {
    auto *item = m_Resources->currentItem();
    if(item && m_Graph) m_Mode->setCurrentIndex(2);
  });
  QObject::connect(m_Resources,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem *item,int) {
    const uint32_t eid = item->data(0,Qt::UserRole+1).toUInt();
    if(eid) m_Ctx.SetEventID({this},eid,eid);
    m_Resource->click();
  });
  split->addWidget(inspector); split->setStretchFactor(0,1); split->setCollapsible(1,false); split->setSizes({1050,360});
  m_Status = new QLabel(); m_Status->setMargin(10); root->addWidget(m_Status);
  QObject::connect(m_Rebuild,&QPushButton::clicked,this,[this] { Build(); });
  QObject::connect(m_Cancel,&QPushButton::clicked,this,[this] {
    if(m_Cancelled) m_Cancelled->store(true);
    ++m_Generation; m_Cancel->setEnabled(false); m_Rebuild->setEnabled(m_Ctx.IsCaptureLoaded());
    m_Timing->setEnabled(m_Graph != nullptr);
    m_Status->setText(tr("Cancelled. The previous graph is retained."));
  });
  QObject::connect(fit,&QPushButton::clicked,this,[this] { Fit(); });
  auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape),this);
  escape->setContext(Qt::WidgetWithChildrenShortcut);
  QObject::connect(escape,&QShortcut::activated,this,[this] {
    if(m_Mode->currentIndex() != 0) { m_Mode->setCurrentIndex(0); Display(); Fit(); }
  });
  QObject::connect(m_Search,&QLineEdit::textChanged,this,[this] { if(m_Mode->currentIndex() != 2) Display(); });
  QObject::connect(m_Bindings,&QCheckBox::toggled,this,[this] { if(m_Mode->currentIndex() != 2) Display(); });
  QObject::connect(m_Mode,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int mode) {
    m_Overview->setVisible(mode != 0);
    if(mode == 1 && m_Selected < 0)
    {
      m_Status->setText(tr("Select a pass before opening Pass Focus."));
      m_Mode->setCurrentIndex(0); return;
    }
    if(mode == 2)
    {
      auto *item = m_Resources->currentItem();
      if(item) ShowHistory(item->data(0,Qt::UserRole).toInt());
      else { m_Status->setText(tr("Select a resource in the Inspector before opening Resource History.")); m_Mode->setCurrentIndex(0); }
    }
    else Display();
  });
  QObject::connect(m_Jump,&QPushButton::clicked,this,[this] {
    if(m_Graph && m_Selected >= 0) {
      const auto &pass = m_Graph->passes[m_Selected];
      const uint32_t eid = pass.primaryEvent ? pass.primaryEvent : pass.first;
      m_Ctx.SetEventID({this},eid,eid);
    }
  });
  QObject::connect(m_Resources,&QTreeWidget::itemSelectionChanged,this,[this] {
    auto *item = m_Resources->currentItem();
    m_Resource->setEnabled(item != nullptr); m_History->setEnabled(item != nullptr);
    if(item && m_Graph)
    {
      const int ri = item->data(0,Qt::UserRole).toInt();
      m_Resource->setText(m_Graph->resources[ri].texture ? tr("Open texture") : tr("Inspect resource"));
    }
  });
  QObject::connect(m_Resource,&QPushButton::clicked,this,[this] {
    auto *item = m_Resources->currentItem();
    if(!item || !m_Graph) return;
    const int ri = item->data(0,Qt::UserRole).toInt();
    if(m_Graph->resources[ri].texture)
      m_Ctx.GetTextureViewer()->ViewTexture(m_Graph->resources[ri].id,CompType::Typeless,true);
    else
      m_Ctx.GetResourceInspector()->Inspect(m_Graph->resources[ri].id);
  });
  QObject::connect(m_Scene,&QGraphicsScene::selectionChanged,this,[this] {
    if(m_Selecting) return;
    const uint64_t selection = ++m_SelectionGeneration;
    const auto items = m_Scene->selectedItems(); if(items.isEmpty()) return;
    // Never destroy graphics items or enter replay's nested event loop from Qt's
    // selection/mouse dispatch. Carry IDs, not item pointers, into the queued callback.
    const QString type = items.first()->data(1).toString();
    const int index = items.first()->data(0).toInt();
    const auto graph = m_Graph;
    QTimer::singleShot(0,this,[this,selection,graph,type,index] {
      if(selection != m_SelectionGeneration || !graph || graph != m_Graph) return;
      if(type == lit("pass"))
      {
        if(index < 0 || index >= graph->passes.size()) return;
        Select(index,m_Mode->currentIndex() == 1);
        const auto &pass = graph->passes[index];
        const uint32_t eid = pass.primaryEvent ? pass.primaryEvent : pass.first;
        if(eid) m_Ctx.SetEventID({this},eid,eid);
        if(selection == m_SelectionGeneration && graph == m_Graph) LoadResourcePreviews(-1);
      }
      else if(type == lit("link")) ShowLink(index);
    });
  });
  OnCaptureClosed();
  m_Ctx.AddCaptureViewer(this);
}
RenderGraphViewer::~RenderGraphViewer()
{
  if(m_Cancelled) m_Cancelled->store(true);
  m_Ctx.RemoveCaptureViewer(this);
}
void RenderGraphViewer::OnCaptureLoaded()
{
  m_Rebuild->setEnabled(true);
  Build();
}
void RenderGraphViewer::OnCaptureClosed()
{
  ++m_SelectionGeneration;
  ++m_Generation; if(m_Cancelled) m_Cancelled->store(true);
  m_Timings.clear(); m_HasTimings = false; m_HistoryResource = -1;
  m_Mode->setCurrentIndex(0);
  m_Timing->setEnabled(false); m_Timing->setText(tr("Measure GPU"));
  m_History->setEnabled(false);
  m_Overview->setEnabled(false); m_Export->setEnabled(false);
  m_Graph.reset(); m_Selected = -1; m_OpenFocus->setEnabled(false); m_Scene->clear(); m_Resources->clear();
  m_Stats->setText(tr("PASS / RESOURCE DEPENDENCIES"));
  m_Details->setHtml(tr("<h2>Explore the frame</h2><p>Open an RDC capture to trace pass inputs, outputs and resource producers.</p>"));
  m_Status->setText(tr("No capture loaded")); m_Rebuild->setEnabled(false); m_Cancel->setEnabled(false);
  m_Jump->setEnabled(false); m_Resource->setEnabled(false);
}
void RenderGraphViewer::Build()
{
  if(!m_Ctx.IsCaptureLoaded()) return;
  if(m_Cancelled) m_Cancelled->store(true);
  const auto cancelled = m_Cancelled = std::make_shared<std::atomic_bool>(false);
  const uint64_t generation = ++m_Generation;
  m_Timing->setEnabled(false); m_Timing->setText(tr("Measure GPU"));
  m_HasTimings = false; m_Timings.clear(); m_HistoryResource = -1;
  auto graph = std::make_shared<RenderGraph::Snapshot>();
  // Marker paths are the human-facing pass boundary. Clear commands are retained as commands and
  // usage evidence, but never promoted to top-level pass cards.
  struct PendingCommand { RenderGraph::Command command; QVector<uint32_t> apiEvents; };
  std::map<QString,QVector<PendingCommand>> pendingClears;
  std::map<QString,int> lastPassForPath;
  auto appendCommand = [&](int pi,const ActionDescription &a,const QString &name,const QString &kind) {
    auto &p = graph->passes[pi];
    RenderGraph::Command command; command.event = a.eventId; command.name = name; command.kind = kind;
    p.commands.push_back(command); p.events.push_back(a.eventId);
    p.first = p.first == 0 ? a.eventId : qMin(p.first,a.eventId); p.last = qMax(p.last,a.eventId);
    graph->eventPass[a.eventId] = pi;
    for(const APIEvent &event : a.events) graph->eventPass[event.eventId] = pi;
  };
  std::function<void(const rdcarray<ActionDescription> &,QString)> visit;
  visit = [&](const rdcarray<ActionDescription> &actions,QString path) {
    for(const ActionDescription &a : actions)
    {
      const QString actionName(a.GetName(m_Ctx.GetStructuredFile()));
      const QString childPath = path.isEmpty() ? actionName : path + lit(" / ") + actionName;
      const bool marker = bool(a.flags & (ActionFlags::PushMarker | ActionFlags::PopMarker | ActionFlags::SetMarker));
      const bool draw = bool(a.flags & (ActionFlags::Drawcall | ActionFlags::MeshDispatch));
      const bool dispatch = bool(a.flags & ActionFlags::Dispatch);
      const bool transfer = bool(a.flags & (ActionFlags::Copy | ActionFlags::Resolve));
      const bool clear = bool(a.flags & ActionFlags::Clear);
      if(marker || (!a.children.empty() && !(draw || dispatch || transfer || clear)))
      { visit(a.children,childPath); continue; }
      if(clear)
      {
        int owner = -1;
        auto previous = lastPassForPath.find(path);
        if(previous != lastPassForPath.end()) owner = previous->second;
        if(owner >= 0)
        {
          appendCommand(owner,a,actionName,lit("Clear")); ++graph->passes[owner].clears;
        }
        else
        {
          PendingCommand pending; pending.command.event = a.eventId; pending.command.name = actionName;
          pending.command.kind = lit("Clear");
          for(const APIEvent &event : a.events) pending.apiEvents.push_back(event.eventId);
          pendingClears[path].push_back(pending);
        }
      }
      else if(draw || dispatch || transfer)
      {
        const QString kind = dispatch ? lit("Compute") : transfer ? lit("Transfer") : lit("Graphics");
        int pi = -1;
        auto previous = lastPassForPath.find(path);
        // Named marker groups become one readable pass. Without markers, keep each operation as an
        // inferred pass so unrelated frame commands are not collapsed into one "Graphics sequence".
        if(!path.isEmpty() && previous != lastPassForPath.end() &&
           graph->passes[previous->second].kind == kind)
          pi = previous->second;
        if(pi < 0)
        {
          RenderGraph::Pass p; p.path = path; p.kind = kind; p.inferred = path.isEmpty();
          p.hierarchy = Hierarchy(path);
          p.name = path.isEmpty() ? actionName : p.hierarchy.isEmpty() ? actionName : p.hierarchy.takeLast();
          graph->passes.push_back(p); pi = graph->passes.size()-1; lastPassForPath[path] = pi;
          auto pending = pendingClears.find(path);
          if(pending != pendingClears.end())
          {
            for(const PendingCommand &entry : pending->second)
            {
              graph->passes[pi].commands.push_back(entry.command);
              graph->passes[pi].events.push_back(entry.command.event);
              graph->passes[pi].first = graph->passes[pi].first == 0 ? entry.command.event :
                                            qMin(graph->passes[pi].first,entry.command.event);
              graph->eventPass[entry.command.event] = pi;
              for(uint32_t event : entry.apiEvents) graph->eventPass[event] = pi;
              ++graph->passes[pi].clears;
            }
            pendingClears.erase(pending);
          }
        }
        appendCommand(pi,a,actionName,kind);
        auto &p = graph->passes[pi]; if(p.primaryEvent == 0) p.primaryEvent = a.eventId;
        if(draw) ++p.draws; if(dispatch) ++p.dispatches; if(transfer) ++p.transfers;
      }
      if(!a.children.empty()) visit(a.children,childPath);
    }
  };
  visit(m_Ctx.CurRootActions(),QString());
  for(const ResourceDescription &r : m_Ctx.GetResources())
  {
    if(r.type != ResourceType::Texture && r.type != ResourceType::Buffer) continue;
    RenderGraph::Resource resource; resource.id = r.resourceId;
    resource.name = QString(m_Ctx.GetResourceName(r.resourceId));
    resource.texture = r.type == ResourceType::Texture; graph->resources.push_back(resource);
  }
  m_Rebuild->setEnabled(false); m_Cancel->setEnabled(true);
  m_Status->setText(tr("Collecting resource usage… Replay work is cancellable between resources."));
  QPointer<RenderGraphViewer> viewer(this);
  m_Ctx.Replay().AsyncInvoke([graph,cancelled,generation,viewer](IReplayController *controller) {
    for(auto &r : graph->resources)
    {
      if(cancelled->load()) return;
      r.usages = controller->GetUsage(r.id);
    }
    if(cancelled->load()) return;
    LambdaThread *thread = new LambdaThread([graph,cancelled,generation,viewer] {
      if(!RenderGraph::Build(*graph,*cancelled)) return;
      GUIInvoke::call(QCoreApplication::instance(),[graph,cancelled,generation,viewer] {
        if(!viewer || cancelled->load() || viewer->m_Generation != generation) return;
        viewer->m_Graph = graph; viewer->m_Selected = -1; viewer->m_OpenFocus->setEnabled(false);
        viewer->m_Rebuild->setEnabled(true); viewer->m_Cancel->setEnabled(false);
        viewer->m_Timing->setEnabled(true); viewer->m_Overview->setEnabled(true);
        viewer->m_Export->setEnabled(true);
        viewer->m_Details->setHtml(tr("<h2>Frame dependencies</h2><p>Click a pass to inspect it. Double-click or use <b>Focus selected</b> to open its local dependencies.</p><p>Pass cards follow capture marker paths; unmarked operations are explicitly inferred. Clear commands stay inside their owning pass. Attachments and UAV accesses are conservative potential read/write uses.</p><p>GPU timing: not collected.</p>"));
        viewer->Display(); viewer->Fit();
      });
    });
    thread->selfDelete(true); thread->start();
  });
}
void RenderGraphViewer::Fit()
{
  if(!m_Scene->items().isEmpty()) m_View->fitInView(m_Scene->itemsBoundingRect().adjusted(-30,-30,30,30),Qt::KeepAspectRatio);
}
bool RenderGraphViewer::AccessVisible(const RenderGraph::Access &a) const
{
  if(m_Bindings->isChecked()) return true;
  const ResourceUsage u = a.usage;
  return !(u == ResourceUsage::VertexBuffer || u == ResourceUsage::IndexBuffer ||
           (u >= ResourceUsage::VS_Constants && u <= ResourceUsage::All_Constants));
}
void RenderGraphViewer::Display()
{
  if(!m_Graph) return;
  if(m_Mode->currentIndex() == 1 && m_Selected >= 0) { DisplayFocus(); return; }
  m_HistoryResource = -1; m_Overview->setEnabled(true);
  m_Selecting = true; m_Scene->clear();
  const auto &g = *m_Graph;
  const QString query = m_Search->text().trimmed();
  std::set<int> visible;
  for(int pi = 0; pi < g.passes.size(); ++pi)
  {
    const auto &p = g.passes[pi];
    bool matches = query.isEmpty() || p.name.contains(query,Qt::CaseInsensitive) || p.path.contains(query,Qt::CaseInsensitive);
    bool number = false; const uint32_t eid = query.toUInt(&number);
    if(number && eid >= p.first && eid <= p.last) matches = true;
    if(!matches) for(const auto &a : g.accesses)
      if(AccessVisible(a) && a.pass == pi && g.resources[a.resource].name.contains(query,Qt::CaseInsensitive)) { matches = true; break; }
    if(matches) visible.insert(pi);
  }
  const int matched = int(visible.size());
  const int maximumCards = 500;
  if(visible.size() > size_t(maximumCards))
  {
    auto cut = visible.begin(); std::advance(cut,maximumCards);
    visible.erase(cut,visible.end());
  }
  // Coalesce every occurrence of the same full marker path into one container.
  // Sibling groups follow their first EID; their members retain EID order. Grouping can
  // move interleaved work, so horizontal position is not a strict global event timeline.
  QVector<int> ordered;
  for(int pi : visible) ordered.push_back(pi);
  std::stable_sort(ordered.begin(),ordered.end(),[&](int a,int b) {
    return g.passes[a].first < g.passes[b].first;
  });
  struct GroupFrame { QString path; QRectF rect; int depth; };
  QVector<GroupFrame> frames;
  std::map<int,QPointF> positions;
  std::map<int,qreal> heights;
  QFont groupFont = QApplication::font(); groupFont.setPointSizeF(9); groupFont.setBold(true);
  const QFontMetrics groupMetrics(groupFont);
  QVector<QPair<int,int>> layoutLinks;
  for(const auto &link : g.links)
  {
    if(!visible.count(link.source) || !visible.count(link.destination)) continue;
    for(int ai : link.evidence) if(AccessVisible(g.accesses[ai]))
    { layoutLinks.push_back(qMakePair(link.source,link.destination)); break; }
  }
  std::function<QRectF(const QVector<int> &,int)> layout;
  layout = [&](const QVector<int> &members,int depth) {
    // Partition by ancestor inside this parent, preserving first occurrence order.
    // Ungrouped leaves have their own bucket, even if their display names match.
    QVector<QVector<int>> buckets;
    std::map<QString,int> ancestorBuckets;
    for(int pi : members)
    {
      const auto &ancestors = g.passes[pi].hierarchy;
      if(depth >= ancestors.size()) { buckets.push_back(QVector<int>{pi}); continue; }
      auto found = ancestorBuckets.find(ancestors[depth]);
      if(found == ancestorBuckets.end())
      {
        ancestorBuckets[ancestors[depth]] = buckets.size();
        buckets.push_back(QVector<int>{pi});
      }
      else buckets[found->second].push_back(pi);
    }
    QVector<QRectF> areas;
    QVector<QPair<int,int>> frameRanges;
    std::map<int,int> owner;
    for(int bi = 0; bi < buckets.size(); ++bi)
    {
      const auto &bucket = buckets[bi];
      for(int pi : bucket) owner[pi] = bi;
      const int frameBegin = frames.size();
      const auto &ancestors = g.passes[bucket.first()].hierarchy;
      QRectF area;
      if(bucket.size() >= 2)
      {
        const QRectF children = layout(bucket,depth+1);
        const QPointF inset(24,48);
        for(int pi : bucket) positions[pi] += inset;
        for(int fi = frameBegin; fi < frames.size(); ++fi) frames[fi].rect.translate(inset);
        area = QRectF(0,0,qMax(children.width()+48,qreal(groupMetrics.width(ancestors[depth])+48)),
                       children.height()+72);
        frames.push_back({ancestors.mid(0,depth+1).join(lit(" › ")),area,depth});
      }
      else
      {
        const int pi = bucket.first(); PassItem probe(g.passes[pi],palette());
        positions[pi] = QPointF(4,4); heights[pi] = probe.cardHeight;
        area = probe.boundingRect().translated(positions[pi]);
      }
      areas.push_back(area); frameRanges.push_back(qMakePair(frameBegin,frames.size()));
    }
    QVector<std::set<int>> predecessors(buckets.size());
    for(const auto &link : layoutLinks)
    {
      auto from = owner.find(link.first), to = owner.find(link.second);
      // Merging interleaved marker groups can create group cycles. Use forward first-EID
      // relations for placement, but retain ALL real dependencies for drawing/inspection.
      if(from != owner.end() && to != owner.end() && from->second < to->second)
        predecessors[to->second].insert(from->second);
    }
    QVector<QRectF> placed;
    QRectF bounds;
    for(int bi = 0; bi < buckets.size(); ++bi)
    {
      qreal x = 0, center = 0;
      for(int parent : predecessors[bi])
      {
        x = qMax(x,placed[parent].right()+150);
        center += placed[parent].center().y();
      }
      if(!predecessors[bi].empty()) center /= predecessors[bi].size();
      // Center a merge on its producers. Sibling branches occupy the nearest free vertical
      // interval, using actual nested group extents rather than a fixed grid or random offset.
      const qreal preferredY = center-areas[bi].height()*0.5;
      QVector<qreal> candidates; candidates.push_back(preferredY);
      for(const QRectF &other : placed)
        if(x < other.right()+80 && x+areas[bi].width()+80 > other.left())
        { candidates.push_back(other.bottom()+80); candidates.push_back(other.top()-80-areas[bi].height()); }
      std::stable_sort(candidates.begin(),candidates.end(),[&](qreal a,qreal b) {
        return qAbs(a-preferredY) < qAbs(b-preferredY);
      });
      QRectF chosen;
      for(qreal y : candidates)
      {
        const QRectF candidate(QPointF(x,y),areas[bi].size());
        bool overlaps = false;
        for(const QRectF &other : placed)
          if(candidate.intersects(other.adjusted(-60,-60,60,60))) { overlaps = true; break; }
        if(!overlaps) { chosen = candidate; break; }
      }
      placed.push_back(chosen);
      bounds = bounds.isNull() ? chosen : bounds.united(chosen);
    }
    // Relax both ends of each branch instead of leaving sources stranded at the first
    // greedy free slot. Keep horizontal progress and resolve vertical collisions every sweep.
    QVector<std::set<int>> neighbours = predecessors;
    for(int bi = 0; bi < buckets.size(); ++bi)
      for(int parent : predecessors[bi]) neighbours[parent].insert(bi);
    for(int sweep = 0; sweep < 8; ++sweep)
    {
      for(int step = 0; step < buckets.size(); ++step)
      {
        const int bi = sweep%2 ? buckets.size()-1-step : step;
        if(neighbours[bi].empty()) continue;
        qreal target = 0;
        for(int neighbour : neighbours[bi]) target += placed[neighbour].center().y();
        target /= neighbours[bi].size();
        const qreal preferred = placed[bi].top()+0.6*(target-placed[bi].center().y());
        QVector<qreal> candidates; candidates.push_back(preferred); candidates.push_back(placed[bi].top());
        for(int oi = 0; oi < placed.size(); ++oi)
          if(oi != bi && placed[bi].left() < placed[oi].right()+60 &&
             placed[bi].right()+60 > placed[oi].left())
          {
            candidates.push_back(placed[oi].bottom()+64);
            candidates.push_back(placed[oi].top()-64-placed[bi].height());
          }
        std::stable_sort(candidates.begin(),candidates.end(),[&](qreal a,qreal b) {
          return qAbs(a-preferred) < qAbs(b-preferred);
        });
        for(qreal y : candidates)
        {
          QRectF candidate = placed[bi]; candidate.moveTop(y);
          bool clear = true;
          for(int oi = 0; oi < placed.size(); ++oi)
            if(oi != bi && candidate.intersects(placed[oi].adjusted(-60,-60,60,60)))
            { clear = false; break; }
          if(clear) { placed[bi] = candidate; break; }
        }
      }
    }
    bounds = QRectF();
    for(const QRectF &area : placed) bounds = bounds.isNull() ? area : bounds.united(area);
    for(int bi = 0; bi < buckets.size(); ++bi)
    {
      const QPointF delta = placed[bi].topLeft()-bounds.topLeft();
      for(int pi : buckets[bi]) positions[pi] += delta;
      for(int fi = frameRanges[bi].first; fi < frameRanges[bi].second; ++fi) frames[fi].rect.translate(delta);
    }
    return QRectF(QPointF(0,0),bounds.size());
  };
  const QRectF timelineBounds = layout(ordered,0);
  for(auto &entry : positions) entry.second += QPointF(0,48);
  for(auto &frame : frames) frame.rect.translate(0,48);
  auto pos = [&](int pi) { return positions[pi]; };
  auto *timeLabel = m_Scene->addSimpleText(tr("RESOURCE FLOW → Blue: dependency · Selected pass: amber IN / green OUT · Frames: marker hierarchy"));
  timeLabel->setBrush(palette().color(QPalette::Text)); timeLabel->setPos(0,0);
  for(int li = 0; li < g.links.size(); ++li)
  {
    const auto &l = g.links[li]; if(!visible.count(l.source) || !visible.count(l.destination)) continue;
    const QPointF start = pos(l.source)+QPointF(330,heights[l.source]*0.5),
                  end = pos(l.destination)+QPointF(0,heights[l.destination]*0.5);
    const qreal reach = qMax(qreal(60),qAbs(end.x()-start.x())*0.45);
    QPainterPath path(start);
    path.cubicTo(start+QPointF(reach,0),end-QPointF(reach,0),end);
    bool obstructed = end.x() <= start.x();
    for(int pi : visible)
    {
      if(pi == l.source || pi == l.destination) continue;
      QPainterPath obstacle;
      obstacle.addRect(QRectF(pos(pi),QSizeF(330,heights[pi])).adjusted(-12,-12,12,12));
      if(path.intersects(obstacle)) { obstructed = true; break; }
    }
    if(obstructed)
    {
      // A long-range/backward dependency uses a curved bypass; ordinary local edges stay
      // directly between ports. Never run a bypass vertically through a column of cards.
      const qreal lane = timelineBounds.bottom()+110+(li%12)*16;
      path = QPainterPath(start);
      const qreal exitX = start.x()+28, entryX = end.x()-28;
      path.cubicTo(start+QPointF(28,0),QPointF(exitX,lane),QPointF(exitX,lane));
      path.cubicTo(QPointF(exitX,lane+35),QPointF(entryX,lane+35),QPointF(entryX,lane));
      path.cubicTo(QPointF(entryX,lane),end-QPointF(28,0),end);
    }
    const bool related = l.source == m_Selected || l.destination == m_Selected;
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    const QColor edgeColor = l.destination == m_Selected ? QColor(255,184,0) :
                             l.source == m_Selected ? QColor(31,228,130) :
                             dark ? QColor(90,190,255) : QColor(0,102,170);
    QPen edgePen(edgeColor,related ? 3.5 : 1.0,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
    edgePen.setCosmetic(true);
    auto *edge = new DependencyPath(path,edgePen); m_Scene->addItem(edge);
    edge->setZValue(-1); edge->setFlag(QGraphicsItem::ItemIsSelectable); edge->setCursor(Qt::PointingHandCursor);
    edge->setData(0,li); edge->setData(1,lit("link"));
    std::set<int> resources; QStringList names;
    for(int ai : l.evidence)
      if(AccessVisible(g.accesses[ai]) && resources.insert(g.accesses[ai].resource).second)
        names << g.resources[g.accesses[ai].resource].name;
    if(resources.empty()) { m_Scene->removeItem(edge); delete edge; continue; }
    edge->setToolTip(names.join(lit("\n")));
    // Identify the resources on every connection, not only on selection.
    if(!names.isEmpty())
    {
      const QString label = resources.size() == 1 ? names.first() : tr("%1 + %2 more").arg(names.first()).arg(resources.size()-1);
      auto *text = m_Scene->addSimpleText(label.left(32)); text->setBrush(palette().color(QPalette::Text));
      text->setPos(path.pointAtPercent(0.5)+QPointF(0,-22)); text->setToolTip(names.join(lit("\n")));
    }
    QPolygonF arrow; arrow << end << end+QPointF(-8,-4) << end+QPointF(-8,4);
    auto *head = m_Scene->addPolygon(arrow,Qt::NoPen,edgeColor);
    head->setData(0,li); head->setData(1,lit("arrow"));
  }
  const auto current = g.eventPass.find(m_Ctx.CurEvent());
  const auto timing = m_HasTimings ? RenderGraph::SummariseTimings(g,m_Timings) : QVector<RenderGraph::TimingSummary>();
  double longest = 0.0;
  for(const auto &sample : timing) if(sample.samples) longest = qMax(longest,sample.seconds);
  for(int pi : visible)
  {
    auto *item = new PassItem(g.passes[pi],palette()); item->setPos(pos(pi));
    std::set<int> displayedInputs, displayedOutputs;
    for(const auto &a : g.accesses) if(a.pass == pi && AccessVisible(a))
    {
      if(a.kind == RenderGraph::AccessKind::Read || a.kind == RenderGraph::AccessKind::ReadWrite)
        displayedInputs.insert(a.resource);
      if(a.kind == RenderGraph::AccessKind::Write || a.kind == RenderGraph::AccessKind::ReadWrite)
        displayedOutputs.insert(a.resource);
    }
    item->pass.inputs = int(displayedInputs.size()); item->pass.outputs = int(displayedOutputs.size());
    item->setData(0,pi); item->setData(1,lit("pass"));
    item->openFocus = [this,pi] {
      const auto graph = m_Graph;
      ++m_SelectionGeneration;
      QTimer::singleShot(0,this,[this,pi,graph] {
        if(!graph || graph != m_Graph || pi >= graph->passes.size()) return;
        m_Selected = pi; m_OpenFocus->setEnabled(true);
        if(m_Mode->currentIndex() != 1) m_Mode->setCurrentIndex(1);
        else Display();
        Select(pi,false); Fit();
      });
    };
    item->current = current != g.eventPass.end() && current->second == pi;
    if(!m_HasTimings) item->timing = tr("Not measured");
    else if(timing[pi].samples == 0) item->timing = tr("Not reported");
    else item->timing = tr("%1 ms").arg(timing[pi].seconds*1000.0,0,'f',3);
    item->hot = timing.size() > pi && timing[pi].samples && timing[pi].seconds == longest;
    item->setToolTip(g.passes[pi].path + tr("\nClick: inspect · Double-click: Pass Focus\nMarker-derived pass · resource-level dependencies"));
    m_Scene->addItem(item); item->setSelected(pi == m_Selected);
  }
  // Shared marker ancestors are visual containers. They sit behind both cards and dependency edges,
  // and deliberately have no selection behaviour so a frame cannot be mistaken for a graph node.
  for(int fi = 0; fi < frames.size(); ++fi)
  {
    const auto &group = frames[fi];
    const QColor border = QColor::fromHsv((fi*137+195)%360,155,245);
    QColor fill = border; fill.setAlpha(14);
    auto *container = m_Scene->addRect(group.rect,QPen(border,3),QBrush(fill));
    container->setZValue(-1000+group.depth); container->setAcceptedMouseButtons(Qt::NoButton);
    container->setToolTip(tr("Marker group: %1").arg(group.path));
    auto *label = m_Scene->addSimpleText(group.path.section(lit(" › "),-1),groupFont);
    label->setBrush(border); label->setPos(group.rect.left()+16,group.rect.top()+12);
    label->setAcceptedMouseButtons(Qt::NoButton);
  }
  if(visible.empty())
  {
    auto *empty = m_Scene->addText(tr("No passes match the current filters"));
    empty->setPos(0,40);
    empty->setDefaultTextColor(palette().color(QPalette::Disabled,QPalette::Text));
  }
  m_Scene->setSceneRect(m_Scene->itemsBoundingRect().adjusted(-40,-40,40,40));
  m_Stats->setText(tr("%1 PASSES     /     %2 RESOURCES     /     %3 CONNECTIONS     /     %4 VISIBLE")
                    .arg(g.passes.size()).arg(g.resources.size()).arg(g.links.size()).arg(visible.size()));
  const QString capped = matched > maximumCards ? tr(" · showing first %1 of %2 matched passes").arg(maximumCards).arg(matched) : QString();
  m_Status->setText(tr("Dependency-driven branches / merges · Shared marker groups · EIDs indicate capture order, not GPU time · %1 external reads · %2 unassigned · %3 unknown accesses%4")
                     .arg(g.externalReads).arg(g.unassigned).arg(g.unsupported).arg(capped));
  m_Selecting = false;
}
void RenderGraphViewer::DisplayFocus()
{
  if(!m_Graph || m_Selected < 0 || m_Selected >= m_Graph->passes.size()) return;
  const auto &g = *m_Graph; m_HistoryResource = -1; m_Selecting = true; m_Scene->clear();
  const int current = m_Selected;
  std::set<int> producers, consumers, inputResources, outputResources;
  for(const auto &link : g.links)
  {
    if(link.destination == current)
      for(int ai : link.evidence) if(AccessVisible(g.accesses[ai]))
        { producers.insert(link.source); inputResources.insert(g.accesses[ai].resource); }
    if(link.source == current)
      for(int ai : link.evidence) if(AccessVisible(g.accesses[ai]))
        { consumers.insert(link.destination); outputResources.insert(g.accesses[ai].resource); }
  }
  auto passCard = [&](int pi, double x, double y, bool selected) {
    auto *item = new PassItem(g.passes[pi],palette()); item->setPos(x,y);
    item->setData(0,pi); item->setData(1,lit("pass")); item->timing = TimingText(pi);
    item->openFocus = [this,pi] {
      const auto graph = m_Graph;
      QTimer::singleShot(0,this,[this,pi,graph] {
        if(graph && graph == m_Graph) Select(pi,false);
      });
    };
    item->current = pi == current; m_Scene->addItem(item); item->setSelected(selected);
  };
  auto resourceCard = [&](int ri, double x, double y, const QString &direction) {
    const QRectF area(x,y,260,74);
    auto *box = m_Scene->addRect(area,QPen(QColor(38,187,255),2),palette().color(QPalette::Base));
    box->setData(0,ri); box->setToolTip(g.resources[ri].name);
    auto *name = m_Scene->addText(g.resources[ri].name); name->setDefaultTextColor(palette().color(QPalette::Text));
    name->setTextWidth(236); name->setPos(x+12,y+8);
    auto *type = m_Scene->addSimpleText(direction + lit(" · ") + (g.resources[ri].texture ? tr("Texture") : tr("Buffer")));
    type->setBrush(QColor(38,187,255)); type->setPos(x+12,y+47);
  };
  auto connector = [&](QPointF a,QPointF b) {
    QPainterPath path(a); const double dx = qMax(35.0,(b.x()-a.x())*0.5);
    path.cubicTo(a+QPointF(dx,0),b-QPointF(dx,0),b);
    m_Scene->addPath(path,QPen(palette().color(QPalette::Mid),2))->setZValue(-1);
    m_Scene->addPolygon(QPolygonF() << b << b+QPointF(-8,-4) << b+QPointF(-8,4),Qt::NoPen,palette().color(QPalette::Mid));
  };
  const double passX[3] = {0,900,1800}, resourceX[2] = {455,1355};
  std::map<int,double> producerY, consumerY, inputY, outputY, passHeights;
  for(int pi : producers) { PassItem probe(g.passes[pi],palette()); passHeights[pi] = probe.cardHeight; }
  for(int pi : consumers) { PassItem probe(g.passes[pi],palette()); passHeights[pi] = probe.cardHeight; }
  { PassItem probe(g.passes[current],palette()); passHeights[current] = probe.cardHeight; }
  qreal passY = 0; for(int pi : producers) { producerY[pi] = passY; passCard(pi,passX[0],passY,false); passY += passHeights[pi]+45; }
  int row = 0;
  row = 0; for(int ri : inputResources) { inputY[ri] = row*125; resourceCard(ri,resourceX[0],inputY[ri],tr("Input")); ++row; }
  const double currentY = qMax(0,(row-1)*62); passCard(current,passX[1],currentY,true);
  row = 0; for(int ri : outputResources) { outputY[ri] = row*125; resourceCard(ri,resourceX[1],outputY[ri],tr("Output")); ++row; }
  passY = 0; for(int pi : consumers) { consumerY[pi] = passY; passCard(pi,passX[2],passY,false); passY += passHeights[pi]+45; }
  // Draw only evidence-backed relations. Shared resources are bundled visually but retain their exact
  // producer/consumer pair rather than implying an all-to-all relationship.
  std::set<std::pair<int,int>> drawnInputs, drawnOutputs;
  for(const auto &link : g.links)
  {
    for(int ai : link.evidence)
    {
      const auto &a = g.accesses[ai]; if(!AccessVisible(a)) continue;
      if(link.destination == current && inputY.count(a.resource) && producerY.count(link.source))
      {
        const auto key = std::make_pair(link.source,a.resource);
        if(drawnInputs.insert(key).second)
          connector(QPointF(passX[0]+330,producerY[link.source]+passHeights[link.source]*0.5),QPointF(resourceX[0],inputY[a.resource]+37));
      }
      if(link.source == current && outputY.count(a.resource) && consumerY.count(link.destination))
      {
        const auto key = std::make_pair(a.resource,link.destination);
        if(drawnOutputs.insert(key).second)
          connector(QPointF(resourceX[1]+260,outputY[a.resource]+37),QPointF(passX[2],consumerY[link.destination]+passHeights[link.destination]*0.5));
      }
    }
  }
  for(const auto &entry : inputY)
    connector(QPointF(resourceX[0]+260,entry.second+37),QPointF(passX[1],currentY+passHeights[current]*0.5));
  for(const auto &entry : outputY)
    connector(QPointF(passX[1]+330,currentY+passHeights[current]*0.5),QPointF(resourceX[1],entry.second+37));
  m_Scene->setSceneRect(m_Scene->itemsBoundingRect().adjusted(-40,-40,40,40)); m_Selecting = false;
  m_Stats->setText(tr("PASS FOCUS     /     %1 PRODUCERS     /     %2 INPUTS     /     %3 OUTPUTS     /     %4 CONSUMERS")
                   .arg(producers.size()).arg(inputResources.size()).arg(outputResources.size()).arg(consumers.size()));
  m_Status->setText(tr("Pass Focus · press Esc or click ← Overview to return · resource-level precision"));
}
void RenderGraphViewer::Select(int pi, bool redraw)
{
  if(!m_Graph || pi < 0 || pi >= m_Graph->passes.size()) return;
  m_HistoryResource = -1;
  ++m_PreviewGeneration;
  for(QGraphicsItem *item : m_Scene->items())
    if(item->data(1).toString() == lit("texturePreview")) delete item;
  m_Selected = pi; m_OpenFocus->setEnabled(true); const auto &g = *m_Graph; const auto &p = g.passes[pi];
  m_Jump->setEnabled(true); m_Resources->clear(); m_Resource->setEnabled(false);
  m_Details->setHtml(tr("<h2>%1</h2><p>%2</p><p><b>%3</b> · EID %4–%5</p><p>%6 draws · %7 dispatches · %8 transfers · %9 clears</p><p>%10 input resources · %11 output resources</p><p>%12</p><p>Versions denote observed writes, not allocations. Potential read/write does not prove actual shader access.</p>")
    .arg(Html(p.name)).arg(Html(p.path)).arg(Html(p.kind)).arg(p.first).arg(p.last).arg(p.draws).arg(p.dispatches).arg(p.transfers).arg(p.clears).arg(p.inputs).arg(p.outputs)
    .arg(p.inferred ? tr("Pass boundary inferred from an unmarked action.") : tr("Pass boundary derived from the capture marker path.")));
  if(!p.outputNames.isEmpty())
  {
    QStringList outputs; for(const QString &output : p.outputNames) outputs << Html(output);
    m_Details->append(tr("<p><b>Outputs</b><br>%1</p>").arg(outputs.join(lit("<br>"))));
  }
  if(!p.commands.isEmpty())
  {
    QString commands = tr("<p><b>Commands</b></p><ol>");
    for(const auto &command : p.commands)
      commands += tr("<li>EID %1 · %2 · %3</li>").arg(command.event).arg(Html(command.kind)).arg(Html(command.name));
    commands += lit("</ol>"); m_Details->append(commands);
  }
  for(const auto &a : g.accesses) if(a.pass == pi && AccessVisible(a))
  {
    auto *item = new QTreeWidgetItem(m_Resources,{g.resources[a.resource].name,
      tr("%1 · v%2 · EID %3").arg(RenderGraph::AccessName(a.kind)).arg(a.version).arg(a.event)});
    item->setData(0,Qt::UserRole,a.resource);
    item->setData(0,Qt::UserRole+1,a.event);
    item->setToolTip(1,ToQStr(a.usage));
  }
  m_Details->append(tr("<p>GPU timing: %1</p>").arg(Html(TimingText(pi))));
  m_Resources->resizeColumnToContents(0);
  // Update connection emphasis in place; rebuilding the scene here can delete the card
  // while Qt is still dispatching its mouse event.
  if(m_Mode->currentIndex() == 0)
    for(QGraphicsItem *item : m_Scene->items())
    {
      const QString type = item->data(1).toString();
      if(type != lit("link") && type != lit("arrow")) continue;
      const int li = item->data(0).toInt();
      if(li < 0 || li >= g.links.size()) continue;
      const auto &link = g.links[li];
      const bool related = link.source == pi || link.destination == pi;
      const bool dark = palette().color(QPalette::Window).lightness() < 128;
      const QColor color = link.destination == pi ? QColor(255,184,0) :
                           link.source == pi ? QColor(31,228,130) :
                           dark ? QColor(90,190,255) : QColor(0,102,170);
      if(type == lit("link"))
      {
        auto *edge = static_cast<QGraphicsPathItem *>(item);
        QPen pen(color,related ? 3.5 : 1.0,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin);
        pen.setCosmetic(true); edge->setPen(pen);
      }
      else static_cast<QGraphicsPolygonItem *>(item)->setBrush(color);
    }
  if(redraw) Display();
}
void RenderGraphViewer::ShowLink(int li)
{
  if(!m_Graph || li < 0 || li >= m_Graph->links.size()) return;
  const auto &g = *m_Graph; const auto &l = g.links[li];
  m_Jump->setEnabled(false); m_Resources->clear(); m_Resource->setEnabled(false);
  m_Details->setHtml(tr("<h2>Resource dependency</h2><p><b>%1</b> → <b>%2</b></p><p>Usage-derived read-after-write evidence. Attachment/UAV reads are conservative. This is not a GPU scheduling or critical-path claim.</p>")
    .arg(Html(g.passes[l.source].name)).arg(Html(g.passes[l.destination].name)));
  for(int ai : l.evidence) {
    const auto &a = g.accesses[ai];
    if(!AccessVisible(a)) continue;
    auto *item = new QTreeWidgetItem(m_Resources,{g.resources[a.resource].name,tr("v%1 · read EID %2").arg(a.version).arg(a.event)});
    item->setData(0,Qt::UserRole,a.resource);
    item->setData(0,Qt::UserRole+1,a.event);
  }
  LoadResourcePreviews(li);
}
void RenderGraphViewer::LoadResourcePreviews(int link)
{
  for(QGraphicsItem *item : m_Scene->items())
    if(item->data(1).toString() == lit("texturePreview")) delete item;
  const uint64_t preview = ++m_PreviewGeneration, generation = m_Generation;
  if(!m_Graph || !m_Ctx.IsCaptureLoaded()) return;
  const uint32_t eid = m_Ctx.CurEvent();
  const auto graph = m_Graph;
  const int selectedPass = m_Selected;
  QVector<int> resources;
  for(int row = 0; row < m_Resources->topLevelItemCount(); ++row)
  {
    auto *item = m_Resources->topLevelItem(row);
    const int ri = item->data(0,Qt::UserRole).toInt();
    item->setIcon(0,QIcon());
    if(graph->resources[ri].texture && !resources.contains(ri) && resources.size() < 32)
      resources.push_back(ri);
  }
  m_Resources->setIconSize(QSize(96,64));
  m_Details->append(tr("<p>Texture previews: current EID %1, mip 0 / slice 0. Not a producer snapshot. Up to 32 unique textures; double-click a resource to open it at its evidence EID.</p>").arg(eid));
  const auto cancelled = m_Cancelled;
  QPointer<RenderGraphViewer> viewer(this);
  m_Ctx.Replay().AsyncInvoke(lit("RenderGraphPreviews"),[viewer,graph,resources,eid,preview,generation,cancelled,link,selectedPass](IReplayController *controller) {
    if(cancelled && cancelled->load()) return;
    std::map<int,QImage> images;
    IReplayOutput *output = resources.isEmpty() ? nullptr :
        controller->CreateOutput(CreateHeadlessWindowingData(96,64),ReplayOutputType::Texture);
    if(output)
    {
      for(int ri : resources)
      {
        if(cancelled && cancelled->load()) break;
        const bytebuf bytes = output->DrawThumbnail(96,64,graph->resources[ri].id,
                                                    Subresource{0,0,~0U},CompType::Typeless);
        if(bytes.size() >= 96*64*4)
          images[ri] = QImage(bytes.data(),96,64,QImage::Format_RGBA8888).copy();
      }
      output->Shutdown();
    }
    GUIInvoke::call(QCoreApplication::instance(),[viewer,images,preview,generation,eid,link,selectedPass] {
      if(!viewer || viewer->m_Generation != generation || viewer->m_PreviewGeneration != preview ||
         viewer->m_Ctx.CurEvent() != eid) return;
      for(int row = 0; row < viewer->m_Resources->topLevelItemCount(); ++row)
      {
        auto *item = viewer->m_Resources->topLevelItem(row);
        auto found = images.find(item->data(0,Qt::UserRole).toInt());
        if(found != images.end()) item->setIcon(0,QIcon(QPixmap::fromImage(found->second)));
        else item->setToolTip(0,viewer->tr("No thumbnail returned (buffer, preview limit, or unsupported texture/output)."));
      }
      if(images.empty())
      {
        viewer->m_Status->setText(viewer->tr("No texture preview available for this selection: buffers have no image; the replay driver may not support headless thumbnails."));
        return;
      }
      QGraphicsPathItem *connection = nullptr;
      QGraphicsItem *selectedCard = nullptr;
      for(QGraphicsItem *item : viewer->m_Scene->items())
      {
        if(item->data(1).toString() == lit("link") && item->data(0).toInt() == link)
          connection = static_cast<QGraphicsPathItem *>(item);
        if(item->data(1).toString() == lit("pass") && item->data(0).toInt() == selectedPass)
          selectedCard = item;
      }
      if(!connection && !selectedCard) return;
      const QPointF anchor = connection ? connection->path().pointAtPercent(0.5)+QPointF(0,26) :
                                         selectedCard->sceneBoundingRect().bottomLeft()+QPointF(0,24);
      int index = 0;
      for(const auto &entry : images)
      {
        auto *thumb = new TexturePreviewItem();
        thumb->image = entry.second; thumb->colors = viewer->palette();
        const auto &resource = viewer->m_Graph->resources[entry.first];
        thumb->name = resource.name;
        thumb->setData(1,lit("texturePreview")); thumb->setZValue(2);
        thumb->setCursor(Qt::PointingHandCursor);
        thumb->setToolTip(viewer->tr("%1 · preview EID %2 · mip 0 / slice 0\nClick to open texture").arg(resource.name).arg(eid));
        const ResourceId id = resource.id;
        thumb->open = [viewer,id] {
          if(!viewer) return;
          QTimer::singleShot(0,viewer.data(),[viewer,id] {
            if(viewer && viewer->m_Ctx.IsCaptureLoaded())
              viewer->m_Ctx.GetTextureViewer()->ViewTexture(id,CompType::Typeless,true);
          });
        };
        QPointF location = anchor+QPointF((index%4)*124,(index/4)*110);
        // Keep preview tiles off cards, group titles and other preview tiles.
        bool collision = true;
        while(collision)
        {
          collision = false;
          const QRectF rect = thumb->boundingRect().translated(location).adjusted(-6,-6,6,6);
          for(QGraphicsItem *item : viewer->m_Scene->items())
            if(item->zValue() >= 0 && rect.intersects(item->sceneBoundingRect()))
            { collision = true; location.setY(item->sceneBoundingRect().bottom()+12); break; }
        }
        thumb->setPos(location); viewer->m_Scene->addItem(thumb); ++index;
      }
      viewer->m_Scene->setSceneRect(viewer->m_Scene->itemsBoundingRect().adjusted(-40,-40,40,40));
      viewer->m_Status->setText(viewer->tr("%1 texture previews loaded at EID %2. Also visible in the resource list; click a preview to open its texture.").arg(images.size()).arg(eid));
    });
  });
}
QString RenderGraphViewer::TimingText(int pi) const
{
  if(!m_HasTimings || !m_Graph || pi < 0 || pi >= m_Graph->passes.size())
    return tr("Not measured");
  const auto summaries = RenderGraph::SummariseTimings(*m_Graph,m_Timings);
  const auto &s = summaries[pi];
  if(s.samples == 0) return tr("Not reported");
  const QString coverage = s.samples < s.expected ? tr(" · %1/%2 events").arg(s.samples).arg(s.expected) : QString();
  return tr("%1 ms%2").arg(s.seconds*1000.0,0,'f',3).arg(coverage);
}
void RenderGraphViewer::Measure()
{
  if(!m_Graph || !m_Ctx.IsCaptureLoaded()) return;
  const uint64_t generation = m_Generation;
  m_Timing->setEnabled(false); m_Timing->setText(tr("Measuring…"));
  m_Status->setText(tr("Collecting EventGPUDuration… This replay operation cannot be interrupted safely."));
  QPointer<RenderGraphViewer> viewer(this);
  m_Ctx.Replay().AsyncInvoke([viewer,generation](IReplayController *controller) {
    bool supported = false;
    for(GPUCounter c : controller->EnumerateCounters())
      if(c == GPUCounter::EventGPUDuration) supported = true;
    std::map<uint32_t,double> timings;
    if(supported)
    {
      rdcarray<GPUCounter> requested; requested.push_back(GPUCounter::EventGPUDuration);
      for(const CounterResult &r : controller->FetchCounters(requested))
        if(r.counter == GPUCounter::EventGPUDuration && std::isfinite(r.value.d) && r.value.d >= 0.0)
          timings[r.eventId] = r.value.d;
    }
    GUIInvoke::call(QCoreApplication::instance(),[viewer,generation,supported,timings] {
      if(!viewer || viewer->m_Generation != generation) return;
      viewer->m_Timings = timings; viewer->m_HasTimings = supported;
      viewer->m_Timing->setText(viewer->tr("Measure GPU")); viewer->m_Timing->setEnabled(true);
      viewer->Display();
      viewer->m_Status->setText(!supported ? viewer->tr("EventGPUDuration is not available on this replay driver.") :
        timings.empty() ? viewer->tr("Timing supported, but no valid event samples were reported.") :
        viewer->tr("GPU timing collected for %1 events. Pass totals include only reported leaf actions; no critical-path claim is made.").arg(qulonglong(timings.size())));
    });
  });
}
void RenderGraphViewer::ShowHistory(int ri)
{
  if(!m_Graph || ri < 0 || ri >= m_Graph->resources.size()) return;
  const auto &g = *m_Graph; const auto &resource = g.resources[ri];
  m_HistoryResource = ri; m_Selected = -1; m_Overview->setEnabled(true);
  m_Selecting = true; m_Scene->clear();
  m_Resources->clear(); m_Jump->setEnabled(false);
  const int cardWidth = 270, gap = 100;
  int column = 0;
  for(const auto &a : g.accesses) if(a.resource == ri)
  {
    const QString version = a.outputVersion >= 0 ? tr("produces v%1").arg(a.outputVersion) : tr("reads v%1").arg(a.version);
    auto *rect = m_Scene->addRect(QRectF(column*(cardWidth+gap),0,cardWidth,112),
      QPen(Accent(g.passes[a.pass].kind),2),palette().color(QPalette::Base));
    rect->setToolTip(ToQStr(a.usage));
    auto *title = m_Scene->addText(g.passes[a.pass].name); title->setDefaultTextColor(palette().color(QPalette::Text));
    title->setTextWidth(cardWidth-24); title->setPos(QPointF(column*(cardWidth+gap)+12,10));
    auto *detail = m_Scene->addSimpleText(tr("%1 · EID %2\n%3").arg(RenderGraph::AccessName(a.kind)).arg(a.event).arg(version));
    detail->setBrush(palette().color(QPalette::Disabled,QPalette::Text)); detail->setPos(column*(cardWidth+gap)+12,65);
    if(column > 0)
    {
      const QPointF start((column-1)*(cardWidth+gap)+cardWidth,56), end(column*(cardWidth+gap),56);
      QPainterPath path(start); path.cubicTo(start+QPointF(45,0),end-QPointF(45,0),end);
      QPen progression(palette().color(QPalette::Mid),2,Qt::DashLine);
      auto *progress = m_Scene->addPath(path,progression);
      progress->setToolTip(tr("Chronological history progression; not necessarily a data dependency"));
    }
    auto *item = new QTreeWidgetItem(m_Resources,{g.passes[a.pass].name,
      tr("%1 · v%2 · EID %3").arg(RenderGraph::AccessName(a.kind)).arg(a.version).arg(a.event)});
    item->setData(0,Qt::UserRole,ri); item->setData(0,Qt::UserRole+1,a.event);
    ++column;
  }
  m_Scene->setSceneRect(m_Scene->itemsBoundingRect().adjusted(-30,-30,30,30)); m_Selecting = false;
  m_Details->setHtml(tr("<h2>%1</h2><p>%2 history entries</p><p>Each write creates a logical version. Entries are resource-level usage evidence; subresources and byte ranges are not available in GetUsage.</p><p>Double-click an entry to select its event.</p>")
    .arg(Html(resource.name)).arg(column));
  m_Stats->setText(tr("RESOURCE HISTORY     /     %1 ACCESSES     /     RESOURCE-LEVEL PRECISION").arg(column));
  m_Status->setText(tr("History mode · Writes create logical versions · Discard invalidates prior contents · no allocation-lifetime claim"));
  Fit();
}
void RenderGraphViewer::ExportImage()
{
  if(m_Scene->items().isEmpty()) return;
  const QString filename = QFileDialog::getSaveFileName(this,tr("Export RenderGraph"),lit("rendergraph.png"),
                                                        tr("PNG Image (*.png);;SVG Vector Image (*.svg)"));
  if(filename.isEmpty()) return;
  const QRectF bounds = m_Scene->itemsBoundingRect().adjusted(-24,-24,24,24);
  if(bounds.isEmpty()) return;
  // Cap either dimension while preserving detail and aspect ratio.
  QSize size = bounds.size().toSize(); size.scale(QSize(12000,12000),Qt::KeepAspectRatio);
  size.setWidth(qMax(1,size.width())); size.setHeight(qMax(1,size.height()));
  bool saved = false;
  if(filename.endsWith(lit(".svg"),Qt::CaseInsensitive))
  {
    QSvgGenerator svg; svg.setFileName(filename); svg.setSize(size); svg.setViewBox(QRect(QPoint(0,0),size));
    svg.setTitle(tr("RenderGraph View export"));
    QPainter painter(&svg); painter.fillRect(QRect(QPoint(0,0),size),palette().color(QPalette::Window));
    m_Scene->render(&painter,QRectF(QPointF(0,0),size),bounds,Qt::KeepAspectRatio); saved = painter.isActive(); painter.end();
  }
  else
  {
    QImage image(size,QImage::Format_ARGB32_Premultiplied); image.fill(palette().color(QPalette::Window));
    QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing);
    m_Scene->render(&painter,QRectF(QPointF(0,0),size),bounds,Qt::KeepAspectRatio); painter.end();
    saved = image.save(filename,"PNG");
  }
  if(!saved) m_Status->setText(tr("Could not save %1").arg(filename));
  else m_Status->setText(tr("Exported %1 × %2 graph to %3").arg(size.width()).arg(size.height()).arg(filename));
}
void RenderGraphViewer::OnSelectedEventChanged(uint32_t eventId)
{
  if(!m_Graph || m_HistoryResource >= 0) return;
  const auto found = m_Graph->eventPass.find(eventId);
  if(found != m_Graph->eventPass.end())
  {
    Select(found->second, m_Mode->currentIndex() == 1);
    if(m_Mode->currentIndex() != 1)
    {
      m_Selecting = true;
      for(QGraphicsItem *item : m_Scene->items())
        if(item->data(1).toString() == lit("pass"))
        {
          auto *card = static_cast<PassItem *>(item);
          card->current = item->data(0).toInt() == found->second;
          card->setSelected(card->current); card->update();
        }
      m_Selecting = false;
    }
  }
}
void RenderGraphViewer::OnEventChanged(uint32_t eventId)
{
  for(QGraphicsItem *item : m_Scene->items())
    if(item->data(1).toString() == lit("texturePreview")) delete item;
  ++m_PreviewGeneration;
  for(int row = 0; row < m_Resources->topLevelItemCount(); ++row)
    m_Resources->topLevelItem(row)->setIcon(0,QIcon());
  OnSelectedEventChanged(eventId);
}
