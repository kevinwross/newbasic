10 REM STARTREKNG - A MOUSE-DRIVEN SCREEN 720 STAR TREK CONSOLE
20 REM THE ORIGINAL GAME LOGIC IS BASED ON MIKE MAYFIELD'S STAR TREK
30 REM AND BOB LEEDOM'S SUPER STAR TREK (1978 MICROSOFT BASIC VERSION).
40 SCREEN 720
50 DIM galaxyMap(8,8),baseRows(8,8),baseCols(8,8)
60 DIM klingonRows(200),klingonCols(200),klingonHull(200),starRows(200),starCols(200),systemHealth(8),systemName$(8)
70 DIM courseRows(8),courseCols(8),commandLabel$(7),compassX(8),compassY(8)
80 courseRows(1)=0:courseCols(1)=1:courseRows(2)=-1:courseCols(2)=1:courseRows(3)=-1:courseCols(3)=0:courseRows(4)=-1:courseCols(4)=-1
90 courseRows(5)=0:courseCols(5)=-1:courseRows(6)=1:courseCols(6)=-1:courseRows(7)=1:courseCols(7)=0:courseRows(8)=1:courseCols(8)=1
100 commandLabel$(0)="NAV":commandLabel$(1)="SCAN":commandLabel$(2)="PHASERS":commandLabel$(3)="TORPEDO"
110 commandLabel$(4)="SHIELDS":commandLabel$(5)="DAMAGE":commandLabel$(6)="COMPUTER":commandLabel$(7)="HELP"
125 compassX(1)=1:compassY(1)=0:compassX(2)=.7071:compassY(2)=-.7071:compassX(3)=0:compassY(3)=-1:compassX(4)=-.7071:compassY(4)=-.7071
128 compassX(5)=-1:compassY(5)=0:compassX(6)=-.7071:compassY(6)=.7071:compassX(7)=0:compassY(7)=1:compassX(8)=.7071:compassY(8)=.7071
126 systemName$(1)="WARP":systemName$(2)="IMPULSE":systemName$(3)="PHASERS":systemName$(4)="TORP"
127 systemName$(5)="SHIELDS":systemName$(6)="SENSORS":systemName$(7)="COMPUTER":systemName$(8)="LIFE"
130 PALETTE 0,0
140 PALETTE 1,0+58*256+0
150 PALETTE 2,63+0+0
160 PALETTE 3,63+55*256+0
170 PALETTE 4,0+40*256+63*65536
180 PALETTE 5,0+0+50*65536
190 PALETTE 6,30+30*256+30*65536
200 PALETTE 7,63+63*256+63*65536
210 PALETTE 8,0+8*256+14*65536
220 PALETTE 9,12+12*256+12*65536
230 previousMouse=0
240 GOSUB 1000
250 consoleMode$="COMMAND":statusMessage$="SELECT A COMMAND"
260 GOSUB 5000
270 GOTO 300

300 REM ---- MAIN LOOP ----
310 pointerX=MOUSEX:pointerY=MOUSEY:mouseButton=MOUSEB
320 IF mouseButton=1 AND previousMouse=0 THEN previousMouse=1:GOSUB 3000:GOTO 500
330 previousMouse=mouseButton
340 keyPress$=INKEY$
350 IF keyPress$<>"" THEN GOSUB 4000:GOTO 500
360 GOTO 310
500 GOSUB 5000
510 GOTO 310

1000 REM ---- NEW GAME ----
1010 klingonsLeft=0:starbaseCount=0:gameOver=0:missionWon=0
1020 quadRow=4:quadCol=4:sectorRow=4:sectorCol=4:currentHeading=1:course=1:warpFactor=1
1030 shipEnergy=3000:shieldPower=0:hullIntegrity=100:torpedoCount=10:timeLeft=30:stardate=3400
1040 targetIndex=0:phaserPower=500:transferAmount=100:shieldDirection=1
1045 FOR I=1 TO 8:systemHealth(I)=100:NEXT I
1050 FOR galaxyRow=1 TO 8:FOR galaxyCol=1 TO 8
1060 galaxyMap(galaxyRow,galaxyCol)=0:baseRows(galaxyRow,galaxyCol)=0:baseCols(galaxyRow,galaxyCol)=0
1070 quadrantIndex=(galaxyRow-1)*8+galaxyCol
1080 FOR I=1 TO 3:klingonIndex=(quadrantIndex-1)*3+I:klingonRows(klingonIndex)=0:klingonCols(klingonIndex)=0:klingonHull(klingonIndex)=0:starRows(klingonIndex)=0:starCols(klingonIndex)=0:NEXT I
1090 randomRoll=RND:localKlingons=0
1100 IF randomRoll>.90 THEN localKlingons=1
1110 IF randomRoll>.97 THEN localKlingons=2
1112 IF randomRoll>.995 THEN localKlingons=3
1115 galaxyMap(galaxyRow,galaxyCol)=localKlingons*100:klingonsLeft=klingonsLeft+localKlingons
1120 FOR I=1 TO localKlingons:klingonIndex=(quadrantIndex-1)*3+I:klingonRows(klingonIndex)=INT(RND*8)+1:klingonCols(klingonIndex)=INT(RND*8)+1:klingonHull(klingonIndex)=100:NEXT I
1130 IF RND>.95 AND starbaseCount<4 THEN galaxyMap(galaxyRow,galaxyCol)=galaxyMap(galaxyRow,galaxyCol)+10:baseRows(galaxyRow,galaxyCol)=INT(RND*8)+1:baseCols(galaxyRow,galaxyCol)=INT(RND*8)+1:starbaseCount=starbaseCount+1
1140 starCount=INT(RND*4):galaxyMap(galaxyRow,galaxyCol)=galaxyMap(galaxyRow,galaxyCol)+starCount
1150 FOR I=1 TO starCount:klingonIndex=(quadrantIndex-1)*3+I:starRows(klingonIndex)=INT(RND*8)+1:starCols(klingonIndex)=INT(RND*8)+1:NEXT I
1160 NEXT galaxyCol:NEXT galaxyRow
1170 quadrantIndex=(quadRow-1)*8+quadCol:localKlingons=INT(galaxyMap(quadRow,quadCol)/100):galaxyMap(quadRow,quadCol)=galaxyMap(quadRow,quadCol)-localKlingons*100
1180 FOR I=1 TO localKlingons:klingonIndex=(quadrantIndex-1)*3+I:IF klingonHull(klingonIndex)>0 THEN klingonHull(klingonIndex)=0:klingonsLeft=klingonsLeft-1
1190 NEXT I
1200 IF klingonsLeft<1 THEN klingonRows(1)=1:klingonCols(1)=1:klingonHull(1)=100:klingonsLeft=1:galaxyMap(1,1)=galaxyMap(1,1)+100
1210 RETURN

2000 REM ---- DRAW SHORT RANGE SCANNER ----
2010 LINE (16,64)-(496,544),8,BF
2020 LINE (16,64)-(496,544),4,B
2030 LOCATE 5,4:PRINT "SHORT RANGE SCANNER"
2040 LINE (48,96)-(464,512),4,B
2050 FOR I=0 TO 8
2060 LINE (56+I*50,104)-(56+I*50,504),9
2070 LINE (56,104+I*50)-(456,104+I*50),9
2080 NEXT I
2090 quadrantIndex=(quadRow-1)*8+quadCol
2100 FOR I=1 TO 3
2110 klingonIndex=(quadrantIndex-1)*3+I
2120 IF starRows(klingonIndex)>0 AND starCols(klingonIndex)>0 THEN GOSUB 2180
2130 NEXT I
2140 FOR I=1 TO 3
2150 klingonIndex=(quadrantIndex-1)*3+I
2160 IF klingonHull(klingonIndex)>0 THEN GOSUB 2165
2161 NEXT I
2162 IF baseRows(quadRow,quadCol)>0 THEN GOSUB 2175
2163 GOSUB 2190
2164 RETURN
2165 REM KLINGON
2166 iconX=56+(klingonCols(klingonIndex)-1)*50+25:iconY=104+(klingonRows(klingonIndex)-1)*50+25
2167 LINE (iconX-7,iconY-5)-(iconX,iconY+7),2
2168 LINE (iconX+7,iconY-5)-(iconX,iconY+7),2
2169 IF targetIndex=klingonIndex THEN LINE (iconX-10,iconY-10)-(iconX+10,iconY+10),3,B
2170 RETURN
2175 REM STARBASE
2176 iconX=56+(baseCols(quadRow,quadCol)-1)*50+25:iconY=104+(baseRows(quadRow,quadCol)-1)*50+25
2177 LINE (iconX,iconY-8)-(iconX+8,iconY),4:LINE (iconX+8,iconY)-(iconX,iconY+8),4:LINE (iconX,iconY+8)-(iconX-8,iconY),4:LINE (iconX-8,iconY)-(iconX,iconY-8),4
2178 RETURN
2180 REM STAR
2181 iconX=56+(starCols(klingonIndex)-1)*50+25:iconY=104+(starRows(klingonIndex)-1)*50+25
2182 PSET (iconX,iconY),6:PSET (iconX-2,iconY),6:PSET (iconX+2,iconY),6:PSET (iconX,iconY-2),6:PSET (iconX,iconY+2),6
2183 RETURN
2190 REM ENTERPRISE
2191 iconX=56+(sectorCol-1)*50+25:iconY=104+(sectorRow-1)*50+25
2192 LINE (iconX-8,iconY+7)-(iconX+8,iconY+7),5
2193 LINE (iconX-8,iconY+7)-(iconX,iconY-8),5
2194 LINE (iconX+8,iconY+7)-(iconX,iconY-8),5
2195 LINE (iconX,iconY-2)-(iconX,iconY+7),7
2196 RETURN

2200 REM ---- MAIN VIEWER / GALAXY ----
2210 LINE (512,64)-(912,632),8,BF
2220 LINE (512,64)-(912,632),4,B
2230 LOCATE 5,66:PRINT "GALAXY / LONG RANGE"
2240 FOR mapRow=1 TO 8
2250 LOCATE 6+mapRow,66
2260 FOR mapCol=1 TO 8
2270 mapGlyph$="."
2280 IF mapRow=quadRow AND mapCol=quadCol THEN mapGlyph$="@"
2285 IF mapRow=quadRow AND mapCol=quadCol THEN 2300
2290 IF galaxyMap(mapRow,mapCol)>=100 THEN mapGlyph$="K"
2292 IF mapGlyph$="." AND galaxyMap(mapRow,mapCol)>=10 THEN mapGlyph$="B"
2294 IF mapGlyph$="." AND galaxyMap(mapRow,mapCol)>0 THEN mapGlyph$="*"
2300 PRINT mapGlyph$;
2310 NEXT mapCol
2320 PRINT
2330 NEXT mapRow
2340 LOCATE 16,66:PRINT "QUADRANT";quadRow;",";quadCol;"  SHIP ";sectorRow;",";sectorCol
2350 LOCATE 18,66:PRINT "LOCAL CONTACTS"
2360 contactLine=19:quadrantIndex=(quadRow-1)*8+quadCol:contactCount=0
2370 FOR I=1 TO 3
2380 klingonIndex=(quadrantIndex-1)*3+I
2390 IF klingonHull(klingonIndex)>0 THEN GOSUB 2550:contactCount=1
2395 NEXT I
2396 IF contactCount=0 THEN LOCATE 19,66:PRINT "NONE"
2397 LOCATE 26,66:PRINT "MISSION LOG"
2398 LOCATE 28,66:PRINT statusMessage$
2399 RETURN

2400 REM ---- RIGHT STATUS PAGE ----
2410 LINE (928,64)-(1264,632),8,BF
2420 LINE (928,64)-(1264,632),4,B
2430 LOCATE 5,117:PRINT "NAV STATUS"
2440 LOCATE 7,117:PRINT "QUADRANT"
2445 LOCATE 7,145:PRINT quadRow;",";quadCol
2450 LOCATE 8,117:PRINT "SECTOR"
2455 LOCATE 8,145:PRINT sectorRow;",";sectorCol
2460 LOCATE 9,117:PRINT "ENERGY"
2465 LOCATE 9,145:PRINT INT(shipEnergy)
2467 barLeft=940:barTop=144:barWidth=300:barHeight=8:barValue=shipEnergy:barMax=3000:barColor=1:GOSUB 7000
2470 LOCATE 11,117:PRINT "SHIELDS"
2475 LOCATE 11,145:PRINT INT(shieldPower)
2477 barLeft=940:barTop=176:barWidth=300:barHeight=8:barValue=shieldPower:barMax=2000:barColor=4:GOSUB 7000
2480 LOCATE 13,117:PRINT "HULL"
2485 LOCATE 13,145:PRINT INT(hullIntegrity);"%"
2487 barLeft=940:barTop=208:barWidth=300:barHeight=8:barValue=hullIntegrity:barMax=100:barColor=2:GOSUB 7000
2490 LOCATE 15,117:PRINT "TORPEDOES"
2495 LOCATE 15,145:PRINT torpedoCount
2500 LOCATE 16,117:PRINT "KLINGONS"
2505 LOCATE 16,145:PRINT klingonsLeft
2510 LOCATE 17,117:PRINT "STARBASES"
2515 LOCATE 17,145:PRINT starbaseCount
2520 LOCATE 18,117:PRINT "TIME LEFT"
2525 LOCATE 18,145:PRINT timeLeft
2530 LOCATE 19,117:PRINT "STARDATE"
2535 LOCATE 19,145:PRINT stardate
2540 RETURN

2550 REM CONTACT LINE
2551 contactRange=INT(SQR((klingonRows(klingonIndex)-sectorRow)^2+(klingonCols(klingonIndex)-sectorCol)^2))
2552 LOCATE contactLine,66:PRINT "KLINGON ";klingonRows(klingonIndex);",";klingonCols(klingonIndex);"  RANGE ";contactRange
2553 contactLine=contactLine+1
2554 RETURN

2560 REM DAMAGE CONTROL GAUGES
2562 LOCATE 21,117:PRINT "DAMAGE CONTROL"
2563 LOCATE 22,117:PRINT "NEG    0    POS"
2564 FOR I=1 TO 8
2566 gaugeY=352+(I-1)*32:gaugeRow=23+(I-1)*2
2568 LOCATE gaugeRow,117:PRINT systemName$(I)
2570 LOCATE gaugeRow,127:PRINT INT(systemHealth(I))
2572 LINE (1040,gaugeY)-(1240,gaugeY+10),9,B
2574 LINE (1140,gaugeY)-(1140,gaugeY+10),7
2576 gaugeValue=systemHealth(I):IF gaugeValue>100 THEN gaugeValue=100
2578 IF gaugeValue<-100 THEN gaugeValue=-100
2580 segmentsToDraw=INT(ABS(gaugeValue)/10)
2582 FOR J=1 TO segmentsToDraw
2584 IF gaugeValue>=0 THEN GOSUB 2591
2586 IF gaugeValue<0 THEN GOSUB 2596
2588 NEXT J:LINE (1140,gaugeY)-(1140,gaugeY+10),7
2589 NEXT I
2594 RETURN
2591 sectorRow=1140+(J-1)*10:COL=3:IF J>5 THEN COL=1
2592 LINE (sectorRow,gaugeY+1)-(sectorRow+9,gaugeY+9),COL,BF
2593 RETURN
2596 sectorRow=1140-J*10:COL=2:IF J<2 THEN COL=3
2597 LINE (sectorRow,gaugeY+1)-(sectorRow+9,gaugeY+9),COL,BF
2598 RETURN

2600 REM ---- CONTEXT PANEL ----
2610 LINE (930,300)-(1262,630),8,BF
2620 LINE (930,300)-(1262,630),9,B
2630 IF consoleMode$="COMMAND" THEN GOSUB 2700
2640 IF consoleMode$="NAV" THEN GOSUB 2720
2650 IF consoleMode$="PHA" THEN GOSUB 2790
2660 IF consoleMode$="TOR" THEN GOSUB 2810
2670 IF consoleMode$="SHE" THEN GOSUB 2830
2680 IF consoleMode$="HELP" THEN GOSUB 2860
2682 IF consoleMode$="DAM" THEN GOSUB 2560
2690 RETURN

2700 LOCATE 21,117:PRINT "COMMAND PALETTE"
2710 LOCATE 22,117:PRINT "CLICK A BUTTON BELOW"
2715 LOCATE 23,117:PRINT "OR USE N S P T E D C ?"
2718 RETURN

2720 LOCATE 20,117:PRINT "COMPASS NAVIGATION"
2730 LOCATE 21,117:PRINT "COURSE";course;"  WARP";warpFactor
2740 compassCenterX=1096:compassCenterY=440:compassRadius=78:centerRadius=16:fullCircle=6.283185307:circleAspect=1
2741 CIRCLE (compassCenterX,compassCenterY),centerRadius,9,0,fullCircle,circleAspect
2742 CIRCLE (compassCenterX,compassCenterY),compassRadius,9,0,fullCircle,circleAspect
2743 baseAngle=16*3.14159265/180:baseCos=COS(baseAngle):baseSin=SIN(baseAngle)
2744 FOR tickAngle=0 TO 359 STEP 15
2745 radians=tickAngle*3.14159265/180
2746 tickInner=72:tickColor=9
2747 IF tickAngle MOD 45=0 THEN tickInner=64:tickColor=4
2748 LINE (compassCenterX+COS(radians)*tickInner,compassCenterY+SIN(radians)*tickInner)-(compassCenterX+COS(radians)*compassRadius,compassCenterY+SIN(radians)*compassRadius),tickColor
2749 NEXT tickAngle
2750 FOR I=1 TO 8
2751 armLength=54:armColor=5
2752 IF I MOD 2=1 THEN armLength=72:armColor=4
2753 IF I=course THEN armColor=1
2754 armTipX=compassCenterX+compassX(I)*armLength
2755 armTipY=compassCenterY+compassY(I)*armLength
2756 baseOneX=compassCenterX+(compassX(I)*baseCos-compassY(I)*baseSin)*centerRadius
2757 baseOneY=compassCenterY+(compassX(I)*baseSin+compassY(I)*baseCos)*centerRadius
2758 baseTwoX=compassCenterX+(compassX(I)*baseCos+compassY(I)*baseSin)*centerRadius
2759 baseTwoY=compassCenterY+(-compassX(I)*baseSin+compassY(I)*baseCos)*centerRadius
2760 LINE (baseOneX,baseOneY)-(baseTwoX,baseTwoY),armColor
2761 LINE (baseOneX,baseOneY)-(armTipX,armTipY),armColor
2762 LINE (baseTwoX,baseTwoY)-(armTipX,armTipY),armColor
2763 PAINT (compassCenterX+compassX(I)*(centerRadius+(armLength-centerRadius)*.45),compassCenterY+compassY(I)*(centerRadius+(armLength-centerRadius)*.45)),armColor,armColor
2764 NEXT I
2765 CIRCLE (compassCenterX,compassCenterY),centerRadius,9,0,fullCircle,circleAspect
2766 PAINT (compassCenterX,compassCenterY),0,9
2767 CIRCLE (compassCenterX,compassCenterY),centerRadius,4,0,fullCircle,circleAspect
2768 CIRCLE (compassCenterX,compassCenterY),8,1,0,fullCircle,circleAspect
2769 FOR I=1 TO 8
2770 armLength=54:targetColor=9
2771 IF I MOD 2=1 THEN armLength=72
2772 IF I=course THEN targetColor=1
2773 targetX=compassCenterX+compassX(I)*armLength
2774 targetY=compassCenterY+compassY(I)*armLength
2775 CIRCLE (targetX,targetY),8,targetColor,0,fullCircle,circleAspect
2776 LINE (targetX-3,targetY)-(targetX+3,targetY),7
2777 LINE (targetX,targetY-3)-(targetX,targetY+3),7
2778 NEXT I
2783 buttonX=944:buttonY=550:buttonWidth=58:buttonHeight=30:buttonLabel$="MINUS":buttonColor=4:GOSUB 7200
2784 buttonX=1010:buttonY=550:buttonWidth=58:buttonHeight=30:buttonLabel$="PLUS":buttonColor=4:GOSUB 7200
2785 buttonX=944:buttonY=590:buttonWidth=280:buttonHeight=34:buttonLabel$="ENGAGE":buttonColor=4:GOSUB 7200
2788 RETURN
2790 LOCATE 21,117:PRINT "PHASERS"
2800 LOCATE 22,117:PRINT "POWER";phaserPower
2802 buttonX=944:buttonY=400:buttonWidth=58:buttonHeight=30:buttonLabel$="MINUS":buttonColor=4:GOSUB 7200
2804 buttonX=1010:buttonY=400:buttonWidth=58:buttonHeight=30:buttonLabel$="PLUS":buttonColor=4:GOSUB 7200
2806 buttonX=944:buttonY=450:buttonWidth=280:buttonHeight=34:buttonLabel$="FIRE PHASERS":buttonColor=2:GOSUB 7200
2808 RETURN
2810 LOCATE 21,117:PRINT "TORPEDO"
2820 LOCATE 22,117:PRINT "TARGET";targetIndex
2822 buttonX=944:buttonY=400:buttonWidth=280:buttonHeight=34:buttonLabel$="FIRE TORPEDO":buttonColor=2:GOSUB 7200
2825 LOCATE 28,117:PRINT "CLICK A KLINGON TO LOCK"
2828 RETURN
2830 LOCATE 21,117:PRINT "SHIELDS"
2840 LOCATE 22,117:PRINT "TRANSFER";transferAmount
2842 buttonX=944:buttonY=400:buttonWidth=58:buttonHeight=30:buttonLabel$="MINUS":buttonColor=4:GOSUB 7200
2844 buttonX=1010:buttonY=400:buttonWidth=58:buttonHeight=30:buttonLabel$="PLUS":buttonColor=4:GOSUB 7200
2846 buttonX=944:buttonY=450:buttonWidth=280:buttonHeight=34:buttonLabel$="APPLY":buttonColor=4:GOSUB 7200
2847 buttonX=944:buttonY=486:buttonWidth=130:buttonHeight=30:buttonLabel$="TO SHIELDS":buttonColor=4:GOSUB 7200
2848 buttonX=1084:buttonY=486:buttonWidth=130:buttonHeight=30:buttonLabel$="TO ENERGY":buttonColor=4:GOSUB 7200
2849 LOCATE 34,117
2850 IF shieldDirection=1 THEN PRINT "TO SHIELDS"
2851 IF shieldDirection=-1 THEN PRINT "TO ENERGY"
2852 RETURN
2860 LOCATE 21,117:PRINT "HELP"
2870 LOCATE 22,117:PRINT "CLICK OR KEY TO CLOSE"
2880 RETURN
2890 REM ---- COMMAND BUTTONS ----
2900 LINE (16,648)-(1264,712),8,BF
2910 LINE (16,648)-(1264,712),4,B
2915 FOR I=0 TO 7
2920 buttonX=24+I*152:buttonY=656:buttonWidth=144:buttonHeight=40:buttonLabel$=commandLabel$(I):buttonColor=8
2925 IF consoleMode$="NAV" AND I=0 THEN buttonColor=1
2930 IF consoleMode$="PHA" AND I=2 THEN buttonColor=1
2935 IF consoleMode$="TOR" AND I=3 THEN buttonColor=1
2940 IF consoleMode$="SHE" AND I=4 THEN buttonColor=1
2942 IF consoleMode$="DAM" AND I=5 THEN buttonColor=1
2945 IF consoleMode$="HELP" AND I=7 THEN buttonColor=1
2950 GOSUB 7200
2955 NEXT I
2960 RETURN

5000 REM ---- DRAW EVERYTHING ----
5010 CLS
5020 LINE (0,0)-(1279,719),0,BF
5030 LINE (16,16)-(1264,56),8,BF
5040 LINE (16,16)-(1264,56),4,B
5050 LOCATE 2,3:PRINT "U.S.S. ENTERPRISE  NCC-1701"
5060 LOCATE 2,104:PRINT "STARDATE";stardate
5070 LOCATE 2,140:PRINT "TIME";timeLeft
5080 LOCATE 4,3:PRINT "CONDITION GREEN"
5090 IF galaxyMap(quadRow,quadCol)>=100 THEN LOCATE 4,3:PRINT "RED ALERT - KLINGONS IN QUADRANT"
5100 GOSUB 2000
5110 GOSUB 2200
5120 GOSUB 2400
5130 GOSUB 2600
5140 GOSUB 2890
5150 IF consoleMode$="HELP" THEN GOSUB 8000
5160 IF gameOver=1 THEN GOSUB 8500
5170 RETURN

3000 REM ---- MOUSE CLICK ----
3010 IF gameOver=1 THEN 3900
3020 IF consoleMode$="HELP" THEN consoleMode$="COMMAND":RETURN
3030 IF pointerY>=648 AND pointerY<=712 THEN 3100
3040 IF consoleMode$="NAV" THEN 3200
3050 IF consoleMode$="PHA" THEN 3300
3060 IF consoleMode$="TOR" THEN 3400
3070 IF consoleMode$="SHE" THEN 3500
3080 RETURN

3100 FOR I=0 TO 7
3110 buttonLeft=24+I*152
3120 IF pointerX>=buttonLeft AND pointerX<=buttonLeft+144 AND pointerY>=656 AND pointerY<=696 THEN GOSUB 3150:RETURN
3130 NEXT I
3140 RETURN

3150 IF I=0 THEN consoleMode$="NAV":course=currentHeading:warpFactor=1:statusMessage$="NAVIGATION READY"
3160 IF I=1 THEN consoleMode$="COMMAND":statusMessage$="SHORT RANGE SCAN COMPLETE"
3170 IF I=2 THEN consoleMode$="PHA":statusMessage$="PHASERS READY"
3180 IF I=3 THEN consoleMode$="TOR":statusMessage$="TORPEDO READY"
3190 IF I=4 THEN consoleMode$="SHE":shieldDirection=1:transferAmount=100:statusMessage$="SHIELD CONTROL"
3191 IF I=5 THEN consoleMode$="DAM":statusMessage$="DAMAGE CONTROL"
3192 IF I=6 THEN consoleMode$="COMMAND":statusMessage$="GALACTIC RECORD"
3193 IF I=7 THEN consoleMode$="HELP"
3194 RETURN

3200 REM NAV CONTEXT CLICKS
3210 IF pointerX>=56 AND pointerX<=456 AND pointerY>=104 AND pointerY<=504 THEN GOSUB 3600:RETURN
3220 compassCenterX=1096:compassCenterY=440
3221 FOR I=1 TO 8
3222 armLength=54
3223 IF I MOD 2=1 THEN armLength=72
3224 targetX=compassCenterX+compassX(I)*armLength
3225 targetY=compassCenterY+compassY(I)*armLength
3226 IF ABS(pointerX-targetX)<=16 AND ABS(pointerY-targetY)<=16 THEN course=I:statusMessage$="COURSE SET":RETURN
3227 NEXT I
3235 RETURN
3260 IF pointerX>=944 AND pointerX<=1002 AND pointerY>=550 AND pointerY<=580 THEN warpFactor=warpFactor-1:IF warpFactor<1 THEN warpFactor=1
3270 IF pointerX>=1010 AND pointerX<=1068 AND pointerY>=550 AND pointerY<=580 THEN warpFactor=warpFactor+1:IF warpFactor>8 THEN warpFactor=8
3280 IF pointerX>=944 AND pointerX<=1224 AND pointerY>=590 AND pointerY<=624 THEN GOSUB 6000:RETURN
3290 RETURN

3300 REM PHASER CONTEXT CLICKS
3310 IF pointerX>=944 AND pointerX<=1002 AND pointerY>=400 AND pointerY<=430 THEN phaserPower=phaserPower-100:IF phaserPower<100 THEN phaserPower=100
3320 IF pointerX>=1010 AND pointerX<=1068 AND pointerY>=400 AND pointerY<=430 THEN phaserPower=phaserPower+100:IF phaserPower>1000 THEN phaserPower=1000
3330 IF pointerX>=944 AND pointerX<=1224 AND pointerY>=450 AND pointerY<=484 THEN GOSUB 6900:RETURN
3340 RETURN

3400 REM TORPEDO CONTEXT CLICKS
3410 IF pointerX>=56 AND pointerX<=456 AND pointerY>=104 AND pointerY<=504 THEN GOSUB 3750:RETURN
3420 IF pointerX>=944 AND pointerX<=1224 AND pointerY>=400 AND pointerY<=434 THEN GOSUB 6200:RETURN
3430 RETURN

3500 REM SHIELD CONTEXT CLICKS
3510 IF pointerX>=944 AND pointerX<=1002 AND pointerY>=400 AND pointerY<=430 THEN transferAmount=transferAmount-100:IF transferAmount<100 THEN transferAmount=100
3520 IF pointerX>=1010 AND pointerX<=1068 AND pointerY>=400 AND pointerY<=430 THEN transferAmount=transferAmount+100:IF transferAmount>1000 THEN transferAmount=1000
3530 IF pointerX>=944 AND pointerX<=1224 AND pointerY>=450 AND pointerY<=484 THEN GOSUB 6400:RETURN
3532 IF pointerX>=944 AND pointerX<=1074 AND pointerY>=486 AND pointerY<=516 THEN shieldDirection=1:statusMessage$="ENERGY TO SHIELDS":RETURN
3534 IF pointerX>=1084 AND pointerX<=1214 AND pointerY>=486 AND pointerY<=516 THEN shieldDirection=-1:statusMessage$="SHIELDS TO ENERGY":RETURN
3540 RETURN

3600 REM COURSE FROM SCANNER CLICK
3610 gridCol=INT((pointerX-56)/50)+1:gridRow=INT((pointerY-104)/50)+1
3620 IF gridCol<1 OR gridCol>8 OR gridRow<1 OR gridRow>8 THEN RETURN
3630 rowDelta=gridRow-sectorRow:colDelta=gridCol-sectorCol
3640 IF ABS(rowDelta)>ABS(colDelta)*2 THEN 3680
3650 IF ABS(colDelta)>ABS(rowDelta)*2 THEN 3700
3660 IF rowDelta<0 AND colDelta>0 THEN course=2:RETURN
3662 IF rowDelta<0 AND colDelta<0 THEN course=4:RETURN
3664 IF rowDelta>0 AND colDelta>0 THEN course=8:RETURN
3666 IF rowDelta>0 AND colDelta<0 THEN course=6:RETURN
3670 RETURN
3680 IF rowDelta<0 THEN course=3 ELSE course=7
3690 statusMessage$="COURSE SET TO DESTINATION":RETURN
3700 IF colDelta>0 THEN course=1 ELSE course=5
3710 statusMessage$="COURSE SET TO DESTINATION":RETURN

3750 REM LOCK A KLINGON TARGET
3751 gridCol=INT((pointerX-56)/50)+1:gridRow=INT((pointerY-104)/50)+1
3752 IF gridCol<1 OR gridCol>8 OR gridRow<1 OR gridRow>8 THEN RETURN
3753 quadrantIndex=(quadRow-1)*8+quadCol:targetIndex=0
3754 FOR I=1 TO 3
3755 klingonIndex=(quadrantIndex-1)*3+I
3756 IF klingonHull(klingonIndex)>0 AND klingonRows(klingonIndex)=gridRow AND klingonCols(klingonIndex)=gridCol THEN targetIndex=klingonIndex
3757 NEXT I
3758 IF targetIndex=0 THEN statusMessage$="NO KLINGON AT THAT SECTOR" ELSE statusMessage$="TARGET LOCKED":rowDelta=klingonRows(targetIndex)-sectorRow:colDelta=klingonCols(targetIndex)-sectorCol:GOSUB 3800:currentHeading=course
3759 RETURN

3800 REM COURSE FROM RELATIVE ROW/COLUMN
3810 IF ABS(rowDelta)>ABS(colDelta)*2 THEN 3840
3820 IF ABS(colDelta)>ABS(rowDelta)*2 THEN 3860
3830 IF rowDelta<0 AND colDelta>0 THEN course=2:RETURN
3832 IF rowDelta<0 AND colDelta<0 THEN course=4:RETURN
3834 IF rowDelta>0 AND colDelta>0 THEN course=8:RETURN
3836 IF rowDelta>0 AND colDelta<0 THEN course=6:RETURN
3838 RETURN
3840 IF rowDelta<0 THEN course=3 ELSE course=7
3850 RETURN
3860 IF colDelta>0 THEN course=1 ELSE course=5
3870 RETURN

3900 REM GAME OVER CLICKS
3910 IF pointerX>=400 AND pointerX<=560 AND pointerY>=430 AND pointerY<=470 THEN GOSUB 1000:consoleMode$="COMMAND":statusMessage$="NEW MISSION":RETURN
3920 IF pointerX>=700 AND pointerX<=860 AND pointerY>=430 AND pointerY<=470 THEN END
3930 RETURN

4000 REM ---- KEYBOARD ----
4005 keyCode=ASC(keyPress$)
4010 IF keyCode=81 OR keyCode=113 THEN END
4020 IF gameOver=1 THEN 4180
4030 IF consoleMode$="HELP" THEN consoleMode$="COMMAND":RETURN
4040 IF keyCode=72 OR keyCode=104 OR keyCode=63 THEN consoleMode$="HELP":RETURN
4050 IF keyCode=78 OR keyCode=110 THEN consoleMode$="NAV":course=currentHeading:warpFactor=1:statusMessage$="NAVIGATION READY":RETURN
4060 IF keyCode=83 OR keyCode=115 THEN consoleMode$="COMMAND":statusMessage$="SHORT RANGE SCAN COMPLETE":RETURN
4070 IF keyCode=80 OR keyCode=112 THEN consoleMode$="PHA":statusMessage$="PHASERS READY":RETURN
4080 IF keyCode=84 OR keyCode=116 THEN consoleMode$="TOR":statusMessage$="TORPEDO READY":RETURN
4090 IF keyCode=69 OR keyCode=101 THEN consoleMode$="SHE":transferAmount=100:shieldDirection=1:statusMessage$="SHIELD CONTROL":RETURN
4100 IF keyCode=68 OR keyCode=100 THEN consoleMode$="DAM":statusMessage$="DAMAGE CONTROL":RETURN
4110 IF keyCode=67 OR keyCode=99 THEN consoleMode$="COMMAND":statusMessage$="GALACTIC RECORD":RETURN
4120 IF keyCode=27 THEN consoleMode$="COMMAND":RETURN
4130 IF consoleMode$="NAV" THEN 4200
4140 IF consoleMode$="PHA" THEN 4240
4150 IF consoleMode$="TOR" THEN 4270
4160 IF consoleMode$="SHE" THEN 4280
4170 RETURN
4180 IF keyCode=82 OR keyCode=114 THEN GOSUB 1000:consoleMode$="COMMAND":statusMessage$="NEW MISSION"
4185 RETURN
4200 IF keyCode>=49 AND keyCode<=56 THEN course=keyCode-48:statusMessage$="COURSE SET":RETURN
4210 IF keyCode=43 THEN warpFactor=warpFactor+1:IF warpFactor>8 THEN warpFactor=8
4220 IF keyCode=45 THEN warpFactor=warpFactor-1:IF warpFactor<1 THEN warpFactor=1
4230 IF keyCode=13 THEN GOSUB 6000
4235 RETURN
4240 IF keyCode=43 THEN phaserPower=phaserPower+100:IF phaserPower>1000 THEN phaserPower=1000
4250 IF keyCode=45 THEN phaserPower=phaserPower-100:IF phaserPower<100 THEN phaserPower=100
4260 IF keyCode=13 THEN GOSUB 6900
4265 RETURN
4270 IF keyCode=13 THEN GOSUB 6200
4275 RETURN
4280 IF keyCode=43 THEN transferAmount=transferAmount+100:IF transferAmount>1000 THEN transferAmount=1000
4290 IF keyCode=45 THEN transferAmount=transferAmount-100:IF transferAmount<100 THEN transferAmount=100
4300 IF keyCode=13 THEN GOSUB 6400
4305 RETURN

6000 REM ---- NAVIGATE ----
6010 IF warpFactor<1 THEN warpFactor=1
6020 IF warpFactor>8 THEN warpFactor=8
6030 energyCost=warpFactor*50
6040 IF shipEnergy<energyCost THEN statusMessage$="INSUFFICIENT ENERGY":RETURN
6050 shipEnergy=shipEnergy-energyCost
6060 FOR stepIndex=1 TO warpFactor
6070 nextRow=sectorRow+courseRows(course):nextCol=sectorCol+courseCols(course)
6080 IF nextRow<1 THEN quadRow=quadRow-1:nextRow=8
6090 IF nextRow>8 THEN quadRow=quadRow+1:nextRow=1
6100 IF nextCol<1 THEN quadCol=quadCol-1:nextCol=8
6110 IF nextCol>8 THEN quadCol=quadCol+1:nextCol=1
6120 IF quadRow<1 OR quadRow>8 OR quadCol<1 OR quadCol>8 THEN statusMessage$="GALACTIC PERIMETER":GOSUB 6800:RETURN
6130 sectorRow=nextRow:sectorCol=nextCol
6140 NEXT stepIndex
6150 currentHeading=course:targetIndex=0:timeLeft=timeLeft-1:statusMessage$="ENGINES ENGAGED"
6160 GOSUB 6700
6170 GOSUB 6500
6180 GOSUB 6800
6190 consoleMode$="COMMAND"
6195 RETURN

6900 REM ---- FIRE PHASERS ----
6905 quadrantIndex=(quadRow-1)*8+quadCol:localKlingons=0
6910 FOR I=1 TO 3:klingonIndex=(quadrantIndex-1)*3+I:IF klingonHull(klingonIndex)>0 THEN localKlingons=localKlingons+1
6915 NEXT I
6920 IF localKlingons=0 THEN statusMessage$="NO KLINGONS IN RANGE":RETURN
6925 IF phaserPower>shipEnergy THEN phaserPower=shipEnergy
6930 IF phaserPower<1 THEN statusMessage$="NO ENERGY FOR PHASERS":RETURN
6935 shipEnergy=shipEnergy-phaserPower
6940 FOR I=1 TO 3
6945 klingonIndex=(quadrantIndex-1)*3+I
6950 IF klingonHull(klingonIndex)>0 THEN GOSUB 6970
6955 NEXT I
6960 statusMessage$="PHASERS FIRED":timeLeft=timeLeft-1
6962 GOSUB 6500:GOSUB 6800:consoleMode$="COMMAND"
6965 RETURN
6970 REM PHASER HIT
6975 contactRange=SQR((klingonRows(klingonIndex)-sectorRow)^2+(klingonCols(klingonIndex)-sectorCol)^2)
6980 IF contactRange<.1 THEN contactRange=.1
6982 damageHit=INT(phaserPower/(localKlingons*(contactRange/3+1)))
6985 klingonHull(klingonIndex)=klingonHull(klingonIndex)-damageHit
6988 IF klingonHull(klingonIndex)<=0 THEN klingonHull(klingonIndex)=0:klingonsLeft=klingonsLeft-1:galaxyMap(quadRow,quadCol)=galaxyMap(quadRow,quadCol)-100:IF targetIndex=klingonIndex THEN targetIndex=0
6990 RETURN

6200 REM ---- FIRE TORPEDO ----
6210 IF torpedoCount<1 THEN statusMessage$="NO TORPEDOES LEFT":RETURN
6220 torpedoCount=torpedoCount-1
6230 torpedoRow=sectorRow:torpedoCol=sectorCol:damageHit=0
6240 FOR stepIndex=1 TO 8
6250 torpedoRow=torpedoRow+courseRows(currentHeading):torpedoCol=torpedoCol+courseCols(currentHeading)
6260 IF torpedoRow<1 OR torpedoRow>8 OR torpedoCol<1 OR torpedoCol>8 THEN stepIndex=8:GOTO 6320
6270 quadrantIndex=(quadRow-1)*8+quadCol
6280 FOR I=1 TO 3
6290 klingonIndex=(quadrantIndex-1)*3+I
6300 IF klingonHull(klingonIndex)>0 AND klingonRows(klingonIndex)=torpedoRow AND klingonCols(klingonIndex)=torpedoCol THEN klingonHull(klingonIndex)=0:klingonsLeft=klingonsLeft-1:galaxyMap(quadRow,quadCol)=galaxyMap(quadRow,quadCol)-100:damageHit=1:IF targetIndex=klingonIndex THEN targetIndex=0
6310 NEXT I
6315 IF damageHit=1 THEN stepIndex=8
6320 NEXT stepIndex
6330 IF damageHit=1 THEN statusMessage$="TORPEDO HIT" ELSE statusMessage$="TORPEDO MISSED"
6340 timeLeft=timeLeft-1:GOSUB 6500:GOSUB 6800:consoleMode$="COMMAND"
6350 RETURN

6400 REM ---- SHIELD TRANSFER ----
6405 IF shieldDirection=1 THEN transferQty=transferAmount:IF transferQty>shipEnergy THEN transferQty=shipEnergy:shipEnergy=shipEnergy-transferQty:shieldPower=shieldPower+transferQty:statusMessage$="SHIELDS RAISED"
6410 IF shieldDirection=-1 THEN transferQty=transferAmount:IF transferQty>shieldPower THEN transferQty=shieldPower:shieldPower=shieldPower-transferQty:shipEnergy=shipEnergy+transferQty:statusMessage$="SHIELDS LOWERED"
6415 timeLeft=timeLeft-1:GOSUB 6500:GOSUB 6800:consoleMode$="COMMAND"
6420 RETURN

6500 REM ---- KLINGON ATTACK ----
6510 quadrantIndex=(quadRow-1)*8+quadCol:localKlingons=0
6520 FOR I=1 TO 3:klingonIndex=(quadrantIndex-1)*3+I:IF klingonHull(klingonIndex)>0 THEN localKlingons=localKlingons+1
6530 NEXT I
6540 IF localKlingons=0 THEN RETURN
6550 damageHit=INT(localKlingons*(20+RND*30))
6555 damagedSystem=INT(RND*8)+1:systemHealth(damagedSystem)=systemHealth(damagedSystem)-INT(10+RND*20):IF systemHealth(damagedSystem)<-100 THEN systemHealth(damagedSystem)=-100
6560 IF shieldPower>=damageHit THEN shieldPower=shieldPower-damageHit:statusMessage$="SHIELDS ABSORBED THE HIT"
6570 IF shieldPower<damageHit THEN hullIntegrity=hullIntegrity-(damageHit-shieldPower):shieldPower=0:statusMessage$="HULL BREACH"
6575 statusMessage$="DAMAGE: "+systemName$(damagedSystem)+" HIT"
6580 IF hullIntegrity<0 THEN hullIntegrity=0
6590 RETURN

6600 REM ---- (RESERVED) ----
6610 RETURN

6700 REM ---- STARBASE RESUPPLY ----
6710 IF baseRows(quadRow,quadCol)>0 AND baseCols(quadRow,quadCol)>0 AND baseRows(quadRow,quadCol)=sectorRow AND baseCols(quadRow,quadCol)=sectorCol THEN shipEnergy=3000:torpedoCount=10:shieldPower=0:hullIntegrity=100:statusMessage$="DOCKED - RESUPPLIED"
6720 RETURN

6800 REM ---- MISSION CHECK ----
6810 IF klingonsLeft<1 THEN gameOver=1:missionWon=1:statusMessage$="MISSION ACCOMPLISHED"
6820 IF timeLeft<1 THEN gameOver=1:missionWon=0:statusMessage$="TIME EXPIRED"
6830 IF hullIntegrity<1 THEN gameOver=1:missionWon=0:statusMessage$="ENTERPRISE DESTROYED"
6840 RETURN

7000 REM ---- VALUE BAR ----
7010 IF barValue<0 THEN barValue=0
7020 IF barValue>barMax THEN barValue=barMax
7030 LINE (barLeft,barTop)-(barLeft+barWidth,barTop+barHeight),4,B
7040 fillWidth=INT((barWidth-2)*barValue/barMax)
7050 IF fillWidth>0 THEN LINE (barLeft+1,barTop+1)-(barLeft+fillWidth,barTop+barHeight-1),barColor,BF
7060 RETURN

7200 REM ---- BUTTON ----
7210 LINE (buttonX,buttonY)-(buttonX+buttonWidth,buttonY+buttonHeight),buttonColor,BF
7220 LINE (buttonX,buttonY)-(buttonX+buttonWidth,buttonY+buttonHeight),4,B
7230 LOCATE INT(buttonY/16)+2,INT(buttonX/8)+2:PRINT buttonLabel$;
7240 RETURN

8000 REM ---- HELP OVERLAY ----
8010 CLS:LINE (260,150)-(1020,570),0,BF
8020 LINE (260,150)-(1020,570),4,B
8030 LOCATE 12,38:PRINT "STAR TREK NG - HELP"
8040 LOCATE 15,30:PRINT "MOUSE: CLICK A COMMAND BUTTON ALONG THE BOTTOM"
8050 LOCATE 17,30:PRINT "NAV: PICK A DIRECTION OR CLICK THE SCANNER, THEN ENGAGE"
8060 LOCATE 19,30:PRINT "PHASERS: SET POWER, THEN FIRE"
8070 LOCATE 21,30:PRINT "TORPEDO: LOCK A KLINGON ON THE SCANNER, THEN FIRE"
8080 LOCATE 23,30:PRINT "SHIELDS: SET TRANSFER, THEN APPLY"
8090 LOCATE 25,30:PRINT "KEYS: N S P T E D C ?   Q QUITS"
8100 LOCATE 29,36:PRINT "CLICK OR PRESS A KEY TO CLOSE"
8110 RETURN

8500 REM ---- GAME OVER OVERLAY ----
8510 CLS:LINE (320,210)-(960,520),0,BF
8520 LINE (320,210)-(960,520),2,B
8530 LOCATE 15,44:PRINT "MISSION ENDED"
8540 IF missionWon=1 THEN LOCATE 18,38:PRINT "THE KLINGON FORCE HAS BEEN DEFEATED"
8550 IF missionWon=0 THEN LOCATE 18,38:PRINT "THE ENTERPRISE HAS BEEN LOST"
8560 LOCATE 21,40:PRINT "STARDATE";stardate
8570 LOCATE 23,36:PRINT "ENERGY";INT(shipEnergy);"  HULL";INT(hullIntegrity);"%"
8580 LOCATE 27,38:PRINT "R = RESTART    Q = QUIT"
8590 buttonX=400:buttonY=430:buttonWidth=160:buttonHeight=40:buttonLabel$="RESTART":buttonColor=1:GOSUB 7200
8595 buttonX=700:buttonY=430:buttonWidth=160:buttonHeight=40:buttonLabel$="QUIT":buttonColor=2:GOSUB 7200
8598 RETURN
