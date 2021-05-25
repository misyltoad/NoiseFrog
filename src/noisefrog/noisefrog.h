#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <limits>
#include <array>

#include "rnnoise.h"

namespace NoiseFrog {

  using RNNoiseContext = std::unique_ptr<DenoiseState, void(*)(DenoiseState*)>;

  class NoiseFrogDenoiser {
  public:
    static constexpr uint32_t RNNoiseFrameSize = 120 << 2;

    NoiseFrogDenoiser()
      : m_rnnoise(rnnoise_create(nullptr), [](DenoiseState* obj) { rnnoise_destroy(obj); }) {
    }

    inline void processSamples(const float* in, float* out, float voiceThreshold, uint32_t sampleCount, uint16_t maxGracePeriod) {
      for (uint32_t frameSample = 0; frameSample < sampleCount; frameSample += RNNoiseFrameSize) {
        uint32_t frameSize = std::min(frameSample + RNNoiseFrameSize, sampleCount) - frameSample;

        // Scale from [-1, 1] to [-32767, 32767]
        for (uint32_t i = 0; i < frameSize; i++)
          m_input[i] = in[frameSample + i] * std::numeric_limits<short>::max();

        float voicePercentage = rnnoise_process_frame(m_rnnoise.get(), m_output.data(), m_input.data());

        if (voicePercentage >= voiceThreshold)
          m_gracePeriod = maxGracePeriod;

        if (m_gracePeriod != 0) {
          // Scale from [-32767, 32767] back to [-1, 1]
          for (uint32_t i = 0; i < frameSize; i++)
            out[frameSample + i] = m_output[i] / std::numeric_limits<short>::max();

          m_gracePeriod--;
        } else {
          for (uint32_t i = 0; i < frameSize; i++)
            out[i] = 0.0f;
        }
      }
    }

  private:

    std::array<float, RNNoiseFrameSize> m_input;
    std::array<float, RNNoiseFrameSize> m_output;

    RNNoiseContext m_rnnoise;
    uint16_t m_gracePeriod = 0;

  };

}
