#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <limits>
#include <array>
#include <cstring>
#include <cassert>

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
      voiceThreshold = 0.95f; // HACK

      const float* in_end  = in  + sampleCount;
      const float* out_end = out + sampleCount;

      const uint32_t inputSampleCount       = m_inputCacheSize + sampleCount;
      const uint32_t inputFrames            = inputSampleCount / RNNoiseFrameSize;
      const uint32_t totalOutputSamples     = (RNNoiseFrameSize - m_outputCacheIdx) + inputFrames * RNNoiseFrameSize;
      const uint32_t missedSamples          = sampleCount > totalOutputSamples ? sampleCount - totalOutputSamples : 0;

      // Output our current cached samples.
      {
        for (; out < out_end && m_outputCacheIdx < RNNoiseFrameSize; m_outputCacheIdx++, out++)
          *out = m_outputCache[m_outputCacheIdx];
      }

      // Output blank for any samples we will miss at the start.
      // The idea here is that this will never happen once we start populating
      // the i/o caches.
      // TODO: Do we want to do this at the end?
      {
        for (uint32_t i = 0; i < missedSamples; i++, out++)
          *out = 0.0f;
      }

      assert(m_outputCacheIdx == RNNoiseFrameSize);

      // Process the current frame
      for (uint32_t frame = 0; frame < inputFrames; frame++) {
        std::array<float, RNNoiseFrameSize> input;
        const uint32_t cachedInSamples  = m_inputCacheSize;
        const uint32_t currentInSamples = RNNoiseFrameSize - cachedInSamples;
        std::memcpy(&input[0],               m_inputCache.data(), cachedInSamples  * sizeof(float));
        std::memcpy(&input[cachedInSamples], in,                  currentInSamples * sizeof(float));
        in += currentInSamples;
        m_inputCacheSize = 0;

        // Scale from [-1, 1] to [-32767, 32767]
        for (uint32_t i = 0; i < RNNoiseFrameSize; i++)
          input[i] *= std::numeric_limits<short>::max();

        float voicePercentage = rnnoise_process_frame(m_rnnoise.get(), m_outputCache.data(), input.data());

        if (voicePercentage >= voiceThreshold)
          m_gracePeriod = maxGracePeriod;

        if (m_gracePeriod != 0) {
          // Scale from [-32767, 32767] back to [-1, 1]
          for (uint32_t i = 0; i < RNNoiseFrameSize; i++)
            m_outputCache[i] /= std::numeric_limits<short>::max();

          m_gracePeriod--;
        } else {
          for (uint32_t i = 0; i < RNNoiseFrameSize; i++)
            m_outputCache[i] = 0.0f;
        }

        // Output what we can right now.
        for (m_outputCacheIdx = 0; out < out_end && m_outputCacheIdx < RNNoiseFrameSize; m_outputCacheIdx++, out++)
          *out = m_outputCache[m_outputCacheIdx];
      }

      // Put our missed samples in the input cache
      {
        for (; in < in_end && m_inputCacheSize < RNNoiseFrameSize; m_inputCacheSize++, in++)
          m_inputCache[m_inputCacheSize] = *in;
      }
    }

  private:

    std::array<float, RNNoiseFrameSize> m_inputCache;
    std::array<float, RNNoiseFrameSize> m_outputCache;
    uint32_t m_inputCacheSize = 0;
    uint32_t m_outputCacheIdx = RNNoiseFrameSize;

    RNNoiseContext m_rnnoise;
    uint16_t m_gracePeriod = 0;

  };

}
