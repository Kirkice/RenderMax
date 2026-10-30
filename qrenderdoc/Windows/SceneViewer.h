#pragma once

#include <atomic>
#include <QLabel>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QElapsedTimer>
#include <QWheelEvent>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QtMath>
#include "Code/QRDUtils.h"
#include "Widgets/CustomPaintWidget.h"

// Original-material frame replay. No mesh extraction, shader replacement or capture writes.
// Initial camera binding: Unity OpenGL MatrixVP vec4[4] (verified on Ying_Ultra.rdc).
class SceneViewer : public QWidget, public ICaptureViewer
{
public:
  SceneViewer(ICaptureContext &ctx, QWidget *parent) : QWidget(parent), m_Ctx(ctx)
  {
    setObjectName(lit("sceneViewer"));
    setWindowTitle(tr("Scene Viewer"));
    QVBoxLayout *box = new QVBoxLayout(this);
    QHBoxLayout *bar = new QHBoxLayout();
    QPushButton *bind = new QPushButton(tr("Use selected draw camera"), this);
    QPushButton *reset = new QPushButton(tr("Reset camera"), this);
    m_Speed = new QDoubleSpinBox(this);
    m_Speed->setRange(0.001, 10000);
    m_Speed->setValue(2);
    bar->addWidget(bind); bar->addWidget(reset);
    bar->addWidget(new QLabel(tr("Speed"), this)); bar->addWidget(m_Speed);
    m_Gamma = new QCheckBox(tr("Gamma"), this); m_Gamma->setChecked(true);
    bar->addWidget(m_Gamma); bar->addStretch();
    box->addLayout(bar);
    m_Status = new QLabel(tr("Automatically detecting the main perspective camera. "
                            "Supported: local OpenGL, D3D11, D3D12 and Vulkan replay."), this);
    m_Status->setWordWrap(true); box->addWidget(m_Status);
    m_Render = new CustomPaintWidget(this);
    m_Render->SetContext(ctx); box->addWidget(m_Render, 1);
    box->addWidget(new QLabel(tr("Hold RMB: look + WASD move, QE vertical, Shift accelerate. "
                                 "Replays the captured frame; baked culling/shadows/history remain captured."), this));
    connect(bind, &QPushButton::clicked, this, [this] {
      if(m_Ctx.Replay().CurrentRemote().IsValid())
      { m_Status->setText(tr("Scene camera replay currently requires local replay.")); return; }
      const ActionDescription *a = m_Ctx.CurAction();
      if(!a || !(a->flags & ActionFlags::Drawcall))
      { m_Status->setText(tr("Select a scene draw first.")); return; }
      m_Anchor = a->eventId; m_Position = FloatVector(); m_Rotation = FloatVector(); Refresh();
    });
    connect(m_Gamma, &QCheckBox::toggled, this, [this] { Refresh(); });
    connect(reset, &QPushButton::clicked, this, [this] {
      m_Position = FloatVector(); m_Rotation = FloatVector(); Refresh();
    });
    connect(m_Render, &CustomPaintWidget::clicked, this, [this](QMouseEvent *e) {
      if(e->button() == Qt::RightButton) { m_Look = true; m_Last = e->pos(); m_Render->setFocus(); }
    });
    connect(m_Render, &CustomPaintWidget::unclicked, this, [this](QMouseEvent *e) {
      if(e->button() == Qt::RightButton) { m_Look = false; m_Keys.clear(); }
    });
    connect(m_Render, &CustomPaintWidget::mouseMove, this, [this](QMouseEvent *e) {
      if(m_Look) {
        QPoint d = e->pos() - m_Last; m_Last = e->pos();
        // Scene camera uses the inverse view transform: drag and movement are
        // intentionally negated here so the controls match Unity's viewport.
        m_Rotation.y -= d.x() * 0.003f;
        m_Rotation.x = qBound(-1.5f, m_Rotation.x - d.y() * 0.003f, 1.5f); Refresh();
      }
    });
    connect(m_Render, &CustomPaintWidget::keyPress, this, [this](QKeyEvent *e) {
      if(m_Look) m_Keys.insert(e->key());
    });
    connect(m_Render, &CustomPaintWidget::keyRelease, this, [this](QKeyEvent *e) {
      if(!e->isAutoRepeat()) m_Keys.remove(e->key());
    });
    connect(m_Render, &CustomPaintWidget::mouseWheel, this, [this](QWheelEvent *e) {
      if(m_Look)
      {
        m_Speed->setValue(m_Speed->value() * qPow(1.2, e->angleDelta().y() / 120.0));
        e->accept();
      }
    });
    m_Clock.start();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
      float dt = qMin(0.1f, float(m_Clock.restart()) / 1000.0f);
      if(!m_Look || m_Keys.isEmpty() || !isVisible() || !m_Render->hasFocus()) return;
      float step = float(m_Speed->value()) * dt * (m_Keys.contains(Qt::Key_Shift) ? 4 : 1);
      float forward = float(m_Keys.contains(Qt::Key_S) - m_Keys.contains(Qt::Key_W));
      float right = float(m_Keys.contains(Qt::Key_D) - m_Keys.contains(Qt::Key_A));
      float up = float(m_Keys.contains(Qt::Key_Q) - m_Keys.contains(Qt::Key_E));
      float length = qMax(1.0f, float(qSqrt(forward * forward + right * right + up * up)));
      step /= length;
      // Camera-local coordinates, including pitch. Backend maps this basis to the captured view.
      m_Position.x += step * (right * qCos(m_Rotation.y) + forward * qSin(m_Rotation.y) * qCos(m_Rotation.x));
      m_Position.z += step * (right * qSin(m_Rotation.y) - forward * qCos(m_Rotation.y) * qCos(m_Rotation.x));
      m_Position.y += step * (up - forward * qSin(m_Rotation.x)); Refresh();
    });
    timer->start(33);
    m_Ctx.AddCaptureViewer(this);
  }
  ~SceneViewer()
  {
    m_Render->SetOutput(NULL);
    IReplayOutput *output = m_Output;
    m_Output = NULL;
    if(output && m_Ctx.IsCaptureLoaded())
      m_Ctx.Replay().BlockInvoke([output](IReplayController *) { output->Shutdown(); });
    m_Ctx.RemoveCaptureViewer(this);
  }
  void OnCaptureLoaded() override
  {
    m_Generation++;
    m_Position = FloatVector(); m_Rotation = FloatVector();
    m_Anchor = 0;
    // Backend discovers the dominant perspective colour camera once per capture.
    // ~0U requests automatic binding; 0 leaves camera replay disabled.
    if(!m_Ctx.Replay().CurrentRemote().IsValid()) m_Anchor = ~0U;
    m_End = 0; m_Texture = ResourceId();
    for(const TextureDescription &t : m_Ctx.GetTextures())
      if((t.creationFlags & TextureCategory::SwapBuffer) &&
         !(t.creationFlags & TextureCategory::DepthTarget)) m_Texture = t.resourceId;
    std::function<void(const rdcarray<ActionDescription> &)> visit = [&](const rdcarray<ActionDescription> &actions) {
      for(const ActionDescription &a : actions) {
        if(a.flags & (ActionFlags::Drawcall | ActionFlags::Dispatch | ActionFlags::Present | ActionFlags::Copy | ActionFlags::Clear))
          m_End = qMax(m_End, a.eventId);
        visit(a.children);
      }
    };
    visit(m_Ctx.CurRootActions());
    WindowingData win = m_Render->GetWidgetWindowingData();
    QPointer<SceneViewer> me(this);
    uint32_t generation = m_Generation;
    m_Ctx.Replay().AsyncInvoke([this, me, win, generation](IReplayController *r) {
      if(!me || generation != m_Generation) return;
      m_Output = r->CreateOutput(win, ReplayOutputType::Texture);
      GUIInvoke::call(this, [this, generation] {
        if(generation != m_Generation) return;
        m_Render->SetOutput(m_Output); Refresh();
      });
    });
  }
  void OnCaptureClosed() override
  {
    m_Generation++;
    m_Busy = false;
    m_Render->SetOutput(NULL); m_Output = NULL; m_Anchor = 0; m_Keys.clear(); m_Look = false;
    m_Status->setText(tr("Open a capture and select a scene draw."));
  }
  void OnSelectedEventChanged(uint32_t) override {}
  void OnEventChanged(uint32_t) override { Refresh(); }
protected:
  void showEvent(QShowEvent *e) override { QWidget::showEvent(e); Refresh(); }
  void hideEvent(QHideEvent *e) override
  { m_Look = false; m_Keys.clear(); QWidget::hideEvent(e); }
private:
  void Refresh()
  {
    if(!m_Output || !m_Ctx.IsCaptureLoaded() || !isVisible()) return;
    if(m_Busy) { m_Pending = true; return; }
    m_Pending = false;
    m_Busy = true;
    FloatVector position = m_Position, rotation = m_Rotation;
    uint32_t anchor = m_Anchor, end = m_End, restore = m_Ctx.CurEvent();
    uint32_t generation = m_Generation;
    TextureDisplay tex;
    tex.resourceId = m_Texture;
    tex.scale = -1.0f;
    tex.linearDisplayAsGamma = m_Gamma->isChecked();
    QPointer<SceneViewer> me(this);
    m_Ctx.Replay().AsyncInvoke([this, me, position, rotation, anchor, end, restore, tex, generation](IReplayController *r) {
      if(!me || generation != m_Generation || !m_Output) return;
      if(!anchor) r->SetFrameEvent(end, true);
      m_Output->SetTextureDisplay(tex);
      bool supported = m_Output->SetSceneCamera(anchor, position, rotation, anchor != 0, end);
      m_Output->Display();
      // Present into the independent output first, then restore all capture textures/state.
      if(!anchor) r->SetFrameEvent(restore, true);
      GUIInvoke::call(this, [this, supported, anchor, generation] {
        if(generation != m_Generation) return;
        m_Busy = false;
        m_Status->setText(!supported ? tr("No compatible camera binding was found in this replay.") :
          anchor == ~0U ? tr("Automatic main perspective camera | Shadow/UI passes excluded") :
          anchor ? tr("Camera anchor EID %1 | Original-material BackBuffer replay").arg(anchor) :
          tr("Original BackBuffer. Select a main-scene draw and bind its camera."));
        if(m_Pending) Refresh();
      });
    });
  }
  ICaptureContext &m_Ctx;
  CustomPaintWidget *m_Render = NULL;
  QLabel *m_Status = NULL;
  QDoubleSpinBox *m_Speed = NULL;
  QCheckBox *m_Gamma = NULL;
  IReplayOutput *m_Output = NULL;
  ResourceId m_Texture;
  uint32_t m_Anchor = 0, m_End = 0;
  std::atomic<uint32_t> m_Generation{0};
  bool m_Look = false, m_Busy = false, m_Pending = false;
  QElapsedTimer m_Clock;
  QPoint m_Last;
  QSet<int> m_Keys;
  FloatVector m_Position, m_Rotation;
};
