/*
 *  CUserInterface.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 12/8/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

#ifndef CUserInterface_h
#define CUserInterface_h

#include "CConfiguration.h"
#include "MBtools.h"
#include "CThreadRoot.h"
//#include "CEvent.h" // can NOT include it here - rootcint errors - 


#include <TGButton.h>
#include <TGFrame.h>
#include <TGStatusBar.h>
#include <TH1.h>
#include <TMultiGraph.h>

#include <vector>
#include <string>
#include <map>




class CMMDaq;
class CEvent;
class CConfiguration;
class CPublisher;
class CMutex;
class CGuiTabContents;

class TObject;
class TCanvas;
class TCondition;
class TGMainFrame;
class TGStatusBar;
class TGCanvas;
class TGComboBox;
//class TGCheckButton;
class TGHorizontalFrame;
class TGLabel;
class TGListBox;
class TGListTree;
class TGNumberEntry;

class TGTab;
class TGTextButton;
class TGTextEntry;
class TGTextView;
class TGraph;
class TGVerticalFrame;
class TGViewPort;
class TMultiGraph;
class TPad;
class TRootEmbeddedCanvas;
class TH1;
class TH1F;
class TTimer;
class TGListTreeItem;


// from http://root.cern.ch/root/html/tutorials/gui/buttongroupState.C.html
//class IDList {	
//private:
//   Int_t nID;   // creates unique widget's IDs
//public:
//   IDList() : nID(0) {}
//   ~IDList() {}
//   Int_t GetUnID(void) { return ++nID; }
//};



class CUserInterface :  public TGMainFrame, public CThreadRoot
{
public:
	CUserInterface(CMMDaq* daq, CConfiguration* config);
	~CUserInterface();
	//void attach_signal(pthread_cond_t* action) { m_action_data_published = action;}

	//void handle_published_data(int datatype);
	bool write_enabled() { return m_guiwritefileswitch->IsDown();}
	void update_statusbar(const char* str, int part);
	void update_options_tab();

	void attach(CPublisher* pub) { m_publisher = pub;}
	void detach(CPublisher* pub);
	void attach_signal_request(TCondition* cond_newrequest) { m_cond_newrequest = cond_newrequest;}
	void attach_signal_newdata(TCondition* cond) { m_cond_newdataready = cond;}
	void detach_signal_request(TCondition* cond); 
	void detach_signal_newdata(TCondition* cond); 
	
	void attach_mutex_published_data(CMutex* mut) { m_mx_published = mut;}
	void detach_mutex_published_data(CMutex* mut);
	void internal_detach_mutex_published_data()		{ m_mx_published = 0;	}
	
	void controls_run_started(int number);
	void controls_run_stopped();
	void controls_run_start_failed(const std::string& msg);
	
   const CPublisher* get_publisher() const { return m_publisher;} 
	CConfiguration* get_config() { return m_config;}
   int get_tree_checked_items(std::vector<TGListTreeItem*>& vec);
   int get_item_checked_descendants(TGListTreeItem* first_item, std::vector<TGListTreeItem*>& vec);

   
   friend class CGuiTabContents;
private:
	//members - main
	CMMDaq*				m_daq;
	CConfiguration*	m_config;
	CPublisher*			m_publisher;
	TTimer*				m_timer_subscriber;
	int					m_timer_subscriber_last_update;
	TCondition*			m_cond_newrequest; //is sent to CPublisher to ask for new data
	TCondition*			m_cond_newdataready;
	CMutex*				m_mx_published;
	bool					m_mx_published_detach_request;
	std::map <int, std::pair<unsigned,unsigned> > m_defined_chamber_strip_ranges; //chamber_id -> stipLO, stripHI
	
	std::map<unsigned,unsigned>	m_lookup_chip_plot;
	std::map<unsigned,unsigned>	m_lookup_chamber_plot;
	
   

	//CConfiguration::runtype_t		m_run_type;
	
	
	//members - flow control
	bool m_do_show_next_event;
	std::vector<bool> m_tab_needs_redraw;
	
   std::vector<const CDetChip*>     m_chips_to_draw;
   std::vector<const CDetChamber*>  m_chambers_to_draw;
   
	//members - GUI-related
	IDList IDs;           // Widget IDs generator	
	//gui common above tabs
	TGMainFrame*			m_mainframe;
	TGVerticalFrame*		m_verticalframe;
	TGHorizontalFrame*	m_upper_h_frame;
	TGHorizontalFrame*	m_lower_h_frame;
	//tree:
	TGCanvas*				m_tree_canvas;
	TGViewPort*				m_tree_viewport;
	TGListTree*				m_tree_listbox;
	TGTab*					m_main_tabs;
	//gui per tab
	std::vector<TGCompositeFrame*>      m_compositeframe;       
	std::vector<TRootEmbeddedCanvas*>	m_rootembeddedcanvas; 
   std::vector<TCanvas*>					m_canvas;				 
	std::vector<std::vector<TPad*> >		m_pads;
	std::vector<std::vector<TH1*> >		m_histograms;			 // one for each tab
	std::vector<std::vector<TGraph*> >	m_graphs;	
   std::vector<CGuiTabContents*>       m_gui_tab_contents;
	
	TGStatusBar*	m_guistatusbar;
	TGListBox*		m_guiruntypeselect;
	TGTextButton*	m_guibuttonrun;
	TGTextButton*	m_guibuttonpause;
	TGTextButton*	m_guibuttonstop;
	TGTextButton*	m_guibuttonexit;
	TGTextButton*	m_guibuttonnext;
	TGTextEntry*	m_guicomment;
	TGCheckButton* m_guimonitorswitch;
	TGCheckButton* m_guiwritefileswitch;
//	TGComboBox*		m_guichipselection;
	TGLabel*			m_label_runnumber;
//	TGTextView*		m_guilog;
//	TGCheckButton* m_guicontroller ;
	
//	//main config file group tab
//	TGTextEntry* m_guioptions_configfile;
//	TGTextButton* m_guioptions_buttonload;
//	TGTextButton* m_guioptions_buttonsave;
//	//daq group
//	TGTextEntry *m_guioptions_daq_ipaddress;
//	TGTextEntry *m_guioptions_daq_ipport;
//	TGTextEntry *m_guioptions_daq_logfile;
//	TGTextEntry *m_guioptions_daq_channelmapfile;
//	TGTextEntry *m_guioptions_daq_pedestalfile;
	
	//
	TGTextEntry* m_guioptions_pedfile;
	TGTextButton* m_guioptions_pedfileload;
   TGTextButton* m_guioptions_pedfileclear;
	TGTextButton* m_guioptions_pedfilesave;
	TGNumberEntry* m_guioptions_zerofactor;
   
   TGCheckButton* m_guioptions_cmode_switch;
	TGNumberEntry* m_guioptions_cmode_factor;
private:
	CUserInterface(CUserInterface& );
	CUserInterface& operator= (CUserInterface&);
	void construct_window();
	void construct_options_tab(TGCompositeFrame * tabmainframe);
	void construct_histograms();
	void construct_tree();
	
	
	void request_data_update();
	
	void reset_histograms(std::vector<TH1*>& vec);
   void make_list_of_items_to_draw();

	
	//void check_published_data();

	std::string prepare_canvas(int icanvas, size_t vecsize, size_t columns);
   void update_tabs();
	void update_tab(int tab);
   void refresh_tabs();
	void draw_internals();
	//void draw(std::vector<TH1*>* histvec, int icanvas, int columns);
	void draw_udp();
	void draw_1d_data();
	void draw_2d_data();
   void draw_hitmap();
	void draw_statistics();
	void draw_pedestals();
   void draw_detconfig();

	
	virtual int execute_root_thread() {
      return main((void*)0);
   };
	int main(void*);
	
public:

	void timed_exec(); //TTimer: http://root.cern.ch/root/html/TTimer
	const int gui_selected_run_type() const ;
	
	//gui callbacks
	void CloseWindow();
	void GuiDoExit(); //must be public
	void GuiDoRun();
	void GuiDoPause();
	void GuiDoStop();
	void GuiShowNextEvent() ;
	void GuiDoMonitor();
	void GuiDoOptionsPedestalLoad();
	void GuiDoOptionsPedestalClear();
	void GuiDoOptionsSetZeroFactor();
	void GuiDoOptionsConfigLoad();
	void GuiDoOptionsConfigSave();
   void GuiDoOptionsCommonModeCorrection();
   void GuiDoOptionsCommonModeFactor();
	
	void gui_handle_tree_change(TObject*, Bool_t);
	void gui_handle_select_runtype(Int_t);
	void gui_handle_write();
//	void gui_handle_select_chip(Int_t sel);

	bool gui_save_data_checked() { return m_guiwritefileswitch->IsDown();}
	const std::string gui_comment_line() const;
	const bool gui_run_paused() const { return m_guibuttonpause->IsDown();}

	ClassDef(CUserInterface, 0)
	//virtual void signal_new_data(); // *SIGNAL*
};



#endif
