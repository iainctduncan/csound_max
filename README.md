# Csound6~
## A minimal real time csound class using the Csound6 API
### (c) Iain C.T. Duncan 2022, based on work by Victor Lazzarini, (c) 2005-2013.

Csound6~ is mostly a port of Victor Lazzarini's csound object for Pure Data. It provides
a minimal csound interface using the csound API and thus should provide better performance
and latency than the legacy csound~ object. It does not attempt to port all the features
of the legacy csound object.

## Features
- Allows playing csd, orc, and sco files
- Plays realtime messages with score syntax
- Scores playback time can be manipulated with minimal latency
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
- Csound6~ is currently compiled for Mac (Intel) and Win64. M1 coming shortly!
- Install Csound6 for your platform. The csound6~ object will use the csound version installed on your machine, so there is no need to reinstall csound6~ when you upgrade Csound. Download it from https://csound.com/download.html
- Download the release and expand in your Max packages directory.
- To run the help file, ensure the path to the help folder is in your Max filepaths
- Running with Overdrive enabled and Audio In Interrupt is recommended.

## Reporting Issues
- please create tickets on this projects GitHub issues board.

## Help out
- if you are experienced at compiling on Windows, help on the windows build would be much appreciated


