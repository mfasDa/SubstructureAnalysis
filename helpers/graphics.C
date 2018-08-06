#ifndef __GRAPHICS_C__
#define __GRAPHICS_C__

#ifndef __CLING__
#include <TH1.h>
#endif

struct Style {
  Color_t color;
  Style_t marker;

  template<typename t>
  void SetStyle(t &object) const {
    object.SetMarkerColor(color);
    object.SetMarkerStyle(marker);
    object.SetLineColor(color);  
  }
};

TH1 *make_frame(const char *name, const char *xtitle, const char *ytitle, double *range) {
  auto frame = new TH1F(name, "", 100, range[0], range[1]);
  frame->SetDirectory(nullptr);
  frame->SetStats(false);
  frame->SetXTitle(xtitle);
  frame->SetYTitle(ytitle);
  frame->GetYaxis()->SetRangeUser(range[2], range[3]);
  return frame;
}

template<typename t>
void InitWidget(t &widget) {
  widget.SetBorderSize(0);
  widget.SetFillStyle(0);
  widget.SetTextFont(42);
}
#endif