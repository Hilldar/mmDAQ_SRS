/*
 *  CDetReadout.cpp
 *  DetGeometry
 *
 *  Created by Marcin Byszewski on 12/16/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

#include "CDetReadout.h"
#include "CDetChamber.h"
#include "CMMEvent.h"
#include "CConfiguration.h"

#include "TMath.h"
#include "TH1.h"
#include "TH2.h"

#include <cmath>


CDetAbstractReadout::CDetAbstractReadout(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  double strip_angle_deg, double strip_pitch, std::pair<double, double> rng)
: CDetElement((int)id_num, rdname), m_chamber(pchamber), m_phi(strip_angle_deg*TMath::DegToRad()), m_pitch(strip_pitch), m_vpitch(strip_pitch), m_strip_range(rng), m_chips()
{
//   if (std::fabs(std::cos(m_phi)) < EPS) { // Vertical Y strips (recalcualted in CReadoutV) Y
//      m_vpitch = m_pitch;
//   }
//   else {
//      m_vpitch = -m_pitch/std::cos(m_phi);
//   }
   
   m_vpitch = std::fabs(m_pitch);
}


CReadout::CReadout(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  
                   double strip_angle_deg, double strip_pitch, std::pair<double, double> rng)
: CDetAbstractReadout(pchamber, id_num, rdname, strip_angle_deg, strip_pitch, rng)
{ 
   if (std::fabs(m_phi - TMath::Pi()) < EPS) {
      throw std::string("CReadout should be CReadoutV " );
   }
   else if (std::fabs(m_phi) < EPS) {
      throw std::string("CReadout should be CReadoutH ");
   }
}


//strip counting up along axis Y, --> same as Readout at angle 180deg
CReadoutH::CReadoutH(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  double strip_pitch, std::pair<double, double> rng)
: CDetAbstractReadout(pchamber, id_num, rdname, 180.0, strip_pitch, rng)
{
   m_vpitch = m_pitch;
}


//strip counting up along axis X, --> same as Readout at angle 90deg
CReadoutV::CReadoutV(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  double strip_pitch, std::pair<double, double> rng)
: CDetAbstractReadout(pchamber, id_num, rdname, 90.0, strip_pitch, rng)
{
   m_vpitch = m_pitch; //TODO: shodn't be * -1 <-- counting left right->against Y axis
}



CDetAbstractReadout::~CDetAbstractReadout() 
{

}

TVector2 CDetAbstractReadout::get_strip_line_vec(double strip_number) const 
{
   double offset = strip_number * m_vpitch;
   return  TVector2(offset, m_phi);
}


//constructors end
////////////////////////////////////////////////////////////////////////////////



std::vector<TH1*> CDetAbstractReadout::create_th1_qt(const CMMEvent* const  mmevt) const
{
   std::vector<TH1*> hvec;
   
   if (!m_chamber || !mmevt) {
      return hvec;
   }
   int max_tb_count = mmevt->max_timebin_count();
   int timebin_count = max_tb_count ? max_tb_count : 20;
   std::stringstream ss;
   ss << m_chamber->name() << m_name <<"_qmax"  << "(" << mmevt->event_number() << ")";
   TH1F* h1 = new TH1F(ss.str().c_str(),ss.str().c_str(),
                       m_strip_range.second - m_strip_range.first + 1,
                       m_strip_range.first, m_strip_range.second+1);
//   h1->SetName(ss.str().c_str());
//   h1->SetTitle(ss.str().c_str());
   h1->SetBins(m_strip_range.second - m_strip_range.first + 1, m_strip_range.first, m_strip_range.second+1);
   ss.str("");
   ss << m_chamber->name() << m_name <<"_tqmax"  << "(" << mmevt->event_number() << ")";

   TH2F* h2 = new TH2F(ss.str().c_str(),ss.str().c_str(),
                       m_strip_range.second - m_strip_range.first, m_strip_range.first, m_strip_range.second , 
                       timebin_count+1, 0, timebin_count+1);
//   h2->SetName(ss.str().c_str());
//   h2->SetZTitle(ss.str().c_str());
//   h2->SetBins(m_strip_range.second - m_strip_range.first, m_strip_range.first, m_strip_range.second , timebin_count, 0, timebin_count);
   
   const std::list< CEvent* > events = mmevt->apv_events();
   for (std::list<CEvent*>::const_iterator ievt = events.begin(); ievt != events.end(); ++ievt) {
      (*ievt)->fill_th1_qt(h1, h2, this);
   }
   hvec.push_back(h1);
   hvec.push_back(h2);
   return hvec;
};


TH1* CDetAbstractReadout::create_th2_qt(const CMMEvent* const  mmevt) const 
{
   if (!m_chamber || !mmevt) {
      return new TH2F();
   }
   
   int max_tb_count = mmevt->max_timebin_count();
   int timebin_count = max_tb_count ? max_tb_count : 20;
   std::stringstream ss;
   ss << m_chamber->name() << m_name << "_q"  << "(" << mmevt->event_number() << ")";
   TH2F* h2 = new TH2F();
   h2->SetName(ss.str().c_str());
   h2->SetZTitle(ss.str().c_str());
   h2->SetBins(m_strip_range.second - m_strip_range.first+1, m_strip_range.first, m_strip_range.second+1 , timebin_count, 0, timebin_count);
   
   const std::list< CEvent* > events = mmevt->apv_events();
   for (std::list<CEvent*>::const_iterator ievt = events.begin(); ievt != events.end(); ++ievt) {
      (*ievt)->fill_th2_qt(h2, this);
   }
   return h2;
}


TH1* CDetAbstractReadout::create_stat_qmax() const
{
   TH1F* h0 = new TH1F();
   std::stringstream ss;
   ss << m_chamber->name() << m_name << "_qmax_strip" ;
   h0->SetName(ss.str().c_str());
   h0->SetTitle(ss.str().c_str());
   h0->SetBins(m_strip_range.second - m_strip_range.first+1, m_strip_range.first, m_strip_range.second+1);
   h0->SetLabelSize(0.1, "X");
   h0->SetLabelSize(0.1, "Y");
   return h0;
}

TH1* CDetAbstractReadout::create_stat_tqmax(const CMMEvent* const  mmevt) const
{
   int max_tb_count = mmevt->max_timebin_count();
   int timebin_count = max_tb_count ? max_tb_count : 20;
   TH1F* h0 = new TH1F();
   std::stringstream ss;
   ss << m_chamber->name() << m_name << "_tqmax_strip" ;
   h0->SetName(ss.str().c_str());
   h0->SetTitle(ss.str().c_str());
   h0->SetBins(timebin_count, 0, timebin_count);
   h0->SetLabelSize(0.1, "X");
   h0->SetLabelSize(0.1, "Y");
   return h0;
}

TH1* CDetAbstractReadout::create_stat_nqmax() const
{
   TH1F* h0 = new TH1F();
   std::stringstream ss;
   ss << m_chamber->name() << m_name << "_N_qmax" ;
   h0->SetName(ss.str().c_str());
   h0->SetTitle(ss.str().c_str());
   h0->SetBins(100, 0, 2500);
   h0->SetLabelSize(0.1, "X");
   h0->SetLabelSize(0.1, "Y");
   return h0;
}


void CDetAbstractReadout::fill_stat(TH1* hqmax, TH1* htqmax, TH1* hnqmax, const CMMEvent* const  mmevt, const CConfiguration* const conf) const
{
	int maxq = 0;
	int maxqt = 0;

   //loop events, fill data
   int ii = 0;
   const std::list< CEvent* > events = mmevt->apv_events();
   for (std::list <CEvent*>::const_iterator ievt = events.begin(); ievt != events.end(); ++ievt, ++ii) {
      CApvEvent* apvevt = dynamic_cast<CApvEvent*>(*ievt);
      if (!apvevt) {
         continue;
      }
      
      
      //TODO: replace by m_reduced_strip_data from apvevent
      const std::map<int, std::vector<int16_t> > * const data2d = apvevt->apv2d_data();
      for (std::map<int, std::vector<int16_t> > :: const_iterator ichan = data2d->begin(); ichan != data2d->end(); ++ichan) {
         
         int stripNo = conf->strip_from_channel(ichan->first);
         
			if ((stripNo - 1) < 0)
            continue; // check for unmapped channels
                      //max values
         
         

         std::vector<int16_t>::const_iterator imax = max_element(ichan->second.begin(), ichan->second.end());
         int16_t valqmax = *imax;
         int16_t tbqmax  = imax - ichan->second.begin();
         int apv_id   = CApvEvent::chipId_from_chId(ichan->first); //int apv_id   = (ichan->first >> 8) & 0xFF;
         const CDetChip* thischip = conf->find_defined_chip(apv_id);
         
         if (m_chips.end() == std::find(m_chips.begin(), m_chips.end(), thischip)) {
            continue; // do not plot chips not from this readout
         }
			hqmax->Fill(stripNo, valqmax);
			//if(h1) h1->Fill(tbqmax); // all strips tqmax
			
			if (valqmax > maxq) {
				maxq = valqmax;
				maxqt = tbqmax;
			}
			//chamber_maxq[ichamber] = chamber_maxq[ichamber]>valqmax ? chamber_maxq[ichamber] : valqmax;
		}//for ichan
   }//for ievt
   
   htqmax->Fill(maxqt);
   hnqmax->Fill(maxq);
}


TVector2 CReadout::get_strip_crossing(int strip1, const CDetAbstractReadout* rd2, int strip2) const
{
   TVector2 vec1 = this->get_strip_line_vec(strip1);
   if (rd2) {
      return rd2->get_strip_crossing_at_vec(vec1, strip2);
   }
   else {
      return TVector2( vec1.X(), 0.0); // TODO: show at an angle, not at x axis
   }
}

TVector2 CReadoutH::get_strip_crossing(int strip1, const CDetAbstractReadout* rd2, int strip2) const
{
   double ypos = m_pitch * strip1;
   if (rd2) {
      return rd2->get_strip_crossing_at_y(ypos, strip2);
   }
   else {
      return TVector2( 0.0, ypos);
   }
}

TVector2 CReadoutV::get_strip_crossing(int strip1, const CDetAbstractReadout* rd2, int strip2) const
{
   double xpos = m_pitch * strip1;
   if (rd2) {
      return rd2->get_strip_crossing_at_x(xpos, strip2);
   }
   else {
      return TVector2( xpos, 0.0);
   }
}

TVector2 CReadout::get_strip_crossing_at_x(double xpos, int strip2) const
{
   //y=a1x+b1, x = (y-b)/a
   TVector2 vec2 = get_strip_line_vec(strip2);
   double a2 = tan(vec2.Y());
   double b2 = vec2.X();
   double ypos = a2*xpos + b2;
   return TVector2(xpos,ypos);  
}

TVector2 CReadout::get_strip_crossing_at_y(double ypos, int strip2) const
{
   //y=a1x+b1, x = (y-b)/a
   TVector2 vec2 = get_strip_line_vec(strip2);
   double a2 = tan(vec2.Y());
   double b2 = vec2.X();
   double xpos = (ypos - b2)/a2;
   return TVector2(xpos,ypos); 
}

TVector2 CReadout::get_strip_crossing_at_vec(const TVector2& vec1, int strip2) const
{
   //y=a1x+b1, x = (y-b)/a
   //y=a2x+b2, x = (y-b)/a
   // x = -(b2-b1)/(a2-a1)
   // y = 0.5*(a1+a2)* x   + 0.5*(b1+b2)
   TVector2 vec2 = get_strip_line_vec(strip2);
   
   double b1 = vec1.X();
   double b2 = vec2.X();
   double a1 = tan(vec1.Y());
   double a2 = tan(vec2.Y());
   
   double xpos = -(b2-b1)/(a2-a1);
   double ypos = 0.5*(a1+a2)*xpos + 0.5*(b1+b2);
   return TVector2(xpos,ypos);
}

TVector2 CReadoutH::get_strip_crossing_at_x(double xpos, int strip2) const
{
   return TVector2(xpos, m_pitch*strip2);
}

TVector2 CReadoutH::get_strip_crossing_at_y(double ypos, int strip2) const
{
   return TVector2(0.0, ypos);
}

TVector2 CReadoutH::get_strip_crossing_at_vec(const TVector2& vec1, int strip2) const
{
   double ypos = m_pitch*strip2;
   //vec(offset, phi)
   //y=ax+b, x = (y-b)/a
   double xpos = (ypos-vec1.X())/tan(vec1.Y());
   return TVector2(xpos, ypos);   
}


TVector2 CReadoutV::get_strip_crossing_at_x(double xpos, int strip2) const
{
   return TVector2(xpos, 0.0);
}

TVector2 CReadoutV::get_strip_crossing_at_y(double ypos, int strip2) const
{
   return TVector2(m_pitch*strip2, ypos);  
}

TVector2 CReadoutV::get_strip_crossing_at_vec(const TVector2& vec1, int strip2) const
{
   double xpos = m_pitch*strip2;
   //vec(offset, phi)
   //y=ax+b
   double ypos = tan(vec1.Y())*xpos + vec1.X();
   return TVector2(xpos, ypos);
}





