<CsoundSynthesizer>
<CsInstruments>
sr      = 44100
ksmps   = 16
nchnls  = 2
0dbfs = 0.9


; instrument to print the contents of our table
instr 1
  ; read index p4 from table 1
  itabval table  p4, 1
  prints "table 1 point %i %f", p4, itabval 
endin


</CsInstruments>


<CsScore>
f0 3600
f1 0 8 2  0 1 2 3 4 5 6 7 

;i1 0 1 1.0
;i1 0 1 2.0
;i1 0 1 3.0
;i1 0 1 4.0
e

</CsScore>
</CsoundSynthesizer>


