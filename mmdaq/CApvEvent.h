/*
 *  CApvEvent.h
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/28/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#ifndef CApvEvent_h
#define CApvEvent_h

#define APV_NUMBER_OF_CHANNELS 128
#define APV_PEDESTAL_VECTOR_SIZE 3

#include "CEvent.h"

#include <stdint.h>
#include <vector>
#include <map>
#include <fstream>
#include <string>
#include <iostream>
#include <limits>
#include <cmath>

class CConfiguration;
class CUDPData;
class CDetAbstractReadout;

class CStripDataReduced {
   int m_channel_id;
   int m_timebin;
   int m_max_q;
public:
   CStripDataReduced() : m_channel_id(0), m_timebin(0), m_max_q(0) {};
   CStripDataReduced(int chId, int timebin, int q) : m_channel_id(chId), m_timebin(timebin), m_max_q(q) {};
   int get_channel_id() const { return m_channel_id;}
   //   int get_timebin() const {return m_timebin;}
   int get_charge() const {return m_max_q;}
   void print() const { std::cout << "CStripDataReduced(" << m_channel_id << "," << m_timebin << "," << m_max_q << ")" << std::endl;}
};

 

class CApvEvent : public CEvent {
	
	
public:
	//general use functions to translate unique ID into seq. local numbers
	inline static const int make_channelId(int fecNo, int chipNo, int chanNo) { return (((fecNo<<4 | chipNo ) << 8) | chanNo); };
	inline static const int make_channelId(int chipId,int chanNo)              { return ((chipId << 8)               | chanNo); };
	inline static const int make_chipId(int fecNo, int chipNo)           { return (fecNo<<4 | chipNo ); } ;
	
	inline static const int fecNo_from_chipId(int chipId)  { return (chipId >> 4) & 0xFF  ;}; 	// should limit? & 0xFF
	inline static const int chipNo_from_chipId(int chipId) { return (chipId)      & 0xF   ;};
	
	inline static const int fecNo_from_chId(int chipIdCh)  { return (chipIdCh >> 12) & 0xFF  ;};
	inline static const int chipNo_from_chId(int chipIdCh) { return (chipIdCh >> 8)  & 0xF   ;};
	inline static const int chipId_from_chId(int chipIdCh) { return (chipIdCh >> 8)  & 0xFFF ;}; // should limit? & 0xFFF
	inline static const int chanNo_from_chId(int chipIdCh) { return (chipIdCh)       & 0xFF  ;};
   
   typedef std::vector<int16_t> ApvRawDataCont;
   
	
public:
	CApvEvent(CEvent::event_type_type evtype, int chip_id, uint32_t equipment_type);
   //	CApvEvent(const CConfiguration* config, CEvent::event_type_type evtype);
	CApvEvent(CConfiguration* config, const CUDPData* udpdata,  CEvent::event_type_type evtype, const std::vector<uint32_t>&	data32Vector, int chip_id, uint32_t equipment_type) ;
	
	//CApvEvent(const CConfiguration* config, const uint32_t *pintdata, const int size) ;
	//CApvEvent(int fec_no, unsigned apv_no) : m_apvId( (fec_no << 4) | apv_no), fFECNo(fec_no), fAPVNo(apv_no), fRawData32bits() { };
	virtual ~CApvEvent();
   //	CApvEvent(const CApvEvent&);
   //	CApvEvent& operator=(const CApvEvent& rhs);
   
   virtual CApvEvent* clone() const  { return new CApvEvent(*this); }
   
   
   
   size_t hot_channels_count()	const { return m_number_hot_strips;}
	const int timebin_count()			const { return m_timebin_count;}
	const int presamples_count()		const { return m_presamples_count;}
	void		 apv_id(int val)					{ m_apv_id = val; m_fecNo = fecNo_from_chipId(m_apv_id);  } // m_apv_id >> 4;
	const int apv_id()					const { return m_apv_id;}
	const int apv_no()					const { return chipNo_from_chipId(m_apv_id);}  //  m_apv_id & 0xF;
	const int fec_no()					const { return fecNo_from_chipId(m_apv_id);}   // (m_apv_id >> 4 ) & 0xFF;
	void process_event();
	virtual void print() const;
   virtual void fill_th1_qt(TH1* h1, TH2* h2, const CDetAbstractReadout* readout) const;
   virtual void fill_th2_qt(TH2* hist, const CDetAbstractReadout* readout) const;
   
   
	const long		udp_data_size()		const { return m_udp_data16.size(); }
	const long		data_size()		const { return m_apvevent_data.size(); }
	const int		error_code()			const { return m_bad_event_error_code;}
   
	//DATA SHARING FUNCTIONS
	const std::vector< int16_t>*						const	raw_data(int chNo)	const ;
	const std::vector<int16_t>*						const	udp_data()				const { return &m_udp_data16;}
	const std::map<int, std::vector<int16_t> >*	const apv2d_data()			const { return &m_apvevent_data;}
	std::map<int, std::vector<int16_t> >*					apv2d_data_mutable()			{ return &m_apvevent_data;}
	const std::vector<CStripDataReduced>&  apv_reduced_data() const { return m_reduced_strip_data; }
   
	const std::map<int, std::vector<double> >&		pedestal_data()			const { return m_pedestal;}
	std::map<int, std::vector<double> >&				pedestal_data_mutable()			{ return m_pedestal;}
	const size_t												pedestal_size()		const		{ return m_pedestal.size(); }
   
	void add_pedestal(const unsigned apvChId, const std::vector <double> pedvec);
	
   size_t get_udp_framenumber_srs() const { return m_udp_frame_number_srs;}
   size_t get_udp_framenumber_receiver() const { return m_udp_frame_number_receiver;}
   
private:
	CConfiguration*	m_config;
	
   //	std::string					m_source_ip_address;
	int							m_fecNo;
	int							m_apv_id;
   //	std::vector<uint32_t>	m_udp_data32;
	ApvRawDataCont	m_udp_data16;                    //changed to signed for ZS - untested with RAW APV
   size_t                  m_udp_frame_number_receiver;  // given by the receiver
   size_t                  m_udp_frame_number_srs;       // inside the frame, given by the SRS FEC
	std::map<int, std::vector<int16_t> >	m_apvevent_data	; // stores data: TODO: local_chNo , tbData
   std::vector < CStripDataReduced>       m_reduced_strip_data; // stores only strip id, tbin amd max q
	std::map<int, std::vector<double> >		m_pedestal; //map contains: TODO: local_chNo(0-128), m_pedestal_mean, m_pedestal_stdev, m_pedestal_sigma
	// mean od mean_i, 0-127channels
	//	mean of stdev_i, 0-127 channels
	// stddev of mean_i, 0-127 channels, placeholder for statistics, not an event attribute
	
	
	unsigned						m_bad_event_error_code;
	size_t						m_number_hot_strips;
	int							m_timebin_count;
	int							m_presamples_count;
   
   uint32_t m_equipment_type;
	
   
private:
   
   static int find_apv_header(ApvRawDataCont& data, ApvRawDataCont::iterator& start);
   
	const int	correct_apv_channel_sending_sequence(const int chNo) const;
   const int	mm_strip_from_apv_channel(const int apvIdCh) const;
   const int apv_channel_from_mmstrip(const int stripId) const;
	const bool	check_for_signal(const std::vector<int16_t>& dvec, const int apvIdCh) const;
	
   //	void			find_fec_by_ipaddress();
   //	int			decode_frame();
	void			decode_apv_data( const std::vector<uint32_t>& data32Vector);
   void        decode_zsp_apv_data(const std::vector<uint32_t>& data32Vector);
	int			remove_unmapped_channels();
	void			subtract_pedestals();
	int			remove_dark_channels();
	int			correct_crosstalk();
   void        calculate_reduced_data();
	void			calculate_pedestals();
	
	class ExtractApvIdCh : public std::unary_function<std::pair<int, std::vector<int16_t> >, int> {
   public:
      int operator()(std::pair<int, std::vector<int16_t> > p) const {
         return p.first;
      }
   };
   
   class MMStripIdFromApvChId  : public std::unary_function<int, int> {
      CApvEvent* m_evt;
   public:
      MMStripIdFromApvChId(CApvEvent* evt) : m_evt(evt) {};
      int operator()(int apvChId) const {
         return m_evt->mm_strip_from_apv_channel(apvChId);
      }
   };
   
   template <typename InputIterator>
   class TimebinPedestalCalculator  {
      std::vector<double> mean_;  ///< vector of channels mean (x128)
      std::vector<double> stdev_; ///< vector of channels stddev (x128)
   public:
      TimebinPedestalCalculator(std::vector<double> const& mean, std::vector<double> const& stddev) :
      mean_(mean), stdev_(stddev) { }
      
      bool calculate(double stddev_factor, InputIterator begin, InputIterator end, double& mean, size_t& count) {
         double sum = 0.0;
         size_t ii = 0;
         count = 0;
         while (begin != end) {
           
            if (stddev_factor*stdev_[ii] > std::fabs(mean_[ii] - *begin) ) {
               sum += *begin;
               ++count;
            }
            ++begin;
            ++ii;
         }
         if(count > 0) {
            mean = sum/(double)(count);
            return true;
         }
         return false;
      }
   };
	
};


#endif

