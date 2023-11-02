//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Wed Mar 16 16:03:45 2011 by ROOT version 5.28/00
// from TTree pedestals/apvpedestals
// found on file: run10354.root
//////////////////////////////////////////////////////////

#ifndef CApvPedestalsRoot_h
#define CApvPedestalsRoot_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include <string>
#include <vector>

class CApvPedestalsRoot
{
	TFile				*rootfile;
	int				ferror;
	
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   UInt_t					apv_evt;
   Int_t						time_s;
   Int_t						time_us;
	std::vector<unsigned int> *apv_fecNo;
   std::vector<unsigned int> *apv_id;
   std::vector<unsigned int> *apv_ch;
   std::vector<std::string>  *mm_id;
   std::vector<unsigned int> *mm_strip;
   std::vector<float>			*apv_pedmean;
   std::vector<float>			*apv_pedsigma;
   std::vector<float>			*apv_pedstd;

   // List of branches
   TBranch        *b_apv_evt;   //!
   TBranch        *b_time_s;   //!
   TBranch        *b_time_us;   //!
   TBranch        *b_apv_fecNo;   //!
   TBranch        *b_apv_id;   //!
   TBranch        *b_apv_ch;   //!
   TBranch        *b_mm_id;   //!
   TBranch        *b_mm_strip;   //!
   TBranch        *b_apv_pedmean;   //!
   TBranch        *b_apv_pedsigma;   //!
   TBranch        *b_apv_pedstd;   //!

	CApvPedestalsRoot(const char* filename);
   CApvPedestalsRoot(TTree *tree = 0);
   virtual ~CApvPedestalsRoot();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
	Int_t				  GetEntries();
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
	
	int error() const { return ferror;}
	
private:
	CApvPedestalsRoot(CApvPedestalsRoot&);
	CApvPedestalsRoot& operator=(CApvPedestalsRoot&);
	
	
};

#endif
