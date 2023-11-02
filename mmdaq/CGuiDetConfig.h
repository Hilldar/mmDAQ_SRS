//
//  CGuiDetConfig.h
//  mmdaq
//
//  Created by Marcin Byszewski on 7/11/11.
//  Copyright 2011 CERN - PH/ADE. All rights reserved.
//


#ifndef CGuiDetConfig_h
#define CGuiDetConfig_h

#include "CGuiTabContents.h"

#include <vector>
#include <string>

class TRootEmbeddedCanvas;
class TCanvas;
class TH1;
class TH2F;
class TPad;


class CGuiDetConfig : public CGuiTabContents {
   //   instance variables
   
   enum xscale_settings_type { xscaleChannels = 0, xscaleStrips = 1} ;
   
public:
   CGuiDetConfig(CUserInterface* gui, TGCompositeFrame* tabframe, unsigned num);
   virtual ~CGuiDetConfig();
   virtual void draw();
   virtual void update();
   virtual void handle_run_state(bool) {};
   
   //gui
   void handle_set_xscale(Int_t val);

private:
   
   void draw_channels();
   void draw_strips();
   std::string prepare_canvas(size_t vecsize, size_t columns);

   TRootEmbeddedCanvas* m_rootcanvas;
   TCanvas* m_canvas;
   std::vector<TH1*> m_hists;
   std::vector<TPad*> m_pads;
   xscale_settings_type m_xscale;
   

};

#endif

