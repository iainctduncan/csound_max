sr = 44100
nchnls = 2
0dbfs = 1.0
; ksmps must be a divisor the Max signal vector size
ksmps = 16

instr lfo
  S_chan strget p4
  klfo lfo 1, 1
  kout = p5 * klfo
  chnset klfo, S_chan
endin

;; keep csound score playing for an hour
;f0 3600


