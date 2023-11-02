/*
 *  CUserInterface.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 12/8/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

//#define DRAWGRAPHSMAX 2

#include "CUserInterface.h"
#include "CListener.h"
#include "CPublisher.h"
#include "CConfiguration.h"
#include "CMMDaq.h"
#include "CEvent.h"
#include "CThreadRoot.h"
#include "CMutex.h"
#include "CGuiTabContents.h"
#include "CGuiDetConfig.h"
#include "CGuiUdpRawData.h"


#include "CDetChamber.h"
#include "CDetChip.h"

#include "pthread.h"

//root
#include <TApplication.h>
#include <TCanvas.h>
#include <TCondition.h>
#include <TGButton.h>
#include <TGClient.h>
#include <TGComboBox.h>

#include <TGFrame.h>
#include <TGLabel.h>
#include <TGListTree.h>
#include <TGListBox.h>

#include "TObjString.h"

#include <TGNumberEntry.h>
#include <TGStatusBar.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TGResourcePool.h>
#include <TGTab.h>
#include <TGTextEdit.h>
#include <TGTextEntry.h>

#include <TMultiGraph.h>
#include <TObject.h>

#include <TPad.h>
#include <TROOT.h>
#include <TRootEmbeddedCanvas.h>
#include <TTimer.h>
#include <TStyle.h>

#include <TH1.h>
#include <TH2.h>

//sys
#include <cmath>
#include <sstream>
#include <iostream>
#include <string>


#define NUMBEROFTABS 8
#define TAB_UDP 0
#define TAB_STRIPS 1
#define TAB_1D 2
#define TAB_2D 3
#define TAB_HITMAP 4
#define TAB_STAT 5
#define TAB_INTERNALS 6
#define TAB_DETCONFIG 7
#define RATEGRAPH_MAXPOINTS 100


using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using std::map;


#ifdef __cplusplus
extern "C"  {
#endif
const char* svn_version();
#ifdef __cplusplus
}
#endif

//ClassImp(CUserInterface)

CUserInterface::CUserInterface(CMMDaq* daq, CConfiguration* config)
: CThreadRoot(), m_daq(daq), m_config(config), m_publisher(0),
      m_timer_subscriber(new TTimer()), m_timer_subscriber_last_update(0),
      m_cond_newrequest(0), m_cond_newdataready(0), m_mx_published(0), m_mx_published_detach_request(false),
      m_defined_chamber_strip_ranges(m_config->defined_chamber_strip_ranges()),
      m_do_show_next_event(false), m_tab_needs_redraw(NUMBEROFTABS, false),
      m_chips_to_draw(), m_chambers_to_draw(),
      IDs(), m_mainframe(0), m_verticalframe(0), m_upper_h_frame(0), m_lower_h_frame(0),
      m_tree_canvas(0), m_tree_viewport(0), m_tree_listbox(0), m_main_tabs(0),
      m_compositeframe(NUMBEROFTABS, 0L), m_rootembeddedcanvas(NUMBEROFTABS, 0L), 
      m_canvas(NUMBEROFTABS, 0L), m_pads(NUMBEROFTABS, vector<TPad*>()),
      m_histograms(NUMBEROFTABS, vector<TH1*>()), m_graphs(NUMBEROFTABS, vector<TGraph*>()),
      m_gui_tab_contents(NUMBEROFTABS, 0L),
      m_guistatusbar(0), m_guiruntypeselect(0), m_guibuttonrun(0), m_guibuttonpause(0), m_guibuttonstop(0),
      m_guibuttonexit(0), m_guibuttonnext(0), m_guicomment(0),
      m_guimonitorswitch(0), m_guiwritefileswitch(0), //, m_guichipselection(0) //, m_guilog(0)//, m_guicontroller(0)
      m_label_runnumber(0), m_guioptions_pedfile(0), m_guioptions_pedfileload(0), m_guioptions_pedfileclear(0),
		m_guioptions_pedfilesave(0), m_guioptions_zerofactor(0), m_guioptions_cmode_switch(0),
m_guioptions_cmode_factor(0)
{
   set_thread_name("UI");
   CThreadRoot::lock_mutex();
   construct_window();
   construct_histograms();
   construct_tree();
   //signals

   m_timer_subscriber->Connect("Timeout()", "CUserInterface", this, "timed_exec()");
   m_timer_subscriber->Start(220, kFALSE);
   //m_timer_subscriber->TurnOff();
   CThreadRoot::unlock_mutex();
}


CUserInterface::~CUserInterface()
{
   m_timer_subscriber->TurnOff();
   delete m_timer_subscriber; m_timer_subscriber = 0;
   
   Cleanup();
   //TODO: replace by while loops
   for (std::vector<TH1*>::iterator ih = m_histograms[0].begin() ; ih != m_histograms[0].end(); ++ih) {
      delete *ih;
   }
   for (std::vector<TH1*>::iterator ih = m_histograms[1].begin() ; ih != m_histograms[1].end(); ++ih) {
      delete *ih;
   }
   for (std::vector<TH1*>::iterator ih = m_histograms[2].begin() ; ih != m_histograms[2].end(); ++ih) {
      delete *ih;
   }
   
   for (std::vector<TGraph*>::iterator ig = m_graphs[0].begin() ; ig != m_graphs[0].end(); ++ig) {
      delete *ig; (*ig) = 0;
   }
   for (std::vector<TGraph*>::iterator ig = m_graphs[3].begin() ; ig != m_graphs[3].end(); ++ig) {
      delete *ig; (*ig) = 0;
   }
   
   FreeClear(m_gui_tab_contents);
}


void CUserInterface::construct_window()
{
   //construct frame
   //this->Connect("CloseWindow()", "CUserInterface", this, "CloseWindow()");
   gROOT->SetStyle("Plain");
   gStyle->SetPalette(1);

   // main frame
   m_mainframe = new TGMainFrame(gClient->GetRoot(), 10, 10, kMainFrame | kVerticalFrame);

   // main vertical frame (includes top_horizontal + tabs)
   m_verticalframe = new TGVerticalFrame(m_mainframe, 588, 772, kVerticalFrame);

   //top horizontal frame
   m_upper_h_frame = new TGHorizontalFrame(m_verticalframe, 584, 26, kHorizontalFrame);


   //   //controller checkbox
   //   m_guicontroller = new TGCheckButton(m_upper_h_frame, "Controller");
   //   m_guicontroller->SetTextJustify(36);
   //   m_guicontroller->SetMargins(0, 0, 0, 0);
   //   m_guicontroller->SetWrapLength(-1);
   //   m_upper_h_frame->AddFrame(m_guicontroller, new TGLayoutHints(kLHintsCenterX | kLHintsTop, 2, 2, 2, 2));

   // list box run type
   TGGroupFrame* group1frame = new TGGroupFrame(m_upper_h_frame, "run type", kVerticalFrame);
   m_guiruntypeselect = new TGListBox(group1frame, -1, kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
   m_guiruntypeselect->AddEntry("Physics", 0);
   m_guiruntypeselect->AddEntry("Pedestals", 1);
   m_guiruntypeselect->AddEntry(" ", 2);
   m_guiruntypeselect->AddEntry(" ", 3);

   m_guiruntypeselect->Resize(122, 62);
   m_guiruntypeselect->Select(m_config->run_type() - 1);

   m_guiruntypeselect->Connect("Selected(Int_t)", "CUserInterface", this, "gui_handle_select_runtype(Int_t)");
   group1frame->AddFrame(m_guiruntypeselect, new TGLayoutHints(kLHintsLeft | kLHintsTop, -10, -10, 0, -5));
   m_upper_h_frame->AddFrame(group1frame, new TGLayoutHints(kLHintsLeft, 0, 0, 0, 0));



   TGGroupFrame* group2frame = new TGGroupFrame(m_upper_h_frame, "Control", kVerticalFrame);
   //button run
   m_guibuttonrun = new TGTextButton(group2frame, " Run  ");
   m_guibuttonrun->Resize(110, 22);
   m_guibuttonrun->Connect("Clicked()", "CUserInterface", this, "GuiDoRun()");
   group2frame->AddFrame(m_guibuttonrun, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));

   //button pause
   m_guibuttonpause = new TGTextButton(group2frame, " Pause ");
   m_guibuttonpause->Resize(110, 22);
   m_guibuttonpause->AllowStayDown(true);
   m_guibuttonpause->Connect("Clicked()", "CUserInterface", this, "GuiDoPause()");
   group2frame->AddFrame(m_guibuttonpause, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));


   //button stop
   m_guibuttonstop = new TGTextButton(group2frame, " Stop ");
   m_guibuttonstop->Resize(130, 22);
   m_guibuttonstop->Connect("Clicked()", "CUserInterface", this, "GuiDoStop()");
   group2frame->AddFrame(m_guibuttonstop, new TGLayoutHints(kLHintsLeft , 1, 1, 1, 1));
   //group2frame->ChangeOptions(group2frame->GetOptions() | kFixedSize);
   //group2frame->Resize(40, 90);
   m_upper_h_frame->AddFrame(group2frame, new TGLayoutHints(kLHintsLeft, 0, 0, 0, 0));

   //Save Options
   TGGroupFrame* groupfileframe = new TGGroupFrame(m_upper_h_frame, "Save Options", kVerticalFrame);
   //button write file
   m_guiwritefileswitch = new TGCheckButton(groupfileframe, "Write File");
   m_guiwritefileswitch->Connect("Clicked()", "CUserInterface", this, "gui_handle_write()");
   m_guiwritefileswitch->Resize(102, 22);
   groupfileframe->AddFrame(m_guiwritefileswitch, new TGLayoutHints(kLHintsLeft, 0, 0, 0, 0));
   TGLabel *fgrfile_Lbl1 = new TGLabel(groupfileframe, "Comments saved to log");
   groupfileframe->AddFrame(fgrfile_Lbl1,  new TGLayoutHints(kLHintsLeft | kLHintsTop, 0, 0, 0, 0));

   //Comment line
   m_guicomment = new TGTextEntry(groupfileframe, "test DAQ", IDs.GetUniqueID());
   m_guicomment->Resize(202, 22);
   groupfileframe->AddFrame(m_guicomment, new TGLayoutHints(kLHintsLeft | kLHintsBottom, 0, 0, 0, 0));
   //groupfileframe->ChangeOptions(groupfileframe->GetOptions() | kFixedSize);
   //groupfileframe->Resize(220, 90);
   m_upper_h_frame->AddFrame(groupfileframe, new TGLayoutHints(kLHintsLeft, 0, 0, 0, 0));



   TGGroupFrame* group3frame = new TGGroupFrame(m_upper_h_frame, "", kVerticalFrame);

   //button monitor loop on
   m_guimonitorswitch = new TGCheckButton(group3frame, "Monitor");
   m_guimonitorswitch->Connect("Clicked()", "CUserInterface", this, "GuiDoMonitor()");
   m_guimonitorswitch->Resize(102, 22);
   group3frame->AddFrame(m_guimonitorswitch, new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));

   //button next
   m_guibuttonnext = new TGTextButton(group3frame, "Next Event");
   m_guibuttonnext->Connect("Clicked()", "CUserInterface", this, "GuiShowNextEvent()");
   group3frame->AddFrame(m_guibuttonnext, new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));


   m_upper_h_frame->AddFrame(group3frame, new TGLayoutHints(kLHintsLeft, 0, 0, 0, 0));





//   // combo box
//   m_guichipselection = new TGComboBox(m_upper_h_frame, -1, kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
//   vector<string> chipnames = m_config->defined_chip_names();
//   m_guichipselection->AddEntry("All", 0);
//   int ii = 1;
//   for (vector<string>::iterator ichip = chipnames.begin() ; ichip != chipnames.end(); ++ichip, ++ii) {
//      m_guichipselection->AddEntry(ichip->c_str(), ii);
//   }
//   m_guichipselection->Resize(102, 22);
//   m_guichipselection->Select(0);
//   m_guichipselection->Connect("Selected(Int_t)", "CUserInterface", this, "gui_handle_select_chip(Int_t)");
//   m_upper_h_frame->AddFrame(m_guichipselection, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));


   //button exit
   m_guibuttonexit = new TGTextButton(m_upper_h_frame, "&Exit");
   m_guibuttonexit->Connect("Clicked()", "CUserInterface", this, "GuiDoExit()");
   m_upper_h_frame->AddFrame(m_guibuttonexit, new TGLayoutHints(kLHintsRight, 5, 5, 3, 4));
   m_verticalframe->AddFrame(m_upper_h_frame, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX, 0, 0, 0, 0));
   // LAYOUT: end upper horizontal frame


   /////////////////////////////////////////////////////////////////////////////
   // LAYOUT: lower frame -  horizontal layout (tree + tabs)
   m_lower_h_frame = new TGHorizontalFrame(m_verticalframe, 584, 26, kHorizontalFrame);

   TGVerticalFrame* m_lower_left_v_frame = new TGVerticalFrame(m_lower_h_frame, 125, 150, kVerticalFrame);
   ////run number label
   //set up font
   // label + horizontal line
   TGGC *fTextGC;
   const TGFont *font = gClient->GetFont("-*-times-bold-r-*-*-18-*-*-*-*-*-*-*");
   if (!font)
      font = gClient->GetResourcePool()->GetDefaultFont();
   FontStruct_t labelfont = font->GetFontStruct();
   GCValues_t   gval;
   gval.fMask = kGCBackground | kGCFont | kGCForeground;
   gval.fFont = font->GetFontHandle();
   gClient->GetColorByName("yellow", gval.fBackground);
   fTextGC = gClient->GetGC(&gval, kTRUE);
   //colors
   unsigned long ycolor = 0, bcolor = 0;
   gClient->GetColorByName("blue", bcolor);
   gClient->GetColorByName("green", ycolor);
   m_label_runnumber = new TGLabel(m_lower_left_v_frame, "------", fTextGC->GetGC(), labelfont, kChildFrame);
   //AddFrame(m_label_runnumber, new TGLayoutHints(kLHintsNormal, 5, 5, 3, 4));
   m_label_runnumber->ChangeOptions(m_label_runnumber->GetOptions() | kFixedSize);
   m_label_runnumber->Resize(122, 22);
   TColor *dgreen = gROOT->GetColor(46);

   m_label_runnumber->SetTextColor(dgreen);
   //m_label_runnumber->SetTextColor(ycolor);
   m_label_runnumber->SetTextJustify(kTextCenterX | kTextCenterY);
   m_label_runnumber->Disable();

   m_lower_left_v_frame->AddFrame(m_label_runnumber, new TGLayoutHints(kLHintsLeft | kLHintsTop, 2, 2, 2, 2));


   //tree widget
   // canvas widget
   m_tree_canvas = new TGCanvas(m_lower_left_v_frame, 122, 152);
   // canvas viewport
   m_tree_viewport = m_tree_canvas->GetViewPort();
   // list tree
   m_tree_listbox = new TGListTree(m_tree_canvas, kHorizontalFrame);
   m_tree_listbox->SetName("m_treelistbox");
   m_tree_listbox->SetCheckMode(TGListTree::kRecursive);
   m_tree_listbox->Connect("Checked(TObject*,Bool_t)", "CUserInterface", this, "gui_handle_tree_change(TObject*,Bool_t)");
   m_tree_viewport->AddFrame(m_tree_listbox);
   m_tree_listbox->SetLayoutManager(new TGHorizontalLayout(m_tree_listbox));
   m_tree_listbox->MapSubwindows();
   m_tree_canvas->SetContainer(m_tree_listbox);
   m_tree_canvas->MapSubwindows();
   m_lower_left_v_frame->AddFrame(m_tree_canvas, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandY, 2, 2, 2, 2));

   m_lower_h_frame->AddFrame(m_lower_left_v_frame, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandY, 0, 0, 0, 0));

   // tab widget
   m_main_tabs = new TGTab(m_lower_h_frame, 484, 316);
//   TGCompositeFrame* composite_frame_tab[NUMBEROFTABS+1];
   std::vector<string> tabtitles;
	tabtitles.push_back("Raw Frame");
	tabtitles.push_back("Raw Strip");
	tabtitles.push_back("Event 1D");
	tabtitles.push_back("Event 3D");
   tabtitles.push_back("Hit Map 2D");
	tabtitles.push_back("Statistics");
	tabtitles.push_back("Internals");
   tabtitles.push_back("Pedestals");
   
	assert(tabtitles.size() == NUMBEROFTABS);

   int lasttab = 0;
   for (int i = 0; i < NUMBEROFTABS; i++) {

      //container of tab i
      m_compositeframe[i] = m_main_tabs->AddTab(tabtitles[i].c_str());
      m_compositeframe[i]->SetLayoutManager(new TGVerticalLayout(m_compositeframe[i]));
      // embedded canvas in tab i
      
      if (i == TAB_UDP || i == TAB_HITMAP || i == TAB_DETCONFIG) {
         continue;
      }
      
      m_rootembeddedcanvas[i] = new TRootEmbeddedCanvas(Form("ecanvas%d", i), m_compositeframe[i], 580, 313);
      //Int_t wfRootEmbeddedCanvas = frootembeddedcanvas[i]->GetCanvasWindowId();
      //fcanvas[i] = new TCanvas(Form("canvas%d",i), 10, 10, wfRootEmbeddedCanvas);
      //frootembeddedcanvas[i]->AdoptCanvas(fcanvas[i]);
      m_canvas[i] = m_rootembeddedcanvas[i]->GetCanvas();
      m_canvas[i]->SetName(Form("canvas%d", i));
      m_compositeframe[i]->AddFrame(m_rootembeddedcanvas[i], new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY));
      lasttab = i;
   }
   lasttab++;

   /////////////////////////////////////////////////////////////////////////////
   // things displayed inside tabs:
   m_gui_tab_contents[TAB_UDP] = new CGuiUdpRawData(this, m_compositeframe[TAB_UDP], TAB_UDP);
   m_gui_tab_contents[TAB_HITMAP] = new CGuiHitMap(this, m_compositeframe[TAB_HITMAP], TAB_HITMAP);
   m_gui_tab_contents[TAB_DETCONFIG] = new CGuiDetConfig(this, m_compositeframe[TAB_DETCONFIG], TAB_DETCONFIG);

   
   //options tab
   m_compositeframe[lasttab] = m_main_tabs->AddTab("Options");
   construct_options_tab(m_compositeframe[lasttab]);


   //tabs - finish up
   m_main_tabs->SetTab(0);
   m_main_tabs->Resize(m_main_tabs->GetDefaultSize());
   m_lower_h_frame->AddFrame(m_main_tabs, new TGLayoutHints(kLHintsNormal | kLHintsExpandX | kLHintsExpandY));
   //end tabs

   m_verticalframe->AddFrame(m_lower_h_frame, new TGLayoutHints(kLHintsLeft | kLHintsTop | kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
   // LAYOUT: end lower horizontal frame



   /////////////////////////////////////////////////////////////////////////////
   // status bar
   Int_t parts[] = {33, 10, 10, 47};
   m_guistatusbar = new TGStatusBar(m_mainframe, 50, 10, kHorizontalFrame);
   m_guistatusbar->SetParts(parts, 4);
   m_mainframe->AddFrame(m_guistatusbar, new TGLayoutHints(kLHintsBottom | kLHintsLeft | kLHintsExpandX, 0, 0, 2, 0));

   /////////////////////////////////////////////////////////////////////////////
   // main frame - finish up
   m_mainframe->AddFrame(m_verticalframe, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 1, 1, 1, 1));
   m_mainframe->SetMWMHints(kMWMDecorAll, kMWMFuncAll, kMWMInputModeless);
   m_mainframe->SetWindowName("mmDAQ");
   m_mainframe->MapSubwindows();
   m_mainframe->Resize(m_mainframe->GetDefaultSize());
   m_mainframe->MapWindow();
   m_mainframe->Resize(750, 650);
   //fmainframe->MoveResize(2, 2, (unsigned)(gClient->GetDisplayWidth()*0.9), (unsigned)(gClient->GetDisplayHeight()*0.9));
   m_mainframe->Connect("CloseWindow()", "CUserInterface", this, "CloseWindow()");


}

void CUserInterface::construct_options_tab(TGCompositeFrame * tabmainframe)
{
   /*mmdaq.DAQIPAddress: 127.0.0.1
    mmdaq.DAQIPPort: 6006
    mmdaq.LogFile: /data/mmega2010/apv_data/mmdaq.log
    mmdaq.ChannelMapFile: /sw/mmdaq/config/20110218_R14_Xrays.map
    mmdaq.PathToPedestalFile: /data/mmega2010/apv_data/root/
    mmdaq.ZeroTresholdFactor: 10.0
    */

   tabmainframe->SetLayoutManager(new TGVerticalLayout(tabmainframe));


   // main vertical frame (includes top_horizontal + tabs)

   TGVerticalFrame* tab_verticalframe = new TGVerticalFrame(tabmainframe, 588, 772, kVerticalFrame);

   TGGroupFrame* pedframe = new TGGroupFrame(tab_verticalframe, "Pedestal File", kHorizontalFrame);
	m_guioptions_pedfile = new TGTextEntry(pedframe, "", IDs.GetUniqueID());
   m_guioptions_pedfile->Resize(262, 22);
   pedframe->AddFrame(m_guioptions_pedfile, new TGLayoutHints(kLHintsLeft | kLHintsTop, 1, 1, 1, 1));
   //button Load
   m_guioptions_pedfileload = new TGTextButton(pedframe, "Load");
   m_guioptions_pedfileload->Resize(19, 22);
   m_guioptions_pedfileload->Connect("Clicked()", "CUserInterface", this, "GuiDoOptionsPedestalLoad()");
   pedframe->AddFrame(m_guioptions_pedfileload, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
	//button Clear
   m_guioptions_pedfileclear = new TGTextButton(pedframe, "Clear");
   m_guioptions_pedfileclear->Resize(19, 22);
   m_guioptions_pedfileclear->Connect("Clicked()", "CUserInterface", this, "GuiDoOptionsPedestalClear()");
   pedframe->AddFrame(m_guioptions_pedfileclear, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
   tab_verticalframe->AddFrame(pedframe, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
	// zs factor
	TGGroupFrame* sigmaframe = new TGGroupFrame(tab_verticalframe, "Zero Suppression Factor", kHorizontalFrame);
   TGLabel *fsigmaLbl1 = new TGLabel(sigmaframe, "Factor");
   sigmaframe->AddFrame(fsigmaLbl1,  new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));
   m_guioptions_zerofactor = new TGNumberEntry(sigmaframe, m_config->sigma_cut(), 5, IDs.GetUniqueID(), TGNumberEntry::kNESReal, TGNumberEntry::kNEAPositive, TGNumberEntry::kNELLimitMin, 0.0);
	m_guioptions_zerofactor->Connect("ValueSet(Long_t)", "CUserInterface", this, "GuiDoOptionsSetZeroFactor()");
	sigmaframe->AddFrame(m_guioptions_zerofactor, new TGLayoutHints(kLHintsLeft | kLHintsTop, 1, 1, 1, 1));
	tab_verticalframe->AddFrame(sigmaframe, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
	
	// common mode
   TGGroupFrame* cmodeframe = new TGGroupFrame(tab_verticalframe, "RAW Common Mode Correction", kHorizontalFrame);
   //button monitor loop on
   m_guioptions_cmode_switch = new TGCheckButton(cmodeframe, "On");
   m_guioptions_cmode_switch->Connect("Clicked()", "CUserInterface", this, "GuiDoOptionsCommonModeCorrection()");
   m_guioptions_cmode_switch->Resize(102, 22);
   m_guioptions_cmode_switch->SetOn();
   cmodeframe->AddFrame(m_guioptions_cmode_switch, new TGLayoutHints(kLHintsLeft, 5, 5, 3, 4));
	tab_verticalframe->AddFrame(cmodeframe, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));

   TGLabel *fcommodeLbl1 = new TGLabel(cmodeframe, "Signal if: Pedestal Sigma x");
   cmodeframe->AddFrame(fcommodeLbl1,  new TGLayoutHints(kLHintsLeft | kLHintsTop, 5, 5, 3, 4));
   
   m_guioptions_cmode_factor = new TGNumberEntry(cmodeframe, 10.0, 5, IDs.GetUniqueID(), TGNumberEntry::kNESReal, TGNumberEntry::kNEAPositive, TGNumberEntry::kNELLimitMin, 0.0);
	m_guioptions_cmode_factor->Connect("ValueSet(Long_t)", "CUserInterface", this, "GuiDoOptionsCommonModeFactor()");
	cmodeframe->AddFrame(m_guioptions_cmode_factor, new TGLayoutHints(kLHintsLeft | kLHintsTop, 1, 1, 1, 1));

   
   TGLabel *svnverlabel = new TGLabel(tabmainframe, TString("SVN version:") + TString(svn_version()) );
   tabmainframe->AddFrame(svnverlabel, new TGLayoutHints(kLHintsLeft | kLHintsBottom, 2, 2, 2, 2));

   
   
   tabmainframe->AddFrame(tab_verticalframe, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 1, 1, 1, 1));

}



void CUserInterface::construct_histograms()
{
   size_t N = m_config->chip_count();
   for (size_t i = 0; i < N; ++i) {
      //udp
//      m_graphs[TAB_UDP].push_back(new TGraph());
//      m_graphs[TAB_UDP][i]->SetLineColor(i + 1);
//      m_graphs[TAB_UDP][i]->SetMarkerColor(i + 1);

      
      //pedestals
      m_graphs[1].push_back(new TGraphErrors(0));
      m_graphs[1][i]->SetLineColor(4);
      //m_graphs[1][i]->SetMarkerColor(4);
      m_graphs[1][i]->SetMarkerStyle(kFullDotLarge);
   }
   std::vector <CDetChamber*> chambers = m_config->defined_chambers();

   for (std::vector <CDetChamber*> :: iterator ichamb = chambers.begin(); ichamb != chambers.end(); ++ichamb) {
      std::map <int, std::pair<unsigned, unsigned> > :: iterator itf = m_defined_chamber_strip_ranges.find((*ichamb)->get_id());
      //std::pair<unsigned,unsigned> strip_range = itf->second;
      int lostrip = itf->second.first;
      int histrip = itf->second.second;

      std::stringstream ss;
//		ss << (*ichamb)->name() << "_qmax";
//      m_histograms[1].push_back(new TH1F(ss.str().c_str(), ss.str().c_str() , histrip - lostrip, lostrip, histrip));
//		ss.str("");
//		ss << (*ichamb)->name() << "_tqmax";
//		m_histograms[1].push_back(new TH2F(ss.str().c_str(), ss.str().c_str() , histrip - lostrip, lostrip, histrip, 20, 0, 19));

      ss.str((*ichamb)->name());
      m_histograms[TAB_2D].push_back(new TH2F((*ichamb)->name().c_str(), (*ichamb)->name().c_str() , histrip - lostrip, lostrip, histrip, 20, 0, 19));
   }

   //m_histograms[0].push_back(new TH1F(Form("raw_udp_%d", i), Form("raw_udp_%d", i), 128, 0, 127));

   //internals tab=4
   m_histograms[TAB_INTERNALS].push_back(new TH1F("queues", "queues", 5, 0, 5));
   m_graphs[TAB_INTERNALS].push_back(new TGraph());
   m_graphs[TAB_INTERNALS][0]->SetMarkerStyle(kFullDotLarge);
   for (int ipoint = 1; ipoint < RATEGRAPH_MAXPOINTS; ++ipoint) {
      m_graphs[TAB_INTERNALS][0]->SetPoint(ipoint, ipoint, 0);
   }
}


void CUserInterface::construct_tree()
{

   const TGPicture *popen;       //used for list tree items
   const TGPicture *pclose;      //used for list tree items

   //clean tree
   TGListTreeItem* FirstItem = m_tree_listbox->GetFirstItem();
   while (FirstItem) {
      m_tree_listbox->DeleteChildren(FirstItem);
      m_tree_listbox->DeleteItem(FirstItem);
      FirstItem = m_tree_listbox->GetFirstItem();
   }
   m_tree_listbox->ClearViewPort();


   TList* itemlist = m_tree_listbox->GetList();
   //clear tree
   TObjLink *lnk = itemlist->FirstLink();
   while (lnk) {
      TObjLink* nextlnk = lnk->Next();
      lnk->GetObject()->Delete();
      lnk = nextlnk;
   }

   popen = gClient->GetPicture("ofolder_t.xpm");
   pclose = gClient->GetPicture("folder_t.xpm");

   TGListTreeItem *item0 = NULL; // placeholder for 'ALL'

   //chamber list
   const std::vector<CDetChamber*> list_of_chambers = m_config->defined_chambers();
   for (std::vector<CDetChamber*> :: const_iterator ichamb = list_of_chambers.begin(); ichamb != list_of_chambers.end(); ++ichamb) {
      //insert chamber
      TGListTreeItem *item1 = m_tree_listbox->AddItem(item0, (*ichamb)->name().c_str());
      item1->SetPictures(popen, pclose);
      item1->SetCheckBox(1);
      item1->CheckItem(1);
      item1->SetUserData((*ichamb));
      m_tree_listbox->OpenItem(item1);

      //get list of chips for chamber
      const std::vector< CDetChip*> list_of_chips = m_config->chips_on_chamber((*ichamb)->get_id());
      for (std::vector<  CDetChip*> :: const_iterator ichip = list_of_chips.begin(); ichip != list_of_chips.end(); ++ichip) {
         //insert chip
         TString full_name = NumberToString((*ichip)->fec()->get_id()).c_str();
         full_name = full_name + TString(":") + TString((*ichip)->name().c_str());
         TGListTreeItem *item2 = m_tree_listbox->AddItem(item1, full_name);
         item2->SetPictures(popen, pclose);
         item2->SetCheckBox(1);
         item2->CheckItem(1);
         item2->SetUserData((*ichip));
         m_tree_listbox->OpenItem(item2);

      }
   }
}





/* main loop for the internal thread
  that marks tabs for update on signal */
int CUserInterface::main(void*)
{
   set_thread_name("ui_main");
   while (1) {
      //wait for signal
      int rc = m_cond_newdataready->Wait();  // waiting for new data
      //cout << "CUserInterface::main():got new data rc =" << rc << endl;

      if (rc < 0) {
         cerr << "CPublisher::main(): m_cond_newrequest.Wait() returned " << rc << endl;
      }

      for (int itab = 0; itab < NUMBEROFTABS; ++itab) {
         m_tab_needs_redraw[itab] = true;
      }
   }
   return 0;
}

void CUserInterface::detach(CPublisher* pub)
{
   assert(m_publisher = pub);
   m_publisher = 0;
}


void CUserInterface::detach_signal_request(TCondition* cond)
{
   assert(m_cond_newrequest == cond);
   m_cond_newrequest = 0;
}

void CUserInterface::detach_signal_newdata(TCondition* cond)
{
   assert(m_cond_newdataready == cond);
   m_cond_newrequest = 0;
}

void CUserInterface::detach_mutex_published_data(CMutex* mut)
{
   assert(m_mx_published == mut);
   m_mx_published_detach_request = true;
}


void CUserInterface::request_data_update()
{
   assert(m_cond_newrequest);
	//ask only if able to receive
	if (m_mx_published) {
		m_cond_newrequest->Signal(); 
	}
   
}


void CUserInterface::refresh_tabs()
{
   for (int itab = 0; itab < NUMBEROFTABS; ++itab) {
      m_tab_needs_redraw[itab] = true;
   }
}
void CUserInterface::update_tabs()
{
   CThreadRoot::lock_mutex();
   for (int itab = 0; itab < NUMBEROFTABS; itab++) {
      if (m_tab_needs_redraw[itab]) {
         update_tab(itab);
      }
   }
   CThreadRoot::unlock_mutex();
}

void CUserInterface::timed_exec()
{
   // Actions after timer's time-out
   update_tabs();

   


   //check_published_data();


   int now = (int)time(NULL);
   if (m_timer_subscriber_last_update == -1) m_timer_subscriber_last_update = now;
   if (m_timer_subscriber_last_update != -1) {
      if (now - m_timer_subscriber_last_update >= 1) {
         m_daq->update_internal_queue_sizes();
         //CThreadRoot::lock_mutex();
         draw_internals();
         //CThreadRoot::unlock_mutex();
      }
   }

	if (m_mx_published_detach_request) {
		internal_detach_mutex_published_data();
		m_mx_published_detach_request = false;
	}
	
   if (m_guimonitorswitch->IsDown() || m_do_show_next_event) {
      m_do_show_next_event = false;
      //cout << "CUserInterface::timed_exec():requesting data " << endl;
      request_data_update();
   }
   //m_timer_subscriber->Reset();
}


//______________________________________________________________________________
void CUserInterface::CloseWindow()
{
   m_daq->stop();
   gApplication->Terminate(0);
}



void CUserInterface::make_list_of_items_to_draw()
{
   m_chips_to_draw.clear();
   m_chambers_to_draw.clear();
   
   //   TList* checked_tlist = new TList;
	TList checked_tlist;
   m_tree_listbox->GetChecked(&checked_tlist);
   TObjLink *lnk = checked_tlist.FirstLink();
   while (lnk) {
      TObjString* obj = dynamic_cast<TObjString*>(lnk->GetObject());    //->Draw();
      string text = obj->String().Data();
      // cout << " checked: " << text << endl;
      //   guitree_item_t itemtype = gti_none;
      
      // if (itemtype == gti_none) {
      //look for chip
      const std::vector<CDetChip*>&	chips =  m_config->defined_chips();
      
      for (std::vector<CDetChip*>::const_iterator ichip = chips.begin(); ichip != chips.end(); ++ichip) {
         std::stringstream ss;
         ss << "APV " <<  CApvEvent::fecNo_from_chipId((*ichip)->get_id()) << ":" << CApvEvent::chipNo_from_chipId((*ichip)->get_id());
         if (ss.str() == text) {
            m_chips_to_draw.push_back(*ichip);
            const CDetChamber* chmb = m_config->get_chamber_by_chipId((*ichip)->get_id());
            if (chmb) {
               m_chambers_to_draw.push_back(chmb );

            }
            break;
         }
      }
      lnk = lnk->Next();
   }
   MakeElementsUnique(m_chambers_to_draw);
//	m_tab_do_redraw = vector<bool>(NUMBEROFTABS,true);
}




void CUserInterface::gui_handle_tree_change(TObject*, Bool_t)
{
   //TODO: implement gui_handle_tree_change()
   make_list_of_items_to_draw();
   refresh_tabs();
}

int CUserInterface::get_tree_checked_items(std::vector<TGListTreeItem*>& vec)
{
   int count = 0;
   TGListTreeItem* item = m_tree_listbox->GetFirstItem();
   
   while (item) {
      if(item->IsChecked()) {
         ++count;
         //         std::cout << count << " checked is " << item->GetText() << std::endl;
         vec.push_back( item);
      }
      get_item_checked_descendants(item->GetFirstChild(), vec);
      item = item->GetNextSibling();
   }
   return count;
}

int CUserInterface::get_item_checked_descendants(TGListTreeItem* first_item, std::vector<TGListTreeItem*>& vec)
{
   int count = 0;
   TGListTreeItem* item = first_item; //->GetFirstChild();
   while (item) {
      if(item->IsChecked()) {
         ++count;
         //         std::cout << count << " checked is " << item->GetText() << std::endl;
         vec.push_back( item);
      }
      get_item_checked_descendants(item->GetFirstChild(), vec);
      item = item->GetNextSibling();
   }
   return count;
}


void CUserInterface::gui_handle_select_runtype(Int_t sel)
{
   string str("Run Type: ");
   if (sel == 0) {
      //m_run_type = CConfiguration::runtypePhysics;
      m_config->run_type(CConfiguration::runtypePhysics);
      str += "Physics";
   }
   else if (sel == 1) {
      //m_run_type = CConfiguration::runtypePedestals;
      m_config->run_type(CConfiguration::runtypePedestals);
      str += "Pedestals";
   }
   update_statusbar(str.c_str(), 0);
}


void CUserInterface::gui_handle_write()
{

}

const int CUserInterface::gui_selected_run_type() const
{
   if (!m_guiruntypeselect) {
      return CEvent::eventTypeBad;
   }

   int sel = m_guiruntypeselect->GetSelected();
   switch (sel) {
      case 0:
         return 0;//CEvent::eventTypePhysics; // cant include CEvent.h
         break;
      case 1:
         return 1;//CEvent::eventTypePedestals; // cant include CEvent.h ...
         break;

      default:
         break;
   }

   return CEvent::eventTypeBad;
}

const std::string CUserInterface::gui_comment_line() const
{
   return std::string(m_guicomment->GetText());
}

void CUserInterface::GuiDoExit()
{
   m_daq->stop();
   gApplication->Terminate(0);

}

void CUserInterface::GuiDoRun()
{
   //m_timer_subscriber->TurnOn();
   //m_guiwritefileswitch->SetEnabled( false);
   //m_guiruntypeselect->SetEditDisabled();//Enabled( false);

   for (std::vector<CGuiTabContents*>::iterator it = m_gui_tab_contents.begin(); it != m_gui_tab_contents.end(); ++it) {
      if (CGuiTabContents* tab = dynamic_cast<CGuiTabContents*>(*it) ) {
         tab->handle_run_state(true);
      }
   }
   m_guibuttonrun->SetOn(true);
   m_daq->run();

}


void CUserInterface::GuiDoPause()
{
   m_daq->pause_receiver(m_guibuttonpause->IsOn());
}


void CUserInterface::GuiDoStop()
{
   for (std::vector<CGuiTabContents*>::iterator it = m_gui_tab_contents.begin(); it != m_gui_tab_contents.end(); ++it) {
      if (CGuiTabContents* tab = dynamic_cast<CGuiTabContents*>(*it) ) {
         tab->handle_run_state(false);
      }
   }
   
   m_guibuttonrun->SetOn(false);
   m_guibuttonpause->SetOn(false);
   m_daq->stop();
}

void CUserInterface::GuiShowNextEvent()
{
   m_do_show_next_event = true;
   //cout << "CUserInterface::GuiShowNextEvent()"<< endl;
   //TODO: switch on current tab
//   m_daq->request_data(CMMDaq::dt_udp_data); //if on tab 0
//   m_daq->request_data(CMMDaq::dt_1d_data); //if on tab 0
//   m_daq->request_data(CMMDaq::dt_2d_data); //if on tab 0

}

void CUserInterface::GuiDoMonitor()
{
}

void CUserInterface::GuiDoOptionsPedestalLoad()
{
	m_config->load_pedestals( m_guioptions_pedfile->GetText());
	std::stringstream ss;
	ss << "Loaded pedestal for " << m_config->pedestal_event()->pedestal_size() << " channels";
	update_statusbar(ss.str().c_str(), 3);
   refresh_tabs();
}


void CUserInterface::GuiDoOptionsPedestalClear()
{
	m_config->clear_pedestals();
	std::stringstream ss;
	ss << "Current pedestal data for " << m_config->pedestal_event()->pedestal_size() << " channels";
	update_statusbar(ss.str().c_str(), 3);
	refresh_tabs();
}


void CUserInterface::GuiDoOptionsSetZeroFactor()
{
	m_config->sigma_cut(m_guioptions_zerofactor->GetNumber());

}


void CUserInterface::GuiDoOptionsConfigLoad()
{
//	try {
//		m_config->load(m_guioptions_configfile->GetText());
//	}
//	catch (std::string str) {
//		cerr << "GuiDoOptionsConfigLoad(): " << str << endl;
//		return;
//	}
//	update_options_tab();
//	construct_tree();

}

void CUserInterface::GuiDoOptionsConfigSave()
{
//	try {
//		m_config->save(m_guioptions_configfile->GetText());
//	}
//	catch (std::string str) {
//		cerr << "GuiDoOptionsConfigSave(): " << str << endl;
//		return;
//	}
}


void CUserInterface::GuiDoOptionsCommonModeCorrection()
{
   m_config->set_common_mode_enabled(m_guioptions_cmode_switch->IsOn());
}

void CUserInterface::GuiDoOptionsCommonModeFactor()
{
   m_config->set_common_mode_factor(m_guioptions_cmode_factor->GetNumber());
}


void CUserInterface::controls_run_started(int number)
{
   std::stringstream ss;
   ss << number;
   m_label_runnumber->SetText(ss.str().c_str());
   m_guibuttonrun->SetEnabled(false);
   m_label_runnumber->Enable();
   m_guioptions_pedfileload->SetEnabled(false);
   m_guioptions_pedfileclear->SetEnabled(false);
   update_statusbar("Running.", 1); // print_progress(queuesize);

}

void CUserInterface::controls_run_stopped()
{
   m_guimonitorswitch->SetOn(false);
   m_guibuttonrun->SetEnabled(true);
   m_label_runnumber->Disable();
   m_guioptions_pedfileload->SetEnabled(true);
   m_guioptions_pedfileclear->SetEnabled(true);
   update_statusbar("Stopped.", 1); // print_progress(queuesize);

}


void CUserInterface::controls_run_start_failed(const std::string& msg)
{
   m_guimonitorswitch->SetOn(false);
   m_guibuttonrun->SetEnabled(true);
   m_label_runnumber->SetText("failed");
   m_label_runnumber->Disable();
   m_guioptions_pedfileload->SetEnabled(true);
   m_guioptions_pedfileclear->SetEnabled(true);
   update_statusbar(msg.c_str(), 1); // print_progress(queuesize);

}


void CUserInterface::update_options_tab()
{
//	m_guioptions_configfile->SetText(m_config->config_file(), false);
//	m_guioptions_daq_ipaddress->SetText(m_config->daq_ip_address(), false);
//	m_guioptions_daq_ipport->SetText(m_config->daq_ip_port(), false);
//	m_guioptions_daq_logfile->SetText(m_config->log_filename(), false);
//	m_guioptions_daq_channelmapfile->SetText("", false);
//	m_guioptions_daq_pedestalfile->SetText(m_config->pedestal_filename(), false);
}


void CUserInterface::update_statusbar(const char* str, int part)
{
   if (!str || part > 3) {
      return;
   }

   lock_mutex();
   m_guistatusbar->SetText(str, part);
   unlock_mutex();
}



void CUserInterface::update_tab(int sel)
{
	if (sel != m_main_tabs->GetCurrent() ) {
		return;
	}
   switch (sel) {
      case TAB_UDP:
         draw_udp();
         break;
			
      case TAB_1D:
         draw_1d_data();
         break;
      case TAB_HITMAP:
         draw_hitmap();
         break;

      case TAB_STAT:
         draw_statistics();
         break;
			
      case TAB_2D:
         draw_2d_data();
         break;
         
      case TAB_DETCONFIG:
         draw_detconfig();
         break;
      default:
         break;
   }
	
	if (m_mx_published_detach_request) {
		internal_detach_mutex_published_data();
		m_mx_published_detach_request = false;
	}
}


void CUserInterface::draw_internals()
{
   return;
   
   lock_mutex();
   int icanvas = TAB_INTERNALS;

   m_canvas[icanvas]->Divide(2, 2);

   TH1* th1 = m_histograms[icanvas][0];
   if (!th1) {
      CThreadRoot::unlock_mutex(); return;
   }
   TH1F *h1 = dynamic_cast<TH1F*>(th1);
   if (!h1) {
      CThreadRoot::unlock_mutex(); return;
   }
   h1->Reset();
   h1->SetTitle("Data queues");
   h1->Fill(0.0, m_daq->internal_queue_size(0));
   h1->Fill(1.0, m_daq->internal_queue_size(1));
   h1->Fill(2.0, m_daq->internal_queue_size(2));
   h1->Fill(3.0, m_daq->internal_queue_size(3));

   //Rate
   TGraph* g1 = m_graphs[icanvas][0];
   if (!g1) {
      CThreadRoot::unlock_mutex(); return;
   }
//   TGraph *g1 = dynamic_cast<TGraph*>(g1);
//   if (!g1) {
//      return;
//   }
//   h1->Reset();
//   h1->SetTitle("Data rate");


   for (int ipoint = 1; ipoint < RATEGRAPH_MAXPOINTS; ++ipoint) {
      Double_t xpoint = 0;
      Double_t ypoint = 0;
      g1->GetPoint(ipoint, xpoint, ypoint);
      g1->SetPoint(ipoint - 1, xpoint - 1, ypoint);
   }
   Double_t ypoint = m_daq->current_rate();
   g1->SetPoint(RATEGRAPH_MAXPOINTS - 1, RATEGRAPH_MAXPOINTS - 1, ypoint);


   string canvasname = prepare_canvas(icanvas, 4, 2);

   m_canvas[icanvas]->cd(1);
   h1->Draw();
   m_canvas[icanvas]->cd(2);
   g1->Draw("AL*");


   for (int i = 0; i < 2; ++i) {
      std::stringstream out;
      out << canvasname << 1;
      string padname = out.str();
      TPad* pad = (TPad*) m_canvas[icanvas]->GetPrimitive(padname.c_str());
      if (pad) pad->Modified();
   }

   m_canvas[icanvas]->Modified();
   m_canvas[icanvas]->Update();

   CThreadRoot::unlock_mutex();
}




void CUserInterface::draw_udp()
{
   m_tab_needs_redraw[TAB_UDP] = false;
   m_gui_tab_contents[TAB_UDP]->draw();
   m_gui_tab_contents[TAB_UDP]->update();
   
   /////////////////////////////////////////////////////////////////////////////
   
   /*
    int icanvas = TAB_UDP;

   FreeClear(m_graphs[icanvas]);
   m_tab_needs_redraw[icanvas] = false;

   int request_id = PUBLISHER_REQUEST_ID_UDP;
   //lock
//   bool locked = false;
//   if (m_mx_published) {
//      m_mx_published->lock(); locked = true;
//   }
   const vector<TH1*> hists(m_publisher->get_published_data(request_id));
   for (vector<TH1*> :: const_iterator ih = hists.begin() ; ih != hists.end(); ++ih) {
      m_graphs[icanvas].push_back(new TGraph(*ih));
   }
   //unlock
 //  if (locked && m_mx_published) {
//		cout << "draw1d: m_mx_published->unlock()" << endl;
//		m_mx_published->unlock();
//	}

   if (!m_canvas[icanvas]) {
      Error("Draw(vector<>)", "fcanvas = 0");
      //CThreadRoot::unlock_mutex();
      return;
   }
   std::vector<TGraph*>* graphvec = &m_graphs[icanvas];
   if (!graphvec || !graphvec->size()) {
      //Error("Draw(vector<>)", "hv or hv.size() = 0");
      //CThreadRoot::unlock_mutex();
      return;
   }



   size_t vecsize = graphvec->size();
//   float fontsize = 0.02 + 0.01 * (float)vecsize;
   float fontsize = 0.02 + 0.007 * (float)vecsize/2.0;

   int columns = (int) std::ceil(double(vecsize) / 4.0);

   string canvasname = prepare_canvas(icanvas, vecsize, columns);
   for (unsigned i = 0; i < vecsize; i++) {
      m_canvas[icanvas]->cd(i + 1);
      m_graphs[icanvas][i]->GetXaxis()->SetLabelSize(fontsize);
      m_graphs[icanvas][i]->GetYaxis()->SetLabelSize(fontsize);
      m_graphs[icanvas][i]->Draw("ALP");

      m_canvas[icanvas]->cd(i + 1);
      std::stringstream out;
      out << canvasname << i + 1;
      string padname = out.str();
      TPad* pad = (TPad*) m_canvas[icanvas]->GetPrimitive(padname.c_str());
      if (pad) pad->Modified();
   }
   m_canvas[icanvas]->cd();
   m_canvas[icanvas]->Update();
   //CThreadRoot::unlock_mutex();
    
    */
   
}


void CUserInterface::draw_1d_data()
{
   int icanvas = TAB_1D;
   //reset_histograms(m_histograms[icanvas]);
   FreeClear(m_histograms[icanvas]);
   m_tab_needs_redraw[icanvas] = false;

   int request_id = PUBLISHER_REQUEST_ID_1D;
   //lock
//   bool locked = false;
//   if (m_mx_published) {
//      m_mx_published->lock(); locked = true;
//   }
   const vector<TH1*> hists (m_publisher->get_published_data(request_id));
   int jj = 0;
   for (vector<TH1*> :: const_iterator ih = hists.begin() ; ih != hists.end(); ++ih, ++jj) {
      TH1F* hist_qmax = dynamic_cast<TH1F*>(*ih);
      TH2F* hist_tqmax = dynamic_cast<TH2F*>(*ih);

      if (hist_qmax) {
         TH1F* h1 = (TH1F*) hist_qmax->Clone("hist_qmax");
         h1->SetDirectory(0);
         m_histograms[icanvas].push_back(h1);

      }
      else if (hist_tqmax) {
         TH2F* h2 = (TH2F*) hist_tqmax->Clone("hist_tqmax");
         h2->SetDirectory(0);
         m_histograms[icanvas].push_back(h2);
      }
   }
   //unlock
//   if (locked && m_mx_published) {
//		cout << "draw1d: m_mx_published->unlock()" << endl;
//		m_mx_published->unlock();
//	}


   //plot data
   if (!m_histograms[icanvas].size()) {
      //Error("Draw(vector)", "hv or hv.size() = 0");
      //CThreadRoot::unlock_mutex();
      return;
   }

   size_t vecsize = m_histograms[icanvas].size();
   if (!vecsize) {
      return;
   }
   float fontsize = 0.02 + 0.007 * (float)vecsize/2.0;
   
   int columns = 2;//(int) std::ceil(vecsize / 4);

   string canvasname = prepare_canvas(icanvas, vecsize, columns);

   int ii = 0;
   for (std::vector<TH1*>::iterator ih = m_histograms[icanvas].begin() ; ih != m_histograms[icanvas].end(); ++ih, ++ii) {
      m_canvas[icanvas]->cd(ii + 1);
      TH1F* hist_qmax = dynamic_cast<TH1F*>(*ih);
      TH2F* hist_tqmax = dynamic_cast<TH2F*>(*ih);

      if (hist_qmax) {
         hist_qmax->GetXaxis()->SetLabelSize(fontsize);
         hist_qmax->GetYaxis()->SetLabelSize(fontsize);
         hist_qmax->SetFillColor(38);
         //hist_qmax->SetBarWidth(2.0+fontsize);
         hist_qmax->Draw("");
      }
      else if (hist_tqmax) {
         hist_tqmax->GetXaxis()->SetLabelSize(fontsize);
         hist_tqmax->GetYaxis()->SetLabelSize(fontsize);
         hist_tqmax->SetMarkerStyle(kFullDotMedium);
         hist_tqmax->Draw("SCAT");
      }

      m_canvas[icanvas]->cd(ii + 1);
      std::stringstream out;
      out << canvasname << ii + 1;
      string padname = out.str();
      TPad* pad = (TPad*) m_canvas[icanvas]->GetPrimitive(padname.c_str());
      if (pad) {
         if (ii % 2) pad->SetGrid(1, 1);
         pad->Modified();
      }
   }

   m_canvas[icanvas]->cd();
   m_canvas[icanvas]->Update();
   //CThreadRoot::unlock_mutex();

}


void CUserInterface::draw_2d_data()
{
   int icanvas = TAB_2D;
   //reset_histograms(m_histograms[icanvas]);
   FreeClear(m_histograms[icanvas]);
   m_tab_needs_redraw[icanvas] = false;
   int request_id = PUBLISHER_REQUEST_ID_2D;
   //lock
	//   bool locked = false;
	//   if (m_mx_published) {
	//      m_mx_published->lock(); locked = true;
	//   }
   const vector<TH1*> hists (m_publisher->get_published_data(request_id));
   
   for (vector<TH1*> :: const_iterator ih = hists.begin(); ih != hists.end(); ++ih) {
      TH2F* h2 = dynamic_cast<TH2F*>(*ih);
      if (h2) {
         TH2F* h2c = (TH2F*) h2->Clone("hist_q");
         h2c->SetDirectory(0);
         m_histograms[icanvas].push_back(h2c);
      }
	}
   //unlock
	//   if (locked && m_mx_published) {
	//		cout << "draw1d: m_mx_published->unlock()" << endl;
	//		m_mx_published->unlock();
	//	}
	
	
   //plot data
   if (!m_histograms[icanvas].size()) {
      //Error("Draw(vector)", "hv or hv.size() = 0");
      //CThreadRoot::unlock_mutex();
      return;
   }
	
   size_t vecsize = m_histograms[icanvas].size();
   if (!vecsize) {
      return;
   }
   size_t columns = 2;//(int) std::ceil(vecsize / 4);
	
   string canvasname = prepare_canvas(icanvas, vecsize, columns);
	
   int ii = 0;
   for (std::vector<TH1*>::iterator ih = m_histograms[icanvas].begin() ; 
        ih != m_histograms[icanvas].end(); ++ih, ++ii) {
      
      m_canvas[icanvas]->cd(ii + 1);
      TH2F* h2 = dynamic_cast<TH2F*>(*ih);
      if (h2) {
         h2->GetXaxis()->SetLabelSize(0.03);
         h2->GetYaxis()->SetLabelSize(0.03);
         h2->Draw("lego2");
      }
      m_canvas[icanvas]->cd(ii + 1);
      std::stringstream out;
      out << canvasname << ii + 1;
      string padname = out.str();
      TPad* pad = (TPad*) m_canvas[icanvas]->GetPrimitive(padname.c_str());
      if (pad) {
         pad->Modified();
      }
   }
	
   m_canvas[icanvas]->cd();
   m_canvas[icanvas]->Update();
   //CThreadRoot::unlock_mutex();
	
}


void CUserInterface::draw_hitmap() {
   m_tab_needs_redraw[TAB_HITMAP] = false;
   m_gui_tab_contents[TAB_HITMAP]->draw();
   m_gui_tab_contents[TAB_HITMAP]->update();
}

void CUserInterface::draw_detconfig()
{
   m_tab_needs_redraw[TAB_DETCONFIG] = false;
   m_gui_tab_contents[TAB_DETCONFIG]->draw();
   m_gui_tab_contents[TAB_DETCONFIG]->update();
}

void CUserInterface::draw_statistics()
{
	int icanvas = TAB_STAT;
	int request_id = PUBLISHER_REQUEST_ID_STATISTICS;
   FreeClear(m_histograms[icanvas]);
   m_tab_needs_redraw[icanvas] = false;
	
   const vector<TH1*> hists (m_publisher->get_published_data(request_id));

	//copy data
	int jj = 0;
	for (vector<TH1*> :: const_iterator ih = hists.begin() ; ih != hists.end(); ++ih, ++jj) {
      TH1F* h0 = dynamic_cast<TH1F*>(*ih);
		if (!h0) {
			continue;
		}
		if (jj%3 == 0) {
			TH1F* h0c = (TH1F*) h0->Clone("hist_qmax_strip");
         h0c->SetDirectory(0);
         m_histograms[icanvas].push_back(h0c);
		}
		else if (jj%3 == 1) {
			TH1F* h0c = (TH1F*) h0->Clone("hist_tqmax");
         h0c->SetDirectory(0);
         m_histograms[icanvas].push_back(h0c);
		}
		else if (jj%3 == 2) {
			TH1F* h0c = (TH1F*) h0->Clone("hist_N_qmax");
         h0c->SetDirectory(0);
         m_histograms[icanvas].push_back(h0c);
		}
	}
	
	
	//draw
   if (!m_histograms[icanvas].size()) {
      return;
   }
	size_t vecsize = m_histograms[icanvas].size();
   if (!vecsize) {
      return;
   }
   float fontsize = 0.02 + 0.005 * (float)vecsize/3.0;

   
   size_t columns = 3;//(int) std::ceil(vecsize / 4); //TODO: better: numofhists/numofchambers (= 3)
	string canvasname = prepare_canvas(icanvas, vecsize, columns);

	int ii = 0;
   for (std::vector<TH1*>::iterator ih = m_histograms[icanvas].begin() ; ih != m_histograms[icanvas].end(); ++ih, ++ii) {
      m_canvas[icanvas]->cd(ii + 1);
		
      TH1F* h0 = dynamic_cast<TH1F*>(*ih);
//      h0->SetLabelSize(0.1, "X");
//      h0->SetLabelSize(0.1, "Y");
		h0->GetXaxis()->SetLabelSize(fontsize);
		h0->GetYaxis()->SetLabelSize(fontsize);
		h0->SetFillColor(38);
		//h0->SetBarWidth(1.0+fontsize);
		h0->Draw("");
		
      m_canvas[icanvas]->cd(ii + 1);
      std::stringstream out;
      out << canvasname << ii + 1;
      string padname = out.str();
      TPad* pad = (TPad*) m_canvas[icanvas]->GetPrimitive(padname.c_str());
      if (pad) {
         pad->Modified();
      }
   }
	
   m_canvas[icanvas]->cd();
   m_canvas[icanvas]->Update();
	
}


void CUserInterface::reset_histograms(std::vector<TH1*>& vec)
{
   for (vector<TH1*> :: iterator ih = vec.begin(); ih != vec.end(); ++ih) {
      (*ih)->Reset();
   }
}


void CUserInterface::draw_pedestals()
{

//	m_ped_graphs[0]->Clear();
//	m_ped_graphs[0]->SetTitle( m_config->chip_list()[0].c_str() );

   int icanvas = 1;

   const std::map<int, double>* data = m_config->pedestal_mean_map();
   const std::vector<std::string > chip_names =  m_config->defined_chip_names();
   const std::vector<CDetChip*>	defined_chips = m_config->defined_chips();
   const std::vector<int> chip_ids = m_config->defined_chip_ids();

   for (std::vector<TGraph*>::iterator ig = m_graphs[icanvas].begin() ; ig != m_graphs[icanvas].end(); ++ig) {
      int n = (*ig)->GetN();
      for (int i = 0; i < n ; i++)
         (*ig)->RemovePoint(i);
   }


   int ii = 0;
   vector<int> point_counter(chip_ids.size(), 0);

   for (map<int, double>::const_iterator ichan = data->begin(); ichan != data->end(); ++ichan, ++ii) {
      int apv_id = CApvEvent::chipId_from_chId(ichan->first); //int apv_id = (ichan->first >> 8) & 0xFF;
      int ch_no  = CApvEvent::chanNo_from_chId(ichan->first); //int ch_no  = (ichan->first) & 0xFF;
      int pos = 0;
      bool found_id = false;
      for (std::vector<int> :: const_iterator icid = chip_ids.begin(); icid != chip_ids.end(); ++icid, ++pos) {
         if (*icid == apv_id) {
            found_id = true;
            break;
         }
      }
      if (!found_id) {
         // cerr << "CUserInterface::draw_pedestals() not found apvid = " << apv_id << endl;
         continue;
      }
      TGraphErrors *ge = dynamic_cast<TGraphErrors*>(m_graphs[icanvas][pos]);
      //cout << "setting: "<< pos << " @ " << point_counter[pos] << " apv_id:" << apv_id << " ch_no:" << ch_no << " val:" << ichan->second << endl;
      ge->SetPoint(point_counter[pos], ch_no, ichan->second);

      double stdev = 0.0;
//      bool found = 
		m_config->pedestal_stdev(ichan->first, stdev);
      ge->SetPointError(ii, 0, stdev);
      ge->SetTitle(chip_names[pos].c_str());
      point_counter[pos]++;
   }




   if (!m_canvas[icanvas]) {
      Error("Draw(vector<>)", "fcanvas = 0");
      return;
   }
   std::vector<TGraph*>* graphvec = &m_graphs[1];
   if (!graphvec || !graphvec->size()) {
      Error("Draw(vector<>)", "hv or hv.size() = 0");
      return;
   }
   size_t vecsize = graphvec->size();
   size_t columns = std::ceil(vecsize / 4);
//   int selected = m_guichipselection->GetSelected();
//   if (selected > 0) {
//      string canvasname = prepare_canvas(icanvas, 1, 1);
//      m_canvas[icanvas]->cd();
//      m_graphs[1][selected-1]->Draw("ALP");
//      m_canvas[icanvas]->cd();
//      m_canvas[icanvas]->Update();
//      return;
//   }

   string canvasname = prepare_canvas(icanvas, vecsize, columns);
   for (unsigned i = 0; i < vecsize; i++) {
      m_canvas[icanvas]->cd(i + 1);
      m_graphs[1][i]->Draw("ALP");

      m_canvas[icanvas]->cd(i + 1);
      std::stringstream out;
      out << canvasname << i + 1;
      string padname = out.str();
      TPad* pad = (TPad*) m_canvas[icanvas]->GetPrimitive(padname.c_str());
      if (pad) pad->Modified();
   }
   m_canvas[icanvas]->cd();
   m_canvas[icanvas]->Update();



}


std::string CUserInterface::prepare_canvas(int icanvas, size_t vecsize, size_t columns)
{
   if (columns < 1) columns = 1;
   //prepare canvas

   if (m_pads[icanvas].size() != vecsize) {
      m_canvas[icanvas]->Clear();
      m_pads[icanvas].clear();

      if (vecsize > 1) {
         int n = (int)std::ceil((double)(vecsize) / (double)columns);
         m_canvas[icanvas]->Divide((int)columns, n);
      }
      else {
         m_canvas[icanvas]->Divide(1, 1);
      }

//     TList* subpadlist = m_canvas[icanvas]->GetListOfPrimitives();
//		TObjLink *lnk = subpadlist->FirstLink();
//		while (lnk) {
//			TObject* ob = lnk->GetObject();
//			TPad* apad = dynamic_cast<TPad*> (ob);
//			if (!apad) {
//				cout << "apad == NULL" << endl;
//			}
//			m_pads[icanvas].push_back(apad);
//
//			lnk = lnk->Next();
//		}
//
//
//      int numpads = subpadlist->GetSize();
//      cout << "cleared numpads = " << numpads << "vecsize=" << vecsize << endl;

   }
   std::stringstream canvasname;
   canvasname << m_canvas[icanvas]->GetName() << "_";
   return canvasname.str();
}

//
//void CUserInterface::draw(std::vector<TH1*>* histvec, int icanvas, int columns)
//{
//   //	std::vector<TH1*>	fvechist[NUMBEROFTABS]; // one for each tab
//   if (!m_canvas[icanvas]) {
//      Error("Draw(vector<>)", "fcanvas = 0");
//      return;
//   }
//   if (!histvec || !histvec->size()) {
//      Error("Draw(vector<>)", "hv or hv.size() = 0");
//      return;
//   }
//   unsigned hvsize = histvec->size();
//
//
//   string canvasname = prepare_canvas(icanvas, hvsize, columns);
//
//   //construct a member copy to make hist persistent on updating window
//   for (unsigned i = 0; i < hvsize; i++) {
//      m_canvas[icanvas]->cd(i + 1);
//      //TH1F* h = (TH1F*) histvec->at(i).Clone();
//      histvec->at(i)->Draw();
//
//      //histvec[icanvas].push_back(h);
//   }
//
//   //set pads Modified()
//   for (unsigned i = 0; i < hvsize; i++) {
//      m_canvas[icanvas]->cd(i + 1);
//      std::stringstream out;
//      out << canvasname << i + 1;
//      string padname = out.str();
//      TPad* pad = (TPad*) m_canvas[icanvas]->GetPrimitive(padname.c_str());
//      if (pad) pad->Modified();
//   }
//   m_canvas[icanvas]->cd();
//   m_canvas[icanvas]->Update();
//
//}




