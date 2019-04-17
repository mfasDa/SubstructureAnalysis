#ifndef __GRAPHICSPAD_C__
#define __GRAPHICSPAD_C__

#include "../meta/root.C"
#include "../meta/root6tools.C"
#include "../helpers/graphics.C"

class GraphicsPad {
public:
    GraphicsPad(TVirtualPad *underlyingPad) : mPad(underlyingPad), mFrame(nullptr), mLegend(nullptr) {}
    ~GraphicsPad() = default;

    void Margins(double left, double right, double bottom, double top){
        if(left > 0) mPad->SetLeftMargin(left);
        if(right > 0) mPad->SetRightMargin(right);
        if(bottom > 0) mPad->SetBottomMargin(bottom);
        if(top > 0) mPad->SetTopMargin(top);
    }

    void Logy() {
        mPad->SetLogy();
    }

    void Frame(const std::string_view name, const std::string_view xtitle, const std::string_view ytitle, double xmin, double xmax, double ymin, double ymax) {
        mPad->cd();
        mFrame = new ROOT6tools::TAxisFrame(name.data(), xtitle.data(), ytitle.data(), xmin, xmax, ymin, ymax);
        mFrame->Draw("axis");
    }

    void FrameTextSize(double size) {
        mFrame->GetXaxis()->SetTitleSize(size);
        mFrame->GetYaxis()->SetTitleSize(size);
        mFrame->GetXaxis()->SetLabelSize(size);
        mFrame->GetYaxis()->SetLabelSize(size);
    }

    void FrameOffsets(double offsetX, double offsetY) {
        if(offsetX > 0.) mFrame->GetXaxis()->SetTitleOffset(offsetX); 
        if(offsetY > 0.) mFrame->GetYaxis()->SetTitleOffset(offsetY); 
    }

    void Legend(double xmin, double ymin, double xmax, double ymax) {
        mLegend = new ROOT6tools::TDefaultLegend(xmin, ymin, xmax, ymax);
        mPad->cd();
        mLegend->Draw();
    }

    void Label(double xmin, double ymin, double xmax, double ymax, const std::string_view text) {
        mPad->cd();
        (new ROOT6tools::TNDCLabel(xmin, ymin, xmax, ymax, text.data()))->Draw();
    }

    template<typename t>
    void Draw(t *object, Style &obstyle, const std::string_view title = "") {
        mPad->cd();
        obstyle.SetStyle<t>(*object);
        object->Draw("epsame");
        if(mLegend && title.length()){
            mLegend->AddEntry(object, title.data(), "lep");
        }
    }

private:
    TVirtualPad                     *mPad;
    ROOT6tools::TAxisFrame          *mFrame;
    ROOT6tools::TDefaultLegend      *mLegend;
};

#endif