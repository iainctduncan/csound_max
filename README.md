# Csound6~
## A minimal real time csound class using the Csound6 API
## (c) Iain C.T. Duncan 2022, based on work by Victor Lazzarini, (c) 2005-2013.

Csound6~ is mostly a port of Victor Lazzarini's csound object for Pure Data. It provides
a minimal csound interface using the csound API and thus should provide better performance
and latency than the legacy csound~ object. It does not attempt to port all the features
of the legacy csound object.

## Features
- Allows playing csd, orc, and sco files
- Plays realtime messages with score syntax
- Scores playback time can be manipulated with no latecy
- Supports up to 32 inlets and outlets, corresponding to number of csound channels
- Can receive realtime modulation data using the chnget and invalue opcodes

## Limitations
- Ksmps must be an even divisor of the Max signal vector size so that there are one
  or more even kpasses per audio vector calculation.
- Realtime events are limited to "i", "f", and "e".
- Table writing and reading opcodes are not yet implemented, but are planned.
- Output control messages are not yet implemented, but are planned.
- Csound midi opcodes are not supported. They may be ported if there is sufficient demand, though
  using Max midi and communicating with score messages is recommended instead.

## Installation
- Csound6~ is currently compiled for Mac (Intel) only. Windows and M1 coming shortly!
- Download the release and expand in your Max packages directory.
- To run the help file, ensure the path to the help folder is in your Max filepaths


