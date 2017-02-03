Sine Synth
===========

The simplest good sounding sine synth I was able to make.

Features
--------

- Polyphonic (number of voices defined at compile time)
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

### Performance

- Replace trigonometric function with wavetable
- Render only active voices

### Usability

- Reduce ADSR parameters
- Test weird combinations of ADSR parameters and check if clicking is expected or not

License
-------

This software is available as Free Software under the terms of the [GPL](https://opensource.org/licenses/GPL-3.0).
