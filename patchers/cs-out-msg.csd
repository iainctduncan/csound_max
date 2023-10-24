<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 16
nchnls  = 2
0dbfs = 0.9


; instrument that uses outvalue to send out control messages
instr 1
  ioutval init p4
  outvalue "chan1", ioutval
endin

; instrument that writes to a named channel, but only 
; receives numeric pfields (channel name is static)
instr 2
  ; amplitude 1, frequency
  klfo lfo 1, 1
  kmult init p4
  kout = kmult * klfo
  chnset kout, "lfo1"
endin

; instrument that gets the channel name from p4
; WARNING: if you send a numeric pfield to p4 by accident, this will crash
; though this will be fixed in the next csound release
instr 3
  S_chan strget p4
  klfo lfo 1, 1
  chnset klfo, S_chan
endin

</CsInstruments>

<CsScore>
f0 3600
; turn on i3 for 4 seconds and write to chan "lfo2"
;i3 0 2 "lfo2" 


e

</CsScore>
</CsoundSynthesizer>

