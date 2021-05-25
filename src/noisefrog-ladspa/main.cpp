#include "../noisefrog/noisefrog.h"

#include "ladspa_harness.h"

namespace NoiseFrog {

  namespace Plugins {
    enum Plugin {
      NoiseFrogMono,
      Count,
    };
  }

  class NoiseFrogMono {
  public:
    static constexpr Plugins::Plugin PluginIndex = Plugins::NoiseFrogMono;

    struct Ports {
      enum Port {
        VoiceActivationThreshold,
        GracePeriod,
        Input,
        Output,

        Count,
      };
    };

    static constexpr LADSPAHarnessInfo<Ports::Count> LADSPAInfo = {
        .uniqueID            = U'\U0001f438',
        .label               = "noisefrog",
        .properties          = LADSPA_PROPERTY_REALTIME | LADSPA_PROPERTY_HARD_RT_CAPABLE,
        .name                = "NoiseFrog Noise Reduction (Mono)",
        .maker               = "Joshua Ashton",
        .copyright           = "2021 Joshua Ashton - Licensed under MIT",
        .ports               = {{
          // VoiceActivationThreshold
          {
            .descriptor      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
            .name            = "Voice Activation Threshold",
            .rangeDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE,
            .lowerBound      = 0.0f,
            .upperBound      = 1.0f,
          },
          // GracePeriod
          {
            .descriptor      = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
            .name            = "Grace Period (in frames)",
            .rangeDescriptor = LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_INTEGER,
            .lowerBound      = 0.0f,
            .upperBound      = 40.0f,
          },
          // Input
          {
            .descriptor      = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
            .name            = "Input",
          },
          // Output
          {
            .descriptor      = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
            .name            = "Output",
          },
        }},
    };

    NoiseFrogMono(unsigned long sampleRate)
      : m_denoiser() {
      (void)sampleRate;
    }

    void ConnectPort(unsigned long port, LADSPA_Data* data) {
      m_ports[port] = data;
    }

    void Run(unsigned long sampleCount) {
      const float voiceThreshold = std::clamp(*m_ports[Ports::VoiceActivationThreshold], 0.0f, 1.0f);
      const uint16_t gracePeriod = uint16_t(std::clamp(*m_ports[Ports::GracePeriod], 0.0f, 40.0f));
      const float* in = m_ports[Ports::Input];
      float* out = m_ports[Ports::Output];

      m_denoiser.processSamples(in, out, voiceThreshold, uint32_t(sampleCount), gracePeriod);
    }

  private:

    NoiseFrogDenoiser m_denoiser;
    std::array<LADSPA_Data*, Ports::Count> m_ports = {};

  };

  using NoiseFrogMonoPlugin = LADSPAHarness<NoiseFrogMono>;

}

extern "C" const LADSPA_Descriptor* ladspa_descriptor(unsigned long index) {
  switch (index) {
    case NoiseFrog::NoiseFrogMono::PluginIndex:
      return &NoiseFrog::NoiseFrogMonoPlugin::Descriptor;
    default:
      return nullptr;
  }
}