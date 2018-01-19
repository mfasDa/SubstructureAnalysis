#ifndef __CLING__
#include <TString.h>
#include <TMath.h>
#endif

void CreateTexFile()
{

  const Int_t nRuns = 98;
  Int_t runs[nRuns] = {186320, 186319, 186208, 186205, 186167, 186165, 186164, 186163, 
                       185784, 185778, 185776, 185775, 185768, 185765, 185764, 185738, 
                       185701, 185699, 185698, 185697, 185687, 185589, 185588, 185583, 
                       185582, 185581, 185580, 185578, 185575, 185574, 185465, 185461, 
                       185375, 185371, 185363, 185362, 185361, 185360, 185359, 185356, 
                       185351, 185350, 185349, 185303, 185302, 185300, 185299, 185296, 
                       185293, 185292, 185291, 185289, 185288, 185284, 185282, 185221, 
                       185217, 185208, 185206, 185203, 185198, 185196, 185189, 185164, 
                       185160, 185134, 185132, 185127, 185126, 185116, 185031, 185029, 
                       184990, 184988, 184987, 184968, 184967, 184964, 184938, 184933, 
                       184928, 184786, 184784, 184687, 184682, 184678, 184673, 184371, 
                       184215, 184209, 184208, 184188, 184138, 184137, 184135, 184132, 
                       184127, 183916};

  TString name = "PhiCent10kINT7Run"; //186320.eps";

  FILE *latex_file;
  latex_file = fopen("phiDist.tex", "w");

  Int_t irun = 0;

  Int_t nSlides = TMath::FloorNint(nRuns / 12.);

  Printf("nSlides: %d", TMath::FloorNint(nRuns / 12.));

  for (Int_t k = 0; k < nSlides; k++)
  {

    Printf("k=%d irun: %d", k, irun);

    fprintf(latex_file, "\\begin{frame}\n");
    fprintf(latex_file, "\\frametitle{%s}\n", "$\\varphi$ of hybrid tracks");

    for (Int_t j = 0; j < 3; j++)
    {
      fprintf(latex_file, "\\begin{columns}\n");
      for (Int_t i = 0; i < 4; i++)
      {

        fprintf(latex_file, "\\begin{column}{0.25\\textwidth}\n");
        fprintf(latex_file, "   \\includegraphics[width=\\linewidth]{%d/%s%d.eps}\n", runs[irun], name.Data(), runs[irun]);
        fprintf(latex_file, "   \\end{column}\n");
        irun++;
      }
      fprintf(latex_file, "   \\end{columns}\n\n");
    }

    fprintf(latex_file, "\\end{frame}\n\n");
  }

  fprintf(latex_file, "\\begin{frame}\n");
  fprintf(latex_file, "\\frametitle{%s}\n", "\$\\varphi$ of hybrid tracks");
  fprintf(latex_file, "\\begin{columns}\n");

  Int_t runsLeft = nRuns - irun;

  for (Int_t j = 0; j < TMath::FloorNint(runsLeft / 4.); j++)
  {
    for (Int_t i = irun; i < 4; i++)
    {
      fprintf(latex_file, "\\begin{column}{0.25\\textwidth}\n");
      fprintf(latex_file, "   \\includegraphics[width=\\linewidth]{%d/%s%d.eps}\n", runs[irun], name.Data(), runs[irun]);
      fprintf(latex_file, "   \\end{column}\n");
      irun++;
    }
  }
  fprintf(latex_file, "   \\end{columns}\n\n");
  fprintf(latex_file, "\\end{frame}\n\n");

  fclose(latex_file);
}
