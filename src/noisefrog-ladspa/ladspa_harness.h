#pragma once

#include "ladspa.h"

// This code is a little bit of a constexpr mess
// in order to make the general interface for exposing
// an LADSPA plugin a little bit nicer.
// Should be pretty easy to follow however :)

namespace NoiseFrog {

  struct LADSPAHarnessPort {
    LADSPA_PortDescriptor descriptor;
    const char*           name;

    LADSPA_PortRangeHintDescriptor rangeDescriptor;
    LADSPA_Data lowerBound;
    LADSPA_Data upperBound;
  };

  template <size_t N>
  struct LADSPAHarnessInfo {
    static constexpr size_t PortCount = N;

    unsigned long     uniqueID;
    const char*       label;
    LADSPA_Properties properties;
    const char*       name;
    const char*       maker;
    const char*       copyright;
    std::array<LADSPAHarnessPort, PortCount> ports;

    constexpr auto getPortDescriptors() const {
      std::array<LADSPA_PortDescriptor, PortCount> descriptors;
      for (size_t i = 0; i < PortCount; i++)
        descriptors[i] = ports[i].descriptor;
      return descriptors;
    }

    constexpr auto getPortNames() const {
      std::array<const char*, PortCount> names;
      for (size_t i = 0; i < PortCount; i++)
        names[i] = ports[i].name;
      return names;
    }

    constexpr auto getPortRanges() const {
      std::array<LADSPA_PortRangeHint, PortCount> ranges;
      for (size_t i = 0; i < PortCount; i++)
        ranges[i] = LADSPA_PortRangeHint { ports[i].rangeDescriptor, ports[i].lowerBound, ports[i].upperBound };
      return ranges;
    }
  };

  template <typename Plugin>
  class LADSPAHarness {
  public:
    static constexpr std::array<LADSPA_PortDescriptor, Plugin::LADSPAInfo.PortCount> portDescriptors = Plugin::LADSPAInfo.getPortDescriptors();
    static constexpr std::array<const char*, Plugin::LADSPAInfo.PortCount> portNames = Plugin::LADSPAInfo.getPortNames();
    static constexpr std::array<LADSPA_PortRangeHint, Plugin::LADSPAInfo.PortCount> portRanges = Plugin::LADSPAInfo.getPortRanges();

    static LADSPA_Handle handle_cast(Plugin* ptr) {
      return reinterpret_cast<LADSPA_Handle>(ptr);
    }

    static Plugin* handle_cast(LADSPA_Handle handle) {
      return reinterpret_cast<Plugin*>(handle);
    }

    static LADSPA_Handle ladspa_instantiate(const LADSPA_Descriptor* descriptor, unsigned long sampleRate) {
      return descriptor == &Descriptor
        ? handle_cast(new Plugin(sampleRate))
        : nullptr;
    }

    static void ladspa_connect_port(LADSPA_Handle instance, unsigned long port, LADSPA_Data* data) {
      handle_cast(instance)->ConnectPort(port, data);
    }

    static void ladspa_run(LADSPA_Handle instance, unsigned long sampleCount) {
      handle_cast(instance)->Run(sampleCount);
    }

    static void ladspa_cleanup(LADSPA_Handle instance) {
      delete handle_cast(instance);
    }

    static constexpr auto getDescriptor() {
      return LADSPA_Descriptor {
        .UniqueID            = Plugin::LADSPAInfo.uniqueID,
        .Label               = Plugin::LADSPAInfo.label,
        .Properties          = Plugin::LADSPAInfo.properties,
        .Name                = Plugin::LADSPAInfo.name,
        .Maker               = Plugin::LADSPAInfo.maker,
        .Copyright           = Plugin::LADSPAInfo.copyright,
        .PortCount           = Plugin::LADSPAInfo.PortCount,
        .PortDescriptors     = portDescriptors.data(),
        .PortNames           = portNames.data(),
        .PortRangeHints      = portRanges.data(),
        .ImplementationData  = nullptr,

        .instantiate         = &ladspa_instantiate,
        .connect_port        = &ladspa_connect_port,
        .activate            = nullptr,
        .run                 = &ladspa_run,
        .run_adding          = nullptr,
        .set_run_adding_gain = nullptr,
        .deactivate          = nullptr,
        .cleanup             = &ladspa_cleanup,
      };
    }

    static constexpr LADSPA_Descriptor Descriptor = getDescriptor();
  };

}