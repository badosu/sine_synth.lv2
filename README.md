Sine Synth
===========

A very simple, efficient and good sounding sine synth.

Features
--------

- Polyphonic (128 voices, defined at compile time)
- ADSR Envelope
- MIDI Input

Install
-------

```bash
make rutabaga
make
make install # Install the bundle at `~/.lv2`, run as root to install under `/usr/lib/lv2`
```

Motivation
----------

- Educational purposes
- PoC for a more complex synth

Areas of Improvement
--------------------

### Sound

- Test weird combinations of ADSR parameters and check if clicking is expected or not
- Apply normalization on voice output if improvements are noticeable

### GUI

- Apply custom style
- Implement visual envelope
- Group ADSR controls
- Group Pan and Volume controls

License
-------

This software is available as Free Software under the terms of the [GPL](https://opensource.org/licenses/GPL-3.0).
