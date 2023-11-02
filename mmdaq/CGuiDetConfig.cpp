//
//  CGuiDetConfig.cpp
//  mmdaq
//
//  Created by Marcin Byszewski on 7/11/11.
//  Copyright 2011 CERN - PH/ADE. All rights reserved.
//

#include "CGuiDetConfig.h"
#include "CDetChip.h"
#include "CDetChamber.h"
#include "CDetReadout.h"
#include "CApvEvent.h"

#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>
#include <TH2.h>
#include <TGListTree.h>
#include <TGButtonGroup.h>

#include <boost/foreach.hpp>

CGuiDetConfig::CGuiDetConfig(CUserInterface* gui, TGCompositeFrame* tabframe, unsigned num) 
: CGuiTabContents(gui, tabframe, num) ,
m_rootcanvas(0), m_canvas(0), m_hists(), m_pads(), m_xscale(xscaleChannels)
{
   TGButtonGroup* br = new TGButtonGroup(m_tabframe,"X axes",kHorizontalFrame);
   TGRadioButton* fR[2] = {0,0};
   fR[0] = new TGRadioButton(br,new TGHotString("&Channels"));
   fR[1] = new TGRadioButton(br,new TGHotString("&Strips "));
   fR[0]->SetState(kButtonDown);
   br->Connect("Pressed(Int_t)", "CGuiDetConfig", this,
               "handle_set_xscale(Int_t)");
   m_tabframe->AddFrame(br, new TGLayoutHints(kLHintsExpandX));
   
   
   m_rootcanvas = new TRootEmbeddedCanvas(Form("tabrootcanvas%d", num), tabframe, 580, 313);
   m_canvas = m_rootcanvas->GetCanvas();
   m_canvas->SetName(Form("tabcanvas%d", num));
   
   tabframe->AddFrame(m_rootcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY));

}

CGuiDetConfig::~CGuiDetConfig() 
{
   FreeClear(m_hists);

   delete m_canvas;
   delete m_rootcanvas;
};


std::string CGuiDetConfig::prepare_canvas(size_t vecsize, size_t columns)
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

void CGuiDetConfig::handle_set_xscale(Int_t val)
{
   if(val == 1) {
      m_xscale = xscaleChannels;
   }
   else if (val == 2) {
      m_xscale = xscaleStrips;
   }
   draw();
}


void CGuiDetConfig::draw()
{
   switch (m_xscale) {
      case xscaleChannels:
         draw_channels();
         break;
      case xscaleStrips:
         draw_strips();
         break;
         
      default:
         break;
   }
}

///draw pedestals vs chip channels
void CGuiDetConfig::draw_channels()
{
   std::vector<TGListTreeItem*> all_items;
   m_gui->get_tree_checked_items(all_items);
   std::vector<const CDetChip*> all_chips;
   BOOST_FOREACH(TGListTreeItem* item, all_items) {
      const CDetElement* base = static_cast<const CDetElement*>(item->GetUserData());
      if (const CDetChip* ch = dynamic_cast<const CDetChip*>(base)) {
         all_chips.push_back(ch);
      }
   }
   
   if (all_chips.empty()) {

      return;
   }
   
   m_canvas->cd();
   FreeClear(m_hists);
   //get ped by chip
   
//   const std::vector<CDetChip*>	defined_chips = m_gui->get_config()->defined_chips();
   
   FreeClear(m_hists);
   
   BOOST_FOREACH(const CDetChip* chip, all_chips) {
      int chipId = chip->chipId();

      std::string chipname = chip->name();
      TH1F* histmean = new TH1F(std::string(chipname+"_pedmean").c_str(),
                      std::string(chipname+"_pedmean").c_str(),
                      128, 0, 128);//, 100, 0, 300);

      TH1F* histstdev = new TH1F(std::string(chipname+"_pedstdev").c_str(),
                                std::string(chipname+"_pedstdev").c_str(),
                                128, 0, 128);//, 100, 0, 30);
      
      //histmean->SetBit(TH1::kCanRebin);
      //histstdev->SetBit(TH1::kCanRebin);
      std::vector<double> pedmean = m_gui->get_config()->pedestal_chip_mean_vector(chipId);
      std::vector<double> pedstdev = m_gui->get_config()->pedestal_chip_stdev_vector(chipId);
      for (size_t ii = 0; ii < 128;++ii) {
         histmean->Fill(ii,  pedmean[ii]);
         histstdev->Fill(ii, pedstdev[ii]);
      }
      m_hists.push_back(histmean);
      m_hists.push_back(histstdev);      
   }
   
   
   /// draw
   std::string canvas_name = prepare_canvas(m_hists.size(), 2);
   float fontsize = 0.02 + 0.005 * (float)m_hists.size()/3.0;

   int ii = 0;
   for (std::vector<TH1*>::iterator ih = m_hists.begin() ; ih != m_hists.end(); ++ih, ++ii) {
      if(TH1F* h2 = dynamic_cast<TH1F*>(*ih)) {
         m_canvas->cd(ii + 1);
         h2->GetXaxis()->SetLabelSize(fontsize);
         h2->GetYaxis()->SetLabelSize(fontsize);
         h2->SetMarkerStyle(kFullDotMedium);
         h2->Draw("P0");
      }
   }
   
   m_canvas->cd();
   m_canvas->Update();

}


///draw pedestals vs strips
void CGuiDetConfig::draw_strips()
{
   
   m_canvas->cd();
   FreeClear(m_hists);
   
   //get selected items
   std::vector<TGListTreeItem*> all_items;
   m_gui->get_tree_checked_items(all_items);
//   std::vector<const CDetAbstractReadout *> selected_readouts;
   BOOST_FOREACH(TGListTreeItem* item, all_items) {
      const CDetElement* base = static_cast<const CDetElement*>(item->GetUserData());
      if (const CDetChamber* ch = dynamic_cast<const CDetChamber*>(base)) {
         const std::vector<CDetAbstractReadout*>& readouts = ch->get_readouts();
         
         BOOST_FOREACH(const CDetAbstractReadout* rd, readouts) {
            
            //      int chipId = chip->chipId();
            std::string rdname = ch->name() + "-" + rd->name();
            std::pair<double, double> strip_rng = rd->get_strip_range();
            TH1F* histmean = new TH1F(std::string(rdname+"_pedmean").c_str(),
                                      std::string(rdname+"_pedmean").c_str(),
                                      strip_rng.second - strip_rng.first + 1,
                                      strip_rng.first, strip_rng.second + 1);//,
                                     // 100, 0, 300);
            
            TH1F* histstdev = new TH1F(std::string(rdname+"_pedstdev").c_str(),
                                       std::string(rdname+"_pedstdev").c_str(),
                                       strip_rng.second - strip_rng.first + 1,
                                       strip_rng.first, strip_rng.second + 1);//,
                                       //100, 0, 30);
            
            //histmean->SetBit(TH1::kCanRebin);
            //histstdev->SetBit(TH1::kCanRebin);
            
            const std::vector<CDetChip*>& chips = rd->get_chips();
            BOOST_FOREACH(const CDetChip* ch, chips) {
               int apvid = ch->chipId();
               for (unsigned ii = 0; ii < 128; ++ii) {
                  int apvIdCh = CApvEvent::make_channelId(apvid, ii);
                  int strip =  m_gui->get_config()->strip_from_channel(apvIdCh) - 1;
                  if (strip >= 0 && strip >= strip_rng.first && strip <= strip_rng.second) {
                     double pedmean = 0.0;
                     double pedstdev = 0.0;
                     bool pedisfound = true;
                     pedisfound &= m_gui->get_config()->pedestal_mean(apvIdCh, pedmean);
                     pedisfound &= m_gui->get_config()->pedestal_stdev(apvIdCh, pedstdev);
                     histmean->Fill(strip,  pedmean);
                     histstdev->Fill(strip, pedstdev);
                  }
               }
            }
            
            m_hists.push_back(histmean);
            m_hists.push_back(histstdev);
         }
         
         
         
//         selected_readouts.insert(selected_readouts.end(), readouts.begin(), readouts.end());
      }
   }
      
   
//   if (selected_readouts.empty()) {
//      return;
//   }
   
  
//   FreeClear(m_hists);
   
   //moved rd loop from here
   
   
   /// draw
   std::string canvas_name = prepare_canvas(m_hists.size(), 2);
   float fontsize = 0.02 + 0.005 * (float)m_hists.size()/3.0;
   
   int ii = 0;
   for (std::vector<TH1*>::iterator ih = m_hists.begin() ; ih != m_hists.end(); ++ih, ++ii) {
      if(TH1F* h1 = dynamic_cast<TH1F*>(*ih)) {
         m_canvas->cd(ii + 1);
         h1->GetXaxis()->SetLabelSize(fontsize);
         h1->GetYaxis()->SetLabelSize(fontsize);
         h1->SetMarkerStyle(kFullDotMedium);
         h1->Draw("P0");
      }
   }
   
   m_canvas->cd();
   m_canvas->Update();
   
   
}



void CGuiDetConfig::update() 
{
   m_canvas->Modified();
   m_canvas->Update();
}

