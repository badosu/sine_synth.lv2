#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "sine_synth.h"

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#define N_VOICES (128)
#define N_TABLE_SIZE (2048)
#define PI (3.14159265358979323846)
#define TWO_PI (2 * PI)
#define PIOVR2 (PI/2)
#define ROOT2OVR2 (sqrt(2) * 0.5)
#define TABLE_INCREMENT (TWO_PI/N_TABLE_SIZE)

const float MIDI_NOTES[128] = {
  8.1757989156, 8.6619572180, 9.1770239974, 9.7227182413, 10.3008611535,
  10.9133822323, 11.5623257097, 12.2498573744, 12.9782717994,
  13.7500000000, 14.5676175474, 15.4338531643, 16.3515978313,
  17.3239144361, 18.3540479948, 19.4454364826, 20.6017223071,
  21.8267644646, 23.1246514195, 24.4997147489, 25.9565435987,
  27.5000000000, 29.1352350949, 30.8677063285, 32.7031956626,
  34.6478288721, 36.7080959897, 38.8908729653, 41.2034446141,
  43.6535289291, 46.2493028390, 48.9994294977, 51.9130871975,
  55.0000000000, 58.2704701898, 61.7354126570, 65.4063913251,
  69.2956577442, 73.4161919794, 77.7817459305, 82.4068892282,
  87.3070578583, 92.4986056779, 97.9988589954, 103.8261743950,
  110.0000000000, 116.5409403795, 123.4708253140, 130.8127826503,
  138.5913154884, 146.8323839587, 155.5634918610, 164.8137784564,
  174.6141157165, 184.9972113558, 195.9977179909, 207.6523487900,
  220.0000000000, 233.0818807590, 246.9416506281, 261.6255653006,
  277.1826309769, 293.6647679174, 311.1269837221, 329.6275569129,
  349.2282314330, 369.9944227116, 391.9954359817, 415.3046975799,
  440.0000000000, 466.1637615181, 493.8833012561, 523.2511306012,
  554.3652619537, 587.3295358348, 622.2539674442, 659.2551138257,
  698.4564628660, 739.9888454233, 783.9908719635, 830.6093951599,
  880.0000000000, 932.3275230362, 987.7666025122, 1046.5022612024,
  1108.7305239075, 1174.6590716696, 1244.5079348883, 1318.5102276515,
  1396.9129257320, 1479.9776908465, 1567.9817439270, 1661.2187903198,
  1760.0000000000, 1864.6550460724, 1975.5332050245, 2093.0045224048,
  2217.4610478150, 2349.3181433393, 2489.0158697766, 2637.0204553030,
  2793.8258514640, 2959.9553816931, 3135.9634878540, 3322.4375806396,
  3520.0000000000, 3729.3100921447, 3951.0664100490, 4186.0090448096,
  4434.9220956300, 4698.6362866785, 4978.0317395533, 5274.0409106059,
  5587.6517029281, 5919.9107633862, 6271.9269757080, 6644.8751612791,
  7040.0000000000, 7458.6201842894, 7902.1328200980, 8372.0180896192,
  8869.8441912599, 9397.2725733570, 9956.0634791066, 10548.0818212118,
  11175.3034058561, 11839.8215267723, 12543.8539514160
};

typedef enum {
  ATTACK = 0,
  HOLD,
  DECAY,
  SUSTAIN,
  RELEASE
} VoiceStatus;

typedef struct {
  uint8_t note;
  uint8_t velocity;
  float phase;
  float phase_increment;

  float envelope_index;
  float envelope_level;
  float released_envelope_level;

  float attack_level;
  float sustain_level;

  float attack_duration;
  float hold_duration;
  float decay_duration;
  float release_duration;

  VoiceStatus status;
} Voice;

typedef struct {
  double sample_rate;
  double sample_rate_ms;
  
  const LV2_Atom_Sequence* control;
  const float* volume;
  const float* panning;
  const float* attack_time;
  const float* hold_time;
  const float* sustain_level;
  const float* decay_time;
  const float* release_time;

  float volume_coef;
  float pan_left;
  float pan_right;
  float attack_duration;
  float hold_duration;
  float decay_duration;
  float release_duration;

  float* out_left;
  float* out_right;

  float wave_table[N_TABLE_SIZE];

  Voice* voices[N_VOICES];
  uint8_t active_voices_i[N_VOICES];
  uint8_t active_voices_n;

  LV2_URID_Map* map;

  struct {
    LV2_URID midi_MidiEvent;
  } uris;
} SineSynth;

/*
 * Calculate adsr for current voice
 */
static float
adsr(Voice* voice) {
  float level;

  switch(voice->status) {
  case SUSTAIN:
    level = voice->sustain_level;

    break;
  case ATTACK:
    if (voice->envelope_index > voice->attack_duration) {
      if (voice->hold_duration > 0) {
        voice->status = HOLD;
      }
      else {
        voice->status = DECAY;
      }
      voice->envelope_index = 0;

      level = voice->attack_level;
    }
    else {
      level = voice->envelope_index * (voice->attack_level / voice->attack_duration);
    }

    break;
  case HOLD:
    if (voice->envelope_index > voice->hold_duration) {
      voice->status = DECAY;
      voice->envelope_index = 0;
    }

    level = voice->attack_level;

    break;
  case DECAY:
    if (voice->envelope_index > voice->decay_duration) {
      if (voice->sustain_level > 0) {
        voice->status = SUSTAIN;
        voice->envelope_index = 0;
      }
      else {
        voice->velocity = 0; // Avoid processing SUSTAIN and RELEASE, would work without this
      }

      level = voice->sustain_level;
    }
    else {
      level = (voice->sustain_level - voice->attack_level) * (voice->envelope_index / voice->decay_duration) + voice->attack_level;
    }

    break;
  case RELEASE:
    if (voice->envelope_index > voice->release_duration) {
      voice->velocity = 0;

      level = 0;
    }
    else {
      level = voice->released_envelope_level * (voice->release_duration - voice->envelope_index)/voice->release_duration;
    }

    break;
  }

  return level;
}

static void
fill_wave_table(SineSynth* self) {
  for(int i=0; i<N_TABLE_SIZE; i++) {
    self->wave_table[i] = sin(i * TABLE_INCREMENT);
  }
}

static float
sin_table(float phase, SineSynth* self) {
  return self->wave_table[(int)roundf(phase/TABLE_INCREMENT)];
}

/*
 * Render a voice sample
 */
static float
tick_voice(Voice* voice, SineSynth* self) {
  float val = sin_table(voice->phase, self);

  voice->phase += voice->phase_increment;
  if (voice->phase > TWO_PI) {
    voice->phase -= TWO_PI;
  }

  voice->envelope_level = adsr(voice);

  if (voice->status != SUSTAIN) {
    voice->envelope_index++;
  }

  return val * voice->envelope_level;
}

/*
 * Get active voice assigned to note
 */
static Voice*
get_active_voice(uint8_t note, SineSynth* self) {
  for(int i_voice=0; i_voice < self->active_voices_n; i_voice++) {
    int ai_voice = self->active_voices_i[i_voice];
    Voice* voice = self->voices[ai_voice];

    if (voice->note == note) {
      return voice;
    }
  }

  return NULL;
}

/*
 * Activate voice and return it
 */
static Voice*
activate_voice(SineSynth* self) {
  for(int i_voice=0; i_voice < N_VOICES; i_voice++) {
    Voice* voice = self->voices[i_voice];

    if (voice->velocity == 0) {
      self->active_voices_i[self->active_voices_n++] = i_voice;

      return voice;
    }
  }

  return NULL;
}

static void
note_on(uint8_t note, uint8_t velocity, SineSynth* self) {
  Voice* voice = get_active_voice(note, self);

  if (voice != NULL) {
    // Voice is in release phase, reattack from current envelope level
    voice->status = ATTACK;
    voice->envelope_index = voice->attack_duration * (voice->envelope_level / voice->attack_level);

    return;
  }

  // Activate voice and assign note to it
  voice = activate_voice(self);

  if (voice != NULL) {
    voice->note = note;
    voice->velocity = velocity;

    voice->phase = 0;
    voice->phase_increment = (MIDI_NOTES[note] * TWO_PI) / self->sample_rate;

    voice->status = ATTACK;
    voice->envelope_index = 0;
    voice->released_envelope_level = 0;

    voice->attack_level = 1;
    voice->attack_duration = self->attack_duration;
    voice->hold_duration = self->hold_duration;
    voice->decay_duration = self->decay_duration;
    voice->release_duration = self->release_duration;
    voice->sustain_level = *self->sustain_level;
  }
}

static void
note_off(uint8_t note, SineSynth* self) {
  Voice* voice = get_active_voice(note, self);

  if (voice != NULL) {
    voice->status = RELEASE;
    voice->envelope_index = 0;
    voice->released_envelope_level = voice->envelope_level;
  }
}

static void
deactivate_voice(uint8_t index, SineSynth* self) {
  self->active_voices_n--;

  for(uint8_t i = index; i < self->active_voices_n; i++) {
    self->active_voices_i[i] = self->active_voices_i[i+1];
  }
}

static void
render_samples(uint32_t from, uint32_t to, SineSynth* self) {
  float* const out_left  = self->out_left;
  float* const out_right = self->out_right;

  for (uint32_t pos = from; pos < to; pos++) {
    out_right[pos] = 0;
    out_left[pos]  = 0;

    for(uint8_t i_voice=0; i_voice < self->active_voices_n; i_voice++) {
      uint8_t ai_voice = self->active_voices_i[i_voice];
      Voice* voice = self->voices[ai_voice];

      if (voice->velocity > 0) {
        float out = tick_voice(voice, self) * self->volume_coef;

        out_right[pos] += self->pan_right * out;
        out_left[pos]  += self->pan_left  * out;
      }
      else {
        deactivate_voice(i_voice, self);

        i_voice--;
      }
    }
  }
}

/* Recalculate params every run() call to avoid calculating for each
   sample.
   Changing these parameters on the host will only have effect on the
   next run() call. */
static void
recalculate_params(SineSynth* self) {
  self->attack_duration  = (*self->attack_time)  * self->sample_rate_ms;
  self->hold_duration    = (*self->hold_time)    * self->sample_rate_ms;
  self->decay_duration   = (*self->decay_time)   * self->sample_rate_ms;
  self->release_duration = (*self->release_time) * self->sample_rate_ms;

  self->volume_coef = DB_CO(*(self->volume));

  float angle     = (*(self->panning)) * PIOVR2 * 0.5 + PI;
  float sin_angle = sin_table(angle, self);
  float cos_angle = sin_table(angle + PIOVR2, self);

  self->pan_left  = ROOT2OVR2 * (cos_angle - sin_angle);
  self->pan_right = ROOT2OVR2 * (cos_angle + sin_angle);
}

/* -----------------
 * LV2 Audio functions
 * See: http://lv2plug.in/doc/html/group__lv2core.html#structLV2__Descriptor
 * -----------------
 */

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
  LV2_URID_Map* map = NULL;
  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID__map)) {
      map = (LV2_URID_Map*)features[i]->data;
      break;
    }
  }

  if (!map) {
    fprintf(stderr, "Host does not support urid:map.\n");
    return NULL;
  }

  SineSynth* self = (SineSynth*)malloc(sizeof(SineSynth));

  self->map = map;
  self->uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
  self->sample_rate    = rate;
  self->sample_rate_ms = rate / 1000.0;

  for (uint8_t i_voice = 0; i_voice < N_VOICES; i_voice++) {
    Voice* voice = (Voice*)malloc(sizeof(Voice));

    voice->velocity = 0;

    self->voices[i_voice] = voice;
  }

  self->active_voices_n = 0;

  fill_wave_table(self);
  
  return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
  SineSynth* self = (SineSynth*)instance;

  switch ((PortIndex)port) {
  case PORT_MIDI_IN:
    self->control = (const LV2_Atom_Sequence*)data;
    break;
  case PORT_VOLUME:
    self->volume = (const float*)data;
    break;
  case PORT_PANNING:
    self->panning = (const float*)data;
    break;
  case PORT_ATTACK_TIME:
    self->attack_time = (const float*)data;
    break;
  case PORT_HOLD_TIME:
    self->hold_time = (const float*)data;
    break;
  case PORT_SUSTAIN_LEVEL:
    self->sustain_level = (const float*)data;
    break;
  case PORT_DECAY_TIME:
    self->decay_time = (const float*)data;
    break;
  case PORT_RELEASE_TIME:
    self->release_time = (const float*)data;
    break;
  case PORT_AUDIO_OUT_LEFT:
    self->out_left  = (float*)data;
    break;
  case PORT_AUDIO_OUT_RIGHT:
    self->out_right = (float*)data;
    break;
  }
}

/*
 * LV2 Audio function, should not allocate resources to be Real-Time
 * capable
 */
static void
activate(LV2_Handle instance)
{
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
  SineSynth* self = (SineSynth*)instance;

  recalculate_params(self);

  uint32_t samples_done = 0;

  LV2_ATOM_SEQUENCE_FOREACH(self->control, ev) {
    if (ev->body.type == self->uris.midi_MidiEvent) {
      const uint8_t* const msg = (const uint8_t*)(ev + 1);

      render_samples(samples_done, ev->time.frames, self);
      samples_done = ev->time.frames;

      switch (lv2_midi_message_type(msg)) {
      case LV2_MIDI_MSG_NOTE_ON:
        note_on(msg[1], msg[2], self);

        break;
      case LV2_MIDI_MSG_NOTE_OFF:
        note_off(msg[1], self);

        break;
      default: break;
      }
    }
  }

  render_samples(samples_done, n_samples, self);
}

/*
 * Free resources allocated on activate()
 */
static void
deactivate(LV2_Handle instance)
{
}

/*
 * Free resources allocated on instantiate()
 */
static void
cleanup(LV2_Handle instance)
{
  SineSynth* self = (SineSynth*)instance;

  for (uint8_t i_voice = 0; i_voice < N_VOICES; i_voice++) {
    free(self->voices[i_voice]);
  }

  free(self);
}

const void*
extension_data(const char* uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {
  SINE_SYNTH_URI,
  instantiate,
  connect_port,
  activate,
  run,
  deactivate,
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  switch (index) {
  case 0:
    return &descriptor;
  default:
    return NULL;
  }
}
