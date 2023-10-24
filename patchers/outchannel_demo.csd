<CsoundSynthesizer>
<CsInstruments>
sr = 44100
nchnls = 2
0dbfs = 1.0
ksmps = 32

instr lfo
  ; p4 chan, p5 amp, p6 cps
  S_chan strget p4
  klfo lfo p5, p6
  chnset klfo, S_chan
endin


</CsInstruments>

<CsScore>
; keep csound score playing for an hour
f0 3600

; sample score statement to make an lfo  
;  instr start  dur    chan        amp     cps
;i lfo   0      10      lfo_name    127     0.5

e
</CsScore>
</CsoundSynthesizer>

