//
//  CGuiTabContents.cpp
//  mmdaq
//
//  Created by Marcin Byszewski on 7/7/11.
//  Copyright 2011 CERN - PH/ADE. All rights reserved.
//

#include "CGuiTabContents.h"
#include "CUserInterface.h"
#include "CPublisher.h"
#include "CDetChamber.h"

#include "TGFrame.h"
#include <TRootEmbeddedCanvas.h>
#include "TCanvas.h"
#include "TGButton.h"
#include "TGComboBox.h"
#include "TGLayout.h"
#include "TH2.h"
#include "TGLabel.h"

#include <string>
#include <vector>
#include <sstream>

//#define DETECTOR_SIZE_X_MM 90
//#define DETECTOR_SIZE_Y_MM 90
//#define MAX_NUM_READOUTS 3
//#define READOUT_TYPE_X 0
//#define READOUT_TYPE_XY 1
//#define READOUT_TYPE_XUV 2

CGuiTabContents::~CGuiTabContents()  {}




CGuiHitMap::CGuiHitMap(CUserInterface* gui, TGCompositeFrame* tabframe, unsigned num)
: CGuiTabContents(gui, tabframe, num), m_readout_type(0), m_readout_labels(), m_readout_source(), m_hist2d(), m_pads()
{
   
   
   TGHorizontalFrame* top_h_frame = new TGHorizontalFrame(tabframe, 584, 26, kHorizontalFrame);
   m_readout_type = new TGComboBox(top_h_frame);
   
   const std::vector<CDetChamber*>& chambers = m_gui->get_config()->defined_chambers();
   for (std::vector<CDetChamber*>::const_iterator icham = chambers.begin(); icham != chambers.end(); ++icham) {
      m_readout_type->AddEntry((*icham)->name().c_str(), (*icham)->get_id() );
   }
   
   
   m_readout_type->Select(0);
   m_readout_type->Resize(60, 22);
   m_readout_type->Connect("Selected(Int_t)", "CGuiHitMap", this, "gui_handle_select_chamber(Int_t)");
   top_h_frame->AddFrame(m_readout_type, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
   
   tabframe->AddFrame(top_h_frame, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX, 0, 0, 0, 0));
   
   
   m_rootcanvas = new TRootEmbeddedCanvas(Form("tabrootcanvas%d", num), tabframe, 580, 313);
   
   //Int_t wfRootEmbeddedCanvas = frootembeddedcanvas[i]->GetCanvasWindowId();
   //fcanvas[i] = new TCanvas(Form("canvas%d",i), 10, 10, wfRootEmbeddedCanvas);
   //frootembeddedcanvas[i]->AdoptCanvas(fcanvas[i]);
   
   m_canvas = m_rootcanvas->GetCanvas();
   m_canvas->SetName(Form("tabcanvas%d", num));
   
   tabframe->AddFrame(m_rootcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY));
   
   
};

CGuiHitMap::~CGuiHitMap(){
   FreeClear(m_hist2d);
   delete m_canvas;
   delete m_rootcanvas;
   delete m_readout_type;
   FreeClear(m_readout_source);
   FreeClear(m_readout_labels);
}

void CGuiHitMap::gui_handle_select_chamber(Int_t selection)
{   
 
   update(); //?
}

std::string CGuiHitMap::prepare_canvas(int icanvas, size_t vecsize, size_t columns)
{
   if (columns < 1) columns = 1;
   //prepare canvas
   
   if (m_pads.size() != vecsize) {
      m_canvas->Clear();
      m_pads.clear();
      
      if (vecsize > 1) {
         int n = (int)std::ceil((double)(vecsize) / (double)columns);
         m_canvas->Divide( static_cast<int>(columns), n);
      }
      else {
         m_canvas->Divide(1, 1);
      }
   }
   std::stringstream canvasname;
   canvasname << m_canvas->GetName() << "_";
   return canvasname.str();
}

/**
 get data from publisher - a 2d histogram of the selected chamber to display , or all ?
 */
void CGuiHitMap::draw()
{
   m_canvas->cd();
   int request_id = PUBLISHER_REQUEST_ID_2DHITMAP; // will use 1d data with strip, charge info divided into chambers
   const std::vector<TH1*> hists (m_gui->get_publisher()->get_published_data(request_id));
   
//   std::cout << "CGuiHitMap::draw() " << hists.size() << std::endl;
   
   
   int jj = 0;
   FreeClear(m_hist2d);
   m_hist2d.reserve(hists.size());
   for (std::vector<TH1*> :: const_iterator ih = hists.begin() ; ih != hists.end(); ++ih, ++jj) {
      TH2F* hist = dynamic_cast<TH2F*>(*ih);
      m_hist2d.push_back( (TH2F*) hist->Clone("hist_hitmap") );
      m_hist2d.back()->SetDirectory(0);
   }
   
   //plot data
   size_t vecsize = m_hist2d.size();
   if (!vecsize) {
      return;
   }
   int columns = 2;//(int) std::ceil(vecsize / 4);
   
   std::string canvasname = prepare_canvas(m_number, vecsize, columns);
   
   int ii = 0;
   for (std::vector<TH2F*>::iterator ih = m_hist2d.begin() ; ih !=m_hist2d.end(); ++ih, ++ii) {
      m_canvas->cd(ii + 1);
      m_canvas->SetGrayscale();
      (*ih)->GetXaxis()->SetLabelSize(0.05);
      (*ih)->GetYaxis()->SetLabelSize(0.05);
//      (*ih)->Draw();
      (*ih)->Draw("COLZ");
      
      m_canvas->cd(ii + 1);
      std::stringstream out;
      out << canvasname << ii + 1;
      std::string padname = out.str();
      TPad* pad = (TPad*) m_canvas->GetPrimitive(padname.c_str());
      if (pad) {
         //         if (ii % 2) pad->SetGrid(1, 1);
         pad->Modified();
      }
   }
   
   m_canvas->cd();
   m_canvas->Update();
   
   
   
   //make dropbox with : map type: x, x-y, x-u-v
   //make dropbox with chamber names to select them as coordinate sources
   
   
   
   //todo: decide which 'chamber' is x, u,v
   //todo: get data out of th1 q_vs_strip (i%0)?
   //todo: plot strips x, u, v
   
   
   
   
}



void CGuiHitMap::update() {
   m_canvas->Modified();
   m_canvas->Update();
}