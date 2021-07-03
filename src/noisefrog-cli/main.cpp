#include <pulse/pulseaudio.h>
#include <stdio.h>
#include <string.h>

static void context_drain_complete(pa_context *context, void*) {
  pa_context_disconnect(context);
}

static void complete_action(pa_context* context) {
  pa_operation *o;

  if (!(o = pa_context_drain(context, context_drain_complete, nullptr)))
    pa_context_disconnect(context);
  else
    pa_operation_unref(o);
}

static void exit_signal_callback(pa_mainloop_api* mainloop_api, pa_signal_event*, int, void*) {
  mainloop_api->quit(mainloop_api, 1);
}

static void module_load_callback(pa_context* context, uint32_t idx, void*) {
  if (idx == PA_INVALID_INDEX)
    printf("Froggy!\n");

  complete_action(context);
}

static void get_source_info_callback(pa_context *context, const pa_source_info *i, int is_last, void *userdata) {
  pa_mainloop_api* mainloop_api = reinterpret_cast<pa_mainloop_api*>(userdata);

  if (is_last < 0) {
    fprintf(stderr, "Failed to get input information, %s.\n", pa_strerror(pa_context_errno(context)));
    mainloop_api->quit(mainloop_api, 1);
    return;
  }

  if (is_last) {
    // TODO: Hook me up properly to what the user wants via command line.
    pa_operation_unref(pa_context_load_module(
        context,
        "module-ladspa-source",
        "source_name='NoiseFrog Noise Reduction (Mono)' master=43 rate=48000 channels=1 label=noisefrog plugin='/home/joshua/Desktop/Code/NoiseFrog/build/src/noisefrog-ladspa/libnoisefrog-ladspa.so' control=0.9,20",
        module_load_callback,
        nullptr
      ));
    return;
  }

  // Skip over monitors.
  if (i->monitor_of_sink_name != nullptr)
    return;

  printf("Blah: %d - %s\n", i->index, i->name);
}

static void server_info_callback(pa_context* context, const pa_server_info* i, void* userdata) {
  pa_mainloop_api* mainloop_api = reinterpret_cast<pa_mainloop_api*>(userdata);

  if (strstr(i->server_name, "PipeWire") == nullptr) {
    fprintf(stderr, "Not running on PipeWire. Aborting.\n");
    mainloop_api->quit(mainloop_api, 1);
    return;
  }

  pa_operation_unref(pa_context_get_source_info_list(
    context,
    get_source_info_callback,
    reinterpret_cast<void*>(mainloop_api)
  ));
}

static void context_state_callback(pa_context* context, void* userdata) {
  pa_mainloop_api* mainloop_api = reinterpret_cast<pa_mainloop_api*>(userdata);

  switch (pa_context_get_state(context)) {
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;

    case PA_CONTEXT_READY:
      pa_operation_unref(pa_context_get_server_info(
        context,
        server_info_callback,
        reinterpret_cast<void*>(mainloop_api)
      ));
      break;

    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
      mainloop_api->quit(mainloop_api, 1);
      break;
  }
}

int main(int argc, char** argv) {
  (void)(argc);
  (void)(argv);

  pa_mainloop* mainloop = pa_mainloop_new();
  pa_mainloop_api* mainloop_api = pa_mainloop_get_api(mainloop);

  pa_signal_event* pa_sigint = pa_signal_new(SIGINT, exit_signal_callback, nullptr);
  pa_signal_event* pa_sigterm = pa_signal_new(SIGTERM, exit_signal_callback, nullptr);
  signal(SIGPIPE, SIG_IGN);

  pa_proplist *proplist = pa_proplist_new();
  pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "NoiseFrog Manager");
  pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, "noisefrog-cli");
  pa_context* context = pa_context_new_with_proplist(mainloop_api, nullptr, proplist);
  pa_proplist_free(proplist);

  pa_context_connect(context, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr);
  pa_context_set_state_callback(context, context_state_callback, reinterpret_cast<void*>(mainloop_api));

  int returnValue = 1;
  pa_mainloop_run(mainloop, &returnValue);

  pa_context_unref(context);
  pa_signal_free(pa_sigterm);
  pa_signal_free(pa_sigint);
  pa_signal_done();
  pa_mainloop_free(mainloop);
  return returnValue;
}
