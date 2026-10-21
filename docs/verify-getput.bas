10 SCREEN 1
20 CLS
30 CIRCLE (40,40),20,2
40 LINE (20,20)-(60,60),3,B
50 REM Capture the circle in its box, then stamp it twice and XOR it over a block.
60 GET (20,20)-(60,60), A%
70 PUT (110,20), A%
80 PUT (200,20), A%
90 LINE (110,110)-(150,150),1,BF
100 PUT (110,110), A%, XOR
110 GOTO 110
