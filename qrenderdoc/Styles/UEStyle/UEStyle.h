/******************************************************************************
 * The MIT License (MIT)
 ******************************************************************************/

#pragma once

#include "Styles/RDStyle/RDStyle.h"

// A Qt palette/style preset inspired by Unreal Editor's dark Slate/Starship palette.
// It deliberately reuses RenderDoc's existing geometry and painting code: this changes
// colours and conservative widget metrics only, not panel layouts or behaviour.
class UEStyle : public RDStyle
{
public:
  UEStyle();

  void polish(QApplication *app) override;
  void polish(QPalette &palette) override;
  QPalette standardPalette() const override;
  int pixelMetric(PixelMetric metric, const QStyleOption *option = NULL,
                  const QWidget *widget = NULL) const override;

private:
  static void applyPalette(QPalette &palette);
};
