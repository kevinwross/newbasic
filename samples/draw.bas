10 SCREEN 720
11 newColor = 15
15 VIEW (50,50)-(250,250),0,15
14 CLS
17 LINE (0,0)-(100,100), newColor
20 PSET (70,30),newColor
25 COLOR newColor
30 DRAW "R20 TA60 R20 TA60 R20 TA60 R20 TA60 R20 TA60 R20"
31 M$ = "Current Color: " + STR$(newColor)
32 DRAWTEXT (10,180), M$, 15
35 quitString$ = INKEY$
40 IF quitString$ = "" THEN 35
45 if quitString$ = "q" then END
50 newColor = newColor + 1
60 IF newColor > 15 THEN newColor = 1

100 goto 15

