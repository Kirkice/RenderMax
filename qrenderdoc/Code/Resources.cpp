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

#include "Resources.h"
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDirIterator>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QSvgRenderer>
#include <QVector>
#include "QRDUtils.h"

Resources::ResourceSet *Resources::resources = NULL;

// Objects whose icon came from a "rdIcon" property. Kept so a style change can recolour windows that
// are already open - the stored QIcon has the old palette's colour baked into it.
namespace
{
struct LineIconUser
{
  QPointer<QObject> object;
  QString name;
};

QVector<LineIconUser> &lineIconUsers()
{
  static QVector<LineIconUser> users;
  return users;
}

// Windows resolve their own line-art icons when they are shown, so nothing has to be wired into each
// window by hand.
class LineIconFilter : public QObject
{
protected:
  bool eventFilter(QObject *watched, QEvent *event) override
  {
    // Only top-levels: the .ui files put the property on the children, and a window is shown only
    // once its whole descriptor has been built.
    if(event->type() == QEvent::Show)
    {
      QWidget *window = qobject_cast<QWidget *>(watched);

      if(window && window->isWindow())
        Resources::ApplyLineIcons(window);
      // Anything created after its window was shown still gets its icon this way.
      else if(watched->property("rdIcon").isValid())
        Resources::ApplyLineIcons(watched);
    }

    return QObject::eventFilter(watched, event);
  }
};
}    // namespace

void Resources::Initialise()
{
  QList<QString> filenames;

  resources = new Resources::ResourceSet();

#undef RESOURCE_DEF
#define RESOURCE_DEF(name, filename)                                 \
  {                                                                  \
    QString fn = lit(":/" filename);                                 \
    filenames.push_back(fn);                                         \
    if(fn.startsWith(lit(":/Line/")) || fn.startsWith(lit(":/UE/"))) \
    {                                                                \
      /* Line-art: one flat-coloured vector, tinted from the palette when it is first used. Nothing
         is rendered here because the style has not been installed at this point. */               \
      resources->name##_data.vectorPath = fn;                        \
      resources->name##_2x_data.vectorPath = fn;                     \
    }                                                                \
    else                                                             \
    {                                                                \
      resources->name##_data.pixmap = QPixmap(fn);                   \
      resources->name##_data.icon = QIcon();                         \
      resources->name##_data.icon.addFile(fn);                       \
      if(fn.contains(lit(".png")))                                   \
      {                                                              \
        fn = fn.replace(lit(".png"), lit("@2x.png"));                \
        if(QFile::exists(fn))                                        \
        {                                                            \
          resources->name##_2x_data.pixmap = QPixmap(fn);            \
        }                                                            \
        else                                                         \
        {                                                            \
          qWarning() << "Missing high-DPI @2x for " filename;        \
          resources->name##_2x_data.pixmap = resources->name##_data.pixmap; \
        }                                                            \
      }                                                              \
    }                                                                \
  }

  RESOURCE_LIST();

  // From here on every window picks up its line-art icons as it is shown.
  if(qApp)
    qApp->installEventFilter(new LineIconFilter());

  QDirIterator it(lit(":"));
  while(it.hasNext())
  {
    QString filename = it.next();
    if(filenames.contains(filename))
      continue;
    if(!filename.contains(lit(".png")))
      continue;
    if(filename.contains(lit("@2x.png")) || filename.contains(lit("@3x.png")) ||
       filename.contains(lit("@4x.png")))
      continue;

    qCritical() << "Resource not configured for" << filename;
  }
}

// Logical sizes a vector icon is rendered at. Qt picks the closest stored size and scales from
// there, so a handful of stops covers menu items (16), tool buttons (20-24) and toolbars (32-48)
// without keeping a raster around for every size anyone might ask for.
static const int iconSizes[] = {16, 24, 32, 48};

static void RenderVector(const QString &path, int logicalSize, int devicePixelRatio,
                         const QColor &colour, QPixmap &out)
{
  QSvgRenderer svg(path);

  if(!svg.isValid())
  {
    qWarning() << "Failed to load vector icon" << path;
    return;
  }

  const int px = logicalSize * devicePixelRatio;

  QImage im(px, px, QImage::Format_ARGB32_Premultiplied);
  im.fill(Qt::transparent);

  {
    QPainter p(&im);
    svg.render(&p, QRectF(0, 0, px, px));

    // The artwork is stored in a single flat colour so one file serves every theme; the real colour
    // is stamped in here. SourceIn keeps the antialiased coverage, so the edges stay smooth after
    // the recolour instead of being re-rasterised at the new colour.
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(im.rect(), colour);
  }

  out = QPixmap::fromImage(im);
  out.setDevicePixelRatio(devicePixelRatio);
}

void Resources::BuildVectorIcon(Resource &res)
{
  const QPalette pal = QApplication::palette();

  const QColor normal = pal.color(QPalette::Active, QPalette::WindowText);
  const QColor disabled = pal.color(QPalette::Disabled, QPalette::WindowText);
  const QColor selected = pal.color(QPalette::Active, QPalette::HighlightedText);

  QIcon icon;

  for(int size : iconSizes)
  {
    for(int dpr = 1; dpr <= 2; dpr++)
    {
      QPixmap px;

      RenderVector(res.vectorPath, size, dpr, normal, px);
      icon.addPixmap(px, QIcon::Normal, QIcon::Off);
      // Hover is expressed by the button behind the icon, not by second artwork.
      icon.addPixmap(px, QIcon::Active, QIcon::Off);

      RenderVector(res.vectorPath, size, dpr, selected, px);
      icon.addPixmap(px, QIcon::Selected, QIcon::Off);

      RenderVector(res.vectorPath, size, dpr, disabled, px);
      icon.addPixmap(px, QIcon::Disabled, QIcon::Off);
    }
  }

  res.icon = icon;
}

const Resource &Resources::Render(Resource &res, int scale)
{
  // Bitmaps were fully loaded at startup and never change colour.
  if(res.vectorPath.isEmpty())
    return res;

  if(res.icon.isNull() && scale == 1)
    BuildVectorIcon(res);

  if(res.pixmap.isNull() || res.pixmap.devicePixelRatio() != scale)
    RenderVector(res.vectorPath, 16, scale,
                 QApplication::palette().color(QPalette::Active, QPalette::WindowText), res.pixmap);

  return res;
}

void Resources::RefreshIcons()
{
  if(!resources)
    return;

  // Only the vector resources hold palette-dependent state; bitmaps keep what they loaded at
  // startup and would lose it if this cleared them.
#undef RESOURCE_DEF
#define RESOURCE_DEF(name, filename)                   \
  if(!resources->name##_data.vectorPath.isEmpty())     \
  {                                                    \
    resources->name##_data.icon = QIcon();             \
    resources->name##_data.pixmap = QPixmap();         \
  }                                                    \
  if(!resources->name##_2x_data.vectorPath.isEmpty())  \
  {                                                    \
    resources->name##_2x_data.icon = QIcon();          \
    resources->name##_2x_data.pixmap = QPixmap();      \
  }
  RESOURCE_LIST();

  // Recolour the icons of windows that are already open, then forget any that have gone away.
  const QVector<LineIconUser> users = lineIconUsers();

  for(const LineIconUser &user : users)
  {
    if(user.object)
      ApplyLineIcons(user.object.data());
  }

  QVector<LineIconUser> &live = lineIconUsers();

  for(int i = live.count() - 1; i >= 0; i--)
  {
    if(!live[i].object)
      live.removeAt(i);
  }
}

Resources::~Resources()
{
  delete resources;
}

void Resources::ApplyLineIcons(QObject *root)
{
  if(!resources || !root)
    return;

  QList<QObject *> objects = root->findChildren<QObject *>();
  objects.push_back(root);

  QVector<LineIconUser> &users = lineIconUsers();

  for(QObject *object : objects)
  {
    const QString name = object->property("rdIcon").toString();

    if(name.isEmpty())
      continue;

    Resource *res = NULL;
    Resource *res2x = NULL;

#undef RESOURCE_DEF
#define RESOURCE_DEF(icon, filename) \
  if(name == lit(#icon))             \
  {                                  \
    res = &resources->icon##_data;   \
    res2x = &resources->icon##_2x_data; \
  }
    RESOURCE_LIST();

    if(!res || res->vectorPath.isEmpty())
    {
      qWarning() << "Unknown line-art icon" << name << "requested by" << object;
      continue;
    }

    // The icon holds every size and DPR, but a label gets a single pixmap so it needs the one that
    // matches the screen it sits on.
    const bool hidpi =
        object->isWidgetType() && static_cast<QWidget *>(object)->devicePixelRatio() >= 2;

    if(QAbstractButton *button = qobject_cast<QAbstractButton *>(object))
      button->setIcon(Render(*res, 1).icon);
    else if(QAction *action = qobject_cast<QAction *>(object))
      action->setIcon(Render(*res, 1).icon);
    else if(QLabel *label = qobject_cast<QLabel *>(object))
      label->setPixmap(Render(hidpi ? *res2x : *res, hidpi ? 2 : 1).pixmap);
    else
      continue;

    bool recorded = false;

    for(LineIconUser &user : users)
    {
      if(user.object == object)
      {
        user.name = name;
        recorded = true;
        break;
      }
    }

    if(!recorded)
      users.push_back({object, name});
  }
}
