//
//  CGuiTabContents.h
//  mmdaq
//
//  Created by Marcin Byszewski on 7/7/11.
//  Copyright 2011 CERN - PH/ADE. All rights reserved.
//

#ifndef CGuiTabContents_h
#define CGuiTabContents_h

#include "CUserInterface.h"

#include <vector>

class CUserInterface;

class TGCompositeFrame;
class TRootEmbeddedCanvas;
class TCanvas;
class TGTextButton;
class TGComboBox;
class TH1;
class TH2F;
class TGLabel;
class TPad;

class CGuiTabContents {
protected:
   CUserInterface*   m_gui;
   unsigned          m_number;
   TGCompositeFrame* m_tabframe;
   
public:
//   CGuiTabContents() {};
   CGuiTabContents( CUserInterface* gui, TGCompositeFrame* tabframe, unsigned num)
   : m_gui(gui), m_number(num), m_tabframe(tabframe) { };
   virtual ~CGuiTabContents() = 0;
   virtual void draw()   = 0;
   virtual void update() = 0;
   virtual void handle_run_state(bool) = 0;
   unsigned get_number() const { return m_number;}
   TGCompositeFrame* get_frame() { return m_tabframe;}
   std::vector<std::vector<TH1*> >& gui_histograms() const { return m_gui->m_histograms;};
};


class CGuiHitMap : public CGuiTabContents {
   TGComboBox*                m_readout_type;
   std::vector <TGLabel*>     m_readout_labels;  // 1, 2 or tree sources
   std::vector <TGComboBox*>  m_readout_source;  // 1, 2 or tree sources
   
   TRootEmbeddedCanvas* m_rootcanvas;
   TCanvas* m_canvas;
   std::vector<TH2F*>   m_hist2d;
   std::vector<TPad*> m_pads;

   
public:
   CGuiHitMap(CUserInterface* gui, TGCompositeFrame* tabframe, unsigned num);
   virtual ~CGuiHitMap();
   virtual void draw();
   virtual void update() ;
   virtual void handle_run_state(bool) {};
   void gui_handle_select_chamber(Int_t selection);
//   void gui_handle_readout_source_change(int);
   std::string prepare_canvas(int icanvas, size_t vecsize, size_t columns);

};




#endif //CGuiTabContents_h
