#include "TTree.h"
#include "TFile.h"
#include "TChain.h"
#include "TH1.h"
#include "TH2F.h"
#include "TH3F.h"
#include "THn.h"

#include "StThreeVectorF.hh"
#include "StPicoDstMaker/StPicoDst.h"
#include "StPicoDstMaker/StPicoDstMaker.h"
#include "StPicoDstMaker/StPicoEvent.h"
#include "StPicoDstMaker/StPicoTrack.h"
#include "StPicoDstMaker/StPicoBTofPidTraits.h"
#include "StPicoPrescales/StPicoPrescales.h"
#include "StPicoMixedEventMaker.h"
#include "StPicoEventMixer.h"
#include "StD0Hists.h"
#include "StRoot/StRefMultCorr/StRefMultCorr.h"
#include "StRoot/StEventPlane/StEventPlane.h"
#include "kfEvent.h"

#include <vector>

ClassImp(StPicoMixedEventMaker)

// _________________________________________________________
StPicoMixedEventMaker::StPicoMixedEventMaker(char const* name, StPicoDstMaker* picoMaker, StRefMultCorr* grefmultCorrUtil, StEventPlane* eventPlaneMaker,
      char const* outputBaseFileName,  char const* inputPicoList, char const * kfFileList) :
   StMaker(name), mPicoDst(NULL), mPicoDstMaker(picoMaker),  mPicoEvent(NULL),
   mGRefMultCorrUtil(grefmultCorrUtil), mEventPlaneMaker(eventPlaneMaker),
   mOuputFileBaseName(outputBaseFileName), mInputFileName(inputPicoList),
   mEventCounter(0), mKfFileList(kfFileList), mKfChain(NULL)
{
   mGRefMultCorrUtil->print();
   for (int iVz = 0 ; iVz < 10 ; ++iVz)
   {
      for (int iCentrality = 0 ; iCentrality < 9 ; ++iCentrality)
      {
         for (int iEventPlane = 0 ; iEventPlane < 10 ; ++iEventPlane)
         {
            mPicoEventMixer[iVz][iCentrality][iEventPlane] = NULL;
         }
      }
   }

   //   TH1::AddDirectory(false);

   mOutputFile = new TFile(Form("%s.d0Hists.root", mOuputFileBaseName.Data()), "RECREATE");

   mEventPlaneMaker->setFileOut(mOutputFile);
   mD0Hists = new StD0Hists("picoME");

   // -- constructor
}

// _________________________________________________________
StPicoMixedEventMaker::~StPicoMixedEventMaker()
{
   delete mGRefMultCorrUtil;
   for (int iVz = 0 ; iVz < 10 ; ++iVz)
   {
      for (int iCentrality = 0 ; iCentrality < 9 ; ++iCentrality)
      {
         for (int iEventPlane = 0 ; iEventPlane < 10 ; ++iEventPlane)
         {
            delete mPicoEventMixer[iVz][iCentrality][iEventPlane];
         }
      }
   }
}
/*
// Method should load Q vector stuff from Hao, needs fixing
// _________________________________________________________
bool StPicoMixedEventMaker::loadEventPlaneCorr(StEventPlane const * mEventPlane)
{
   //needs to implement, will currently break maker
   if (!mEventPlane)
   {
      LOG_WARN << "No EventPlane ! Skipping! " << endm;
      return kFALSE;
   }
   if (!mEventPlane->getAcceptEvent())
   {
      // LOG_WARN << "StPicoMixedEvent::THistograms and TProiles NOT found! shoudl check the input Qvector files From HaoQiu ! Skipping this run! " << endm;
      return kFALSE;
   }
   return kTRUE;
}
*/
// _________________________________________________________
Int_t StPicoMixedEventMaker::Init()
{
   for (int iVz = 0 ; iVz < 10 ; ++iVz)
   {
      for (int iCentrality = 0 ; iCentrality < 9 ; ++iCentrality)
      {
         for (int iEventPlane = 0 ; iEventPlane < 10 ; ++iEventPlane)
         {
	   mPicoEventMixer[iVz][iCentrality][iEventPlane] = new StPicoEventMixer(iCentrality, iVz, iEventPlane, mEventPlaneMaker, mD0Hists);
	   //         mPicoEventMixer[iVz][iCentrality][iEventPlane]->setEventBuffer(10);
         }
      }
   }
   // mGRefMultCorrUtil = new StRefMultCorr("grefmult");
   // if(!LoadEventPlaneCorr(mRunId)){
   // LOG_WARN << "Event plane calculations unavalable! Skipping"<<endm;
   // return kStOk;
   // }

   // -- reset event to be in a defined state
   //resetEvent();
   mFailedRunnumber = 0;

   mKfChain = new TChain("kfEvent");
   std::ifstream listOfKfFiles;
   listOfKfFiles.open(mKfFileList);
   if (listOfKfFiles.is_open())
   {
      std::string kffile;
      while (getline(listOfKfFiles, kffile))
      {
         LOG_INFO << "StPicoD0AnaMaker - Adding kfVertex tree:" << kffile << endm;
         mKfChain->Add(kffile.c_str());
      }
   }
   else
   {
      LOG_ERROR << "StPicoD0AnaMaker - Could not open list of corresponding kfEvent files. ABORT!" << endm;
      return kStErr;
   }
   mKfEvent = new kfEvent(mKfChain);


   return kStOK;
}

// _________________________________________________________
Int_t StPicoMixedEventMaker::Finish()
{
   // -- Inhertited from StMaker
   //    NOT TO BE OVERWRITTEN by daughter class
   //    daughter class should implement FinishHF()
   for (int iVz = 0 ; iVz < 10 ; ++iVz)
   {
      for (int iCentrality = 0 ; iCentrality < 9 ; ++iCentrality)
      {
         for (int iEventPlane = 0 ; iEventPlane < 10 ; ++iEventPlane)
         {
            mPicoEventMixer[iVz][iCentrality][iEventPlane]->finish();
            //delete mPicoEventMixer[iVz][iCentrality];
         }
      }
   }
   cout<<"StPicoMixedEventMaker::Finish()"<<endl;
   mOutputFile->cd();

   mD0Hists->hD0CentPtEtaMDphi->Write();
   mD0Hists->hD0CentPtEtaMDphiLikeSign->Write();
   mD0Hists->hD0CentPtEtaMDphiMixed->Write();
   mD0Hists->hD0CentPtEtaMDphiLikeSignMixed->Write();

   mD0Hists->hD0CentPtMDphiEtaGap->Write();
   mD0Hists->hD0CentPtMDphiEtaGapLikeSign->Write();
   mD0Hists->hD0CentPtMDphiEtaGapMixed->Write();
   mD0Hists->hD0CentPtMDphiEtaGapLikeSignMixed->Write();
   //
   mD0Hists->mSE_US_PointingAngle->Write();
   mD0Hists->mSE_US_DecayL->Write();
   mD0Hists->mSE_US_Dca12->Write();
   mD0Hists->mSE_US_PionDca2Vtx->Write();
   mD0Hists->mSE_US_KaonDca2Vtx->Write();
   mD0Hists->mSE_US_D0Dca2Vtx->Write();

   //
   mD0Hists->mSE_LS_PointingAngle->Write();
   mD0Hists->mSE_LS_DecayL->Write();
   mD0Hists->mSE_LS_Dca12->Write();
   mD0Hists->mSE_LS_PionDca2Vtx->Write();
   mD0Hists->mSE_LS_KaonDca2Vtx->Write();
   mD0Hists->mSE_LS_D0Dca2Vtx->Write();
   //
   mD0Hists->mME_US_PointingAngle->Write();
   mD0Hists->mME_US_DecayL->Write();
   mD0Hists->mME_US_Dca12->Write();
   mD0Hists->mME_US_PionDca2Vtx->Write();
   mD0Hists->mME_US_KaonDca2Vtx->Write();
   mD0Hists->mME_US_D0Dca2Vtx->Write();

   mOutputFile->Write();
   //   mOutputFile->Close();
   cout<<"StPicoMixedEventMaker::Finish() done"<<endl;

   return kStOK;
}
// _________________________________________________________
void StPicoMixedEventMaker::Clear(Option_t* opt)
{
}
// _________________________________________________________
Int_t StPicoMixedEventMaker::Make()
{
   mKfChain->GetEntry(mEventCounter++);

   if (!mPicoDstMaker)
   {
      LOG_WARN << "No PicoDstMaker! Skipping! " << endm;
      return kStWarn;
   }

   StPicoDst const* picoDst = mPicoDstMaker->picoDst();
   if (!picoDst)
   {
      LOG_WARN << "No picoDst ! Skipping! " << endm;
      return kStWarn;
   }

   // - GRef from Guannan
   if (!mGRefMultCorrUtil)
   {
      LOG_WARN << " No mGRefMultCorrUtil! Skip! " << endl;
      return kStWarn;
   }

   //Load event
   mPicoEvent = (StPicoEvent*)mPicoDst->event();
   if (!mPicoEvent)
   {
      cerr << "Error opening picoDst Event, skip!" << endl;
      return kStWarn;
   }

   if (mPicoEvent->runId() != mKfEvent->mRunId || mPicoEvent->eventId() != mKfEvent->mEventId)
   {
      LOG_ERROR << " StPicoMixedEventMaker - !!!!!!!!!!!! ATTENTION !!!!!!!!!!!!!" << endm;
      LOG_ERROR << " StPicoMixedEventMaker - SOMETHING TERRIBLE JUST HAPPENED. StPicoDst and KfEvent are not in sync." << endm;
      exit(1);
   }

   StThreeVectorF const picoVertexPos = mPicoEvent->primaryVertex();
   StThreeVectorF const vertexPos(mKfEvent->mKfVx, mKfEvent->mKfVy, mKfEvent->mKfVz);
   if (picoVertexPos.x() != mKfEvent->mVx)
   {
      LOG_ERROR << " StPicoMixedEventMaker - !!!!!!!!!!!! ATTENTION !!!!!!!!!!!!!" << endm;
      LOG_ERROR << " StPicoMixedEventMaker - SOMETHING TERRIBLE JUST HAPPENED. StPicoDst and KfEvent vertex are not in sync." << endm;
      exit(1);
   }

   for (int i = 0; i < 32; i++)
      if (mPicoEvent->triggerWord() >> i & 0x1)
         mD0Hists->hTrigger->Fill(i);

   bool isMinBias = kFALSE;
   for (int i = 0; i < 11; i++)
   {
      if (mPicoEvent->triggerWord() & (1 << i)) isMinBias = kTRUE ; //Select MB trigger
   }
   //if (!(isMinBias)) {cout<<"not a mb trigger"<<endl;return 0;}
   bool isVPDMB5 = kFALSE;
   for (int i = 0; i < 5; i++)
   {
      if (mPicoEvent->triggerWord() & (1 << i)) isVPDMB5 = kTRUE ; //Select MB trigger
   }
   if (!(isVPDMB5))
   {
      //cout<<"not a VPDmb trigger"<<endl;
      return kStOk;
   }

   //Remove bad vertices
   mD0Hists->hVzVpdVz->Fill(vertexPos.z(), mPicoEvent->vzVpd());
   mD0Hists->hVzDiff->Fill(mPicoEvent->vzVpd() - vertexPos.z());
   mD0Hists->hVxy->Fill(vertexPos.x(), vertexPos.y());

   if (TMath::Abs(vertexPos.z()) > mxeCuts::maxVz) return kStOk;
   if (TMath::Abs(vertexPos.z() - mPicoEvent->vzVpd()) > mxeCuts::vzVpdVz) return kStOk;
   if (sqrt(TMath::Power(vertexPos.x(), 2) + TMath::Power(vertexPos.y(), 2)) > mxeCuts:: Vrcut) return kStOk;

   mD0Hists->hRefMult->Fill(mPicoEvent->refMult());
   mD0Hists->hGRefMult->Fill(mPicoEvent->grefMult());

   // - GRef from Guannan
   if (!mGRefMultCorrUtil)
   {
      LOG_WARN << " No mGRefMultCorrUtil! Skip! " << endl;
      return kStWarn;
   }

   mGRefMultCorrUtil->init(mPicoEvent->runId());
   mGRefMultCorrUtil->initEvent(mPicoEvent->grefMult(), vertexPos.z(), mPicoEvent->ZDCx()) ;
   int const centrality  = mGRefMultCorrUtil->getCentralityBin9();
   float weight = mGRefMultCorrUtil->getWeight();
   mD0Hists->hCentrality->Fill(centrality);
   mD0Hists->hCentralityWeighted->Fill(centrality, weight);
   if (centrality < 0 || centrality > 8) return kStOk;

   int const vz_bin = (int)((6 + vertexPos.z()) / 1.2) ;
   if (vz_bin < 0  ||  vz_bin > 9) return kStOk;

   /*
   if (mFailedRunnumber != mPicoEvent->runId())
     {
       if (!loadEventPlaneCorr(mEventPlane))
    {
      LOG_WARN << "Event plane calculations unavalable! Skipping" << endm;
      mFailedRunnumber = picoDst->event()->runId();
      return kStOK;
    }
     }
   else  return kStOK;
   */

   float const eventPlane = mEventPlaneMaker->getEventPlane();
   int const eventPlane_bin = (int)(eventPlane / TMath::Pi() * 10.) ;
   if (eventPlane_bin < 0  ||  eventPlane_bin > 9 || mEventPlaneMaker->eventPlaneStatus()) return kStOk;

   mD0Hists->hCentVzPsi->Fill(centrality, vertexPos.z(), eventPlane, weight);

   if (mPicoEventMixer[vz_bin][centrality][eventPlane_bin]->addPicoEvent(picoDst, vertexPos, weight))
     mPicoEventMixer[vz_bin][centrality][eventPlane_bin]->mixEvents();

   return kStOk;
}


