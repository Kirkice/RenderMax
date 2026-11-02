/******************************************************************************
 * The MIT License (MIT)
 ******************************************************************************/

#pragma once

#include "Styles/RDStyle/RDStyle.h"

// Starship surfaces and Slate colours over the existing editor layout/interaction.
// Standard Qt controls use Resources/UE/starship.qss; custom controls use the palette.
class UEStyle : public RDStyle
{
public:
  UEStyle();

  void polish(QApplication *app) override;
  void polish(QPalette &palette) override;
  QPalette standardPalette() const override;
  int pixelMetric(PixelMetric metric, const QStyleOption *option = NULL,
                  const QWidget *widget = NULL) const override;

protected:
  const QBrush &outlineBrush(const QPalette &pal,
                             QPalette::ColorRole role = QPalette::Foreground) const override;
  void drawRoundedRectBorder(const QStyleOption *opt, QPainter *p, const QWidget *widget,
                             QPalette::ColorRole fillRole, bool shadow) const override;

private:
  static void applyPalette(QPalette &palette);
};
