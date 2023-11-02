/*
 *  CDetReadout.h
 *  DetGeometry
 *
 *  Created by Marcin Byszewski on 12/16/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */

#ifndef CDetReadout_h
#define CDetReadout_h


#include "CDetElement.h"
#include "CDetChip.h"

#include "TVector2.h"
#include "TVector3.h"

#include <string>
#include <vector>
#include <utility>

class CConfiguration;
class CDetChamber;
class CDetChip;
class TH1;
class CMMEvent;

class CDetAbstractReadout : public CDetElement
{
public:
		
public:
   CDetAbstractReadout(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  double strip_angle_deg, double strip_pitch, std::pair<double, double> rng);
	virtual ~CDetAbstractReadout();
   virtual CDetAbstractReadout* clone() const = 0;
   virtual TVector2 get_strip_crossing(int strip1, const CDetAbstractReadout* rd2, int strip2) const = 0;
   virtual TVector2 get_strip_crossing_at_x(double xpos, int strip2) const = 0;
   virtual TVector2 get_strip_crossing_at_y(double ypos, int strip2) const = 0;
   virtual TVector2 get_strip_crossing_at_vec(const TVector2& vec1, int strip2) const = 0;
   
   TVector2 get_strip_line_vec(double strip_number) const ;

   void add_chip(CDetChip* pchip) { m_chips.push_back(pchip); }
   const std::vector<CDetChip*>& get_chips() const { return m_chips;}
   const std::pair<double, double>& get_strip_range() const { return  m_strip_range; }
   
   std::vector<TH1*> create_th1_qt(const CMMEvent* const  mmevt) const;
   TH1* create_th2_qt(const CMMEvent* const  mmevt) const;
   TH1* create_stat_qmax() const;
   TH1* create_stat_tqmax(const CMMEvent* const  mmevt) const;
   TH1* create_stat_nqmax() const;
   void fill_stat(TH1* hqmax, TH1* htqmax, TH1* hnqmax, const CMMEvent* const  mmevt, const CConfiguration* const conf) const;
   
   
protected:
	const CDetChamber* m_chamber;
   double m_phi;
   double m_pitch;
	double m_vpitch;
   std::pair<double, double> m_strip_range;
   std::vector<CDetChip*> m_chips;
  
   
public:
   bool has_chip(int chipId) const {
      std::vector<CDetChip*> :: const_iterator found =  std::find_if(m_chips.begin(), m_chips.end(),
                                                                     std::bind2nd(std::mem_fun(&CDetChip::isequal_chipid), chipId)  );
      return  (found != m_chips.end());      
   }
	
};




class CReadout : public CDetAbstractReadout {
public:
   CReadout(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  double strip_angle_deg, double strip_pitch, std::pair<double, double> rng);
   virtual ~CReadout() {};
   virtual CReadout* clone() const { return new CReadout(*this); }
   virtual TVector2 get_strip_crossing(int strip1, const CDetAbstractReadout* rd2, int strip2) const;
   virtual TVector2 get_strip_crossing_at_x(double xpos, int strip2) const;
   virtual TVector2 get_strip_crossing_at_y(double ypos, int strip2) const;
   virtual TVector2 get_strip_crossing_at_vec(const TVector2& vec1, int strip2) const;

};


class CReadoutH : public CDetAbstractReadout {
public:
   CReadoutH(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  double strip_pitch, std::pair<double, double> rng);
   virtual ~CReadoutH() {};
   virtual CReadoutH* clone() const { return new CReadoutH(*this);  }
   virtual TVector2 get_strip_crossing(int strip1, const CDetAbstractReadout* rd2, int strip2) const;
   virtual TVector2 get_strip_crossing_at_x(double xpos, int strip2) const;
   virtual TVector2 get_strip_crossing_at_y(double ypos, int strip2) const;
   virtual TVector2 get_strip_crossing_at_vec(const TVector2& vec1, int strip2) const;

};


class CReadoutV : public CDetAbstractReadout {
public:
   CReadoutV(const CDetChamber *pchamber, size_t id_num, const std::string& rdname,  double strip_pitch, std::pair<double, double> rng);
   virtual ~CReadoutV() {};
   virtual CReadoutV* clone() const { return new CReadoutV(*this); }
   virtual TVector2 get_strip_crossing(int strip1, const CDetAbstractReadout* rd2, int strip2) const;
   virtual TVector2 get_strip_crossing_at_x(double xpos, int strip2) const;
   virtual TVector2 get_strip_crossing_at_y(double ypos, int strip2) const;
   virtual TVector2 get_strip_crossing_at_vec(const TVector2& vec1, int strip2) const;
};



#endif
