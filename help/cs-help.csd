<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 16
nchnls  = 2
0dbfs = 0.9

; VCO housekeeping, from the vco2 example
; user defined waveform -1: trapezoid wave with default parameters (can be
; accessed at ftables starting from 10000)
itmp    ftgen 1, 0, 16384, 7, 0, 2048, 1, 4096, 1, 4096, -1, 4096, -1, 2048, 0
ift     vco2init -1, 10000, 0, 0, 0, 1

; declare a channel called pw1
; name pw1, mode 1, type 2 (lin scale), default 0.5, min 0.001, max 0.999
chn_k   "pw1", 3, 2, 0.5, 0.001, 0.999

; an instr to get give the pw1 channel a starting value
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

</CsInstruments>

<CsScore>
f0 3600     ; keep csound active for 1 hour for realtime events
f1 0 1024 10 1

; set pw1 to 0.5
i 1 0 1 0.5

i2 0 1 0.5 220 
i2 2 1 0.5 440
i2 4 1 0.5 660
i2 6 1 0.5 880
i2 8 1 0.5 660
i2 10 1 0.5 440
i2 12 1 0.5 220 
i1 7 1 0.5 220
i1 33 1 0.5 440
e

</CsScore>
</CsoundSynthesizer>


