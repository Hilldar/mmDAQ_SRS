//
//  CGuiUdpRawData.h
//  mmdaq
//
//  Created by Marcin Byszewski on 2/5/13.
//  Copyright (c) 2013 CERN - PH/ADE. All rights reserved.
//

#ifndef __mmdaq__CGuiUdpRawData__
#define __mmdaq__CGuiUdpRawData__

#include "CGuiDetConfig.h"
#include <iostream>

#include <vector>
#include <string>

class TRootEmbeddedCanvas;
class TCanvas;
class TGraph;
class TPad;


class CGuiUdpRawData : public CGuiTabContents {
   //   instance variables
   
public:
   CGuiUdpRawData(CUserInterface* gui, TGCompositeFrame* tabframe, unsigned num);
   virtual ~CGuiUdpRawData();
   virtual void draw();
   virtual void update();
   virtual void handle_run_state(bool) {} ;

   
private:
   TRootEmbeddedCanvas* m_rootcanvas;
   TCanvas* m_canvas;
   std::map<int, TGraph*> m_graphs;
   std::vector<TPad*> m_pads;
   
   std::string prepare_canvas(size_t vecsize, size_t columns);
   
};


#endif /* defined(__mmdaq__CGuiUdpRawData__) */
