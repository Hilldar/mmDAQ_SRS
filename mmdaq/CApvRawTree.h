//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Nov 11 22:23:18 2010 by ROOT version 5.27/04
// from TTree raw/rawdata
// found on file: run6213.root
//////////////////////////////////////////////////////////

#ifndef CApvRawTree_h
#define CApvRawTree_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include <vector>
#include <string>

class CApvRawTree {
	TFile *rootfile;
	
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
	//TODO: correct to vector ! (or make a new class)
   UInt_t						apv_evt;
   std::vector<Float_t>   *apv_qmax;
   std::vector<int>       *apv_tbqmax;
	std::vector<Int_t>     *apv_fec;
	std::vector<Int_t>     *apv_id;
   std::vector<Int_t>     *apv_ch;
	std::vector<std::string>     *mm_id;
   std::vector<Int_t>     *mm_strip;
   std::vector< std::vector<Short_t> >   *apv_q;
   std::vector<Int_t>     *apv_presamples;
   //std::vector<Int_t>     *apv_tb;

	
   // List of branches
	TBranch        *b_apv_fec;   //!
   TBranch        *b_apv_evt;   //!
   TBranch        *b_apv_id;   //!
   TBranch        *b_apv_ch;   //!
	TBranch        *b_mm_id;   //!
   TBranch        *b_mm_strip;   //!
   TBranch        *b_apv_q;   //!
   TBranch        *b_apv_tb;   //!
   TBranch        *b_apv_qmax;   //!
   TBranch        *b_apv_tbqmax;   //!
	TBranch			*b_apv_presamples;   //!
	
	CApvRawTree(const char* filename);
	CApvRawTree(TFile *file);
   CApvRawTree(TTree *tree=0);
   virtual ~CApvRawTree();
//   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
	Int_t	           GetEntries();
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
	
private:
	CApvRawTree(CApvRawTree&);
	CApvRawTree& operator= (CApvRawTree&);
};

#endif
