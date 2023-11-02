

/*
 *  MBtools.h
 *  batreco
 *
 *  Created by Marcin Byszewski on 10/4/10.
 *  Copyright 2010 CERN. All rights reserved.
 *
 */


#ifndef EPS
#define EPS 0.00000001
#endif


#ifndef MBTools_h
#define MBTools_h

//#include <TString.h>


#include <numeric>
#include <vector>
#include <cmath>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>


//template <class T>
//const char* SplitFileName(const char* file, T parts[])
//{
//   TString filename(file);
//	
//   TString filenameext = "";
//   TString filenamepath = "./";
//   TString filenamebase;
//   int pos1 = filename.Last('/') + 1;
//   int pos2 = filename.Last('.');
//	
//   if	(pos1 == kNPOS && pos2 == kNPOS) {
//      filenamepath = TString("./");
//      filenamebase = filename.Data();
//      filenameext = TString("");
//   }
//   else if (pos1 == kNPOS) {
//      filenamepath = TString("./");
//      filenamebase = TString(filename(0, pos2));
//      filenameext  = TString(filename(pos2 + 1, filename.Length() - pos2));
//   }
//   else if (pos2 == kNPOS) {
//      filenamepath = TString(filename(0, pos1));
//      filenamebase = TString(filename(pos1, filename.Length() - pos1));
//      filenameext  = TString("");
//   }
//   else {
//      filenamepath = TString(filename(0, pos1));
//      filenamebase = TString(filename(pos1, pos2 - pos1));
//      filenameext  = TString(filename(pos2 + 1, filename.Length() - pos2));
//   }
//   parts[0] = filenamepath.Data();
//   parts[1] = filenamebase.Data();
//   parts[2] = filenameext.Data();
//   return filenamebase.Data();
//}


//Text not by const reference so that the function can be used with a character array as argument
template <typename T>
T StringToNumber(const std::string &Text)
{
   std::stringstream ss(Text);
   T result;
   return ss >> result ? result : 0;
}

template <typename T>
std::string NumberToString(const T val)
{
   std::stringstream ss;
   ss << val;
	return ss.str();
}

//______________________________________________________________________________
// functiones used by GetBranchRange ()
//TODO make template
//______________________________________________________________________________


template <class T>
T GetVectorRange(std::vector<T>* v, T range[])
{
   //we are analysing a vector for the event i
   //loop vector
   T vmax = 0, vmin = 0;
   T  ve = 0;                           // j-th value from vector at event i
   if (!v) {
      return -1;
   }
   for (unsigned j = 0; j < v->size(); j++) {
      ve = v->at(j);
      if (fabs(ve) < EPS) continue;            //ignore zero, get any value if max,in=0
      // if (ve == 0) continue;               //ignore zero, get any value if max,in=0
      if (fabs(vmax) < EPS) {
         vmax = ve;
      }
      if (fabs(vmin) < EPS) {
         vmin = ve;
      }
      vmax = vmax > ve ? vmax : ve;        //get the bigger/smaller value
      vmin = vmin < ve ? vmin : ve;        //comparing double with int...
   }
   range[0] = vmin;
   range[1] = vmax;
   return 0;
}


//______________________________________________________________________________
template <class T>
T GetVectorElement(std::vector<T>* invector, unsigned ind)
{
   if (!invector) return (T)0;
   unsigned sz = invector->size();
   if (ind < sz)
      return invector->at(ind);
   return (T)0;
}

//TODO: change to GetMean(<T> container), and return T::value_type sum/n
template <class T>
double GetVectorMean(const std::vector<T>& vec)
{
   if (vec.empty()) {
      return 0.0;
   }
	double sum = std::accumulate(vec.begin(), vec.end(), (double)0.0);
	return sum / (double)(vec.size());
}

template <class T>
double GetVectorMean(T first, T last)
{
   double sum = std::accumulate(first, last, 0.0);
   return sum / (double)(std::distance(first, last) + 1) ;
}


template <typename InputIterator, typename Predicate>
double GetVectorMeanIf(InputIterator begin, InputIterator end,  Predicate p)
{
   double sum = 0.0;
   size_t count = 0;
   while (begin != end) {
      if (p(*begin)) {
         sum += *begin;
         ++count;
      }
      ++begin;
   }
   return sum/(double)(count);  
}

//from S.Meyers Effective STL
template <typename InputIterator, typename OutputIterator, typename Predicate>
OutputIterator copy_if(InputIterator begin, InputIterator end, OutputIterator destBegin, Predicate p) {
   while (begin != end) {
      if (p(*begin)) *destBegin++ = *begin;
      ++begin;
   }
   return destBegin;
}


//template <class T>
//int  GetMean( <T> container)
//{
//   if (!container) return 0.0;
//   unsigned n = container->size();
//   if (!n) return 0;
//	T::value_type sum = std::accumulate(vec->begin(), vec->end(),(T::value_type)0);
//   //T sumd = (T) sum;
//   return static_cast<T::value_type> (sum / n );
//}


template <class T>
T diffsquare( T a, T b) 
{ 
	return (a-b)*(a-b); 
}

template <class T>
double GetVectorStdDev(std::vector<T> vec)
{
   if (vec.empty()) return 0.0;
   size_t n = vec.size();
   if (n<2) return 0;
	double vecmean = GetVectorMean(vec);
	typename std::vector<T>::iterator iter;
	double sumdiffs = 0;
	for (iter = vec.begin(); iter!= vec.end(); iter++) {
		sumdiffs += (*iter - vecmean) * (*iter - vecmean);
	}
   return sqrt( sumdiffs / (n-1) );
}

template <class T>
double GetVectorSumOfDiff(const std::vector<T>* vec, T setmean)
{
   if (!vec)
		return (T)(0.0);
   //unsigned n = vec->size();
	typename std::vector<T>::const_iterator iter;
	double sumdiffs = 0;
	for (iter = vec->begin(); iter!= vec->end(); iter++) {
		sumdiffs += (*iter - setmean);
	}
   return sumdiffs;
}


template <class T>
size_t MakeElementsUnique(T& container )
{
	sort(container.begin(), container.end());
	typename T::iterator iter = unique(container.begin(), container.end());
	container.resize(iter - container.begin());
	return container.size();
}


template <class C> void FreeClear(C & cntr)
{
   for (typename C::iterator it = cntr.begin();
        it != cntr.end(); ++it) {
      delete (*it);
		*it = 0;
   }
   cntr.clear();
}


//template <class C> void FreeClear(C & cntr)
//{
//	while (!cntr.empty()) {
//		typedef typename C::value_type ElementType;
//		ElementType el = cntr.front();
//		cntr.erase(cntr.begin());
//		delete el; el = 0;
//	}
//}


class IDList {	
private:
   //static 
	int nID;   // creates unique widget's IDs
public:
   IDList():nID(0) {}
   ~IDList() {}
   int GetUniqueID(void) { return ++nID; }
};




#endif

