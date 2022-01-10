#ifndef __CLING__
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>

#include "TTreeStream.h"
#endif

std::vector<std::string> tokenize(const std::string_view in, char delim) {
    std::vector<std::string> result;
    std::stringstream parser(in.data());
    std::string tmp;
    while(std::getline(parser, tmp, delim)) result.emplace_back(tmp);
    return result;
}

struct JetData {
    std::string jettype;
    std::string trigger;
    std::string sysvar;
    int R;

    void stream_tags(std::ostream &streamer) const {
        streamer << jettype << "_R" << std::setw(2) << std::setfill('0') << R << "_" << trigger << "_" << sysvar;
    }

    std::string build_dirname() const {
        std::stringstream dirnamebuilder;
        dirnamebuilder << "JetSpectrum_";
        stream_tags(dirnamebuilder);
        return dirnamebuilder.str();
    }

    std::string build_treename() const {
        std::stringstream treenamebuilder;
        treenamebuilder << "Trending_";
        stream_tags(treenamebuilder);
        return treenamebuilder.str();
    }

    void decodeJetDir(const std::string_view in) {
        JetData result;
        auto dirname_tokenized = tokenize(in, '_');
        jettype = dirname_tokenized[1];
        trigger = dirname_tokenized[3];
        sysvar = dirname_tokenized[4];
        auto rstring = dirname_tokenized[2];
        R = std::stoi(rstring.substr(1));
    } 
};

class ValueHandler {
    public:
        ValueHandler(TList *histos) : mHistos(histos) { }
        ~ValueHandler() = default;

        double ratioBinsTH1(const char *histname, int binNum, int binDen) {
            auto hist = getHisto<TH1>(histname);
            if(hist) {
                return hist->GetBinContent(binNum)/hist->GetBinContent(binDen);   
            }
            throw 1; 
        }

        double ratioBinsTH1(const char *histname, const char *binNum, const char * binDen) {
            auto hist = getHisto<TH1>(histname);
            if(hist) {
                return hist->GetBinContent(hist->GetXaxis()->FindBin(binNum))/hist->GetBinContent(hist->GetXaxis()->FindBin(binDen));   
            }
            throw 1; 
        }

        double binContentTH1(const char *histname, int binNumber) {
           auto hist = getHisto<TH1>(histname); 
           if(hist) {
               return hist->GetBinContent(binNumber);
           }
           throw 1;
        }

        double meanTH1(const char *histname) {
           auto hist = getHisto<TH1>(histname); 
           if(hist) {
               return hist->GetMean();
           }
           throw 1;
        }

        double rmsTH1(const char *histname) {
           auto hist = getHisto<TH1>(histname); 
           if(hist) {
               return hist->GetRMS();
           }
           throw 1;
        }

        double meanYTH2(const char *histname, double xmin, double xmax) {
           auto hist = getHisto<TH2>(histname); 
           if(hist) {
               int binmin = hist->GetXaxis()->FindBin(xmin + FLT_EPSILON),
                   binmax = hist->GetXaxis()->FindBin(xmax - FLT_EPSILON);
               std::unique_ptr<TH1> projection(hist->ProjectionY(Form("%s_projection", hist->GetName()), binmin, binmax));
               return projection->GetMean();
           }
           throw 1;
        }

        double rmsYTH2(const char *histname, double xmin, double xmax) {
           auto hist = getHisto<TH2>(histname); 
           if(hist) {
               int binmin = hist->GetXaxis()->FindBin(xmin + FLT_EPSILON),
                   binmax = hist->GetXaxis()->FindBin(xmax - FLT_EPSILON);
               std::unique_ptr<TH1> projection(hist->ProjectionY(Form("%s_projection", hist->GetName()), binmin, binmax));
               return projection->GetRMS();
           }
           throw 1;
        }

        double meanXTH2(const char *histname, double xmin, double xmax) {
           auto hist = getHisto<TH2>(histname); 
           if(hist) {
               int binmin = hist->GetYaxis()->FindBin(xmin + FLT_EPSILON),
                   binmax = hist->GetYaxis()->FindBin(xmax - FLT_EPSILON);
               std::unique_ptr<TH1> projection(hist->ProjectionX(Form("%s_projection", hist->GetName()), binmin, binmax));
               return projection->GetMean();
           }
           throw 1;
        }

        double rmsXTH2(const char *histname, double xmin, double xmax) {
           auto hist = getHisto<TH2>(histname); 
           if(hist) {
               int binmin = hist->GetYaxis()->FindBin(xmin + FLT_EPSILON),
                   binmax = hist->GetYaxis()->FindBin(xmax - FLT_EPSILON);
               std::unique_ptr<TH1> projection(hist->ProjectionX(Form("%s_projection", hist->GetName()), binmin, binmax));
               return projection->GetRMS();
           }
           throw 1;
        }


        double integral1D(const char *histname, double min, double max ) {
           auto hist = getHisto<TH1>(histname); 
           if(hist) {
               int binmin = hist->GetXaxis()->FindBin(min),
                   binmax = hist->GetXaxis()->FindBin(max);
               return hist->Integral(binmin, binmax);
           }
           throw 1;

        }

        double integralY(const char *histname, double ymin, double ymax, int binxmin, int binxmax) {
           auto hist = getHisto<TH2>(histname); 
           if(hist) {
               std::unique_ptr<TH1> projection(hist->ProjectionY(Form("%s_projectiony", hist->GetName()), binxmin, binxmax));
               int binmin = projection->GetXaxis()->FindBin(ymin),
                   binmax = projection->GetXaxis()->FindBin(ymax);
               return projection->Integral(binmin, binmax);
           }
           throw 1;
        }

        double integralX(const char *histname, double xmin, double xmax, int binymin, int binymax) {
           auto hist = getHisto<TH2>(histname); 
           if(hist) {
               std::unique_ptr<TH1> projection(hist->ProjectionX(Form("%s_projectionx", hist->GetName()), binymin, binymax));
               int binmin = projection->GetXaxis()->FindBin(xmin),
                   binmax = projection->GetXaxis()->FindBin(xmax);
               return projection->Integral(binmin, binmax);
           }
           throw 1;
        }

        template<typename T>
        T *getHisto(const char *name) {
            return dynamic_cast<T *>(mHistos->FindObject(name)); 
        }

    private:
        TList *mHistos;       
};

class TrendingHandler{
    public:
        TrendingHandler(const char *filename, int runnumber) :
            mReader(TFile::Open(filename, "READ")),
            mTreeStreamer("JetSpectraTrending.root"),
            mDirectories(),
            mRunNumber(runnumber)
        {
            mDirectories = find_directories(*mReader);
        }
        ~TrendingHandler() = default;

        void build() {
            for(const auto &jdat : mDirectories) {
                makeTrend(jdat);
            }
        }

        double getSpectrumAbsCountRange(ValueHandler &datahandler, int triggercluster, double ptmin, double ptmax) {
            return datahandler.integralY("hJetSpectrumAbs", ptmin + FLT_EPSILON, ptmax - FLT_EPSILON, triggercluster, triggercluster);
        }

        double getSpectrumWeightedCountRange(ValueHandler &datahandler, int triggercluster, double ptmin, double ptmax) {
            return datahandler.integralY("hJetSpectrum", ptmin + FLT_EPSILON, ptmax - FLT_EPSILON, triggercluster, triggercluster);
        }

        void makeTrend(const JetData &datadir){
            enum {
                kTriggerClusterANY=1,
                kTriggerClusterCENT=2,
                kTriggerClusterCENTNOTRD=3
            };
            mReader->cd(datadir.build_dirname().data());
            ValueHandler datahandler(static_cast<TKey *>(gDirectory->GetListOfKeys()->At(0))->ReadObject<TList>());
            double vtxeff = datahandler.ratioBinsTH1("fNormalisationHist", "Vertex reconstruction and quality", "Event selection"),
                   meanvz = datahandler.meanTH1("fHistZVertex"),
                   rmsvz = datahandler.rmsTH1("fHistZVertex"),
                   centnotrdcorrection = datahandler.ratioBinsTH1("hClusterCounterAbs", kTriggerClusterCENTNOTRD, kTriggerClusterCENT),
                   rawevents = datahandler.binContentTH1("hEventCounterAbs", 1),
                   correctedeventsANY = datahandler.binContentTH1("hClusterCounter", kTriggerClusterANY),
                   correctedeventsCENT = datahandler.binContentTH1("hClusterCounter", kTriggerClusterCENT),
                   correctedeventsCENTNOTRD = datahandler.binContentTH1("hClusterCounter", kTriggerClusterCENTNOTRD);
            //std::cout << "CENTNOTRD correction " << centnotrdcorrection << std::endl;

            const int kPtBins = 6;
            std::array<double, kPtBins> ptmin = {{10., 20., 30., 60., 100., 200.}},
                                        ptmax = {{20., 30., 40., 80., 140., 240.}};
            std::array<std::array<double, kPtBins>, 3> valuesAbs, valuesWeighted;
            std::array<double, kPtBins> meanNEF, meanZch, meanNch, meanZne, meanNne,
                                        rmsNEF, rmsZch, rmsNch, rmsZne, rmsNne;
            for(int ipt = 0; ipt < kPtBins; ipt++) {
                for(int icl = 0; icl < 3; icl++){
                    valuesAbs[icl][ipt] = getSpectrumAbsCountRange(datahandler, icl+1, ptmin[ipt], ptmax[ipt]);
                    valuesWeighted[icl][ipt] = getSpectrumWeightedCountRange(datahandler, icl+1, ptmin[ipt], ptmax[ipt]);
                }
                meanNEF[ipt] = datahandler.meanYTH2("hQANEFPt", ptmin[ipt], ptmax[ipt]);
                rmsNEF[ipt] = datahandler.rmsYTH2("hQANEFPt", ptmin[ipt], ptmax[ipt]);
                meanZch[ipt] = datahandler.meanYTH2("hQAZchPt", ptmin[ipt], ptmax[ipt]);
                rmsZch[ipt] = datahandler.rmsYTH2("hQAZchPt", ptmin[ipt], ptmax[ipt]);
                meanNch[ipt] = datahandler.meanYTH2("hQANChPt", ptmin[ipt], ptmax[ipt]);
                rmsNch[ipt] = datahandler.rmsYTH2("hQANChPt", ptmin[ipt], ptmax[ipt]);
                meanZne[ipt] = datahandler.meanYTH2("hQAZnePt", ptmin[ipt], ptmax[ipt]);
                rmsZne[ipt] = datahandler.meanYTH2("hQAZnePt", ptmin[ipt], ptmax[ipt]);
                meanNne[ipt] = datahandler.meanYTH2("hQANnePt", ptmin[ipt], ptmax[ipt]);
                rmsNEF[ipt] = datahandler.meanYTH2("hQANnePt", ptmin[ipt], ptmax[ipt]);
            }
        
            mTreeStreamer << datadir.build_treename().data() 
                          << "run=" << mRunNumber
                          << "vtxeff=" << vtxeff
                          << "events=" << rawevents
                          << "meanvz=" << meanvz
                          << "CENTNOTRDcorrection=" << centnotrdcorrection
                          << "LuminosityANY=" << correctedeventsANY 
                          << "LuminosityCENT=" << correctedeventsCENT 
                          << "LuminosityCENTNOTRD=" << correctedeventsCENTNOTRD 
                          << "AbsANY_10_20=" << valuesAbs[0][0]
                          << "AbsANY_20_30=" << valuesAbs[0][1]
                          << "AbsANY_30_40=" << valuesAbs[0][2]
                          << "AbsANY_60_80=" << valuesAbs[0][3]
                          << "AbsANY_100_140=" << valuesAbs[0][4]
                          << "AbsANY_200_240=" << valuesAbs[0][5]
                          << "AbsCENT_10_20=" << valuesAbs[1][0]
                          << "AbsCENT_20_30=" << valuesAbs[1][1]
                          << "AbsCENT_30_40=" << valuesAbs[1][2]
                          << "AbsCENT_60_80=" << valuesAbs[1][3]
                          << "AbsCENT_100_140=" << valuesAbs[1][4]
                          << "AbsCENT_200_240=" << valuesAbs[1][5]
                          << "AbsCENTNOTRD_10_20=" << valuesAbs[2][0]
                          << "AbsCENTNOTRD_20_30=" << valuesAbs[2][1]
                          << "AbsCENTNOTRD_30_40=" << valuesAbs[2][2]
                          << "AbsCENTNOTRD_60_80=" << valuesAbs[2][3]
                          << "AbsCENTNOTRD_100_140=" << valuesAbs[2][4]
                          << "AbsCENTNOTRD_200_240=" << valuesAbs[2][5]
                          << "WeightedANY_10_20=" << valuesWeighted[0][0]
                          << "WeightedANY_20_30=" << valuesWeighted[0][1]
                          << "WeightedANY_30_40=" << valuesWeighted[0][2]
                          << "WeightedANY_60_80=" << valuesWeighted[0][3]
                          << "WeightedANY_100_140=" << valuesWeighted[0][4]
                          << "WeightedANY_200_240=" << valuesWeighted[0][5]
                          << "WeightedCENT_10_20=" << valuesWeighted[1][0]
                          << "WeightedCENT_20_30=" << valuesWeighted[1][1]
                          << "WeightedCENT_30_40=" << valuesWeighted[1][2]
                          << "WeightedCENT_60_80=" << valuesWeighted[1][3]
                          << "WeightedCENT_100_140=" << valuesWeighted[1][4]
                          << "WeightedCENT_200_240=" << valuesWeighted[1][5]
                          << "WeightedCENTNOTRD_10_20=" << valuesWeighted[2][0]
                          << "WeightedCENTNOTRD_20_30=" << valuesWeighted[2][1]
                          << "WeightedCENTNOTRD_30_40=" << valuesWeighted[2][2]
                          << "WeightedCENTNOTRD_60_80=" << valuesWeighted[2][3]
                          << "WeightedCENTNOTRD_100_140=" << valuesWeighted[2][4]
                          << "WeightedCENTNOTRD_200_240=" << valuesWeighted[2][5]
                          << "MeanNEF_10_20=" << meanNEF[0]
                          << "MeanNEF_20_30=" << meanNEF[1]
                          << "MeanNEF_30_40=" << meanNEF[2]
                          << "MeanNEF_60_80=" << meanNEF[3]
                          << "MeanNEF_100_140=" << meanNEF[4]
                          << "MeanNEF_200_240=" << meanNEF[5]
                          << "RMSNEF_10_20=" << rmsNEF[0]
                          << "RMSNEF_20_30=" << rmsNEF[1]
                          << "RMSNEF_30_40=" << rmsNEF[2]
                          << "RMSNEF_60_80=" << rmsNEF[3]
                          << "RMSNEF_100_140=" << rmsNEF[4]
                          << "RMSNEF_200_240=" << rmsNEF[5]
                          << "MeanZch_10_20=" << meanZch[0]
                          << "MeanZch_20_30=" << meanZch[1]
                          << "MeanZch_30_40=" << meanZch[2]
                          << "MeanZch_60_80=" << meanZch[3]
                          << "MeanZch_100_140=" << meanZch[4]
                          << "MeanZch_200_240=" << meanZch[5]
                          << "RMSZch_10_20=" << rmsZch[0]
                          << "RMSZch_20_30=" << rmsZch[1]
                          << "RMSZch_30_40=" << rmsZch[2]
                          << "RMSZch_60_80=" << rmsZch[3]
                          << "RMSZch_100_140=" << rmsZch[4]
                          << "RMSZch_200_240=" << rmsZch[5]
                          << "MeanNch_10_20=" << meanNch[0]
                          << "MeanNch_20_30=" << meanNch[1]
                          << "MeanNch_30_40=" << meanNch[2]
                          << "MeanNch_60_80=" << meanNch[3]
                          << "MeanNch_100_140=" << meanNch[4]
                          << "MeanNch_200_240=" << meanNch[5]
                          << "RMSNch_10_20=" << rmsNch[0]
                          << "RMSNch_20_30=" << rmsNch[1]
                          << "RMSNch_30_40=" << rmsNch[2]
                          << "RMSNch_60_80=" << rmsNch[3]
                          << "RMSNch_100_140=" << rmsNch[4]
                          << "RMSNch_200_240=" << rmsNch[5]
                          << "MeanZne_10_20=" << meanZne[0]
                          << "MeanZne_20_30=" << meanZne[1]
                          << "MeanZne_30_40=" << meanZne[2]
                          << "MeanZne_60_80=" << meanZne[3]
                          << "MeanZne_100_140=" << meanZne[4]
                          << "MeanZne_200_240=" << meanZne[5]
                          << "RMSZne_10_20=" << rmsZne[0]
                          << "RMSZne_20_30=" << rmsZne[1]
                          << "RMSZne_30_40=" << rmsZne[2]
                          << "RMSZne_60_80=" << rmsZne[3]
                          << "RMSZne_100_140=" << rmsZne[4]
                          << "RMSZne_200_240=" << rmsZne[5]
                          << "MeanNne_10_20=" << meanNne[0]
                          << "MeanNne_20_30=" << meanNne[1]
                          << "MeanNne_30_40=" << meanNne[2]
                          << "MeanNne_60_80=" << meanNne[3]
                          << "MeanNne_100_140=" << meanNne[4]
                          << "MeanNne_200_240=" << meanNne[5]
                          << "RMSNne_10_20=" << rmsNne[0]
                          << "RMSNne_20_30=" << rmsNne[1]
                          << "RMSNne_30_40=" << rmsNne[2]
                          << "RMSNne_60_80=" << rmsNne[3]
                          << "RMSNne_100_140=" << rmsNne[4]
                          << "RMSNne_200_240=" << rmsNne[5]
                          << "\n";
        }

    protected:

        std::vector<JetData> find_directories(TFile &reader) {
           std::vector<JetData> result; 
           for(auto key : TRangeDynCast<TKey>(reader.GetListOfKeys())) {
               std::string_view keyname(key->GetName());
               if(keyname.find("JetSpectrum") == std::string::npos) continue;
               JetData next;
               next.decodeJetDir(keyname);
               result.emplace_back(next);
           }
           return result;
        }

    private:
        std::unique_ptr<TFile> mReader;
        TTreeSRedirector mTreeStreamer;
        std::vector<JetData> mDirectories;
        int mRunNumber;
};



void trendingExtractorData(const char *data, int runnumber){
    TrendingHandler trender(data, runnumber);
    trender.build();
}