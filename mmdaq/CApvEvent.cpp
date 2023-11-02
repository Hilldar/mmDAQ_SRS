/*
 *  CApvEvent.cpp
 *  mmdaq
 *
 *  Created by Marcin Byszewski on 1/28/11.
 *  Copyright 2011 CERN. All rights reserved.
 *
 */

#include "CEvent.h"
#include "CApvEvent.h"
#include "CMMEvent.h"
#include "CDetFec.h"
#include "CUDPData.h"
#include "CConfiguration.h"
#include "MBtools.h"
#include "CDetReadout.h"


#include "TH2.h"


#include <boost/bind.hpp>

#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <utility>
#include <string>

#define NCH 128
#define apvHdrLevel 1750
#define ApvTimeBinLength 140
#define ApvHeaderLength 12

#define MMDAQ_ERROR_DATA_NO_ERROR 0
#define MMDAQ_ERROR_DATA_FRAME_LOST 1
#define MMDAQ_ERROR_DATA_TRUNCATED 2
#define MMDAQ_ERROR_NO_SRS_MARKERS 4
#define MMDAQ_ERROR_APV_NUMBER_OUT_OF_RANGE 8
#define MMDAQ_ERROR_NUMBER_OF_TIMEBINS_OUT_OF_RANGE 16
#define MMDAQ_ERROR_FEC_IP_NOT_FOUND 32
#define MMDAQ_ERROR_FEC_IP_BAD_VALUE 64
#define MMDAQ_ERROR_BAD_EQUIPMENT 128
#define MMDAQ_ERROR_BAD_NUMBER_OF_CHANNELS 256

#define MMDAQ_INT16_SIZE 2

#define MMDAQ_SINGLE_FECNO 1

// also in udpframe
//ADC raw all data
#define SRS_HEADER_HWTYPE_APV_RAW 0x414443
//ADZ APV zero suppressed: APZ
#define SRS_HEADER_HWTYPE_APV_ZSP 0x41505A
//ADZ : 0x41445A

#define SRS_ZS_HEADER_SIZE 1
#define SRS_APV_ZS_DATA_BITMASK 0x1FFF


using std::vector;
using std::pair;
using std::cerr;
using std::cout;
using std::endl;

CApvEvent::CApvEvent(CEvent::event_type_type evtype, int chip_id, uint32_t equipment_type)
: CEvent(evtype), m_config(0), //m_source_ip_address(),	
m_fecNo(fecNo_from_chipId(chip_id)), m_apv_id(chip_id),
m_udp_data16(),
m_udp_frame_number_receiver(0), m_udp_frame_number_srs(0), m_apvevent_data(), m_reduced_strip_data(), m_pedestal(),
m_bad_event_error_code(0), m_number_hot_strips(0), m_timebin_count(0), m_presamples_count(0),
m_equipment_type(equipment_type)
{
   set_empty(false);
}


//CApvEvent::CApvEvent(const CConfiguration* config, CEvent::event_type_type evtype)
//      : CEvent(evtype), m_config(config), m_source_ip_address(), m_fecNo(0), m_apv_id(0), 
//		m_udp_data32(), m_udp_data16(), 
//      m_udp_frame_number_receiver(0), m_udp_frame_number_srs(0), m_apvevent_data(), m_pedestal(), 
//		m_bad_event_error_code(0), m_number_hot_strips(0), m_timebin_count(0),  m_presamples_count(0)
//{
//   set_empty(false);
//}


CApvEvent::CApvEvent(CConfiguration* config, const CUDPData* udpdata, CEvent::event_type_type evtype, const std::vector<uint32_t>&	data32Vector, int chip_id, uint32_t equipment_type)
: CEvent(evtype, udpdata->get_timeval()), m_config(config), //m_source_ip_address(udpdata->get_ipaddress()),
m_fecNo(fecNo_from_chipId(chip_id)), m_apv_id(chip_id), 
m_udp_data16(), 
m_udp_frame_number_receiver(udpdata->get_idnumber()), m_udp_frame_number_srs(0), m_apvevent_data(), m_reduced_strip_data(),
m_pedestal(), m_bad_event_error_code(0), m_number_hot_strips(0), m_timebin_count(0),  m_presamples_count(0),
m_equipment_type(equipment_type)
{
   m_event_bad = false;
   set_empty(false);
//   find_fec_by_ipaddress();
   
   switch (m_equipment_type) {
      case SRS_HEADER_HWTYPE_APV_RAW:
         decode_apv_data(data32Vector);
         break;
      case SRS_HEADER_HWTYPE_APV_ZSP:
         decode_zsp_apv_data(data32Vector);
         break;         
      default:
         std::cerr << "CApvEvent bad HW code"<< std::endl;
         m_event_bad = true;
         m_event_empty = true;
         m_bad_event_error_code = MMDAQ_ERROR_BAD_EQUIPMENT;
         return;
   }
//   calculate_reduced_data();

//   decode_frame();
   process_event();
}



CApvEvent::~CApvEvent()
{}


void CApvEvent::decode_zsp_apv_data(const std::vector<uint32_t>& data32Vector)
{
   
   /* Raffaele Giordano ZS data format:
    struct APV_HEADER
    {
    BYTE    APV_ID;     // APV Identifier number on the FEC card (0 to 15) 
    BYTE    N_CHANNELS; // the number of channels which will be following the header  
    
    BYTE    N_SAMPLES;  // the number of samples per channel
    BYTE    ZS_ERROR;   // Error code from the Zero Suppression Block, meaning have to be defined
    
    
    //TODO: add following word
    WORD16 FLAGS; // bit 0 : ‘0’ – Classic zero suppression, ‘1’ – Zero suppression with peak finding
    
    WORD16  RESERVED;   // 16 bits reserved for future use
    };
    
    struct CHAN_INFO
    {
    BYTE    CHAN_ID;             // Channel identifier,
    // APV physical channels are 0 to 127, 
    // 128 could be used for the commond mode average
    // 129 for error codes from the APV (pipeline address(8 bits) & error bit)
    BYTE    RESERVED;            // 8-bits reserved for future use
    WORD16  CHANDATA[N_SAMPLES]; // 16 bit words, actual data will be 13-bits wide
    };
    */
   
   
   //TODO: new data format decode
//   CUDPData::endian_swap16_copy((*diter);
   uint32_t w32_1 =   *(data32Vector.begin() + 1) ; // skip header info field
//   std::cout << " w32_1      = "   << std::hex<< std::setfill('0') << std::setw(8) << w32_1 << std::endl;
   uint16_t w16a = (w32_1 >>16) & 0xFFFF;
   uint16_t w16b = w32_1 & 0xFFFF;
   
//   uint8_t apv_id_unused      = (w16a  )     & 0xFF;//not as in manual
   uint8_t number_of_channels = (w16a >> 8 ) & 0xFF;//not as in manual
   
   uint8_t number_of_samples  = (w16b      ) & 0xFF; // number of time bins
//   uint8_t zs_error_code      = (w16b >> 8 ) & 0xFF;
   
   uint32_t w32_2 =  *(data32Vector.begin() + 2);
//   std::cout << " w32_2      = "   << std::hex<< std::setfill('0') << std::setw(8) << w32_2 << std::endl;
   w16a = (w32_2 >> 16) & 0xFFFF; //flags
   w16b = w32_2 & 0xFFFF; //reserved
  
   bool peak_mode = w16a & 0x1;
//   uint16_t reserved_w16 = w16b;

   
   if (peak_mode) {
      std::cerr << "CApvEvent::decode_zsp_apv_data() FEC ZS is in peak mode - unsupported" << std::endl;
      return;
   }
   
   if (number_of_channels < 1) {
//      std::cerr << "CApvEvent::decode_zsp_apv_data() no channels in ZS data (empty event)" << std::endl;
      return;
   }
   
   
//   std::cout << " apv_id_unused      = "  << std::setw(2) << (int)apv_id_unused << std::endl;
//   std::cout << " number_of_channels = "  << std::setw(2) << (int)number_of_channels << std::endl;
//   std::cout << " number_of_samples  = "  << std::setw(2) << (int)number_of_samples << std::endl;
//   std::cout << " zs_error_code      = "  << std::setw(2) << (int)zs_error_code << std::endl;
//   std::cout << " reserved_w32      = "
//   << std::hex<< std::setfill('0') << std::setw(8) << reserved_w16 << std::endl;
   
   
//   uint32_t w32_3 =  *(data32Vector.begin() + 3);
//   std::cout << " w32_3      = "   << std::hex<< std::setfill('0') << std::setw(8) << w32_3 << std::endl;
//   w32_3 =  *(data32Vector.begin() + 4);
//   std::cout << " w32_4      = "   << std::hex<< std::setfill('0') << std::setw(8) << w32_3 << std::endl;
//   w32_3 =  *(data32Vector.begin() + 5);
//   std::cout << " w32_5      = "   << std::hex<< std::setfill('0') << std::setw(8) << w32_3 << std::endl;

   
   //transform data to words 16bit
   std::vector<int16_t> data16;
   data16.reserve(number_of_channels*(number_of_samples+2));
   for (size_t ii = 3; ii < data32Vector.size(); ++ii) {
      uint32_t w32 =  CUDPData::endian_swap32_copy(data32Vector[ii]);
      
      int16_t w16sa = (w32) & 0xFFFF;
      int16_t w16sb = (w32 >> 16 ) & 0xFFFF;
      data16.push_back(w16sa);
      data16.push_back(w16sb);
   }
   

   

//   
//   //skip 2x 16bit words reserved
//   diter++;
////   std::cout << "CApvEvent::decode_zsp_apv_data() hdrwrd3: " << std::hex << *diter << std::dec << std::endl;
//   diter++;
////   std::cout << "CApvEvent::decode_zsp_apv_data() hdrwrd4: " << std::hex << *diter << std::dec << std::endl;
//
//   
//   //TODO: size check:
//   int zs_header_size = 5;
//   if (m_udp_data16.size() != number_of_channels*(number_of_samples + 1) + zs_header_size ) {
//      std::cerr << "CApvEvent::decode_zsp_apv_data() bad zero-suppressed data size" << number_of_channels*(number_of_samples + 2) + 5 << std::endl;
//      std::cerr << "CApvEvent::decode_zsp_apv_data() number_of_channels= " << (int)number_of_channels << std::endl;
//      std::cerr << "CApvEvent::decode_zsp_apv_data() number_of_samples = " << (int)number_of_samples << std::endl;
//      std::cerr << "CApvEvent::decode_zsp_apv_data() data16.size() (incl. 4 header words) = " << m_udp_data16.size() << std::endl;
//      m_bad_event_error_code = 0;
//      m_event_bad = true;
//      return;
//   }
//   else {
////      std::cerr << "CApvEvent::decode_zsp_apv_data() number_of_channels= " << (int)number_of_channels << std::endl;
////      std::cerr << "CApvEvent::decode_zsp_apv_data() number_of_samples = " << (int)number_of_samples << std::endl;
//   }
   
   
   uint32_t apv_channel_sent = 0;
   
//   bool data_flag = false;
//   size_t header_counter = 0;
//   size_t data_counter = 0;
//   size_t channel_counter = 0;
   
   typedef std::map<uint32_t, std::vector<int16_t> > ApvDataMap;
   ApvDataMap apvevent_data;
   ApvDataMap commonmode_data;
   ApvDataMap error_data;
   
      size_t jj  = 0;
   // read channel data
   std::vector<int16_t>::iterator diter = data16.begin();

   
   //      struct CHAN_INFO
   //      {
   //         BYTE    CHAN_ID;             // Channel identifier,
   //         // APV physical channels are 0 to 127,
   //         // 128 could be used for the commond mode average
   //         // 129 for error codes from the APV (pipeline address(8 bits) & error bit)
   //         BYTE    RESERVED;            // 8-bits reserved for future use
   //         WORD16  CHANDATA[N_SAMPLES]; // 16 bit words, actual data will be 13-bits wide
   //      };
   
   
   //check size of vector
   int apvIdCh = 0;
   for (;  diter != data16.end(); ++diter, ++jj) {
      if (jj%(number_of_samples + 1) == 0) {
         apv_channel_sent = (*diter) & 0xFF;
         int chan_no = correct_apv_channel_sending_sequence(apv_channel_sent);
         apvIdCh = make_channelId(m_apv_id,chan_no);
         
      }
      else {
         m_apvevent_data[apvIdCh].push_back( -(*diter));
      }
   } // for diter

      
    
   //move addidtional channels(error common mode) out of data container
   ApvDataMap::iterator found128 = apvevent_data.find(128);
   if (found128 != apvevent_data.end()) {
      commonmode_data[128].assign(found128->second.begin(), found128->second.end());
      apvevent_data.erase(found128);
   }
   
   ApvDataMap::iterator found129 = apvevent_data.find(129);   
   if (found129 != apvevent_data.end()) {
      commonmode_data[129].assign(found129->second.begin(), found129->second.end());
      error_data.erase(found129);
   }   

   
      m_timebin_count = number_of_samples; // - 1;
   
// m_event_bad = false;
   
   //set:
//   m_bad_event_error_code |= MMDAQ_ERROR_DATA_TRUNCATED;
//   m_event_bad = true;

//   m_presamples_count
   
}


int CApvEvent::find_apv_header(ApvRawDataCont& data, ApvRawDataCont::iterator& start)
{
   ApvRawDataCont::iterator apv_header = data.end();
   
   short apvHdrCounter = 0;
   for (ApvRawDataCont::iterator diter = start; diter != data.end(); ++diter) {
      if (*diter < apvHdrLevel) {
         if (!apvHdrCounter) {
            apv_header = diter;
         }
         else if (apvHdrCounter == 2) {
            if (std::distance(diter, data.end()) < ApvTimeBinLength) {
               // error truncated data
               // m_bad_event_error_code |= MMDAQ_ERROR_DATA_TRUNCATED;
               // m_event_bad = true;
               start = data.end();
               return -1; // data truncated
            }
            start = apv_header;
            return 1; // found the header (3rd value < threshold)
         }
         ++apvHdrCounter;
      }
      else {
         apvHdrCounter = 0;
         continue;
      }
   }
   start = data.end();
   return 0; // not found
}


void CApvEvent::decode_apv_data(const std::vector<uint32_t>& data32Vector)
{
//   Replaced push by reintepret_cast and assign, verify data
   m_udp_data16.reserve(2*data32Vector.size() + 2);
   for (vector<uint32_t>::const_iterator idatum32 = data32Vector.begin() + 3; idatum32 != data32Vector.end(); ++idatum32) {
      m_udp_data16.push_back((*idatum32) & 0xFFFF);
      m_udp_data16.push_back((*idatum32 >> 16) & 0xFFFF);
   }

   if (m_udp_data16.empty()) {
      m_event_bad = true;
      m_bad_event_error_code = MMDAQ_ERROR_DATA_TRUNCATED;
   }
   //this is not ok
//   const uint32_t* p32 = &data32Vector[0];
//   const uint16_t* p16 = reinterpret_cast<const uint16_t*>(&data32Vector[0]);
//   m_udp_data16.assign(p16, p16+data32Vector.size()*2);
   
   //find first apv header
   ApvRawDataCont::iterator apvhdr = m_udp_data16.begin();
   ApvRawDataCont::iterator first_apvhdr = m_udp_data16.end();
   
   double chip_pedestal = 0.0;
   bool chip_ped_available = false;
   if (m_config->pedestal_chip_mean(m_apv_id, chip_pedestal)) {
      chip_ped_available = true;
   }

   //count headers and full time bin data
   short number_of_timebins = 0;
   //loop time bins
   TimebinPedestalCalculator<ApvRawDataCont::iterator> tbcalc(m_config->pedestal_chip_mean_vector(m_apv_id),
                                                              m_config->pedestal_chip_stdev_vector(m_apv_id));
   
   double cmode_signal_factor = m_config->get_common_mode_factor();
   bool cmode_enabled = m_config->is_common_mode_enabled();
   do {
      int rc = find_apv_header(m_udp_data16, apvhdr);
      if (rc < 0) {
         m_bad_event_error_code |= MMDAQ_ERROR_DATA_TRUNCATED;
         m_event_bad = true;
         break;
      }
      if(apvhdr == m_udp_data16.end()) {
         break; // not found or truncated
      }
      if (!number_of_timebins) {
         first_apvhdr = apvhdr;
      }
      ++number_of_timebins;
      //correct common mode with out channels with signal, modify raw data
      if (chip_ped_available && cmode_enabled) {  
         // ignore signals > x*stddev
//         double pedmean = GetVectorMean(apvhdr+ApvHeaderLength, apvhdr+ApvTimeBinLength);
         double pedmean = 0.0;
         size_t count = 0;
         if (tbcalc.calculate(cmode_signal_factor,
                              apvhdr+ApvHeaderLength,
                              apvhdr+ApvTimeBinLength,
                              pedmean, count) ) {
            int16_t peddiff = chip_pedestal - pedmean;
            std::transform(apvhdr+ApvHeaderLength,
                           apvhdr+ApvTimeBinLength,
                           apvhdr+ApvHeaderLength,
                           std::bind2nd(std::plus<int16_t>(), peddiff));
         }
      }
      std::advance(apvhdr, ApvTimeBinLength); // move past the apv data
   } while (apvhdr != m_udp_data16.end());
   
   if (number_of_timebins < 3 || number_of_timebins > 33) {
      m_event_bad = true;
      m_bad_event_error_code |= MMDAQ_ERROR_NUMBER_OF_TIMEBINS_OUT_OF_RANGE;
      return;
   }
   
   m_timebin_count = number_of_timebins;
   // last good data, i.e. not truncated  
   ApvRawDataCont::iterator data_end = first_apvhdr;
   std::advance(data_end, ApvTimeBinLength*(number_of_timebins));
   // number_of_timebins gives count of good timebins
   
   if (m_udp_data16.size() > 1 && (number_of_timebins < 1 || number_of_timebins > 30)) {
      // happens when frame size too small or  no data is send by apv - only timing  signals
      m_event_bad = true;
      m_bad_event_error_code |= MMDAQ_ERROR_NUMBER_OF_TIMEBINS_OUT_OF_RANGE;
   }
   
   //repartition into channels
   size_t jj = 0;
   for (vector<int16_t>::iterator diter = first_apvhdr; diter != data_end ; ++diter, ++jj) {
      //apv_header
      if (jj%ApvTimeBinLength < ApvHeaderLength) {
         //apv header
         continue;
      }
      else {
         //channel data
         int chan = jj%ApvTimeBinLength - ApvHeaderLength;
         assert(chan >= 0 && chan < 128);
         int goodchnum = correct_apv_channel_sending_sequence(chan);
			int apvIdCh = make_channelId(m_apv_id, goodchnum);
         m_apvevent_data[apvIdCh].push_back(*diter);
      }
   }
}



const int CApvEvent::correct_apv_channel_sending_sequence(const int chNo) const
{
   return (32 *(chNo % 4)) + (8 *(int)(chNo / 4)) - (31 *(int)(chNo / 16));
}



void CApvEvent::process_event()
{
   //TODO:
   bool m_flow_control_subtract_pedestals = true;
   bool m_flow_control_remove_dark_channels = true;
   bool m_flow_control_correct_crosstalk = true;

	//if pedestal not loaded do not execute
	if(m_config->pedestal_event()->pedestal_size() == 0) {
		m_flow_control_subtract_pedestals = false;
		m_flow_control_remove_dark_channels = false;
		m_flow_control_correct_crosstalk = false;
	}
	
	
   //std::map<int, std::vector<int16_t> > live_channels;

   if (m_event_bad) {
      m_apvevent_data.clear();
      return;
   }
   
   remove_unmapped_channels();

   if (m_event_type == CEvent::eventTypePedestals) {
      calculate_pedestals();
      return;
   }
   else 	if (m_event_type == CEvent::eventTypePhysics) {
      if (m_flow_control_subtract_pedestals) {
         subtract_pedestals();
      }
      if (m_flow_control_remove_dark_channels) {
         remove_dark_channels();
      }
		if (m_flow_control_correct_crosstalk) {
			correct_crosstalk();
			remove_dark_channels();
		}
   }
   m_number_hot_strips = m_apvevent_data.size();
   m_event_empty = (m_number_hot_strips == 0);
	
   //fill reduced data holder with max q and corresponding time bin
   calculate_reduced_data();
}


int CApvEvent::remove_unmapped_channels()
{
   //removes all channels from m_apvevent_data that are not mapped:
   //that is, mm_strip_from_apv_channel() returns -1 for unique_channel_id

   int counter = 0;
   std::map<int, std::vector<int16_t> >::iterator  ichan = m_apvevent_data.begin();
   while (ichan != m_apvevent_data.end()) {
      int apvIdCh = ichan->first;

      //TODO:change apvIdCh to: mm_strip(apv_id)[chNo] -> return vector/map for this apv, lookup chNo localy
      //TODO: the above requires many changes

      if (mm_strip_from_apv_channel(apvIdCh) == -1) {
         m_apvevent_data.erase(ichan++);  //channel is not connected to anything based on the map file - remove
//			cout << "remove_unmapped_channels(): not mapped: m_apv_id:" << m_apv_id
//			<<" chId: " << ichan->first << " chNo:" << (ichan->first & 0xFF) << " : "  << endl;
         ++counter;
         continue;
      }
      ++ichan;
   }
   return counter;
}


//pedvec[0] = mean;//mean
//pedvec[1] = stdev; //stdev
//pedvec[2] = sigma;//sigma
void CApvEvent::add_pedestal(const unsigned apvChId, const std::vector <double> pedvec)
{
   m_pedestal[apvChId] = vector<double>();
   m_pedestal[apvChId].assign(pedvec.begin(), pedvec.end());
//	for (vector <double> :: const_iterator iv = pedvec.begin(); iv != pedvec.end(); ++iv) {
//		m_pedestal[apvChId].push_back(*iv);
//	}
}


void CApvEvent::subtract_pedestals()
{
	if(!m_config) {
		return;
	}
	// bool debug_hasbad = false;
   std::map<int, std::vector<int16_t> >::iterator  ichan = m_apvevent_data.begin();
   while (ichan != m_apvevent_data.end()) {
      int apvIdCh = ichan->first;
//      long summitb = 0;
      
            
      double pedestal = 0.0;
      bool pedisfound = m_config->pedestal_mean(apvIdCh, pedestal);
      if (pedisfound) {
         //TODO: error handling here - should CHECK BEFORE if good pedestal file?
         //right now just ignore
      }
      if (pedestal > 32767) {
         cerr << "Error: CApvEvent::subtract_pedestals():: overflow in pedestal value" << endl;
      }
      
      for (std::vector<int16_t>::iterator itb = ichan->second.begin(); itb != ichan->second.end(); ++itb) {
         short new_val = static_cast<short>(pedestal) - *itb;
         *itb =  new_val;
//         summitb += *itb;
      }

//      if (summitb > 10000) {
//         m_debug_bad_channels.push_back(ichan->first &0xFF);
//				cout << "bad channel " << (ichan->first &0xFF) << endl;
//         debug_hasbad = true;
//      }
      ++ichan;
   }
}



/** 
 remove channels without data , optionally keep neighbours
 */
int CApvEvent::remove_dark_channels()
{
 
   // this no good -within one apv only - must be done on all strips in MMEvent
   
//   //TODO: flag as dark channels, do not remove immediately
//   
//   //put all of the channel id into dark_channels, and remove those with signal
//   std::vector<int> bright_channels;
//   bright_channels.reserve(m_apvevent_data.size());
//   std::map<int, std::vector<int16_t> >::iterator  ichan = m_apvevent_data.begin();
//   while (ichan != m_apvevent_data.end()) {
//      int apvIdCh = ichan->first;
//      std::vector<int16_t>& channel_data = ichan->second;
//      if (check_for_signal(channel_data, apvIdCh) ) {
//         bright_channels.push_back(apvIdCh);
//      }
//   }
//   std::sort(bright_channels.begin(), bright_channels.end());
//   std::vector<int> neighbouring_channels;
//   for (std::vector<int>::iterator it = bright_channels.begin(); it != bright_channels.end(); ++it) {
//      int left  = apv_channel_from_mmstrip(mm_strip_from_apv_channel(*it - 1) );
//      int right = apv_channel_from_mmstrip(mm_strip_from_apv_channel(*it + 1) );
//      if (left > 0) {
//         neighbouring_channels.push_back( left );
//      }
//      if (right > 0) {
//         neighbouring_channels.push_back( right );
//      }
//   }
//   std::sort(neighbouring_channels.begin(), neighbouring_channels.end());
//   std::vector<int> keep_channels;
//   std::set_union(bright_channels.begin(), bright_channels.end(),
//                  neighbouring_channels.begin(), neighbouring_channels.end(),
//                  std::back_inserter(keep_channels));
//   
//   //remove those that are not in keep ...
//   
//   
//   // get strip numbers of dark channels, sort, on missing value (bright channel)
//   MMStripIdFromApvChId mmstripno(this);
//   std::vector<int> bright_strips;
//   bright_strips.reserve(bright_channels.size());
//   std::transform(bright_channels.begin(), bright_channels.end(),
//                  std::back_inserter(bright_strips), mmstripno);
//   std::sort(bright_strips.begin(), bright_strips.end());
//   
   
   
//   
//   std::transform(m_apvevent_data.begin(), m_apvevent_data.end(),
//                  std::inserter(dark_channels, dark_channels.begin()), ExtractApvIdCh());
//                 
//   
//   
   
  
   
   
   //TODO: remove the flag if neighbour strip has data
   //mm_strip_from_apv_channel(apvIdCh)
   
   
   //TODO: remove dark channels
   
   
   //old immediate removal
   int counter = 0;
   std::map<int, std::vector<int16_t> >::iterator  ichan = m_apvevent_data.begin();
   while (ichan != m_apvevent_data.end()) {
      int apvIdCh = ichan->first;
      std::vector<int16_t>& channel_data = ichan->second;
      bool signal_found = check_for_signal(channel_data, apvIdCh);
      if (!signal_found) {
         m_apvevent_data.erase(ichan++);  //channel has no data - remove
         ++counter;
         continue;
      }
      ++ichan;
   }
   return counter;
}


int CApvEvent::correct_crosstalk()
{
	if (!m_config) {
		return 0;
	}
	
	const std::map<unsigned, std::vector <unsigned> > crosstalkmap	= m_config->crosstalk_map();
   std::map<int, std::vector<int16_t> >::const_iterator  ichan = m_apvevent_data.begin();
   while (ichan != m_apvevent_data.end()) {
		
      int apvIdCh = ichan->first;      
		unsigned chnum = chanNo_from_chId(apvIdCh); //= apvIdCh & 0xFF; //ignore chip num and fec num
		std::map<unsigned, std::vector <unsigned> >::const_iterator ictalks = crosstalkmap.find(chnum);
      if (ictalks == crosstalkmap.end()) {
			++ichan;	
			continue; //this channel doesn't talk
		}

		//so this channel talks, apply correction: same apv, change chnum
		unsigned ghostApvId = chipId_from_chId(apvIdCh); //unsigned ghostApvId = apvIdCh >> 8;
		unsigned ghostChnum = ictalks->second.at(1);	 //mistake: must be 1 //vector<int>(3) read from -> CConfiguration -> LoadCrosstalkMapFile()
		
		unsigned ghostIdCh = make_channelId(ghostApvId,ghostChnum); //unsigned ghostIdCh = (ghostApvId << 8) | ghostChnum;
		double ghostCorrection = (double)(ictalks->second.at(2)) / 100.0;  //mistake: must be at 2  
		std::map<int, std::vector<int16_t> >::iterator ighost = m_apvevent_data.find(ghostIdCh);
		if (ighost == m_apvevent_data.end()) {
			++ichan;
			continue; // no ghost, nothing to correct
		}
      if (ichan->second.size() != ighost->second.size()) {
         std::cerr << "ERROR: correct_crosstalk(): number of timebins varies between channels ! Data Corrupted. ";
         std::cerr << " -> " << ichan->second.size() << " != " << ighost->second.size() << endl;
			++ichan;
         continue;
      }

		//apply correction to vector of ighost: m_apvevent_data[apvIdCh] that is ichan->second
      int tbin = 0;
      for (std::vector<int16_t>::const_iterator itb = ichan->second.begin() ; itb != ichan->second.end() ; ++itb , ++tbin) {
         ighost->second[tbin] = static_cast<int16_t>((double)(ighost->second[tbin]) - std::fabs((double)(*itb) * ghostCorrection)  );
      }

		++ichan;
   }
   return 0;
}


void CApvEvent::calculate_reduced_data()
{
   m_reduced_strip_data.clear();
   m_reduced_strip_data.reserve(m_apvevent_data.size());
   for ( std::map<int, std::vector<int16_t> > :: const_iterator ichan = m_apvevent_data.begin(); ichan != m_apvevent_data.end(); ++ichan) {
      std::vector<int16_t>::const_iterator imax = std::max_element(ichan->second.begin(), ichan->second.end());
      int16_t valqmax = *imax;
      int16_t tbqmax  = imax - ichan->second.begin();
      m_reduced_strip_data.push_back(CStripDataReduced(ichan->first, tbqmax, valqmax));
//      m_reduced_strip_data.back().print();
   }
//   std::cout <<"CApvEvent::calculate_reduced_data() m_reduced_strip_data.size()= " << m_reduced_strip_data.size() << std::endl;
}

void CApvEvent::calculate_pedestals()
{
   //into 	std::map<int, std::vector<int16_t> > m_apvevent_data	; // stores data: chNo , tbData
   // push into vec: mean, stdev
   //cout << "calculate pedestals" << endl;

   //int counter = 0;

   std::map<int, std::vector<int16_t> >::iterator  ichan = m_apvevent_data.begin();
   while (ichan != m_apvevent_data.end()) {
		int apvIdCh = ichan->first;
      std::vector<int16_t>& channel_data = ichan->second;

//      m_pedestal_mean.insert( pair<int, double>(ichan->first, GetVectorMean(&channel_data) ) );
		double mean = GetVectorMean(channel_data);
		double stdev = GetVectorStdDev(channel_data);
		double sigma = 0.0;
		
		m_pedestal[apvIdCh].push_back(mean);
		m_pedestal[apvIdCh].push_back(stdev);
		m_pedestal[apvIdCh].push_back(sigma);
		
//      m_pedestal_mean[chNum]  =  GetVectorMean(&channel_data)  ;
//      m_pedestal_stdev[chNum] =  GetVectorStdDev(&channel_data);
//      m_pedestal_sigma[chNum] = 0.0;
     
		//cout << ichan->first << " m_pedestal_mean= " << m_pedestal_mean[ichan->first] <<  " m_pedestal_stdev= " << m_pedestal_stdev[ichan->first] << endl;

//		if (fabs(m_pedestal_mean[chNum]) < 0.1) {
//			cout << "CApvEvent::calculate_pedestals() : m_pedestal_mean[chNum] = " << m_pedestal_mean[chNum] << " : channel_data.size()=" << channel_data.size()<<endl;
////			m_event_bad = true;
//		}

      ++ichan;
   }
}


const int CApvEvent::mm_strip_from_apv_channel(const int apvIdCh) const
{
   if (!m_config || apvIdCh < 0) {
		return -1;
	}
	//get mapping  information from CConfiguration for the unique_channel_id
   return m_config->strip_from_channel(apvIdCh) - 1;
   //return -1 means was not mapped
}

const int CApvEvent::apv_channel_from_mmstrip(const int stripId) const
{
   if (!m_config || stripId < 0 ) {
		return -1;
	}
	//get mapping  information from CConfiguration for the unique_channel_id
   return m_config->strip_from_channel(stripId);
   //return -1 means was not mapped
}


/// check for signal in the strip - TODO: add time over threshold (min number of consecutive TB overt charge value)
const bool CApvEvent::check_for_signal(const std::vector<int16_t>& dvec, const int apvIdCh) const
{
	if (!m_config) {
		return true;
	}
   double pedstddev = 0.0;
   //bool found = m_config->pedestal_stdev(apvIdCh, pedstddev);
	m_config->pedestal_stdev(apvIdCh, pedstddev);
   double sumdiff = GetVectorSumOfDiff(&dvec, int16_t(0));
   double ntimebins = static_cast<double>(dvec.size());
   return (sumdiff/ntimebins > m_config->sigma_cut() * pedstddev);
}


void CApvEvent::print() const
{
   cout << " - print() fec#:" << fec_no() << " apv#:" << apv_no() << " :";
   cout << " bad_event (" << m_event_bad << "), ";
   cout << " bad_code (" << m_bad_event_error_code << "), ";
   cout << " type (" << m_event_type << ") ";


   if (m_event_type == eventTypePhysics) {

      if (m_apvevent_data.empty()) {
         cout << " m_apvevent_data is empty (m_event_empty=" << m_event_empty << ")." << endl;
         return;
      }
      else {
         cout << endl << "         vector<int16_t>:" << endl;
      }

      for (std::map<int, std::vector<int16_t> >::const_iterator imap = m_apvevent_data.begin(); imap != m_apvevent_data.end(); ++imap) {
         cout << " " << std::setw(3) << imap->first << " [ ";
         for (std::vector<int16_t>::const_iterator itb = imap->second.begin(); itb != imap->second.end(); ++itb) {
            cout << std::setw(4) << *itb << " ";
         }
         cout << " ]" <<  endl;
      }
   }
	else if (m_event_type == eventTypePedestals) {

		cout << "m_pedestal:" << endl;
		for (std::map<int, vector<double> > :: const_iterator it = m_pedestal.begin() ; it != m_pedestal.end(); ++it) {
			cout << std::setw(3) << it->first << ":( ";
			for (vector<double> :: const_iterator iv = it->second.begin(); iv != it->second.end(); ++iv) {
				cout << std::setw(6) << *iv << " "	;
			}
			cout << ")  "<<endl;
		}
	}

}

const std::vector<int16_t>* const CApvEvent::raw_data(int chNo) const
{
   std::map<int, std::vector<int16_t> >::const_iterator ichan = m_apvevent_data.find(chNo);
   return &(ichan->second);
}

//void CApvEvent::find_fec_by_ipaddress()
//{
//	if (!m_config) {
//		return;
//	}
//   const std::vector<CDetFec*> fecs = m_config->defined_fecs();
//   for (vector<CDetFec*>:: const_iterator ifec = fecs.begin(); ifec != fecs.end(); ++ifec) {
//      if ((*ifec)->ipaddress() == m_source_ip_address) {
//         m_fecNo = (*ifec)->get_id();
//         if (m_fecNo < 1) {
//            m_event_bad = true;
//            m_bad_event_error_code |= MMDAQ_ERROR_FEC_IP_BAD_VALUE;
//         }
//         return;
//      }
//   }
//   m_event_bad = true;
//   m_bad_event_error_code |= MMDAQ_ERROR_FEC_IP_NOT_FOUND;
//}


//TODO: this f should use m_reduced_strip_data
void CApvEvent::fill_th1_qt(TH1* h1, TH2* h2, const CDetAbstractReadout* readout) const 
{
   for (std::map<int, std::vector<int16_t> > :: const_iterator ichan = m_apvevent_data.begin(); ichan != m_apvevent_data.end(); ++ichan) {
      if ((m_config->strip_from_channel(ichan->first) - 1) < 0)
         continue; // check for unmapped channels
      
      //max values
      std::vector<int16_t>::const_iterator imax = max_element(ichan->second.begin(), ichan->second.end());
      int16_t valqmax = *imax;
      int16_t tbqmax  = imax - ichan->second.begin();
      int apv_id   = CApvEvent::chipId_from_chId(ichan->first); 
      const CDetChip* thischip = m_config->find_defined_chip(apv_id);
      
      const std::vector<CDetChip*>& chips = readout->get_chips();
      if (chips.end() == std::find(chips.begin(), chips.end(), thischip)) {
         continue; // do not plot chips not from this readout
      }
      int strip = m_config->strip_from_channel(ichan->first);
      h1->Fill(strip, valqmax);
      h2->Fill(strip, tbqmax);
   }//for ichan
}


void CApvEvent::fill_th2_qt(TH2 *hist, const CDetAbstractReadout* readout) const
{
   for (std::map<int, std::vector<int16_t> > :: const_iterator ichan = m_apvevent_data.begin(); 
        ichan != m_apvevent_data.end(); ++ichan) {
      int strip = m_config->strip_from_channel(ichan->first);
      if ((strip - 1) < 0)
         continue; // check for unmapped channels
      
      int apv_id = CApvEvent::chipId_from_chId(ichan->first);
      const CDetChip* thischip = m_config->find_defined_chip(apv_id);
      
      const std::vector<CDetChip*>& chips = readout->get_chips();
      if (chips.end() == std::find(chips.begin(), chips.end(), thischip)) {
         continue; // do not plot chips not from this readout
      }

      int tb_no = 0;
      for (vector<int16_t>::const_iterator itb = ichan->second.begin(); itb != ichan->second.end(); ++itb, ++tb_no) {
         hist->Fill(strip, tb_no, *itb);
      }
   }//for ichan
}


