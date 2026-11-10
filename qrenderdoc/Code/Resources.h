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

#include <QIcon>
#include <QPixmap>
#include <QWidget>

#define RESOURCE_LIST()                                                        \
  RESOURCE_DEF(add, "Line/add.svg")                                            \
  RESOURCE_DEF(align, "Line/align.svg")                                        \
  RESOURCE_DEF(arrow_in, "Line/arrow_in.svg")                                  \
  RESOURCE_DEF(arrow_join, "Line/arrow_join.svg")                              \
  RESOURCE_DEF(arrow_left, "Line/arrow_left.svg")                              \
  RESOURCE_DEF(arrow_refresh, "Line/arrow_refresh.svg")                        \
  RESOURCE_DEF(arrow_right, "Line/arrow_right.svg")                            \
  RESOURCE_DEF(arrow_undo, "Line/arrow_undo.svg")                              \
  RESOURCE_DEF(asterisk_orange, "asterisk_orange.png")                         \
  RESOURCE_DEF(bookmark, "Line/bookmark.svg")                                  \
  RESOURCE_DEF(bug, "Line/bug.svg")                                            \
  RESOURCE_DEF(chart_curve, "Line/chart_curve.svg")                            \
  RESOURCE_DEF(cog, "Line/cog.svg")                                            \
  RESOURCE_DEF(columns, "Line/columns.svg")                                    \
  RESOURCE_DEF(color_wheel, "Line/color_wheel.svg")                            \
  RESOURCE_DEF(copy, "Line/copy.svg")                                          \
  RESOURCE_DEF(connect, "Line/connect.svg")                                    \
  RESOURCE_DEF(control_cursor_blue, "control_cursor_blue.png")                 \
  RESOURCE_DEF(control_end_blue, "control_end_blue.png")                       \
  RESOURCE_DEF(control_play_blue, "control_play_blue.png")                     \
  RESOURCE_DEF(control_nan_blue, "control_nan_blue.png")                       \
  RESOURCE_DEF(control_reverse_blue, "control_reverse_blue.png")               \
  RESOURCE_DEF(control_reverse_cursor_blue, "control_reverse_cursor_blue.png") \
  RESOURCE_DEF(control_reverse_nan_blue, "control_reverse_nan_blue.png")       \
  RESOURCE_DEF(control_reverse_sample_blue, "control_reverse_sample_blue.png") \
  RESOURCE_DEF(control_sample_blue, "control_sample_blue.png")                 \
  RESOURCE_DEF(control_start_blue, "control_start_blue.png")                   \
  RESOURCE_DEF(cross, "Line/cross.svg")                                        \
  RESOURCE_DEF(checkerboard, "checkerboard.png")                               \
  RESOURCE_DEF(del, "Line/del.svg")                                            \
  RESOURCE_DEF(disconnect, "Line/disconnect.svg")                              \
  RESOURCE_DEF(draw_vertex, "Line/draw_vertex.svg")                            \
  RESOURCE_DEF(find, "Line/find.svg")                                          \
  RESOURCE_DEF(filter, "Line/filter.svg")                                      \
  RESOURCE_DEF(arrow_out, "Line/arrow_out.svg")                                \
  RESOURCE_DEF(flag_green, "flag_green.png")                                   \
  RESOURCE_DEF(flip_y, "Line/flip_y.svg")                                      \
  RESOURCE_DEF(folder, "Line/folder.svg")                                      \
  RESOURCE_DEF(folder_page_white, "Line/folder_page_white.svg")                \
  RESOURCE_DEF(help, "Line/help.svg")                                          \
  RESOURCE_DEF(hourglass, "Line/hourglass.svg")                                \
  RESOURCE_DEF(house, "Line/house.svg")                                        \
  RESOURCE_DEF(information, "Line/information.svg")                            \
  RESOURCE_DEF(link, "Line/link.svg")                                          \
  RESOURCE_DEF(page_go, "Line/page_go.svg")                                    \
  RESOURCE_DEF(page_white_code, "Line/page_white_code.svg")                    \
  RESOURCE_DEF(page_white_database, "Line/page_white_database.svg")            \
  RESOURCE_DEF(page_white_delete, "Line/page_white_delete.svg")                \
  RESOURCE_DEF(page_white_edit, "Line/page_white_edit.svg")                    \
  RESOURCE_DEF(page_white_stack, "Line/page_white_stack.svg")                  \
  RESOURCE_DEF(pixel_history, "Line/pixel_history.svg")                        \
  RESOURCE_DEF(plugin, "Line/plugin.svg")                                      \
  RESOURCE_DEF(plugin_add, "Line/plugin_add.svg")                              \
  RESOURCE_DEF(save, "Line/save.svg")                                          \
  RESOURCE_DEF(text_add, "Line/text_add.svg")                                  \
  RESOURCE_DEF(tick, "Line/tick.svg")                                          \
  RESOURCE_DEF(time, "Line/time.svg")                                          \
  RESOURCE_DEF(upfolder, "Line/upfolder.svg")                                  \
  RESOURCE_DEF(update, "Line/update.svg")                                      \
  RESOURCE_DEF(wand, "Line/wand.svg")                                          \
  RESOURCE_DEF(wireframe_mesh, "wireframe_mesh.png")                           \
  RESOURCE_DEF(wrench, "Line/wrench.svg")                                      \
  RESOURCE_DEF(zoom, "Line/zoom.svg")                                          \
  RESOURCE_DEF(topo_linelist, "topologies/topo_linelist.svg")                  \
  RESOURCE_DEF(topo_linelist_adj, "topologies/topo_linelist_adj.svg")          \
  RESOURCE_DEF(topo_linestrip, "topologies/topo_linestrip.svg")                \
  RESOURCE_DEF(topo_linestrip_adj, "topologies/topo_linestrip_adj.svg")        \
  RESOURCE_DEF(topo_patch, "topologies/topo_patch.svg")                        \
  RESOURCE_DEF(topo_pointlist, "topologies/topo_pointlist.svg")                \
  RESOURCE_DEF(topo_trilist, "topologies/topo_trilist.svg")                    \
  RESOURCE_DEF(topo_trilist_adj, "topologies/topo_trilist_adj.svg")            \
  RESOURCE_DEF(topo_tristrip, "topologies/topo_tristrip.svg")                  \
  RESOURCE_DEF(topo_tristrip_adj, "topologies/topo_tristrip_adj.svg")          \
  RESOURCE_DEF(action, "action.png")                                           \
  RESOURCE_DEF(action_hover, "action_hover.png")                               \
  RESOURCE_DEF(bookmark_blue, "bookmark_blue.png")                             \
  RESOURCE_DEF(ue_save, "UE/save.svg")                                         \
  RESOURCE_DEF(ue_find, "UE/find.svg")                                         \
  RESOURCE_DEF(ue_filter, "UE/filter.svg")                                     \
  RESOURCE_DEF(ue_debug, "UE/debug.svg")

struct Resource
{
  QIcon icon;
  QPixmap pixmap;
  // Set for resources that live as vector artwork (the line-art icons). Those are drawn in one flat
  // colour and take that colour from the palette, so they cannot be rendered at startup - the
  // palette is not final until the style has been installed. They are built on first use instead,
  // and rebuilt whenever the style changes.
  QString vectorPath;
};

class Resources
{
public:
  static void Initialise();
  // Drop every built vector icon so the next use re-renders it against the current palette. Called
  // after the style is installed: these icons are the theme's text colour, so a cached copy from
  // the previous theme would be the wrong colour - invisible, in the light theme's case.
  static void RefreshIcons();
  // Install the line-art icons named by "rdIcon" dynamic properties on a widget tree. The .ui files
  // can only name the artwork, because a QIcon built straight from a resource path bakes in the
  // colour the artwork was drawn in - which reads on one theme and disappears on the other. This
  // resolves the names to palettised icons once the widgets exist, and the same call is repeated by
  // RefreshIcons() for windows that are already open when the style changes.
  static void ApplyLineIcons(QObject *root);
  ~Resources();

#undef RESOURCE_DEF
#define RESOURCE_DEF(name, filename)    \
  static const Resource &name()         \
  {                                     \
    return Render(resources->name##_data, 1); \
  }                                     \
  static const Resource &name##_2x()    \
  {                                     \
    return Render(resources->name##_2x_data, 2); \
  }
  RESOURCE_LIST()

private:
  static const Resource &Render(Resource &res, int scale);
  static void BuildVectorIcon(Resource &res);
#undef RESOURCE_DEF
#define RESOURCE_DEF(name, filename) Resource name##_data, name##_2x_data;

  struct ResourceSet
  {
    RESOURCE_LIST();
  };

  static ResourceSet *resources;
};

// helper forwarding structs

struct Pixmaps
{
#undef RESOURCE_DEF
#define RESOURCE_DEF(name, filename)               \
  static const QPixmap &name(int devicePixelRatio) \
  {                                                \
    if(devicePixelRatio == 1)                      \
      return Resources::name().pixmap;             \
    else                                           \
      return Resources::name##_2x().pixmap;        \
  }                                                \
  static const QPixmap &name(QWidget *widget)      \
  {                                                \
    return name(widget->devicePixelRatio());       \
  }
  RESOURCE_LIST()
};

struct Icons
{
#undef RESOURCE_DEF
#define RESOURCE_DEF(name, filename) \
  static const QIcon &name()         \
  {                                  \
    return Resources::name().icon;   \
  }
  RESOURCE_LIST()
};
