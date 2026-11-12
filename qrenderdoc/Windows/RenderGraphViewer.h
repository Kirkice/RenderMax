#pragma once
#include <QFrame>
#include <memory>
#include <atomic>
#include "Code/Interface/QRDInterface.h"
#include "Code/RenderGraph.h"

class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTextBrowser;
class QCheckBox;
class QComboBox;

class RenderGraphViewer : public QFrame, public ICaptureViewer
{
public:
  explicit RenderGraphViewer(ICaptureContext &ctx, QWidget *parent = nullptr);
  ~RenderGraphViewer();
  void OnCaptureLoaded() override;
  void OnCaptureClosed() override;
  void OnSelectedEventChanged(uint32_t eventId) override;
  void OnEventChanged(uint32_t eventId) override;
private:
  ICaptureContext &m_Ctx;
  QGraphicsScene *m_Scene;
  QGraphicsView *m_View;
  QLabel *m_Status, *m_Stats;
  QLineEdit *m_Search;
  QTreeWidget *m_Resources;
  QTextBrowser *m_Details;
  QPushButton *m_Rebuild, *m_Cancel, *m_Jump, *m_Resource, *m_Timing, *m_History;
  QPushButton *m_Overview, *m_Export, *m_OpenFocus;
  std::map<uint32_t, double> m_Timings;
  bool m_HasTimings = false;
  int m_HistoryResource = -1;
  QCheckBox *m_Bindings;
  QComboBox *m_Mode;
  std::shared_ptr<RenderGraph::Snapshot> m_Graph;
  std::shared_ptr<std::atomic_bool> m_Cancelled;
  uint64_t m_Generation = 0, m_PreviewGeneration = 0, m_SelectionGeneration = 0;
  int m_Selected = -1;
  bool m_Selecting = false;
  bool AccessVisible(const RenderGraph::Access &access) const;
  void Build();
  void Display();
  void DisplayFocus();
  void Select(int pass, bool redraw = true);
  void ShowLink(int link);
  void LoadResourcePreviews(int link);
  void ShowHistory(int resource);
  void Measure();
  void ExportImage();
  QString TimingText(int pass) const;
  void Fit();
};
