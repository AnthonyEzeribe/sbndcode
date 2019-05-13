////////////////////////////////////////////////////////////////////////
//// File:        DigiPMTSBNDAlg.h
////
//// This algorithm is used for the electronic response of PMTs
//// Created by L. Paulucci and F. Marinho
//// Based on OpDetDigitizerDUNE_module.cc
//////////////////////////////////////////////////////////////////////////

#ifndef DIGIPMTSBNDALG_H
#define DIGIPMTSBNDALG_H

#include "fhiclcpp/types/Atom.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <sstream>
#include <fstream>

#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardata/DetectorInfoServices/DetectorClocksServiceStandard.h"
#include "lardataobj/Simulation/SimPhotons.h"
#include "lardata/DetectorInfoServices/LArPropertiesService.h"

#include "TMath.h"
#include "TRandom3.h"
#include "TF1.h"
#include "TH1D.h"

namespace opdet{

  class DigiPMTSBNDAlg{

    public:

    struct ConfigurationParameters_t {
      double TransitTime; //ns
      double TTS; //ns
      double PMTChargeToADC; //voltage to ADC conversion scale
      double PMTBaseline; //waveform baseline in ADC
      double PMTFallTime; //pulse decaying time constant (exponential) in ns
      double PMTRiseTime; //pulse rising time constant (exponential) in ns
      double PMTMeanAmplitude; //mean amplitude for single pe in pC
      double PMTBaselineRMS; //Pedestal RMS in ADC counts
      double PMTDarkNoiseRate; //in Hz
      double PMTReadoutWindow; //waveform time interval (ns)
      double PMTPreTrigger; //in ns
      double PMTSaturation; //in number of p.e.
      double QEDirect; //PMT quantum efficiency for direct (VUV) light
      double QERefl; //PMT quantum efficiency for reflected (TPB converted) light

      detinfo::LArProperties const* larProp = nullptr; //< LarProperties service provider.
      detinfo::DetectorClocks const* timeService = nullptr; //< DetectorClocks service provider.
    };// ConfigurationParameters_t

    //Default constructor
    DigiPMTSBNDAlg(ConfigurationParameters_t const& config);
    //Default destructor 
    ~DigiPMTSBNDAlg();
                                                                                                 
    void ConstructWaveform(int ch, sim::SimPhotons const& simphotons, std::vector<std::vector<short unsigned int>>& waveforms, std::string pdtype, std::map<int,sim::SimPhotons> auxmap, double& t_min);
    void ConstructWaveformLite(int ch, sim::SimPhotonsLite const& litesimphotons, std::vector<std::vector<short unsigned int>>& waveforms, std::string pdtype, std::map<int,sim::SimPhotonsLite> auxmap, double& t_min);

    private:

    ConfigurationParameters_t fParams;
    // Declare member data here.
    double fSampling;       //wave sampling frequency (GHz)
    double fQEDirect;
    double fQERefl;
    unsigned int fNsamples; //Samples per waveform
    double sigma1;
    double sigma2;
    double tadd; //to add pre trigger considering there is electron transit time

    void AddSPE(size_t time_bin, std::vector<double>& wave); // add single pulse to auxiliary waveform
    double Pulse1PE(double time) ;
    double Transittimespread(double fwhm);

    std::vector<double> wsp; //single photon pulse vector
    int pulsesize; //size of 1PE waveform
    TH1D* timeTPB; //histogram for getting the TPB emission time for coated PMTs
    std::unordered_map< raw::Channel_t,std::vector<double> > fFullWaveforms;  

    void CreatePDWaveform(sim::SimPhotons const& SimPhotons, double t_min, std::vector<double>& wave, int ch, std::string pdtype, std::map<int,sim::SimPhotons> auxmap); 
    void CreatePDWaveformLite(sim::SimPhotonsLite const& litesimphotons, double t_min, std::vector<double>& wave, int ch, std::string pdtype, std::map<int, sim::SimPhotonsLite> auxmap);
    void CreateSaturation(std::vector<double>& wave);//Including saturation effects
    void AddLineNoise(std::vector<double>& wave); //add noise to baseline
    void AddDarkNoise(std::vector<double>& wave); //add dark noise
    double FindMinimumTime(sim::SimPhotons const&, int ch, std::string pdtype, std::map<int,sim::SimPhotons> auxmap);
    double FindMinimumTimeLite(sim::SimPhotonsLite const& litesimphotons, int ch, std::string pdtype, std::map<int, sim::SimPhotonsLite> auxmap);  
  };//class DigiPMTSBNDAlg

  class DigiPMTSBNDAlgMaker{

    public:
    struct Config {
      using Name = fhicl::Name;
      using Comment = fhicl::Comment;

      fhicl::Atom<double> pmtreadoutWindow {
        Name("PMTReadoutWindow"),
        Comment("Waveform time interval (ns)")
      };

      fhicl::Atom<double> pmtpreTrigger {
        Name("PMTPreTrigger"),
        Comment("Pre trigger window in ns")
      };

      fhicl::Atom<double> transitTime {
        Name("TransitTime"),
        Comment("Single pe: Time of maximum amplitude in the PMT pulse in ns")
      };

      fhicl::Atom<double> tts {
        Name("TTS"),
        Comment("Single pe: Transit time spread in ns")
      };

      fhicl::Atom<double> pmtmeanAmplitude {
        Name("PMTMeanAmplitude"),
        Comment("Single pe: mean amplitude of PMT pulse in pC")
      };

      fhicl::Atom<double> pmtriseTime {
        Name("PMTRiseTime"),
        Comment("Single pe: Pulse rise time constant (exponential), from 0.1 to 0.9 of maximum amplitude")
      };

       fhicl::Atom<double> pmtfallTime {
          Name("PMTFallTime"),
          Comment("Single pe: Pulse decay time constant (exponential), from 0.1 to 0.9 of maximum amplitude")
       };

       fhicl::Atom<double> pmtchargeToADC {
          Name("PMTChargeToADC"),
          Comment("Charge to ADC convertion factor")
       };

       fhicl::Atom<double> pmtbaseline {
          Name("PMTBaseline"),
          Comment("Waveform baseline in ADC")
       };

       fhicl::Atom<double> pmtbaselineRMS {
          Name("PMTBaselineRMS"),
          Comment("RMS of the electronics noise fluctuations in ADC counts")
       };

       fhicl::Atom<double> pmtdarkNoiseRate {
          Name("PMTDarkNoiseRate"),
          Comment("Dark noise rate in Hz")
       };
   
       fhicl::Atom<double> pmtsaturation {
          Name("PMTSaturation"),
          Comment("Saturation in number of p.e.")
       };

       fhicl::Atom<double> qEDirect {
          Name("QEDirect"),
          Comment("PMT quantum efficiency for direct (VUV) light")
       };

       fhicl::Atom<double> qERefl {
          Name("QERefl"),
          Comment("PMT quantum efficiency for reflected (TPB emitted)light")
       };

    };    //struct Config

    DigiPMTSBNDAlgMaker(Config const& config); //Constructor

    std::unique_ptr<DigiPMTSBNDAlg> operator()(
        detinfo::LArProperties const& larProp,
        detinfo::DetectorClocks const& detClocks
        ) const;
       
    private:
      // Part of the configuration learned from configuration files.
    DigiPMTSBNDAlg::ConfigurationParameters_t fBaseConfig;
  }; //class DigiPMTSBNDAlgMaker

} //namespace

#endif //DIGIPMTSBNDALG