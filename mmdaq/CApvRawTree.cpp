#include "CApvRawTree.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TString.h>
#include <sys/stat.h>


//TODO:: ADD EXCEPTIONS, THROW on bad file
CApvRawTree::CApvRawTree(const char* filename)
: rootfile(0), fChain(0), fCurrent(0), 
apv_evt(0), apv_qmax(0), apv_tbqmax(0), apv_fec(0), apv_id(0), apv_ch(0), mm_id(0), mm_strip(0), apv_q(0),apv_presamples(0),
b_apv_fec(0), b_apv_evt(0), b_apv_id(0), b_apv_ch(0), b_mm_id(0), b_mm_strip(0), b_apv_q(0), b_apv_tb(0), b_apv_qmax(0), b_apv_tbqmax(0), b_apv_presamples(0)
{
	TString thename(filename);
	struct stat stFileInfo;
	if (filename && strlen(filename) && !stat(thename.Data(), &stFileInfo)) {
		rootfile = new TFile(thename);
		if (rootfile->IsOpen()) {
			TTree *tree = (TTree*)rootfile->Get("raw");
			Init(tree);
		}
	}
	else {
		CApvRawTree((TTree*)0);
	}
}


CApvRawTree::CApvRawTree(TFile *file)
: rootfile(0), fChain(0), fCurrent(0), 
apv_evt(0), apv_qmax(0), apv_tbqmax(0), apv_fec(0), apv_id(0), apv_ch(0), mm_id(0), mm_strip(0), apv_q(0), apv_presamples(0),
b_apv_fec(0), b_apv_evt(0), b_apv_id(0), b_apv_ch(0), b_mm_id(0), b_mm_strip(0), b_apv_q(0), b_apv_tb(0), b_apv_qmax(0), b_apv_tbqmax(0), b_apv_presamples(0)
{
	if(file == 0) 
		CApvRawTree((TTree*)0);
	else{
		TTree* tree = (TTree*)file->Get("raw");
		Init(tree);
	}
}


CApvRawTree::CApvRawTree(TTree *tree)
: rootfile(0), fChain(0), fCurrent(0), 
apv_evt(0), apv_qmax(0), apv_tbqmax(0), apv_fec(0), apv_id(0), apv_ch(0), mm_id(0), mm_strip(0), apv_q(0), apv_presamples(0),
b_apv_fec(0), b_apv_evt(0), b_apv_id(0), b_apv_ch(0), b_mm_id(0), b_mm_strip(0), b_apv_q(0), b_apv_tb(0), b_apv_qmax(0), b_apv_tbqmax(0), b_apv_presamples(0)
{
	// if parameter tree is not specified (or zero), connect the file
	// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("run6213.root");
      if (!f) {
         f = new TFile("run6213.root");
      }
      tree = (TTree*)gDirectory->Get("raw");
   }
   Init(tree);
}

CApvRawTree::~CApvRawTree()
{
   if (rootfile) {
		delete rootfile;// automatically calls rootfile->Close(); 
		rootfile = 0;
	}
	if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t CApvRawTree::GetEntry(Long64_t entry)
{
	// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}

Int_t CApvRawTree::GetEntries()
{
   if (!fChain) return -1;
   return (int)fChain->GetEntries();
}

Long64_t CApvRawTree::LoadTree(Long64_t entry)
{
	// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (!fChain->InheritsFrom(TChain::Class()))  return centry;
   TChain *chain = (TChain*)fChain;
   if (chain->GetTreeNumber() != fCurrent) {
      fCurrent = chain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void CApvRawTree::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).
	
   // Set object pointer
	apv_fec = 0;
   apv_id = 0;
   apv_ch = 0;
	mm_id = 0;
   mm_strip = 0;
   apv_q = 0;
	apv_presamples = 0;
   //apv_tb = 0;
	apv_qmax = 0;
	apv_tbqmax = 0;
   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);
	
	fChain->SetBranchAddress("apv_fecNo", &apv_fec, &b_apv_fec);
   fChain->SetBranchAddress("apv_evt", &apv_evt, &b_apv_evt);
   fChain->SetBranchAddress("apv_id", &apv_id, &b_apv_id);
   fChain->SetBranchAddress("apv_ch", &apv_ch, &b_apv_ch);
   fChain->SetBranchAddress("mm_id", &mm_id, &b_mm_id);
   fChain->SetBranchAddress("mm_strip", &mm_strip, &b_mm_strip);
   fChain->SetBranchAddress("apv_q", &apv_q, &b_apv_q);
   fChain->SetBranchAddress("apv_presamples", &apv_presamples, &b_apv_presamples);
	
   
   fChain->SetBranchAddress("apv_qmax", &apv_qmax, &b_apv_qmax);
   fChain->SetBranchAddress("apv_tbqmax", &apv_tbqmax, &b_apv_tbqmax);
   Notify();
}

Bool_t CApvRawTree::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.
	
   return kTRUE;
}

void CApvRawTree::Show(Long64_t entry)
{
	// Print contents of entry.
	// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
//Int_t CApvRawTree::Cut(Long64_t entry)
//{
//	// This function may be called from Loop.
//	// returns  1 if entry is accepted.
//	// returns -1 otherwise.
//   return 1;
//}


void CApvRawTree::Loop()
{
   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;
      // if (Cut(ientry) < 0) continue;
   }
}
