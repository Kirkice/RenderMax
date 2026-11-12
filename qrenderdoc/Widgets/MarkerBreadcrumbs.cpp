/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2021-2026 Baldur Karlsson
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

#include "MarkerBreadcrumbs.h"
#include <QAction>
#include <QMenu>
#include <QStyle>
#include <QStylePainter>
#include <QStyleOptionToolButton>
#include <QScopedValueRollback>
#include <QSet>
#include "Code/QRDUtils.h"
#include "Code/Resources.h"
#include "Widgets/Extended/RDLabel.h"
#include "Widgets/Extended/RDToolButton.h"

// Width of the stylesheet's `QToolButton::menu-indicator` image, from the theme's QSS. The style
// hands the arrow to the stylesheet and then reports PM_MenuButtonIndicator as 0, so this is the
// only value that says how much room the arrow takes.
static const int kStyledMenuArrowWidth = 12;

class BreadcrumbButton : public RDToolButton
{
public:
  BreadcrumbButton(bool root) : m_Root(root) {}
protected:
  void paintEvent(QPaintEvent *) override
  {
    QStylePainter painter(this);
    painter.setClipRect(rect());
    QStyleOptionToolButton option;
    initStyleOption(&option);
    if(m_Root)
    {
      const QIcon icon = option.icon;
      option.icon = QIcon();
      painter.drawComplexControl(QStyle::CC_ToolButton,option);
      // The root button is anchored at x=0 in the breadcrumb bar; do not let theme
      // padding or the menu arrow move its icon away from the literal 6px inset.
      icon.paint(&painter,QRect(6,(height()-iconSize().height())/2,
                               iconSize().width(),iconSize().height()),Qt::AlignCenter,
                 isEnabled() ? QIcon::Normal : QIcon::Disabled);
    }
    else
    {
      const int arrow = menu() ? qMax(kStyledMenuArrowWidth,
          style()->pixelMetric(QStyle::PM_MenuButtonIndicator,&option,this)) : 0;
      option.text = fontMetrics().elidedText(text(),Qt::ElideRight,qMax(0,width()-arrow-16));
      painter.drawComplexControl(QStyle::CC_ToolButton,option);
    }
  }
private:
  bool m_Root;
};

BreadcrumbsLayout::BreadcrumbsLayout(QWidget *parent, RDToolButton *elidedItems) : QLayout(parent)
{
  m_ElidedItems = elidedItems;
}

BreadcrumbsLayout::~BreadcrumbsLayout()
{
}

void BreadcrumbsLayout::clear()
{
  while(!m_Items.isEmpty())
  {
    QLayoutItem *item = m_Items.takeAt(0);
    delete item->widget();
    delete item;
  }
}

void BreadcrumbsLayout::addItem(QLayoutItem *item)
{
  m_Items.push_back(item);
}

int BreadcrumbsLayout::count() const
{
  return m_Items.count();
}

Qt::Orientations BreadcrumbsLayout::expandingDirections() const
{
  return 0;
}

QSize BreadcrumbsLayout::minimumSize() const
{
  QSize ret(0, 16);
  if(!m_Items.isEmpty())
    ret = m_Items[0]->minimumSize();
  ret.setWidth(100);
  return ret + QSize(contentsMargins().left() + contentsMargins().right(),
                     contentsMargins().top() + contentsMargins().bottom());
}

void BreadcrumbsLayout::setGeometry(const QRect &rect)
{
  if(m_InGeometry) return;
  QScopedValueRollback<bool> guard(m_InGeometry,true);
  QLayout::setGeometry(rect);
  const QRect area = rect.marginsRemoved(contentsMargins());
  QSet<QWidget *> visible;
  bool showEllipsis = false;
  auto applyVisibility = [&]() {
    for(QLayoutItem *item : m_Items)
    {
      QWidget *widget = item->widget();
      const bool show = visible.contains(widget);
      if(widget->isHidden() == show) widget->setVisible(show);
    }
    if(m_ElidedItems->isHidden() == showEllipsis) m_ElidedItems->setVisible(showEllipsis);
  };
  if(m_Items.isEmpty() || area.width() <= 0) { applyVisibility(); return; }

  int x = area.left();
  const int right = area.right()+1;
  auto place = [&](QLayoutItem *item,int width) {
    if(width <= 0) return;
    visible.insert(item->widget());
    item->widget()->setGeometry(QRect(x,area.top(),width,area.height()));
    x += width;
  };
  // QWidgetItem respects minimum sizes, so never compress a root/menu button below
  // its minimum width. Hidden ancestors must actually be hidden, not assigned a zero
  // rectangle which Qt can expand back to their minimum size on top of neighbours.
  place(m_Items[0],qMax(m_Items[0]->widget()->sizeHint().width(),
                       m_Items[0]->widget()->minimumWidth()));
  if(m_Items.size() == 1 || x >= right) { applyVisibility(); return; }
  int total = 0;
  for(int i = 1; i < m_Items.size(); ++i) total += m_Items[i]->widget()->sizeHint().width();
  if(total <= right-x)
  {
    for(int i = 1; i < m_Items.size(); ++i) place(m_Items[i],m_Items[i]->widget()->sizeHint().width());
  }
  else
  {
    const int ellipsis = qMax(28,m_ElidedItems->sizeHint().width());
    if(right-x >= ellipsis)
    {
      m_ElidedItems->setGeometry(x,area.top(),ellipsis,area.height());
      showEllipsis = true; x += ellipsis;
      int first = m_Items.size()-1;
      int used = qMin(m_Items[first]->widget()->sizeHint().width(),right-x);
      while(first > 1 && used+m_Items[first-1]->widget()->sizeHint().width() <= right-x)
      { --first; used += m_Items[first]->widget()->sizeHint().width(); }
      for(int i = first; i < m_Items.size(); ++i)
      {
        const int width = qMin(m_Items[i]->widget()->sizeHint().width(),right-x);
        if(width >= qMax(40,m_Items[i]->widget()->minimumWidth())) place(m_Items[i],width);
      }
    }
  }
  applyVisibility();
  m_PrevRect = rect;
}

QSize BreadcrumbsLayout::sizeHint() const
{
  QSize ret(0, 16);
  for(QLayoutItem *item : m_Items)
  {
    QSize s = item->widget()->sizeHint();

    ret.setWidth(ret.width() + s.width());
    ret.setHeight(qMax(ret.height(), s.height()));
  }
  return ret;
}

QLayoutItem *BreadcrumbsLayout::itemAt(int index) const
{
  return index >= 0 && index < m_Items.count() ? m_Items[index] : NULL;
}

QLayoutItem *BreadcrumbsLayout::takeAt(int index)
{
  if(index >= 0 && index < m_Items.count())
    return m_Items.takeAt(index);

  return NULL;
}

MarkerBreadcrumbs::MarkerBreadcrumbs(ICaptureContext &ctx, IEventBrowser *browser, QWidget *parent)
    : QFrame(parent), m_Ctx(ctx), m_Browser(browser)
{
  setFont(Formatter::PreferredFont());

  m_ElidedItems = new RDToolButton(this);
  m_ElidedItems->setAutoRaise(true);
  m_ElidedItems->setText(lit("..."));

  m_Layout = new BreadcrumbsLayout(this, m_ElidedItems);
  m_Layout->setContentsMargins(QMargins(0, 2, 0, 2));
  m_Layout->setMargin(0);
  setLayout(m_Layout);

  m_ElidedMenu = new QMenu(this);

  QObject::connect(m_ElidedItems, &RDToolButton::clicked, this,
                   &MarkerBreadcrumbs::elidedItemsClicked);
}

MarkerBreadcrumbs::~MarkerBreadcrumbs()
{
}

void MarkerBreadcrumbs::OnEventChanged(uint32_t eventId)
{
  const ActionDescription *parent = m_Browser->GetActionForEID(m_Ctx.CurEvent());

  if(parent != NULL && !(parent->flags & ActionFlags::PushMarker))
    parent = parent->parent;

  if(m_CurParent == parent && m_Layout->count() != 0)
    return;

  m_CurParent = parent;
  m_Layout->clear();

  QVector<const ActionDescription *> path;

  while(parent)
  {
    path.push_back(parent);
    parent = parent->parent;
  }

  AddPathButton(NULL);

  m_Path.clear();
  for(int i = path.count() - 1; i >= 0; i--)
  {
    m_Path.push_back(path[i]);
    AddPathButton(path[i]);
  }
}

void MarkerBreadcrumbs::ForceRefresh()
{
  m_CurParent = NULL;
  m_Layout->clear();

  OnEventChanged(m_Ctx.CurEvent());
}

void MarkerBreadcrumbs::ConfigurePathMenu(QMenu *menu, const ActionDescription *action)
{
  const rdcarray<ActionDescription> &actions = action ? action->children : m_Ctx.CurRootActions();

  menu->clear();
  for(const ActionDescription &child : actions)
  {
    if((child.flags & ActionFlags::PushMarker) && m_Browser->IsAPIEventVisible(child.eventId))
    {
      QAction *menuAction = new QAction(child.customName, menu);

      uint32_t eid = child.eventId;

      if(child.IsFakeMarker())
        eid = child.children[0].eventId;

      QObject::connect(menuAction, &QAction::triggered,
                       [this, eid]() { m_Ctx.SetEventID({}, eid, eid); });

      menu->addAction(menuAction);
    }
  }
}

void MarkerBreadcrumbs::elidedItemsClicked()
{
  m_ElidedMenu->clear();

  for(int i = 0; i < m_Layout->count(); i++)
  {
    RDToolButton *button = qobject_cast<RDToolButton *>(m_Layout->itemAt(i)->widget());

    if(button && button->isHidden())
    {
      QAction *action = new QAction(button->text(), m_ElidedMenu);

      QObject::connect(action, &QAction::triggered, [button]() { button->click(); });

      m_ElidedMenu->addAction(action);
    }
  }

  m_ElidedMenu->move(mapToGlobal(m_ElidedItems->geometry().bottomLeft()));
  m_ElidedMenu->show();
}

void MarkerBreadcrumbs::AddPathButton(const ActionDescription *action)
{
  RDToolButton *b = new BreadcrumbButton(action == NULL);
  b->setText(action ? QString(action->customName) : QString());
  if(action) b->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
  b->setToolTip(b->text());
  if(!action)
  {
    b->setIcon(Icons::house());
    b->setToolButtonStyle(Qt::ToolButtonIconOnly);
    b->setToolTip(tr("Capture Root"));
    b->setFixedWidth(6 + b->iconSize().width() + kStyledMenuArrowWidth + 8);
  }

  bool hasChildMarkers = false;

  for(const ActionDescription &child : action ? action->children : m_Ctx.CurRootActions())
  {
    if((child.flags & ActionFlags::PushMarker) && m_Browser->IsAPIEventVisible(child.eventId))
    {
      hasChildMarkers = true;
      break;
    }
  }

  if(hasChildMarkers)
  {
    QMenu *menu = new QMenu(b);

    b->setPopupMode(QToolButton::MenuButtonPopup);
    b->setMenu(menu);

    // A button that carries only an icon is sized for that icon, and the dropdown arrow is then
    // drawn inside the same width. The style is supposed to include PM_MenuButtonIndicator in the
    // size for exactly this reason, but the stylesheet takes the arrow over and reports that metric
    // as 0 while drawing `QToolButton::menu-indicator` from the sheet, so the record has to make
    // the room itself - otherwise the arrow lands on the right-hand side of the icon.
    const int arrow = b->style()->pixelMetric(QStyle::PM_MenuButtonIndicator, NULL, b);
    if(action && arrow == 0 && b->toolButtonStyle() == Qt::ToolButtonIconOnly)
      b->setMinimumWidth(b->sizeHint().width() + kStyledMenuArrowWidth);

    QObject::connect(menu, &QMenu::aboutToShow,
                     [this, menu, action]() { ConfigurePathMenu(menu, action); });
  }

  uint32_t eid = action ? action->eventId : 0;

  if(action && action->IsFakeMarker())
    eid = action->children[0].eventId;

  b->setAutoRaise(true);
  QObject::connect(b, &RDToolButton::clicked, [this, eid]() { m_Ctx.SetEventID({}, eid, eid); });
  m_Layout->addWidget(b);
}
