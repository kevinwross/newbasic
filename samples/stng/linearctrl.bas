10  SCREEN 720
20  CLS
30  DIM ctrlBound(1,1)
40  ctrlBound(0,0)=80           ' left
50  ctrlBound(0,1)=80           ' top
60  ctrlBound(1,0)=100          ' width
70  ctrlBound(1,1)=20           ' height
80  GOSUB 1000
90  while INKEY$<>"q" : goto 90 : wend
100 end

1000 REM --- DrawLinearScale -----------------------------------------
1010 REM ctrlBound(0,0)=left, ctrlBound(0,1)=top
1020 REM ctrlBound(1,0)=width, ctrlBound(1,1)=height
1030 ctrlLeft=ctrlBound(0,0): ctrlTop=ctrlBound(0,1)
1040 ctrlWidth=ctrlBound(1,0): ctrlHeight=ctrlBound(1,1)
1050 IF ctrlWidth<=0 OR ctrlHeight<=0 THEN RETURN
1060 ctrlRight=ctrlLeft+ctrlWidth-1
1070 ctrlBottom=ctrlTop+ctrlHeight-1
1080 VIEW (ctrlLeft,ctrlTop)-(ctrlRight,ctrlBottom),0,15
1090 CLS 1
1100 LINE (0,0)-(ctrlWidth-1,ctrlHeight-1),15,B
1110 REM --- optional simple 0..10 scale along the bottom ---
1120 baseY=ctrlHeight-9
1130 LINE (6,baseY)-(ctrlWidth-7,baseY),15
1140 FOR tickNum=0 TO 10
1150   tickX=6+INT(tickNum*(ctrlWidth-13)/10)
1160   tickLen=3
1170   IF tickNum MOD 5=0 THEN tickLen=6
1180   LINE (tickX,baseY)-(tickX,baseY+tickLen),15
1190 NEXT tickNum
1200 VIEW
1210 RETURN