10 REM baslib control gallery: panel, button, label, click target, slider, gauge, dial.
20 REM Host lines stay below 500000; merge baslib.bas before RUN.
30 SCREEN 720
40 CALL blInit(0, 0)
50 IF blStatus% <> blStatusOk% THEN GOTO 9000
60 REM --- panel ---
70 P% = blCreate%(1, 2, 0, 40, 30, 640, 520, "", 0, 0, 0)
80 CALL blSetLayout(P%, 1, 12, 8)
90 IF blStatus% <> blStatusOk% THEN GOTO 9000
100 REM --- header ---
110 H% = blCreate%(P%, 3, 5, 0, 0, 100, 24, "baslib control gallery", 0, 0, 0)
120 blUser#(H%,13) = 4
130 REM --- button ---
140 B% = blCreate%(P%, 3, 1, 0, 0, 100, 32, "Click me", 100, 0, 0)
150 blUser#(B%,13) = 6
160 REM --- slider ---
170 S1% = blCreate%(P%, 3, 5, 0, 0, 100, 20, "Slider - click at a position", 0, 0, 0)
180 blUser#(S1%,13) = 2
190 S% = blCreate%(P%, 3, 2, 0, 0, 100, 28, "", 150, 0, 0)
200 CALL blSetValue(S%, 25)
210 REM --- gauge ---
220 G1% = blCreate%(P%, 3, 5, 0, 0, 100, 20, "Gauge - read only", 0, 0, 0)
230 blUser#(G1%,13) = 2
240 G% = blCreate%(P%, 3, 3, 0, 0, 100, 28, "", 0, 0, 0)
250 CALL blSetValue(G%, 60)
260 REM --- dial ---
270 D1% = blCreate%(P%, 3, 5, 0, 0, 100, 20, "Dial - click at a position", 0, 0, 0)
280 blUser#(D1%,13) = 2
290 D% = blCreate%(P%, 3, 4, 0, 0, 100, 96, "", 160, 0, 0)
300 CALL blSetValue(D%, 35)
310 REM --- readout ---
320 R% = blCreate%(P%, 3, 5, 0, 0, 100, 76, "Slider 25" + CHR$(13) + "Gauge 60" + CHR$(13) + "Dial 35", 0, 0, 0)
330 blUser#(R%,13) = 6
340 REM --- invisible click target ---
350 T% = blCreate%(P%, 4, 0, 0, 0, 100, 48, "", 200, 0, 0)
360 CALL blSetClickMask(T%, 1)
370 CALL blPaintAll
380 REM --- action-line table ---
390 CALL blSetActionLine(100, 1010)
400 CALL blSetActionLine(200, 2010)
410 CALL blSetActionLine(150, 3010)
420 CALL blSetActionLine(160, 4010)
430 REM --- main loop ---
440 C% = blPollInput%()
450 WHILE blEventCount%() > 0
460   E% = blNextEvent%()
470   A% = blEventAction%()
480   IF A% <> blActionNone% THEN GOSUB blActionLine&(A%)
490 WEND
500 K$ = INKEY$ : IF K$ = CHR$(27) THEN END
510 CALL blPaintAll
520 SLEEP 16
530 GOTO 440
1000 REM --- button action ---
1010 CALL blSetText(R%, "Button clicked" + CHR$(13) + "at " + TIME$)
1020 CALL blPaintAll
1030 RETURN
2000 REM --- click-target action ---
2010 CALL blSetText(R%, "Target clicked" + CHR$(13) + "at " + TIME$)
2020 CALL blPaintAll
2030 RETURN
3000 REM --- slider action ---
3010 GOSUB 8000
3020 RETURN
4000 REM --- dial action ---
4010 GOSUB 8000
4020 RETURN
8000 REM --- refresh the value readout ---
8010 SV# = blGetValue#(S%)
8020 GV# = blGetValue#(G%)
8030 DV# = blGetValue#(D%)
8040 CALL blSetText(R%, "Slider " + STR$(INT(SV#)) + CHR$(13) + "Gauge " + STR$(INT(GV#)) + CHR$(13) + "Dial " + STR$(INT(DV#)))
8050 CALL blPaintAll
8060 RETURN
9000 PRINT "Control gallery failed at status"; blStatus% : END
