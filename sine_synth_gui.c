#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include <uv.h>

#include "rutabaga/rutabaga.h"
#include "rutabaga/container.h"
#include "rutabaga/window.h"
#include "rutabaga/layout.h"
#include "rutabaga/keyboard.h"

#include "rutabaga/widgets/button.h"
#include "rutabaga/widgets/text-input.h"
#include "rutabaga/widgets/knob.h"
#include "rutabaga/widgets/spinbox.h"

#include "sine_synth.h"
#include "asprintf.h"

#define SINE_SYNTH_UI_URI  "http://bado.so/plugins/sine_synth#ui"

struct ControlStruct;

typedef struct {
  LV2UI_Controller controller;
  LV2UI_Write_Function write_function;

  struct rutabaga* rtb;

  struct rtb_window* win;
  struct rtb_label* monitor;

  struct ControlStruct* volume;
  struct ControlStruct* panning;

  struct ControlStruct* attack;
  struct ControlStruct* hold;
  struct ControlStruct* decay;
  struct ControlStruct* sustain;
  struct ControlStruct* release;
} SineSynthGui;

typedef struct ControlStruct {
  SineSynthGui* gui;

  PortIndex port;
  char* label;
  char* units;

  struct rtb_knob* knob;
} Control;

static void
print_control(Control* control, float value) {
  char* label;
  asprintf(&label, "%s:   %.2f %s", control->label, value, control->units);

  rtb_label_set_text(control->gui->monitor, (rtb_utf8_t*)label);
}

static int
control_value(struct rtb_element *element,
    const struct rtb_event *_e, void *data)
{
  const struct rtb_value_event *e = RTB_EVENT_AS(_e, rtb_value_event);
  Control* control = (Control*)data;

  float value = e->value;
  SineSynthGui* gui = control->gui;

  gui->write_function(gui->controller, control->port, sizeof(value), 0, &value);

  if (element->mouse_in == 1) {
    print_control(control, value);
  }

  return 0;
}

static int
control_mouse_enter(struct rtb_element* element,
    const struct rtb_event* _e, void* data)
{
  Control* control = (Control*)data;

  float value = RTB_VALUE_ELEMENT(control->knob)->value;

  print_control(control, value);

  return 0;
}

static void
add_knob(Control* control, float min, float max, float def,
rtb_container_t* container) {
  struct rtb_knob* knob = rtb_knob_new();

  knob->min    = min;
  knob->max    = max;
  knob->origin = def;

  rtb_container_add(container, RTB_ELEMENT(knob));
  rtb_register_handler(RTB_ELEMENT(knob), RTB_VALUE_CHANGE, control_value, control);
  rtb_register_handler(RTB_ELEMENT(knob), RTB_MOUSE_ENTER, control_mouse_enter, control);

  control->knob = knob;
}

static Control*
init_control(char* label, char* units, PortIndex port,
SineSynthGui* gui) {
  Control* control = (Control*)malloc(sizeof(Control));

  control->gui = gui;
  control->label = label;
  control->units = units;
  control->port = port;

  return control;
}

void
build_ui(SineSynthGui* gui) {
  rtb_container_t* win = RTB_ELEMENT(gui->rtb->win);

  rtb_container_t* upper = rtb_container_new();
	rtb_elem_set_layout(upper, rtb_layout_hpack_center);
	rtb_elem_set_size_cb(upper, rtb_size_hfill);

  rtb_container_t* lower = rtb_container_new();
	rtb_elem_set_layout(lower, rtb_layout_hpack_center);
	rtb_elem_set_size_cb(lower, rtb_size_hfill);

  gui->monitor = rtb_label_new((rtb_utf8_t*)"Sine Synth");

  gui->volume = init_control("Volume", "dB", PORT_VOLUME, gui);
  add_knob(gui->volume, -90, 24, -15, lower);

  gui->panning = init_control("Panning", "", PORT_PANNING, gui);
  add_knob(gui->panning, -1, 1, 0, lower);

  gui->attack = init_control("Attack", "ms", PORT_ATTACK_TIME, gui);
  add_knob(gui->attack, 1, 5000, 25, lower);

  gui->hold = init_control("Hold", "ms", PORT_HOLD_TIME, gui);
  add_knob(gui->hold, 0, 5000, 0, lower);

  gui->decay = init_control("Decay", "ms", PORT_DECAY_TIME, gui);
  add_knob(gui->decay, 1, 5000, 25, lower);

  gui->sustain = init_control("Sustain", "", PORT_SUSTAIN_LEVEL, gui);
  add_knob(gui->sustain, 0, 1, 0.7, lower);

  gui->release = init_control("Release", "ms", PORT_RELEASE_TIME, gui);
  add_knob(gui->release, 1, 5000, 100, lower);

  rtb_container_add(upper, RTB_ELEMENT(gui->monitor));

  rtb_container_add(win, upper);
  rtb_container_add(win, lower);
}

static int
idle(LV2UI_Handle handle)
{
  SineSynthGui* gui = (SineSynthGui*)handle;

  uv_run(&(gui->rtb)->event_loop, UV_RUN_ONCE);

  return 0;
}

static const LV2UI_Idle_Interface idle_iface = { idle };

LV2UI_Handle
instantiate(const struct _LV2UI_Descriptor* descriptor,
    const char* plugin_uri, const char* bundle_path,
    LV2UI_Write_Function write_function,
    LV2UI_Controller controller, LV2UI_Widget* widget,
    const LV2_Feature* const* features) {

  if (strcmp(plugin_uri, SINE_SYNTH_URI) != 0) {
    fprintf(stderr, "Error: this GUI does not support plugin with URI %s\n", plugin_uri);
    return NULL;
  }

  SineSynthGui* gui = (SineSynthGui*)malloc(sizeof(SineSynthGui));

  gui->controller = controller;
  gui->write_function = write_function;

  LV2UI_Resize* resize;
  void* x_window = 0;
  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_UI__parent)) {
      x_window = features[i]->data;
    } else if (!strcmp(features[i]->URI, LV2_UI__resize)) {
      resize = (LV2UI_Resize*)features[i]->data;
    }
  }

  int width = 500;
  int height = 100;

  gui->rtb = rtb_new();
  gui->win = rtb_window_open_under(gui->rtb, (uintptr_t)x_window, width, height, "Sine Synth");

  if (!gui->win) {
    fprintf(stderr, "Couldn't open window\n");
    rtb_free(gui->rtb);

    return NULL;
  }

  rtb_container_t* win_container = RTB_ELEMENT(gui->win);

  rtb_elem_set_layout(win_container, rtb_layout_vpack_middle);
  rtb_elem_set_size_cb(win_container, rtb_size_vfill);

  build_ui(gui);

  rtb_event_loop_init(gui->rtb);

  if (resize) {
    resize->ui_resize(resize->handle, width, height);
  }

  return (LV2UI_Handle)gui;
}

void
cleanup(LV2UI_Handle ui) {
  SineSynthGui* gui = (SineSynthGui*)ui;

  rtb_window_lock(gui->win);
  rtb_event_loop_fini(gui->rtb);
  rtb_window_close(gui->win);

  rtb_free(gui->rtb);
}

void
port_event(LV2UI_Handle ui,
    uint32_t port_index,
    uint32_t buffer_size,
    uint32_t format,
    const void* buffer) {
  SineSynthGui* gui = (SineSynthGui*)ui;
  float* pval = (float*)buffer;

  if (format != 0) {
    return;
  }

  switch((PortIndex)port_index) {
  case PORT_VOLUME: 
    rtb_value_element_set_value(RTB_VALUE_ELEMENT(gui->volume->knob), *pval);
    break;
  //case PORT_PANNING:
  //  rtb_value_element_set_value(RTB_VALUE_ELEMENT(gui->panning), *pval);
  //  break;
  //case PORT_ATTACK_TIME:
  //  rtb_value_element_set_value(RTB_VALUE_ELEMENT(gui->attack), *pval);
  //  break;
  //case PORT_HOLD_TIME:
  //  rtb_value_element_set_value(RTB_VALUE_ELEMENT(gui->hold), *pval);
  //  break;
  //case PORT_DECAY_TIME:
  //  rtb_value_element_set_value(RTB_VALUE_ELEMENT(gui->decay), *pval);
  //  break;
  //case PORT_SUSTAIN_LEVEL:
  //  rtb_value_element_set_value(RTB_VALUE_ELEMENT(gui->sustain), *pval);
  //  break;
  //case PORT_RELEASE_TIME:
  //  rtb_value_element_set_value(RTB_VALUE_ELEMENT(gui->release), *pval);
  //  break;
  default:
    break;
  }
}

const void*
extension_data(const char* uri) {
  if (!strcmp(uri, LV2_UI__idleInterface)) { return &idle_iface; }

  return NULL;
}

static LV2UI_Descriptor descriptor = {
  SINE_SYNTH_UI_URI, instantiate, cleanup, port_event, extension_data
};

const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index) {
  switch (index) {
    case 0:  return &descriptor;
    default: return NULL;
  }
}
