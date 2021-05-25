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
      const uint32_t newSamples         = sampleCount;
      const uint32_t totalInputSamples  = newSamples + m_inputCacheSize;
      const uint32_t samplesToProcess   = (totalInputSamples / RNNoiseFrameSize) * RNNoiseFrameSize;
      const uint32_t framesToProcess    = samplesToProcess / RNNoiseFrameSize;
      const uint32_t missedSamples      = totalInputSamples - samplesToProcess;
      const uint32_t totalOutputSamples = m_outputCacheSize + samplesToProcess;
      const uint32_t samplesToSkip      = totalOutputSamples < sampleCount ? sampleCount - totalOutputSamples : 0;

      uint32_t outputCursor = 0;
      uint32_t inputCursor = 0;

      // Deal with the number of samples to skip to keep
      // a full frame going.
      for (uint32_t i = 0; i < samplesToSkip && outputCursor < sampleCount; outputCursor++, i++)
        out[outputCursor] = 0.0f;

      // Deal with the output cache
      for (; m_outputCacheSize && outputCursor < sampleCount; outputCursor++, m_outputCacheSize--)
        out[outputCursor] = m_output[RNNoiseFrameSize - m_outputCacheSize] / std::numeric_limits<short>::max();

      for (uint32_t frame = 0; frame < framesToProcess; frame++) {
        const uint32_t newSamples  = RNNoiseFrameSize - m_inputCacheSize;

        for (uint32_t i = 0; i < newSamples; inputCursor++, i++)
          m_input[m_inputCacheSize + i] = in[inputCursor] * std::numeric_limits<short>::max();

        float voicePercentage = rnnoise_process_frame(m_rnnoise.get(), m_output.data(), m_input.data());

        if (voicePercentage >= voiceThreshold)
          m_gracePeriod = maxGracePeriod;

        m_outputCacheSize = RNNoiseFrameSize;
        if (m_gracePeriod != 0) {
          // Scale from [-32767, 32767] back to [-1, 1]
          for (uint32_t i = 0; i < RNNoiseFrameSize && outputCursor < sampleCount; outputCursor++, i++, m_outputCacheSize--)
            out[outputCursor] = m_output[i] / std::numeric_limits<short>::max();

          m_gracePeriod--;
        } else {
          for (uint32_t i = 0; i < RNNoiseFrameSize && outputCursor < sampleCount; outputCursor++, m_outputCacheSize--)
            out[outputCursor] = 0.0f;
        }

        m_inputCacheSize = 0;
      }

      // Store the missed samples for next time.
      for (uint32_t i = 0; i < missedSamples; inputCursor++, i++)
        m_input[i] = in[inputCursor] * std::numeric_limits<short>::max();

      m_inputCacheSize = missedSamples;
    }

  private:

    std::array<float, RNNoiseFrameSize> m_input;
    std::array<float, RNNoiseFrameSize> m_output;

    RNNoiseContext m_rnnoise;
    uint32_t m_inputCacheSize = 0;
    uint32_t m_outputCacheSize = 0;
    uint16_t m_gracePeriod = 0;

  };

}
