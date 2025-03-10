#include "TCanvas.h"
#include "TLatex.h"
#include "TString.h"
#include "TH1D.h"
#include "TLegend.h"
#include "TROOT.h"
#include "THnSparse.h"
#include "TFile.h"
#include "TH1F.h"
#include "TStyle.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "runlist.h"


using json = nlohmann::json;

const std::string DIRECTORY = "/Users/sangwoo/cernbox/workspace/f0_draw/pass4_small/";
const std::string results = "/AnalysisResults.root";

void InvMassRot();

int main() {
	InvMassRot();
}

void InvMassRot() {
  auto Runs = runlist();

  for (auto &[runName, runInfos] : Runs.items()) {
      // runs.push_back(DIRECTORY + runName); // push_back: append
    // std::cout << "Run number: " << runName << ", Run info: " << runInfos << std::endl;
    std::cout << "Run number: " << runName << std::endl;

    //configuration
    const std::string colName = runInfos["colName"];
    const int nmult = runInfos["nmult"];
    const int npt = runInfos["npt"];
    const double mass_min = runInfos["mass"][0];
    const double mass_max = runInfos["mass"][1];
    
    //normalization for rotational background
    const double norm_min = 1.85;
    const double norm_max = 2.0;

    // TFile* fin = new TFile("/Volumes/Seul/HP_results/239395.root");
    TFile *fin = new TFile((DIRECTORY + runName + results).c_str(), "read");

    THnSparse* hInvMassUS = (THnSparse*)fin->Get("lf-f0980pbpbanalysis/hInvMass_f0980_US_EPA");
    THnSparse* hInvMassUSRot = (THnSparse*)fin->Get("lf-f0980pbpbanalysis/hInvMass_f0980_USRot_EPA");

    //	centralitys
    std::vector<int> m_min = runInfos["m_min"];
    std::vector<int> m_max = runInfos["m_max"];
    //	pt
    std::vector<double> p_min = runInfos["p_min"];
    std::vector<double> p_max = runInfos["p_max"];

    TH1D* hProjInvMassUS[nmult][npt];
    TH1D* hProjInvMassUSRot[nmult][npt];
    TH1D* hProjInvMassSubRot[nmult][npt];
    TH1D* hProjInvMassSub_D[nmult][npt];
    TH1D* hProjInvMassSub_S[nmult][npt];

    TCanvas *c1 = new TCanvas(Form("c1_%s",runName.c_str()), "Invariant mass distribution Rotation", 1400, 600);
    c1->Divide(2, 1, 0.001, 0.001);

    TFile *fout = new TFile((DIRECTORY + runName + "/"+ "InvMassOutRot.root").c_str(),"recreate");
    TDirectory *dir_US_EPA = fout -> mkdir("f0980_US_EPA");
    TDirectory *dir_USRot_EPA = fout -> mkdir("f0980_USRot_EPA");
    TDirectory *dir_SUBRot_EPA = fout -> mkdir("f0980_SUBRot_EPA");

    //	centrality = j
    for (int j = 0; j < nmult; j++) {
      hInvMassUS->GetAxis(2)->SetRangeUser(m_min[j], m_max[j] - 0.001);
      hInvMassUSRot->GetAxis(2)->SetRangeUser(m_min[j], m_max[j] - 0.001);

      for (int k = 0; k < npt; k++) {
        hInvMassUS->GetAxis(1)->SetRangeUser(p_min[k], p_max[k] - 0.001);
        hInvMassUSRot->GetAxis(1)->SetRangeUser(p_min[k], p_max[k] - 0.001);

        hProjInvMassUS[j][k] = hInvMassUS->Projection(0, "e");
        hProjInvMassUS[j][k]->SetName(Form("hProjInvMassUS_%d_%d", j, k));
        hProjInvMassUSRot[j][k] = hInvMassUSRot->Projection(0, "e");
        hProjInvMassUSRot[j][k]->SetName(Form("hProjInvMassUSRot_%d_%d", j, k));

        // rotational method
        double US_integral = hProjInvMassUS[j][k] -> Integral(hProjInvMassUS[j][k] -> FindBin(norm_min), hProjInvMassUS[j][k] -> FindBin(norm_max));
        double USRot_integral = hProjInvMassUSRot[j][k] -> Integral(hProjInvMassUSRot[j][k] -> FindBin(norm_min), hProjInvMassUSRot[j][k] -> FindBin(norm_max));
        double norm_factor = (USRot_integral > 0) ? (US_integral / USRot_integral) : 1.0;

        hProjInvMassUSRot[j][k]->Scale(norm_factor);

        hProjInvMassSubRot[j][k] = (TH1D *)hProjInvMassUS[j][k]->Clone();
        hProjInvMassSubRot[j][k]->SetName(Form("hProjInvMassSubRot_%d_%d", j, k));
        hProjInvMassSubRot[j][k]->Add(hProjInvMassUSRot[j][k], -1.0);

        for (int q = 0; q < hProjInvMassSubRot[j][k]->GetNbinsX(); q++) {
          auto binContent = hProjInvMassSubRot[j][k]->GetBinContent(q + 1);
          auto errUS = hProjInvMassUS[j][k]->GetBinError(q + 1);
          auto errUSRot = hProjInvMassUSRot[j][k]->GetBinError(q + 1);
          auto binError = sqrt(errUS * errUS + errUSRot * errUSRot);
          hProjInvMassSubRot[j][k]->SetBinError(q + 1, binError);
        }

        dir_USRot_EPA -> cd();
        hProjInvMassUSRot[j][k] -> Write();
        dir_SUBRot_EPA -> cd();
        hProjInvMassSubRot[j][k] -> Write();

        //	Draw
        if (j == 8 && k == 3) {
          c1->cd(1);
          gStyle->SetOptTitle(0);
          gStyle->SetOptStat(0);

          hProjInvMassSub_D[j][k] = (TH1D *)hProjInvMassUS[j][k]->Clone();

          hProjInvMassSub_D[j][k]->Draw("");
          hProjInvMassSub_D[j][k]->SetMarkerColor(kBlack);
          hProjInvMassSub_D[j][k]->SetMarkerStyle(4);
          hProjInvMassSub_D[j][k]->SetMarkerSize(0.5);
          hProjInvMassSub_D[j][k]->GetXaxis()->SetTitle("M_{#pi#pi} (GeV/c^{2})");
          hProjInvMassSub_D[j][k]->GetXaxis()->SetRangeUser(mass_min, mass_max);
          hProjInvMassSub_D[j][k]->GetYaxis()->SetTitle("Counts");
          hProjInvMassUSRot[j][k]->Draw("same");
          hProjInvMassUSRot[j][k]->SetMarkerColor(kRed);
          hProjInvMassUSRot[j][k]->SetMarkerStyle(4);
          hProjInvMassUSRot[j][k]->SetMarkerSize(0.5);

          TLegend *leg = new TLegend(0.48, 0.13, 0.88, 0.45);
          leg->SetTextSize(0.038);
          leg->SetLineWidth(0.0);
          leg->SetFillStyle(0);
          leg->AddEntry((TObject *)0, "LHC23zzh_pass4_small", "");
          leg->AddEntry((TObject *)0, Form("train number : %s", runName.c_str()), "");
          leg->AddEntry((TObject *)0, Form("FT0C %d#font[122]{-}%d %%", m_min[j], m_max[j]), "");
          leg->AddEntry((TObject *)0, "PbPb 5.36 TeV, |#it{y}| < 0.5", "");
          // leg->AddEntry( (TObject*)0, Form("#it{p}_{T,lead} > 5 GeV/#it{c},
          // %s",trnsName[r]), "");
          leg->AddEntry((TObject *)0, Form("%.1lf < #it{p}_{T} < %.1lf GeV/#it{c}", p_min[k], p_max[k]), "");
          leg->AddEntry(hProjInvMassSub_D[j][k], "Unlike-sign pairs", "p");
          leg->AddEntry(hProjInvMassUSRot[j][k], "UnLike-sign rotation pairs", "p");
          leg->Draw();
        }

        if (j == 8 && k == 3) {
          c1->cd(2);
          hProjInvMassSub_S[j][k] = (TH1D *)hProjInvMassSubRot[j][k]->Clone();
          hProjInvMassSub_S[j][k]->Draw("l");

          hProjInvMassSub_S[j][k]->GetXaxis()->SetTitle("M_{#pi#pi} (GeV/c^{2})");
          hProjInvMassSub_S[j][k]->GetXaxis()->SetRangeUser(mass_min, mass_max);
          hProjInvMassSub_S[j][k]->GetYaxis()->SetTitle("Counts");
          hProjInvMassSub_S[j][k]->SetMarkerColor(kBlue);
          hProjInvMassSub_S[j][k]->SetMarkerStyle(4);
          hProjInvMassSub_S[j][k]->SetMarkerSize(0.8);

        }
      }
    }
    // fout->Close();
    c1->SaveAs((DIRECTORY + runName + "/" + "plot_c/bg_subtraction_Rot.pdf").c_str());

  }
}