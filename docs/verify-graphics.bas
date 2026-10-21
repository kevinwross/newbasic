10 SCREEN 1
20 CLS
30 REM A grey frame marks the exact canvas edges, so a screenshot can be mapped
40 REM back to canvas coordinates and every shape checked by colour. The colours
50 REM are chosen to exercise the SCREEN 1 palette: 1 is green, 2 red, 3 brown.
60 LINE (0,0)-(319,199),7,B
70 CIRCLE (80,100),40,1
80 LINE (0,199)-(319,0),5
90 LINE (200,20)-(260,60),4,BF
100 PSET (10,10),5
110 PSET (310,190),3
120 LINE (160,20)-STEP(20,20),4,B
125 REM The brown square below uses DRAW absolute M, which also draws a connector from
126 REM the current position (the canvas centre) to the square. That line is correct
127 REM behaviour, not a stray mark: DRAW BM would move without drawing it.
130 DRAW "C3M300,40R10D10L10U10"
140 GOTO 140
