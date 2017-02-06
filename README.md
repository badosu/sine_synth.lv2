Sine Synth
===========

The simplest efficient, good sounding sine synth I was able to make.

Features
--------

- Polyphonic (128 voices, defined at compile time)
- ADSR Envelope
- MIDI Input

Install
-------

Just `make` and `sudo make install`, that should install the bundle at `/usr/local/lib/lv2`.

Motivation
----------

- Educational purposes
- PoC for a more complex synth

Areas of Improvement
--------------------

### Usability

- Simplify ADSR parameters

### Sound

- Test weird combinations of ADSR parameters and check if clicking is expected or not
- Apply normalization on voice output if improvements are noticeable

License
-------

This software is available as Free Software under the terms of the [GPL](https://opensource.org/licenses/GPL-3.0).
