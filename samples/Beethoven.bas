10 REM Beethoven.bas - the "Ode to Joy" from the Ninth Symphony, sixteen bars in D major.
11 REM
12 REM The melody in four phrases of four bars. Every note is a quarter note unless it carries a
13 REM length of its own, so the two long notes that end phrases (E in bar 4, D in bars 8, 12 and
14 REM 16) are written as halves and each bar still adds up to four beats.
15 REM
16 REM PLAY takes MML: T is the tempo in quarter notes a minute, O the octave, L the default note
17 REM length, and a digit after a note sets that note's length. T150 is half again as fast as the
18 REM T100 the first version used.
19 REM
20 PLAY "T150 O3 L4 F# F# G A"
30 PLAY "T150 O3 L4 A G F# E"
40 PLAY "T150 O3 L4 D D E F#"
50 PLAY "T150 O3 L4 F# E E2"
60 PLAY "T150 O3 L4 F# F# G A"
70 PLAY "T150 O3 L4 A G F# E"
80 PLAY "T150 O3 L4 D D E F#"
90 PLAY "T150 O3 L4 E D D2"
100 PLAY "T150 O3 L4 E E F# D"
110 PLAY "T150 O3 L4 E F# G F#"
120 PLAY "T150 O3 L4 D E F# G"
130 PLAY "T150 O3 L4 F# E D2"
140 PLAY "T150 O3 L4 E E F# D"
150 PLAY "T150 O3 L4 E F# G F#"
160 PLAY "T150 O3 L4 D E F# G"
170 PLAY "T150 O3 L4 F# E D2"
180 END
