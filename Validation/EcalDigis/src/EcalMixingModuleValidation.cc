/*
 * \file EcalMixingModuleValidation.cc
 *
 * $Date: 2007/08/08 08:05:56 $
 * $Revision: 1.11 $
 * \author F. Cossutti
 *
*/

#include <Validation/EcalDigis/interface/EcalMixingModuleValidation.h>
#include <DataFormats/EcalDetId/interface/EBDetId.h>
#include <DataFormats/EcalDetId/interface/EEDetId.h>
#include <DataFormats/EcalDetId/interface/ESDetId.h>
#include "CalibCalorimetry/EcalTrivialCondModules/interface/EcalTrivialConditionRetriever.h"
#include "Geometry/Records/interface/IdealGeometryRecord.h"
#include "DataFormats/EcalDigi/interface/EcalDataFrame.h"

using namespace cms;
using namespace edm;
using namespace std;

EcalMixingModuleValidation::EcalMixingModuleValidation(const ParameterSet& ps):
  HepMCLabel(ps.getParameter<std::string>("moduleLabelMC")),
  EBdigiCollection_(ps.getParameter<edm::InputTag>("EBdigiCollection")),
  EEdigiCollection_(ps.getParameter<edm::InputTag>("EEdigiCollection")),
  ESdigiCollection_(ps.getParameter<edm::InputTag>("ESdigiCollection")){


  // needed for MixingModule checks

  double simHitToPhotoelectronsBarrel = ps.getParameter<double>("simHitToPhotoelectronsBarrel");
  double simHitToPhotoelectronsEndcap = ps.getParameter<double>("simHitToPhotoelectronsEndcap");
  double photoelectronsToAnalogBarrel = ps.getParameter<double>("photoelectronsToAnalogBarrel");
  double photoelectronsToAnalogEndcap = ps.getParameter<double>("photoelectronsToAnalogEndcap");
  double samplingFactor = ps.getParameter<double>("samplingFactor");
  double timePhase = ps.getParameter<double>("timePhase");
  int readoutFrameSize = ps.getParameter<int>("readoutFrameSize");
  int binOfMaximum = ps.getParameter<int>("binOfMaximum");
  bool doPhotostatistics = ps.getParameter<bool>("doPhotostatistics");
  bool syncPhase = ps.getParameter<bool>("syncPhase");

  doPhotostatistics = false;
    
  theParameterMap = new EcalSimParameterMap(simHitToPhotoelectronsBarrel, simHitToPhotoelectronsEndcap, 
                                            photoelectronsToAnalogBarrel, photoelectronsToAnalogEndcap, 
                                            samplingFactor, timePhase, readoutFrameSize, binOfMaximum,
                                            doPhotostatistics, syncPhase);
  theEcalShape = new EcalShape(timePhase);

  theEcalResponse = new CaloHitResponse(theParameterMap, theEcalShape);

  int ESGain = ps.getParameter<int>("ESGain");
  double ESNoiseSigma = ps.getParameter<double> ("ESNoiseSigma");
  int ESBaseline = ps.getParameter<int>("ESBaseline");
  double ESMIPADC = ps.getParameter<double>("ESMIPADC");
  double ESMIPkeV = ps.getParameter<double>("ESMIPkeV");

  theESShape = new ESShape(ESGain);

  theESResponse = new CaloHitResponse(theParameterMap, theESShape);

  double effwei = 1.;
 
  if (ESGain == 0)
    effwei = 1.45;
  else if (ESGain == 1)
    effwei = 0.9066;
  else if (ESGain == 2)
    effwei = 0.8815;
 
  esBaseline_ = (double)ESBaseline;
  esADCtokeV_ = 1000000.*ESMIPADC/ESMIPkeV;
  esThreshold_ = 3.*effwei*ESNoiseSigma/esADCtokeV_;

  theMinBunch = -10;
  theMaxBunch = 10;
    
 
  // verbosity switch
  verbose_ = ps.getUntrackedParameter<bool>("verbose", false);
 
  if ( verbose_ ) {
    cout << " verbose switch is ON" << endl;
  } else {
    cout << " verbose switch is OFF" << endl;
  }
                                                                                                                                          
  dbe_ = 0;
                                                                                                                                          
  // get hold of back-end interface
  dbe_ = Service<DaqMonitorBEInterface>().operator->();
                                                                                                                                          
  if ( dbe_ ) {
    if ( verbose_ ) {
      dbe_->setVerbose(1);
    } else {
      dbe_->setVerbose(0);
    }
  }
                                                                                                                                          
  if ( dbe_ ) {
    if ( verbose_ ) dbe_->showDirStructure();
  }

  gainConv_[1] = 1.;
  gainConv_[2] = 2.;
  gainConv_[3] = 12.;
  gainConv_[0] = 12.;
  barrelADCtoGeV_ = 0.035;
  endcapADCtoGeV_ = 0.06;
 
  meEBDigiMixRatiogt100ADC_ = 0;
  meEEDigiMixRatiogt100ADC_ = 0;
    
  meEBDigiMixRatioOriggt50pc_ = 0;
  meEEDigiMixRatioOriggt40pc_ = 0;
    
  meEBbunchCrossing_ = 0;
  meEEbunchCrossing_ = 0;
  meESbunchCrossing_ = 0;
    
  for ( int i = 0 ; i < nBunch ; i++ ) {
    meEBBunchShape_[i] = 0;
    meEEBunchShape_[i] = 0;
    meESBunchShape_[i] = 0;
  }

  meEBShape_ = 0;
  meEEShape_ = 0;
  meESShape_ = 0;

  meEBShapeRatio_ = 0;
  meEEShapeRatio_ = 0;
  meESShapeRatio_ = 0;
    

  Char_t histo[200];
 
  
  if ( dbe_ ) {
    dbe_->setCurrentFolder("EcalDigiTask");
  
    sprintf (histo, "EcalDigiTask Barrel maximum Digi over sim signal ratio gt 100 ADC" ) ;
    meEBDigiMixRatiogt100ADC_ = dbe_->book1D(histo, histo, 200, 0., 100.) ;
      
    sprintf (histo, "EcalDigiTask Endcap maximum Digi over sim signal ratio gt 100 ADC" ) ;
    meEEDigiMixRatiogt100ADC_ = dbe_->book1D(histo, histo, 200, 0., 100.) ;
      
    sprintf (histo, "EcalDigiTask Barrel maximum Digi over sim signal ratio signal gt 50pc gun" ) ;
    meEBDigiMixRatioOriggt50pc_ = dbe_->book1D(histo, histo, 200, 0., 100.) ;
      
    sprintf (histo, "EcalDigiTask Endcap maximum Digi over sim signal ratio signal gt 40pc gun" ) ;
    meEEDigiMixRatioOriggt40pc_ = dbe_->book1D(histo, histo, 200, 0., 100.) ;
      
    sprintf (histo, "EcalDigiTask Barrel bunch crossing" ) ;
    meEBbunchCrossing_ = dbe_->book1D(histo, histo, 20, -10., 10.) ;
      
    sprintf (histo, "EcalDigiTask Endcap bunch crossing" ) ;
    meEEbunchCrossing_ = dbe_->book1D(histo, histo, 20, -10., 10.) ;
      
    sprintf (histo, "EcalDigiTask Preshower bunch crossing" ) ;
    meESbunchCrossing_ = dbe_->book1D(histo, histo, 20, -10., 10.) ;

    for ( int i = 0 ; i < nBunch ; i++ ) {

      sprintf (histo, "EcalDigiTask Barrel shape bunch crossing %02d", i-10 );
      meEBBunchShape_[i] = dbe_->bookProfile(histo, histo, 10, 0, 10, 4000, 0., 400.);

      sprintf (histo, "EcalDigiTask Endcap shape bunch crossing %02d", i-10 );
      meEEBunchShape_[i] = dbe_->bookProfile(histo, histo, 10, 0, 10, 4000, 0., 400.);

      sprintf (histo, "EcalDigiTask Preshower shape bunch crossing %02d", i-10 );
      meESBunchShape_[i] = dbe_->bookProfile(histo, histo, 3, 0, 3, 4000, 0., 400.);

    }

    sprintf (histo, "EcalDigiTask Barrel shape digi");
    meEBShape_ = dbe_->bookProfile(histo, histo, 10, 0, 10, 4000, 0., 2000.);

    sprintf (histo, "EcalDigiTask Endcap shape digi");
    meEEShape_ = dbe_->bookProfile(histo, histo, 10, 0, 10, 4000, 0., 2000.);

    sprintf (histo, "EcalDigiTask Preshower shape digi");
    meESShape_ = dbe_->bookProfile(histo, histo, 3, 0, 3, 4000, 0., 2000.);

    sprintf (histo, "EcalDigiTask Barrel shape digi ratio");
    meEBShapeRatio_ = dbe_->book1D(histo, histo, 10, 0, 10.);

    sprintf (histo, "EcalDigiTask Endcap shape digi ratio");
    meEEShapeRatio_ = dbe_->book1D(histo, histo, 10, 0, 10.);

    sprintf (histo, "EcalDigiTask Preshower shape digi ratio");
    meESShapeRatio_ = dbe_->book1D(histo, histo, 3, 0, 3.);
     
  }
 
}

EcalMixingModuleValidation::~EcalMixingModuleValidation(){}

void EcalMixingModuleValidation::beginJob(const EventSetup& c){

  checkCalibrations(c);

}

void EcalMixingModuleValidation::endJob(){

  // add shapes for each bunch crossing and divide the digi by the result
  
  std::vector<MonitorElement *> theBunches;
  for ( int i = 0 ; i < nBunch ; i++ ) {
    theBunches.push_back(meEBBunchShape_[i]);
  }
  bunchSumTest(theBunches , meEBShape_ , meEBShapeRatio_ , EcalDataFrame::MAXSAMPLES);
  
  theBunches.clear();
  
  for ( int i = 0 ; i < nBunch ; i++ ) {
    theBunches.push_back(meEEBunchShape_[i]);
  }
  bunchSumTest(theBunches , meEEShape_ , meEEShapeRatio_ , EcalDataFrame::MAXSAMPLES);
  
  theBunches.clear();
  
  for ( int i = 0 ; i < nBunch ; i++ ) {
    theBunches.push_back(meESBunchShape_[i]);
  }
  bunchSumTest(theBunches , meESShape_ , meESShapeRatio_ , ESDataFrame::MAXSAMPLES);
  
}

void EcalMixingModuleValidation::bunchSumTest(std::vector<MonitorElement *> & theBunches, MonitorElement* & theTotal, MonitorElement* & theRatio, int nSample)
{

  std::vector<double> bunchSum;
  bunchSum.reserve(nSample);
  std::vector<double> bunchSumErro;
  bunchSumErro.reserve(nSample);
  std::vector<double> total;
  total.reserve(nSample);
  std::vector<double> totalErro;
  totalErro.reserve(nSample);
  std::vector<double> ratio;
  ratio.reserve(nSample);
  std::vector<double> ratioErro;
  ratioErro.reserve(nSample);


  for ( int iEl = 0 ; iEl < nSample ; iEl++ ) {
    bunchSum[iEl] = 0.;
    bunchSumErro[iEl] = 0.;
    total[iEl] = 0.;
    totalErro[iEl] = 0.;
    ratio[iEl] = 0.;
    ratioErro[iEl] = 0.;
  }

  for ( int iSample = 0 ; iSample < nSample ; iSample++ ) { 

    total[iSample] += theTotal->getBinContent(iSample+1);
    totalErro[iSample] += theTotal->getBinError(iSample+1);

    for ( int iBunch = theMinBunch; iBunch <= theMaxBunch; iBunch++ ) {

      int iHisto = iBunch - theMinBunch;

      bunchSum[iSample] += theBunches[iHisto]->getBinContent(iSample+1);
      bunchSumErro[iSample] += pow(theBunches[iHisto]->getBinError(iSample+1),2);

    }
    bunchSumErro[iSample] = sqrt(bunchSumErro[iSample]);

    if ( bunchSum[iSample] > 0. ) { 
      ratio[iSample] = total[iSample]/bunchSum[iSample];
      ratioErro[iSample] = sqrt(pow(totalErro[iSample]/bunchSum[iSample],2)+
                                pow((total[iSample]*bunchSumErro[iSample])/(bunchSum[iSample]*bunchSum[iSample]),2));
    }

    std::cout << " Sample = " << iSample << " Total = " << total[iSample] << " +- " << totalErro[iSample] << "\n" 
              << " Sum   = " << bunchSum[iSample] << " +- " << bunchSumErro[iSample] << "\n" 
              << " Ratio = " << ratio[iSample] << " +- " << ratioErro[iSample] << std::endl;
      
    theRatio->setBinContent(iSample+1, (float)ratio[iSample]);
    theRatio->setBinError(iSample+1, (float)ratioErro[iSample]);

  }

} 

void EcalMixingModuleValidation::analyze(const Event& e, const EventSetup& c){

  //LogInfo("EventInfo") << " Run = " << e.id().run() << " Event = " << e.id().event();

  checkPedestals(c);

  vector<SimTrack> theSimTracks;
  vector<SimVertex> theSimVertexes;

  Handle<HepMCProduct> MCEvt;
  Handle<CrossingFrame<PCaloHit> > crossingFrame;
  Handle<EBDigiCollection> EcalDigiEB;
  Handle<EEDigiCollection> EcalDigiEE;
  Handle<ESDigiCollection> EcalDigiES;

  
  bool skipMC = false;
  try {
  e.getByLabel(HepMCLabel, MCEvt);
  } catch ( cms::Exception &e ) { skipMC = true; }
  //e.getByType(crossingFrame);

  const EBDigiCollection* EBdigis =0;
  const EEDigiCollection* EEdigis =0;
  const ESDigiCollection* ESdigis =0;

  bool isBarrel = true;
  try {
    e.getByLabel( EBdigiCollection_, EcalDigiEB );
    EBdigis = EcalDigiEB.product();
    LogDebug("DigiInfo") << "total # EBdigis: " << EBdigis->size() ;
    if ( EBdigis->size() == 0 ) isBarrel = false;
  } catch ( cms::Exception &e ) { isBarrel = false; }
  bool isEndcap = true;
  try {
    e.getByLabel( EEdigiCollection_, EcalDigiEE );
    EEdigis = EcalDigiEE.product();
    LogDebug("DigiInfo") << "total # EEdigis: " << EEdigis->size() ;
    if ( EEdigis->size() == 0 ) isEndcap = false;
  } catch ( cms::Exception &e ) { isEndcap = false; }
  bool isPreshower = true;
  try {
    e.getByLabel( ESdigiCollection_, EcalDigiES );
    ESdigis = EcalDigiES.product();
    LogDebug("DigiInfo") << "total # ESdigis: " << ESdigis->size() ;
    if ( ESdigis->size() == 0 ) isPreshower = false;
  } catch ( cms::Exception &e ) { isPreshower = false; }

  double theGunEnergy = 0.;
  if ( ! skipMC ) {
    for ( HepMC::GenEvent::particle_const_iterator p = MCEvt->GetEvent()->particles_begin();
          p != MCEvt->GetEvent()->particles_end(); ++p ) {
      
      theGunEnergy = (*p)->momentum().e();
      }
  }
  // in case no HepMC available, assume an arbitrary average energy for an interesting "gun"
  else { 
    edm::LogWarning("DigiInfo") << "No HepMC available, using 30 GeV as giun energy";
    theGunEnergy = 30.; 
  }

  // BARREL

  // loop over simHits

  if ( isBarrel ) {

    const std::string barrelHitsName ("EcalHitsEB") ;
    e.getByLabel("mix",barrelHitsName,crossingFrame);
    std::auto_ptr<MixCollection<PCaloHit> > 
      barrelHits (new MixCollection<PCaloHit>(crossingFrame.product ()));
    
    MapType ebSignalSimMap;

    double ebSimThreshold = 0.5*theGunEnergy;

    for (MixCollection<PCaloHit>::MixItr hitItr = barrelHits->begin () ;
         hitItr != barrelHits->end () ;
         ++hitItr) {
      
      EBDetId ebid = EBDetId(hitItr->id()) ;
      
      LogDebug("HitInfo") 
        << " CaloHit " << hitItr->getName() << "\n" 
        << " DetID = "<<hitItr->id()<< " EBDetId = " << ebid.ieta() << " " << ebid.iphi() << "\n"	
        << " Time = " << hitItr->time() << " Event id. = " << hitItr->eventId().rawId() << "\n"
        << " Track Id = " << hitItr->geantTrackId() << "\n"
        << " Energy = " << hitItr->energy();

      uint32_t crystid = ebid.rawId();

      if ( hitItr->eventId().rawId() == 0 ) ebSignalSimMap[crystid] += hitItr->energy();
      
      if ( meEBbunchCrossing_ ) meEBbunchCrossing_->Fill(hitItr->eventId().bunchCrossing()); 
      
    }
    
    // loop over Digis
    
    const EBDigiCollection * barrelDigi = EcalDigiEB.product () ;
    
    std::vector<double> ebAnalogSignal ;
    std::vector<double> ebADCCounts ;
    std::vector<double> ebADCGains ;
    ebAnalogSignal.reserve(EBDataFrame::MAXSAMPLES);
    ebADCCounts.reserve(EBDataFrame::MAXSAMPLES);
    ebADCGains.reserve(EBDataFrame::MAXSAMPLES);
    
    for (unsigned int digis=0; digis<EcalDigiEB->size(); ++digis) {

      EBDataFrame ebdf=(*barrelDigi)[digis];
      int nrSamples=ebdf.size();
      
      EBDetId ebid = ebdf.id () ;
      
      double Emax = 0. ;
      int Pmax = 0 ;
      
      for (int sample = 0 ; sample < nrSamples; ++sample) {
	ebAnalogSignal[sample] = 0.;
	ebADCCounts[sample] = 0.;
	ebADCGains[sample] = -1.;
      }
      
      for (int sample = 0 ; sample < nrSamples; ++sample) {

	EcalMGPASample mySample = ebdf[sample];
	
	ebADCCounts[sample] = (mySample.adc()) ;
	ebADCGains[sample]  = (mySample.gainId ()) ;
	ebAnalogSignal[sample] = (ebADCCounts[sample]*gainConv_[(int)ebADCGains[sample]]*barrelADCtoGeV_);
	if (Emax < ebAnalogSignal[sample] ) {
	  Emax = ebAnalogSignal[sample] ;
	  Pmax = sample ;
	}
	LogDebug("DigiInfo") << "EB sample " << sample << " ADC counts = " << ebADCCounts[sample] << " Gain Id = " << ebADCGains[sample] << " Analog eq = " << ebAnalogSignal[sample];
      }
      double pedestalPreSampleAnalog = 0.;
      findPedestal( ebid, (int)ebADCGains[Pmax] , pedestalPreSampleAnalog);
      pedestalPreSampleAnalog *= gainConv_[(int)ebADCGains[Pmax]]*barrelADCtoGeV_;
      double Erec = Emax - pedestalPreSampleAnalog;
      
      if ( ebSignalSimMap[ebid.rawId()] != 0. ) {
	LogDebug("DigiInfo") << " Digi / Signal Hit = " << Erec << " / " << ebSignalSimMap[ebid.rawId()] << " gainConv " << gainConv_[(int)ebADCGains[Pmax]];
	if ( Erec > 100.*barrelADCtoGeV_  && meEBDigiMixRatiogt100ADC_  ) meEBDigiMixRatiogt100ADC_->Fill( Erec/ebSignalSimMap[ebid.rawId()] );
	if ( ebSignalSimMap[ebid.rawId()] > ebSimThreshold  && meEBDigiMixRatioOriggt50pc_ ) meEBDigiMixRatioOriggt50pc_->Fill( Erec/ebSignalSimMap[ebid.rawId()] );
	if ( ebSignalSimMap[ebid.rawId()] > ebSimThreshold  && meEBShape_ ) {
	  for ( int i = 0; i < 10 ; i++ ) {
	    pedestalPreSampleAnalog = 0.;
	    findPedestal( ebid, (int)ebADCGains[i] , pedestalPreSampleAnalog);
	    pedestalPreSampleAnalog *= gainConv_[(int)ebADCGains[i]]*barrelADCtoGeV_;
	    meEBShape_->Fill(i, ebAnalogSignal[i]-pedestalPreSampleAnalog );
	  }
	}
      }
      
    } 
    
    EcalSubdetector thisDet = EcalBarrel;
    computeSDBunchDigi(c, *barrelHits, ebSignalSimMap, thisDet, ebSimThreshold);
  }
  
  
  // ENDCAP

  // loop over simHits

  if ( isEndcap ) {

    const std::string endcapHitsName ("EcalHitsEE") ;
    e.getByLabel("mix",endcapHitsName,crossingFrame);
    std::auto_ptr<MixCollection<PCaloHit> > 
      endcapHits (new MixCollection<PCaloHit>(crossingFrame.product ()));
    
    MapType eeSignalSimMap;

    double eeSimThreshold = 0.4*theGunEnergy;
    
    for (MixCollection<PCaloHit>::MixItr hitItr = endcapHits->begin () ;
         hitItr != endcapHits->end () ;
         ++hitItr) {
      
      EEDetId eeid = EEDetId(hitItr->id()) ;
      
      LogDebug("HitInfo") 
        << " CaloHit " << hitItr->getName() << "\n" 
        << " DetID = "<<hitItr->id()<< " EEDetId side = " << eeid.zside() << " = " << eeid.ix() << " " << eeid.iy() << "\n"
        << " Time = " << hitItr->time() << " Event id. = " << hitItr->eventId().rawId() << "\n"
        << " Track Id = " << hitItr->geantTrackId() << "\n"
        << " Energy = " << hitItr->energy();
      
      uint32_t crystid = eeid.rawId();

      if ( hitItr->eventId().rawId() == 0 ) eeSignalSimMap[crystid] += hitItr->energy();
      
      if ( meEEbunchCrossing_ ) meEEbunchCrossing_->Fill(hitItr->eventId().bunchCrossing()); 

    }
    
    // loop over Digis
    
    const EEDigiCollection * endcapDigi = EcalDigiEE.product () ;
    
    std::vector<double> eeAnalogSignal ;
    std::vector<double> eeADCCounts ;
    std::vector<double> eeADCGains ;
    eeAnalogSignal.reserve(EEDataFrame::MAXSAMPLES);
    eeADCCounts.reserve(EEDataFrame::MAXSAMPLES);
    eeADCGains.reserve(EEDataFrame::MAXSAMPLES);
    
    for (unsigned int digis=0; digis<EcalDigiEE->size(); ++digis) {

      EEDataFrame eedf=(*endcapDigi)[digis];
      int nrSamples=eedf.size();
      
      EEDetId eeid = eedf.id () ;
      
      double Emax = 0. ;
      int Pmax = 0 ;
      
      for (int sample = 0 ; sample < nrSamples; ++sample) {
	eeAnalogSignal[sample] = 0.;
	eeADCCounts[sample] = 0.;
	eeADCGains[sample] = -1.;
      }
      
      for (int sample = 0 ; sample < nrSamples; ++sample) {
	
	EcalMGPASample mySample = eedf[sample];
	
	eeADCCounts[sample] = (mySample.adc()) ;
	eeADCGains[sample]  = (mySample.gainId()) ;
	eeAnalogSignal[sample] = (eeADCCounts[sample]*gainConv_[(int)eeADCGains[sample]]*endcapADCtoGeV_);
	if (Emax < eeAnalogSignal[sample] ) {
	  Emax = eeAnalogSignal[sample] ;
	  Pmax = sample ;
	}
	LogDebug("DigiInfo") << "EE sample " << sample << " ADC counts = " << eeADCCounts[sample] << " Gain Id = " << eeADCGains[sample] << " Analog eq = " << eeAnalogSignal[sample];
      }
      double pedestalPreSampleAnalog = 0.;
      findPedestal( eeid, (int)eeADCGains[Pmax] , pedestalPreSampleAnalog);
      pedestalPreSampleAnalog *= gainConv_[(int)eeADCGains[Pmax]]*endcapADCtoGeV_;
      double Erec = Emax - pedestalPreSampleAnalog;
      
      if ( eeSignalSimMap[eeid.rawId()] != 0. ) {
	LogDebug("DigiInfo") << " Digi / Signal Hit = " << Erec << " / " << eeSignalSimMap[eeid.rawId()] << " gainConv " << gainConv_[(int)eeADCGains[Pmax]];
	if ( Erec > 100.*endcapADCtoGeV_  && meEEDigiMixRatiogt100ADC_  ) meEEDigiMixRatiogt100ADC_->Fill( Erec/eeSignalSimMap[eeid.rawId()] );
	if ( eeSignalSimMap[eeid.rawId()] > eeSimThreshold  && meEEDigiMixRatioOriggt40pc_ ) meEEDigiMixRatioOriggt40pc_->Fill( Erec/eeSignalSimMap[eeid.rawId()] );
	if ( eeSignalSimMap[eeid.rawId()] > eeSimThreshold  && meEBShape_ ) {
	  for ( int i = 0; i < 10 ; i++ ) {
	    pedestalPreSampleAnalog = 0.;
	    findPedestal( eeid, (int)eeADCGains[i] , pedestalPreSampleAnalog);
	    pedestalPreSampleAnalog *= gainConv_[(int)eeADCGains[i]]*endcapADCtoGeV_;
	    meEEShape_->Fill(i, eeAnalogSignal[i]-pedestalPreSampleAnalog );
	  }
          }
      }
    }
    
    EcalSubdetector thisDet = EcalEndcap;
    computeSDBunchDigi(c, *endcapHits, eeSignalSimMap, thisDet, eeSimThreshold);
  }

  if ( isPreshower) {

    const std::string preshowerHitsName ("EcalHitsES") ;
    e.getByLabel("mix",preshowerHitsName,crossingFrame);
    std::auto_ptr<MixCollection<PCaloHit> > 
      preshowerHits (new MixCollection<PCaloHit>(crossingFrame.product ()));
    
    MapType esSignalSimMap;
    
    for (MixCollection<PCaloHit>::MixItr hitItr = preshowerHits->begin () ;
         hitItr != preshowerHits->end () ;
         ++hitItr) {
      
      ESDetId esid = ESDetId(hitItr->id()) ;
      
      LogDebug("HitInfo") 
        << " CaloHit " << hitItr->getName() << "\n" 
        << " DetID = "<<hitItr->id()<< "ESDetId: z side " << esid.zside() << "  plane " << esid.plane() << esid.six() << ',' << esid.siy() << ':' << esid.strip() << "\n"
        << " Time = " << hitItr->time() << " Event id. = " << hitItr->eventId().rawId() << "\n"
        << " Track Id = " << hitItr->geantTrackId() << "\n"
        << " Energy = " << hitItr->energy();

      uint32_t stripid = esid.rawId();

      if ( hitItr->eventId().rawId() == 0 ) esSignalSimMap[stripid] += hitItr->energy();
      
      if ( meESbunchCrossing_ ) meESbunchCrossing_->Fill(hitItr->eventId().bunchCrossing()); 
  
      // loop over Digis

      const ESDigiCollection * preshowerDigi = EcalDigiES.product () ;

      std::vector<double> esADCCounts ;
      std::vector<double> esADCAnalogSignal ;
      esADCCounts.reserve(ESDataFrame::MAXSAMPLES);
      esADCAnalogSignal.reserve(ESDataFrame::MAXSAMPLES);

      for (unsigned int digis=0; digis<EcalDigiES->size(); ++digis) {

	ESDataFrame esdf=(*preshowerDigi)[digis];
	int nrSamples=esdf.size();
	
	ESDetId esid = esdf.id () ;
	
	for (int sample = 0 ; sample < nrSamples; ++sample) {
	  esADCCounts[sample] = 0.;
	  esADCAnalogSignal[sample] = 0.;
	}
	
	for (int sample = 0 ; sample < nrSamples; ++sample) {

	  ESSample mySample = esdf[sample];
	  
	  esADCCounts[sample] = (mySample.adc()) ;
	  esADCAnalogSignal[sample] = (esADCCounts[sample]-esBaseline_)/esADCtokeV_;
	}
	LogDebug("DigiInfo") << "Preshower Digi for ESDetId: z side " << esid.zside() << "  plane " << esid.plane() << esid.six() << ',' << esid.siy() << ':' << esid.strip();
	for ( int i = 0; i < 3 ; i++ ) {
	  LogDebug("DigiInfo") << "sample " << i << " ADC = " << esADCCounts[i] << " Analog eq = " << esADCAnalogSignal[i];
	}
	
	if ( esSignalSimMap[esid.rawId()] > esThreshold_  && meESShape_ ) {
	  for ( int i = 0; i < 3 ; i++ ) {
	    meESShape_->Fill(i, esADCAnalogSignal[i] );
	  }
	}
	
      } 
      
    }
    
    EcalSubdetector thisDet = EcalPreshower;
    computeSDBunchDigi(c, *preshowerHits, esSignalSimMap, thisDet, esThreshold_);
    
  }
  
}                                                                                       

void  EcalMixingModuleValidation::checkCalibrations(const edm::EventSetup & eventSetup) {
  
  // ADC -> GeV Scale
  edm::ESHandle<EcalADCToGeVConstant> pAgc;
  eventSetup.get<EcalADCToGeVConstantRcd>().get(pAgc);
  const EcalADCToGeVConstant* agc = pAgc.product();
  
  EcalMGPAGainRatio * defaultRatios = new EcalMGPAGainRatio();

  gainConv_[1] = 1.;
  gainConv_[2] = defaultRatios->gain12Over6() ;
  gainConv_[3] = gainConv_[2]*(defaultRatios->gain6Over1()) ;
  gainConv_[0] = gainConv_[2]*(defaultRatios->gain6Over1()) ;

  LogDebug("EcalDigi") << " Gains conversions: " << "\n" << " g1 = " << gainConv_[1] << "\n" << " g2 = " << gainConv_[2] << "\n" << " g3 = " << gainConv_[3];

  delete defaultRatios;

  const double barrelADCtoGeV_  = agc->getEBValue();
  LogDebug("EcalDigi") << " Barrel GeV/ADC = " << barrelADCtoGeV_;
  const double endcapADCtoGeV_ = agc->getEEValue();
  LogDebug("EcalDigi") << " Endcap GeV/ADC = " << endcapADCtoGeV_;

}

void EcalMixingModuleValidation::checkPedestals(const edm::EventSetup & eventSetup)
{

  // Pedestals from event setup

  edm::ESHandle<EcalPedestals> dbPed;
  eventSetup.get<EcalPedestalsRcd>().get( dbPed );
  thePedestals=dbPed.product();
  
}

void EcalMixingModuleValidation::findPedestal(const DetId & detId, int gainId, double & ped) const
{
  EcalPedestalsMapIterator mapItr 
    = thePedestals->m_pedestals.find(detId.rawId());
  // should I care if it doesn't get found?
  if(mapItr == thePedestals->m_pedestals.end()) {
    edm::LogError("EcalMMValid") << "Could not find pedestal for " << detId.rawId() << " among the " << thePedestals->m_pedestals.size();
  } else {
    EcalPedestals::Item item = mapItr->second;

    switch(gainId) {
    case 0:
      ped = item.mean_x1;
    case 1:
      ped = item.mean_x12;
      break;
    case 2:
      ped = item.mean_x6;
      break;
    case 3:
      ped = item.mean_x1;
      break;
    default:
      edm::LogError("EcalMMValid") << "Bad Pedestal " << gainId;
      break;
    }
    LogDebug("EcalMMValid") << "Pedestals for " << detId.rawId() << " gain range " << gainId << " : \n" << "Mean = " << ped;
  }
}

void EcalMixingModuleValidation::computeSDBunchDigi(const edm::EventSetup & eventSetup, MixCollection<PCaloHit> & theHits, MapType & SignalSimMap, const EcalSubdetector & thisDet, const double & theSimThreshold)
{

  if ( thisDet != EcalBarrel && thisDet != EcalEndcap && thisDet != EcalPreshower ) {
    edm::LogError("EcalMMValid") << "Invalid subdetector type";
    return;
  }
  bool isCrystal = true;
  if ( thisDet == EcalPreshower ) isCrystal = false;

  // load the geometry

  edm::ESHandle<CaloGeometry> hGeometry;
  eventSetup.get<IdealGeometryRecord>().get(hGeometry);

  const CaloGeometry * pGeometry = &*hGeometry;
  
  // see if we need to update
  if(pGeometry != theGeometry) {
    theGeometry = pGeometry;
    theEcalResponse->setGeometry(theGeometry); 
    theESResponse->setGeometry(theGeometry); 
  }

  // vector of DetId with energy above a fraction of the gun's energy

  std::vector<DetId> theSDId;
  if ( thisDet == EcalBarrel ) { theSDId = theGeometry->getValidDetIds(DetId::Ecal, EcalBarrel); }
  else if ( thisDet == EcalEndcap ) { theSDId = theGeometry->getValidDetIds(DetId::Ecal, EcalEndcap); }
  else if ( thisDet == EcalPreshower ) { theSDId = theGeometry->getValidDetIds(DetId::Ecal, EcalPreshower); }

  std::vector<DetId> theOverThresholdId;
  for ( unsigned int i = 0 ; i < theSDId.size() ; i++ ) {

    int sdId = theSDId[i].rawId();
    if ( SignalSimMap[sdId] > theSimThreshold ) theOverThresholdId.push_back( theSDId[i] );

  }

  int limit = CaloSamples::MAXSAMPLES;
  if ( ! isCrystal ) limit = ESDataFrame::MAXSAMPLES;

   for (int iBunch = theMinBunch ; iBunch <= theMaxBunch ; iBunch++ ) {

     if ( isCrystal ) {
       theEcalResponse->setBunchRange(iBunch, iBunch);
       theEcalResponse->clear();
       theEcalResponse->run(theHits);
     }
     else {
       theESResponse->setBunchRange(iBunch, iBunch);
       theESResponse->clear();
       theESResponse->run(theHits);
     }

     int iHisto = iBunch - theMinBunch;

     for ( std::vector<DetId>::const_iterator idItr = theOverThresholdId.begin() ; idItr != theOverThresholdId.end() ; ++idItr ) {
       
       CaloSamples * analogSignal;
       if ( isCrystal ) 
         { analogSignal = theEcalResponse->findSignal(*idItr); }
       else
         { analogSignal = theESResponse->findSignal(*idItr); }

       if ( analogSignal ) {
        
         (*analogSignal) *= theParameterMap->simParameters(analogSignal->id()).photoelectronsToAnalog();

         for ( int i = 0 ; i < limit ; i++ ) {

           if ( thisDet == EcalBarrel ) { meEBBunchShape_[iHisto]->Fill(i,(float)(*analogSignal)[i]); }
           else if ( thisDet == EcalEndcap ) { meEEBunchShape_[iHisto]->Fill(i,(float)(*analogSignal)[i]); }
           else if ( thisDet == EcalPreshower ) { meESBunchShape_[iHisto]->Fill(i,(float)(*analogSignal)[i]); }
         }
       }

     }

   }

 }
