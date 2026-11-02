/******************************************************************************
 * The MIT License (MIT)
 ******************************************************************************/

#include "UEStyle.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QStyleOption>

UEStyle::UEStyle() : RDStyle(RDStyle::Dark)
{
}

void UEStyle::applyPalette(QPalette &palette)
{
  // sRGB defaults from SlateCore/Private/Styling/StyleColors.cpp.
  const QColor window("#242424");
  const QColor base("#0F0F0F");
  const QColor button("#383838");
  const QColor text("#C0C0C0");
  const QColor muted("#808080");
  const QColor accent("#0070E0");

  palette = QPalette(text, window, QColor("#575757"), base, button, text, base);
  palette.setColor(QPalette::Window, window);
  palette.setColor(QPalette::Base, base);
  palette.setColor(QPalette::AlternateBase, QColor("#1A1A1A"));
  palette.setColor(QPalette::Button, button);
  palette.setColor(QPalette::ButtonText, text);
  palette.setColor(QPalette::Text, text);
  palette.setColor(QPalette::WindowText, text);
  palette.setColor(QPalette::Mid, button);
  palette.setColor(QPalette::Dark, base);
  palette.setColor(QPalette::Light, QColor("#575757"));
  palette.setColor(QPalette::Midlight, QColor("#4C4C4C"));
  palette.setColor(QPalette::Shadow, Qt::black);
  palette.setColor(QPalette::Highlight, accent);
  palette.setColor(QPalette::HighlightedText, Qt::white);
  palette.setColor(QPalette::Link, accent);
  palette.setColor(QPalette::LinkVisited, QColor("#A139BF"));
  palette.setColor(QPalette::BrightText, Qt::white);
  palette.setColor(QPalette::ToolTipBase, button);
  palette.setColor(QPalette::ToolTipText, text);
  palette.setColor(QPalette::Inactive, QPalette::Highlight, QColor("#3F3F3F"));
  palette.setColor(QPalette::Inactive, QPalette::HighlightedText, text);
  palette.setColor(QPalette::Disabled, QPalette::WindowText, muted);
  palette.setColor(QPalette::Disabled, QPalette::Text, muted);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, muted);
  palette.setColor(QPalette::Disabled, QPalette::Highlight, button);
  palette.setColor(QPalette::Disabled, QPalette::Base, QColor("#1A1A1A"));
  palette.setColor(QPalette::Disabled, QPalette::Button, window);
}

void UEStyle::polish(QPalette &palette)
{
  applyPalette(palette);
}

QPalette UEStyle::standardPalette() const
{
  QPalette palette;
  applyPalette(palette);
  return palette;
}

void UEStyle::polish(QApplication *app)
{
  RDStyle::polish(app);
  QFile stylesheet(QStringLiteral(":/UE/starship.qss"));
  if(stylesheet.open(QIODevice::ReadOnly))
    app->setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
  else
    qWarning() << "Could not load UE/Starship stylesheet";
}

const QBrush &UEStyle::outlineBrush(const QPalette &pal, QPalette::ColorRole role) const
{
  // Neutral separators rather than RDStyle's bright foreground-coloured frames.
  return pal.brush(QPalette::Mid);
}

void UEStyle::drawRoundedRectBorder(const QStyleOption *opt, QPainter *p,
                                    const QWidget *widget, QPalette::ColorRole fillRole,
                                    bool shadow) const
{
  // RDStyle's fallback painter outlines focused controls in blue. This helper only
  // consumes the base QStyleOption fields, so preserve all geometry/state except
  // that decorative focus outline; QSS supplies neutral keyboard-focus feedback.
  QStyleOption neutral = *opt;
  neutral.state &= ~State_HasFocus;
  RDStyle::drawRoundedRectBorder(&neutral, p, widget, fillRole, false);
}

int UEStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                         const QWidget *widget) const
{
  return RDStyle::pixelMetric(metric, option, widget);
}
