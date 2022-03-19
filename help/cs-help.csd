<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
; ksmps must be an even divisor of the Max signal vector size
ksmps   = 16
nchnls  = 2
0dbfs = 0.9

; VCO housekeeping, from the vco2 example
; user defined waveform -1: trapezoid wave with default parameters (can be
; accessed at ftables starting from 10000)
itmp    ftgen 1, 0, 16384, 7, 0, 2048, 1, 4096, 1, 4096, -1, 4096, -1, 2048, 0
ift     vco2init -1, 10000, 0, 0, 0, 1

; an instr to get give the pw1 channel a starting value so that
; it doesn't start at 0
instr 1
  chnset  p4, "pw1"
endin

; a VCO instrument that reads the pulse width from the pw1 channel
instr 2
  kpw chnget "pw1"
  aenv expsega 0.001, 0.002, 1, p3 - 0.4, 1, 0.2, 0.001
  aosc vco2    1, p5, 2, kpw
  asig = aosc * aenv * p4
  outs asig, asig
endin

; an example of using the older invalue input mechanism
; not recommended as is less safe in the externals code
instr 3
  kpw invalue "pw"
  aenv expsega 0.001, 0.002, 1, p3 - 0.4, 1, 0.2, 0.001
  aosc vco2    1, p5, 2, kpw
  asig = aosc * aenv * p4
  outs asig, asig
endin

; instr for testing table playback
instr 4
  aenv expsega 0.001, 0.002, 1, p3 - 0.4, 1, 0.2, 0.001
  ; table oscillation of table 1
  aosc oscili 1, p5, 1
  asig = aosc * aenv * p4
  outs asig, asig
endin

; two nameds instrument
instr square
  aenv expsega 0.001, 0.002, 1, p3 - 0.4, 1, 0.2, 0.001
  aosc vco2    1, p5, 2, 0.5
  asig = aosc * aenv * p4
  outs asig, asig
endin

instr pulse
  aenv expsega 0.001, 0.002, 1, p3 - 0.4, 1, 0.2, 0.001
  aosc vco2    1, p5, 2, 0.8
  asig = aosc * aenv * p4
  outs asig, asig
endin

; a name instrument that writes an lfo to channel "lfo1"
instr lfo
  ; amplitude 1, frequency from p4
  klfo lfo 1, p4
  chnset klfo, "lfo1"
endin

; you can even get the channel name from a string pfield
instr make_lfo
  ; get a string for the channel name from p5
  S_chan strget p5
  ; amplitude 1, frequency from p4
  klfo lfo 1, p4
  chnset klfo, S_chan
endin

; instrument that uses outvalue to send out control messages
; it will send a message every time the outvalue opcode runs
instr outvalue_instr
  S_chan strget p4
  ioutval init p5
  outvalue S_chan, ioutval
endin

</CsInstruments>

<CsScore>
; keep csound active for realtime events
; change this to see the bang on score completion
f 0 3600         

; create an 8 point table with Gen 2 (explicit values
f 1 0 8 2   0 0.25 .5 .75 1 .75 .5 .25  

; set pw1 to 0.5
i 1 0 1 0.5

; play some notes
i2 0 1 0.5 220 
i2 2 1 0.5 440
i2 4 1 0.5 660
i2 6 1 0.5 880
i2 8 1 0.5 440
i2 10 1 0.5 220
e

</CsScore>
</CsoundSynthesizer>


