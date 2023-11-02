//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Wed Nov 17 23:11:32 2010 by ROOT version 5.27/04
// from TTree raw/rawdata
// found on file: run6330.root
//////////////////////////////////////////////////////////

#ifndef CApvRawPedRoot_h
#define CApvRawPedRoot_h

#include <TObject.h>
#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

#include <string>
#include <vector>

class CApvRawPedRoot //:public TObject
{
	TFile				*rootfile;
	int				ferror;
	
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   UInt_t          apv_evt;
	std::vector<int>     *apv_fecNo;
   std::vector<int>     *apv_id;
   std::vector<int>     *apv_ch;
   std::vector<std::string>     *mm_id;
   std::vector<int>     *mm_strip;
   std::vector<float>   *apv_pedmean;
   std::vector<float>   *apv_pedsigma;
	std::vector<float>   *apv_pedstd;
	
   // List of branches
   TBranch        *b_apv_evt;   //!
   TBranch        *b_apv_fecNo;   //!
   TBranch        *b_apv_id;   //!
   TBranch        *b_apv_ch;   //!
   TBranch        *b_mm_id;   //!
   TBranch        *b_mm_strip;   //!
   TBranch        *b_apv_pedmean;   //!
   TBranch        *b_apv_pedsigma;   //!
	TBranch        *b_apv_pedstd;   //!
	
	CApvRawPedRoot(const char* filename);
	CApvRawPedRoot(TFile *file);
   CApvRawPedRoot(TTree *tree=0);
   virtual ~CApvRawPedRoot();
   //virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
	Int_t	GetEntries();
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
	
	int error() const { return ferror;}
private:
	CApvRawPedRoot(CApvRawPedRoot&);
	CApvRawPedRoot& operator=(CApvRawPedRoot&);
	
	//ClassDef(CApvRawPedRoot,1)
};

#endif
