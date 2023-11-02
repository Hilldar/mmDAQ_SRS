//
//  CGuiUdpRawData.cpp
//  mmdaq
//
//  Created by Marcin Byszewski on 2/5/13.
//  Copyright (c) 2013 CERN - PH/ADE. All rights reserved.
//

#include "CGuiUdpRawData.h"
#include "CDetChip.h"
#include "CPublisher.h"


#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>
#include <TH2.h>
#include <TGListTree.h>
#include <TGraph.h>
#include <boost/foreach.hpp>


CGuiUdpRawData::CGuiUdpRawData(CUserInterface* gui, TGCompositeFrame* tabframe, unsigned num) :
CGuiTabContents(gui, tabframe, num), m_rootcanvas(0), m_canvas(0), m_graphs(), m_pads()
{
   m_rootcanvas = new TRootEmbeddedCanvas(Form("tabrootcanvas%d", num), tabframe, 580, 313);
   m_canvas = m_rootcanvas->GetCanvas();
   m_canvas->SetName(Form("tabcanvas%d", num));
   
   tabframe->AddFrame(m_rootcanvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY));
//   FreeClear(m_graphs);

}


CGuiUdpRawData::~CGuiUdpRawData()
{
   
   while(!m_graphs.empty()) {
      delete m_graphs.begin()->second;
      m_graphs.erase(m_graphs.begin());
   }
   
   delete m_canvas;
   delete m_rootcanvas;
}



std::string CGuiUdpRawData::prepare_canvas(size_t vecsize, size_t columns)
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


void CGuiUdpRawData::draw()
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

   //sort chips by id
   
   std::sort(all_chips.begin(), all_chips.end(), CDetChip::ChipIdLess() );
   
   int request_id = PUBLISHER_REQUEST_ID_UDP;
   

   std::map<int, TGraph*> graphs;
   const std::map<int, TH1*>& hists(m_gui->get_publisher()->get_published_data_udp(request_id));
   for (std::map<int, TH1*>::const_iterator ih = hists.begin() ; ih != hists.end(); ++ih) {
      if(ih->second) {
         graphs.insert(std::pair<int, TGraph*>(ih->first ,new TGraph(ih->second))); //can be null... todo:
      }
      
   }

   if(graphs.empty()) 
      return;
      
   while(!m_graphs.empty()) {
      delete m_graphs.begin()->second;
      m_graphs.erase(m_graphs.begin());
   }
   
   m_graphs = graphs;

   size_t vecsize = all_chips.size();
   float fontsize = 0.02 + 0.007 * (float)vecsize/2.0;
   int columns = (int) std::ceil(double(vecsize) / 4.0);
   std::string canvasname = prepare_canvas(vecsize, columns);
   
   int ii = 0;
   BOOST_FOREACH(const CDetChip* chip, all_chips) {
      m_canvas->cd(ii + 1);
      int chipId = chip->chipId();
      int chipNo = CApvEvent::chipNo_from_chipId(chipId);
      std::string chipname = chip->name();
      if(m_graphs.find(chipNo) != m_graphs.end()) {
         m_graphs[chipNo]->GetXaxis()->SetLabelSize(fontsize);
         m_graphs[chipNo]->GetYaxis()->SetLabelSize(fontsize);
         m_graphs[chipNo]->Draw("ALP");
         m_graphs[chipNo]->SetTitle(chipname.c_str());
      }
      
      
      std::stringstream out;
      out << canvasname << ii + 1;
      std::string padname = out.str();
      TPad* pad = (TPad*) m_canvas->GetPrimitive(padname.c_str());
      if (pad) pad->Modified();
      ++ii;
   }
   
   
   m_canvas->cd();
   m_canvas->Update();
}


void CGuiUdpRawData::update()
{
   m_canvas->Modified();
   m_canvas->Update();
}



