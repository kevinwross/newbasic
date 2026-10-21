10 SCREEN 1
20 CLS
30 REM A viewport at (60,40)-(260,160): coordinates are relative to it, and drawing
40 REM outside it is clipped.
50 VIEW (60,40)-(260,160)
60 LINE (0,0)-(199,119),2
70 LINE (0,0)-(199,0),4
80 LINE (0,119)-(199,119),3
90 CIRCLE (100,60),30,1
100 LINE (0,0)-(400,300),6
110 GOTO 110
