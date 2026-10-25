10 REM baslib control gallery: panel, button, label, click target, slider, gauge, dial.
20 REM Host lines stay below 500000; merge baslib.bas before RUN.
30 SCREEN 720
40 CALL blInit(0, 0)
50 IF blStatus% <> blStatusOk% THEN GOTO 9000
60 REM --- panel ---
70 PanelH% = blCreate%(1, 2, 0, 40, 30, 640, 520, "", 0, 0, 0)
80 CALL blSetLayout(PanelH%, 1, 12, 8)
90 IF blStatus% <> blStatusOk% THEN GOTO 9000
100 REM --- header ---
110 HeaderH% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 24, "baslib control gallery", 0, 0, 0)
120 blUser#(HeaderH%,13) = 4
130 REM --- button ---
140 ButtonH% = blCreate%(PanelH%, 3, 1, 0, 0, 100, 32, "Click me", 100, 0, 0)
150 blUser#(ButtonH%,13) = 6
160 REM --- slider ---
170 SlideLbl% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 20, "Slider - click at a position", 0, 0, 0)
180 blUser#(SlideLbl%,13) = 2
190 SliderH% = blCreate%(PanelH%, 3, 2, 0, 0, 100, 28, "", 150, 0, 0)
200 CALL blSetValue(SliderH%, 25)
210 REM --- gauge ---
220 GaugeLbl% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 20, "Gauge - read only", 0, 0, 0)
230 blUser#(GaugeLbl%,13) = 2
240 GaugeH% = blCreate%(PanelH%, 3, 3, 0, 0, 100, 28, "", 0, 0, 0)
250 CALL blSetValue(GaugeH%, 60)
260 REM --- dial ---
270 DialLbl% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 20, "Dial - click at a position", 0, 0, 0)
280 blUser#(DialLbl%,13) = 2
290 DialH% = blCreate%(PanelH%, 3, 4, 0, 0, 100, 96, "", 160, 0, 0)
300 CALL blSetValue(DialH%, 35)
310 REM --- readout ---
320 ReadoutH% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 76, "Slider 25" + CHR$(13) + "Gauge 60" + CHR$(13) + "Dial 35", 0, 0, 0)
330 blUser#(ReadoutH%,13) = 6
340 REM --- invisible click target ---
350 TargetH% = blCreate%(PanelH%, 4, 0, 0, 0, 100, 48, "", 200, 0, 0)
360 CALL blSetClickMask(TargetH%, 1)
370 CALL blPaintAll
380 REM --- action-line table ---
390 CALL blSetActionLine(100, 1010)
400 CALL blSetActionLine(200, 2010)
410 CALL blSetActionLine(150, 3010)
420 CALL blSetActionLine(160, 4010)
430 REM --- main loop ---
440 ClickH% = blPollInput%()
450 WHILE blEventCount%() > 0
460   EventTyp% = blNextEvent%()
470   ActionId% = blEventAction%()
480   IF ActionId% <> blActionNone% THEN GOSUB blActionLine&(ActionId%)
490 WEND
500 KeyChar$ = INKEY$ : IF KeyChar$ = CHR$(27) THEN END
510 CALL blUpdateAll
520 SLEEP 16
530 GOTO 440
1000 REM --- button action ---
1010 CALL blSetText(ReadoutH%, "Button clicked" + CHR$(13) + "at " + TIME$)
1020 CALL blUpdateAll
1030 RETURN
2000 REM --- click-target action ---
2010 CALL blSetText(ReadoutH%, "Target clicked" + CHR$(13) + "at " + TIME$)
2020 CALL blUpdateAll
2030 RETURN
3000 REM --- slider action ---
3010 GOSUB 8000
3020 RETURN
4000 REM --- dial action ---
4010 GOSUB 8000
4020 RETURN
8000 REM --- refresh the value readout ---
8010 SlideVal# = blGetValue#(SliderH%)
8020 GaugeVal# = blGetValue#(GaugeH%)
8030 DialVal# = blGetValue#(DialH%)
8040 CALL blSetText(ReadoutH%, "Slider " + STR$(INT(SlideVal#)) + CHR$(13) + "Gauge " + STR$(INT(GaugeVal#)) + CHR$(13) + "Dial " + STR$(INT(DialVal#)))
8050 CALL blUpdateAll
8060 RETURN
9000 PRINT "Control gallery failed at status"; blStatus% : END
