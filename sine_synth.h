#ifndef SINE_SYNTH
#define SINE_SYNTH

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define SINE_SYNTH_URI "http://bado.so/plugins/sine_synth"

typedef enum {
  PORT_MIDI_IN = 0,
  PORT_VOLUME,
  PORT_PANNING,
  PORT_ATTACK_TIME,
  PORT_HOLD_TIME,
  PORT_SUSTAIN_LEVEL,
  PORT_DECAY_TIME,
  PORT_RELEASE_TIME,
  PORT_AUDIO_OUT_LEFT,
  PORT_AUDIO_OUT_RIGHT
} PortIndex;

#endif
