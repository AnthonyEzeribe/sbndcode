////////////////////////////////////////////////////////////////////////
// Class:       opDetDigitizerSBND
// Module Type: producer
// File:        opDetDigitizerSBND_module.cc
//
// Generated at Fri Apr  5 09:21:15 2019 by Laura Paulucci Marinho using artmod
// from cetpkgsupport v1_14_01.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "canvas/Utilities/InputTag.h"
#include "canvas/Utilities/Exception.h"
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/TableFragment.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "nurandom/RandomUtils/NuRandomService.h"
#include "CLHEP/Random/JamesRandom.h"

#include <memory>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <sstream>
#include <fstream>
#include <thread>
#include <cstdlib>
#include <stdexcept>

#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardata/DetectorInfoServices/DetectorClocksServiceStandard.h"
#include "larcore/Geometry/Geometry.h"
#include "lardataobj/Simulation/sim.h"
#include "lardataobj/Simulation/SimChannel.h"
#include "lardataobj/Simulation/SimPhotons.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "lardata/DetectorInfoServices/LArPropertiesService.h"

#include "TMath.h"
#include "TH1D.h"
#include "TRandom3.h"
#include "TF1.h"

#include "sbndcode/OpDetSim/sbndPDMapAlg.h"
#include "sbndcode/OpDetSim/DigiArapucaSBNDAlg.h"
#include "sbndcode/OpDetSim/DigiPMTSBNDAlg.h"
#include "sbndcode/OpDetSim/opDetSBNDTriggerAlg.h"
#include "sbndcode/OpDetSim/opDetDigitizerWorker.h"

namespace opdet {

  /*
  * This module simulates the digitization of SBND photon detectors response.
  *
  * The module is has an interface to the simulation algorithms for PMTs and arapucas,
  * opdet::DigiPMTSBNDAlg e opdet::DigiArapucaSBNDAlg.
  *
  * Input
  * ======
  * The module utilizes as input a collection of `sim::SimPhotons` or `sim::SimPhotonsLite`, each
  * containing the photons propagated to a single optical detector channel.
  *
  * Output
  * =======
  * A collection of optical detector waveforms (`std::vector<raw::OpDetWaveform>`) is produced.
  *
  * Requirements
  * =============
  * This module currently requires LArSoft services:
  * * `DetectorClocksService` for timing conversions and settings
  * * `LArPropertiesService` for the scintillation yield(s)
  *
  */

  class opDetDigitizerSBND;

  class opDetDigitizerSBND : public art::EDProducer {
  public:
    struct Config {
      using Comment = fhicl::Comment;
      using Name = fhicl::Name;

      fhicl::Atom<art::InputTag> InputModuleName {
        Name("InputModule"),
        Comment("Simulated photons to be digitized")
      };
      fhicl::Atom<double> WaveformSize {
        Name("WaveformSize"),
        Comment("Value to initialize the waveform vector in ns. It is resized in the algorithms according to readout window of PDs")
      };
      fhicl::Atom<int> UseLitePhotons {
        Name("UseLitePhotons"),
        Comment("Whether SimPhotonsLite or SimPhotons will be used")
      };

      fhicl::Atom<bool> ApplyTriggers {
        Name("ApplyTriggers"),
        Comment("Whether to apply trigger algorithm to waveforms"),
        true
      };

      fhicl::Atom<unsigned> NThreads {
        Name("NThreads"),
        Comment("Number of threads to split waveform process into. Defaults to 1.\
                     Set 0 to autodetect. Autodection will first check $SBNDCODE_OPDETSIM_NTHREADS for number of threads. \
                     If this is not set, then NThreads is set to the number of hardware cores on the host machine."),
        1
      };

      fhicl::TableFragment<opdet::DigiPMTSBNDAlgMaker::Config> pmtAlgoConfig;
      fhicl::TableFragment<opdet::DigiArapucaSBNDAlgMaker::Config> araAlgoConfig;
      fhicl::TableFragment<opdet::opDetSBNDTriggerAlg::Config> trigAlgoConfig;
    }; // struct Config

    using Parameters = art::EDProducer::Table<Config>;

    explicit opDetDigitizerSBND(Parameters const& config);
    // The destructor generated by the compiler is fine for classes
    // without bare pointers or other resource use.
    // Add a destructor to deal with random number generator pointer
    ~opDetDigitizerSBND();

    // Plugins should not be copied or assigned.
    opDetDigitizerSBND(opDetDigitizerSBND const &) = delete;
    opDetDigitizerSBND(opDetDigitizerSBND &&) = delete;
    opDetDigitizerSBND & operator = (opDetDigitizerSBND const &) = delete;
    opDetDigitizerSBND & operator = (opDetDigitizerSBND &&) = delete;

    // Required functions.
    void produce(art::Event & e) override;

    opdet::sbndPDMapAlg map; //map for photon detector types
    unsigned int nChannels = map.size();
    std::vector<raw::OpDetWaveform> fWaveforms; // holder for un-triggered waveforms

  private:
    bool fApplyTriggers;
    std::unordered_map< raw::Channel_t, std::vector<double> > fFullWaveforms;

    bool fUseLitePhotons;
    unsigned fPMTBaseline;
    unsigned fArapucaBaseline;
    unsigned fNThreads;
    // digitizer workers
    std::vector<opdet::opDetDigitizerWorker> fWorkers;
    std::vector<std::vector<raw::OpDetWaveform>> fTriggeredWaveforms;
    std::vector<std::thread> fWorkerThreads;

    // product containers
    std::vector<art::Handle<std::vector<sim::SimPhotonsLite>>> fPhotonLiteHandles;
    std::vector<art::Handle<std::vector<sim::SimPhotons>>> fPhotonHandles;

    // sync stuff
    opdet::opDetDigitizerWorker::Semaphore fSemStart;
    opdet::opDetDigitizerWorker::Semaphore fSemFinish;
    bool fFinished;

    // trigger algorithm
    opdet::opDetSBNDTriggerAlg fTriggerAlg;
  };

  opDetDigitizerSBND::opDetDigitizerSBND(Parameters const& config)
    : EDProducer{config}
    , fApplyTriggers(config().ApplyTriggers())
    , fUseLitePhotons(config().UseLitePhotons())
    , fPMTBaseline(config().pmtAlgoConfig().pmtbaseline())
    , fArapucaBaseline(config().araAlgoConfig().baseline())
    , fTriggerAlg(config().trigAlgoConfig(), lar::providerFrom<detinfo::DetectorClocksService>(), lar::providerFrom<detinfo::DetectorPropertiesService>())
  {
    auto const *timeService = lar::providerFrom< detinfo::DetectorClocksService >();

    opDetDigitizerWorker::Config wConfig( config().pmtAlgoConfig(), config().araAlgoConfig());

    fNThreads = config().NThreads();
    if (fNThreads == 0) { // autodetect -- first check env var
      const char *env = std::getenv("SBNDCODE_OPDETSIM_NTHREADS");
      // try to parse into positive integer
      if (env != NULL) {
        try {
          int n_threads = std::stoi(env);
          if (n_threads <= 0) {
            throw std::invalid_argument("Expect positive integer");
          }
          fNThreads = n_threads;
        }
        catch (...) {
          mf::LogError("OpDetDigitizer") << "Unable to parse number of threads in environment variable (SBNDCODE_OPDETSIM_NTHREADS): (" << env << "). Setting Number opdet threads to 1." << std::endl;
          fNThreads = 1;
        }
      }
    }

    if (fNThreads == 0) { // autodetect -- now try to get number of cpu's
      fNThreads = std::thread::hardware_concurrency();
    }
    if (fNThreads == 0) { // autodetect failed
      fNThreads = 1;
    }
    mf::LogInfo("OpDetDigitizer") << "Digitizing on n threads: " << fNThreads << std::endl;

    wConfig.nThreads = fNThreads;

    wConfig.UseLitePhotons = config().UseLitePhotons();
    wConfig.InputModuleName = config().InputModuleName();

    wConfig.Sampling = (timeService->OpticalClock().Frequency()) / 1000.0; //in GHz
    wConfig.EnableWindow = fTriggerAlg.TriggerEnableWindow(); // us
    wConfig.Nsamples = (wConfig.EnableWindow[1] - wConfig.EnableWindow[0]) * 1000. /*us -> ns*/ * wConfig.Sampling /* GHz */;

    fFinished = false;

    fWorkers.reserve(fNThreads);
    fTriggeredWaveforms.reserve(fNThreads);
    for (unsigned i = 0; i < fNThreads; i++) {
      // Set random number gen seed from the NuRandomService
      art::ServiceHandle<rndm::NuRandomService> seedSvc;
      CLHEP::HepJamesRandom *engine = new CLHEP::HepJamesRandom;
      seedSvc->registerEngine(rndm::NuRandomService::CLHEPengineSeeder(engine), "opDetDigitizerSBND" + std::to_string(i));

      fTriggeredWaveforms.emplace_back();

      // setup worker
      fWorkers.emplace_back(i, wConfig, engine, fTriggerAlg);
      fWorkers[i].SetPhotonLiteHandles(&fPhotonLiteHandles);
      fWorkers[i].SetPhotonHandles(&fPhotonHandles);
      fWorkers[i].SetWaveformHandle(&fWaveforms);
      fWorkers[i].SetTriggeredWaveformHandle(&fTriggeredWaveforms[i]);

      // start worker thread
      fWorkerThreads.emplace_back(opdet::opDetDigitizerWorkerThread, std::cref(fWorkers[i]), std::ref(fSemStart), std::ref(fSemFinish), fApplyTriggers, &fFinished);
    }

    // Call appropriate produces<>() functions here.
    produces< std::vector< raw::OpDetWaveform > >();
  }

  opDetDigitizerSBND::~opDetDigitizerSBND()
  {
    // cleanup all of the workers
    fFinished = true;
    opdet::StartopDetDigitizerWorkers(fNThreads, fSemStart);

    // join the threads
    for (std::thread &thread : fWorkerThreads) thread.join();

  }

  void opDetDigitizerSBND::produce(art::Event & e)
  {
    std::unique_ptr< std::vector< raw::OpDetWaveform > > pulseVecPtr(std::make_unique< std::vector< raw::OpDetWaveform > > ());
    // Implementation of required member function here.
    std::cout << "Event: " << e.id().event() << std::endl;

    // setup the waveforms
    fWaveforms = std::vector<raw::OpDetWaveform> (nChannels);

    if (fUseLitePhotons == 1) { //using SimPhotonsLite
      fPhotonLiteHandles.clear();
      //Get *ALL* SimPhotonsCollectionLite from Event
      e.getManyByType(fPhotonLiteHandles);
      if (fPhotonLiteHandles.size() == 0)
        mf::LogError("OpDetDigitizer") << "sim::SimPhotonsLite not found -> No Optical Detector Simulation!\n";
    }
    else {
      fPhotonHandles.clear();
      //Get *ALL* SimPhotonsCollection from Event
      e.getManyByType(fPhotonHandles);
      if (fPhotonHandles.size() == 0)
        mf::LogError("OpDetDigitizer") << "sim::SimPhotons not found -> No Optical Detector Simulation!\n";
    }
    // Start the workers!
    // Run the digitizer over the full readout window
    opdet::StartopDetDigitizerWorkers(fNThreads, fSemStart);
    opdet::WaitopDetDigitizerWorkers(fNThreads, fSemFinish);

    if (fApplyTriggers) {
      // find the trigger locations for the waveforms
      for (const raw::OpDetWaveform &waveform : fWaveforms) {
        raw::Channel_t ch = waveform.ChannelNumber();
        // skip light channels which don't correspond to readout channels
        if (ch == std::numeric_limits<raw::Channel_t>::max() /* "NULL" value*/) {
          continue;
        }
        raw::ADC_Count_t baseline = (map.pdType(ch, "barepmt") || map.pdType(ch, "pmt")) ?
                                    fPMTBaseline : fArapucaBaseline;
        fTriggerAlg.FindTriggerLocations(waveform, baseline);
      }

      // combine the triggers
      fTriggerAlg.MergeTriggerLocations();
      // Start the workers!
      // Apply the trigger locations
      opdet::StartopDetDigitizerWorkers(fNThreads, fSemStart);
      opdet::WaitopDetDigitizerWorkers(fNThreads, fSemFinish);

      for (std::vector<raw::OpDetWaveform> &waveforms : fTriggeredWaveforms) {
        // move these waveforms into the pulseVecPtr
        pulseVecPtr->reserve(pulseVecPtr->size() + waveforms.size());
        std::move(waveforms.begin(), waveforms.end(), std::back_inserter(*pulseVecPtr));
      }
      // clean up the vector
      for (unsigned i = 0; i < fTriggeredWaveforms.size(); i++) {
        fTriggeredWaveforms[i] = std::vector<raw::OpDetWaveform>();
      }

      // put the waveforms in the event
      e.put(std::move(pulseVecPtr));
      // clear out the triggers
      fTriggerAlg.ClearTriggerLocations();

    }
    else {
      // put the full waveforms in the event
      for (const raw::OpDetWaveform &waveform : fWaveforms) {
        if (waveform.ChannelNumber() == std::numeric_limits<raw::Channel_t>::max() /* "NULL" value*/) {
          continue;
        }
        pulseVecPtr->push_back(waveform);
      }
      e.put(std::move(pulseVecPtr));
    }

    // clear out the full waveforms
    fWaveforms.clear();

  }//produce end

  DEFINE_ART_MODULE(opdet::opDetDigitizerSBND)

}//closing namespace
