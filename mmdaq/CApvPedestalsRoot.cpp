#include "CApvPedestalsRoot.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>

#include <cstdlib>
#include <iostream>
#include <sys/stat.h>


//TODO:: ADD EXCEPTIONS, THROW on bad file
CApvPedestalsRoot::CApvPedestalsRoot(const char* filename)
: rootfile(0), ferror(0)
{
	TString thename(filename);
	struct stat stFileInfo;
	if (filename && strlen(filename) && !stat(thename.Data(), &stFileInfo)) {
		rootfile = new TFile(thename);
		if (rootfile->IsOpen()) {
			TTree *tree = (TTree*)rootfile->Get("pedestals");
			Init(tree);
		}
	}
	else {
		CApvPedestalsRoot((TTree*)0);
	}
}


CApvPedestalsRoot::CApvPedestalsRoot(TTree *tree)
: rootfile(0), ferror(0)
{
	// if parameter tree is not specified (or zero), connect the file
	// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("run10354.root");
      if (!f) {
         f = new TFile("run10354.root");
      }
      tree = (TTree*)gDirectory->Get("pedestals");
		
   }
   Init(tree);
}


CApvPedestalsRoot::~CApvPedestalsRoot()
{
	if (rootfile) {
		delete rootfile;// automatically calls rootfile->Close(); 
		rootfile = 0;
	}
	if (!fChain) return;
   delete fChain->GetCurrentFile();
}


Int_t CApvPedestalsRoot::GetEntry(Long64_t entry)
{
	// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}

Int_t CApvPedestalsRoot::GetEntries()
{
   if (!fChain) return -1;
   return (int)fChain->GetEntries();
}


Long64_t CApvPedestalsRoot::LoadTree(Long64_t entry)
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

void CApvPedestalsRoot::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).
	
   // Set object pointer
   apv_fecNo = 0;
   apv_id = 0;
   apv_ch = 0;
   mm_id = 0;
   mm_strip = 0;
   apv_pedmean = 0;
   apv_pedsigma = 0;
   apv_pedstd = 0;
   // Set branch addresses and branch pointers
   if (!tree) {
		ferror = 1;
		std::cerr << "CApvPedestalsRoot::Init() signals there is no tree" << std::endl;
		return;
	}
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);
	
	ferror = 0;
   ferror |= fChain->SetBranchAddress("apv_evt", &apv_evt, &b_apv_evt);
   ferror |= fChain->SetBranchAddress("time_s", &time_s, &b_time_s);
   ferror |= fChain->SetBranchAddress("time_us", &time_us, &b_time_us);
   ferror |= fChain->SetBranchAddress("apv_fecNo", &apv_fecNo, &b_apv_fecNo);
   ferror |= fChain->SetBranchAddress("apv_id", &apv_id, &b_apv_id);
   ferror |= fChain->SetBranchAddress("apv_ch", &apv_ch, &b_apv_ch);
   ferror |= fChain->SetBranchAddress("mm_id", &mm_id, &b_mm_id);
   ferror |= fChain->SetBranchAddress("mm_strip", &mm_strip, &b_mm_strip);
	ferror |= fChain->SetBranchAddress("apv_pedmean", &apv_pedmean, &b_apv_pedmean);
   ferror |= fChain->SetBranchAddress("apv_pedsigma", &apv_pedsigma, &b_apv_pedsigma);
   ferror |= fChain->SetBranchAddress("apv_pedstd", &apv_pedstd, &b_apv_pedstd);
	if (ferror < 0 ) {
		std::cerr << "CApvPedestalsRoot::Init(): SetBranchAddress() returned " <<  ferror << ": Check if the file does contain all columns in pedestals tree " << std::endl;
		//exit(1); //throw
	}
   Notify();
}


Bool_t CApvPedestalsRoot::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.
	
   return kTRUE;
}

void CApvPedestalsRoot::Show(Long64_t entry)
{
	// Print contents of entry.
	// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t CApvPedestalsRoot::Cut(Long64_t entry)
{
	// This function may be called from Loop.
	// returns  1 if entry is accepted.
	// returns -1 otherwise.
   return 1;
}


void CApvPedestalsRoot::Loop()
{
//   In a ROOT session, you can do:
//      Root > .L CApvPedestalsRoot.C
//      Root > CApvPedestalsRoot t
//      Root > t.GetEntry(12); // Fill t data members with entry number 12
//      Root > t.Show();       // Show values of entry 12
//      Root > t.Show(16);     // Read and show values of entry 16
//      Root > t.Loop();       // Loop on all entries
//

//     This is the loop skeleton where:
//    jentry is the global entry number in the chain
//    ientry is the entry number in the current Tree
//  Note that the argument to GetEntry must be:
//    jentry for TChain::GetEntry
//    ientry for TTree::GetEntry and TBranch::GetEntry
//
//       To read only selected branches, Insert statements like:
// METHOD1:
//    fChain->SetBranchStatus("*",0);  // disable all branches
//    fChain->SetBranchStatus("branchname",1);  // activate branchname
// METHOD2: replace line
//    fChain->GetEntry(jentry);       //read all branches
//by  b_branchname->GetEntry(ientry); //read only this branch
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
