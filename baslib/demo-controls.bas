10 REM baslib control gallery: panel, button, label, click target, slider, gauge, dial.
20 REM Host lines stay below 500000; merge baslib.bas before RUN.
30 SCREEN 720
40 blReqW% = 0 : blReqH% = 0
50 GOSUB blApiInit
60 IF blStatus% <> blStatusOk% THEN GOTO 9000
70 REM --- panel ---
80 blReqClass% = blClassPanel% : blReqParent% = blRoot%
90 blReqX% = 40 : blReqY% = 30 : blReqW% = 640 : blReqH% = 520
100 blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
110 GOSUB blApiCreate : blPanel% = blRetHandle%
120 IF blStatus% <> blStatusOk% THEN GOTO 9000
130 REM --- header ---
140 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindLabel%
150 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 24
160 blReqText$ = "baslib control gallery" : blReqPad% = 4 : blReqAction% = 0
170 GOSUB blApiCreate : blHeader% = blRetHandle%
180 IF blStatus% <> blStatusOk% THEN GOTO 9000
190 REM --- button ---
200 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindButton%
210 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 32
220 blReqText$ = "Click me" : blReqPad% = 6 : blReqAction% = 100
230 GOSUB blApiCreate : blButton% = blRetHandle%
240 IF blStatus% <> blStatusOk% THEN GOTO 9000
250 REM --- slider ---
260 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindLabel%
270 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 20
280 blReqText$ = "Slider - click at a position" : blReqPad% = 2 : blReqAction% = 0
290 GOSUB blApiCreate : IF blStatus% <> blStatusOk% THEN GOTO 9000
300 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindSlider%
310 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
320 blReqValue# = 25 : blReqPad% = 6 : blReqAction% = 150
330 GOSUB blApiCreate : blSlider% = blRetHandle%
340 IF blStatus% <> blStatusOk% THEN GOTO 9000
350 REM --- gauge ---
360 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindLabel%
370 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 20
380 blReqText$ = "Gauge - read only" : blReqPad% = 2 : blReqAction% = 0
390 GOSUB blApiCreate : IF blStatus% <> blStatusOk% THEN GOTO 9000
400 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindGauge%
410 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
420 blReqValue# = 60 : blReqPad% = 6 : blReqAction% = 0
430 GOSUB blApiCreate : blGauge% = blRetHandle%
440 IF blStatus% <> blStatusOk% THEN GOTO 9000
450 REM --- dial ---
460 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindLabel%
470 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 20
480 blReqText$ = "Dial - click at a position" : blReqPad% = 2 : blReqAction% = 0
490 GOSUB blApiCreate : IF blStatus% <> blStatusOk% THEN GOTO 9000
500 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindDial%
510 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 96
520 blReqValue# = 35 : blReqPad% = 6 : blReqAction% = 160
530 GOSUB blApiCreate : blDial% = blRetHandle%
540 IF blStatus% <> blStatusOk% THEN GOTO 9000
550 REM --- readout ---
560 blReqClass% = blClassUser% : blReqParent% = blPanel% : blReqKind% = blKindLabel%
570 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 76
580 blReqText$ = "Slider 25" + CHR$(13) + "Gauge 60" + CHR$(13) + "Dial 35"
590 blReqPad% = 6 : blReqAction% = 0
600 GOSUB blApiCreate : blReadout% = blRetHandle%
610 IF blStatus% <> blStatusOk% THEN GOTO 9000
620 REM --- invisible click target ---
630 blReqClass% = blClassClickTarget% : blReqParent% = blPanel%
640 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 48
650 blReqText$ = "" : blReqAction% = 200 : blReqButtonMask% = 1
660 GOSUB blApiCreate : blTarget% = blRetHandle%
670 IF blStatus% <> blStatusOk% THEN GOTO 9000
680 GOSUB blApiPaintAll
690 REM --- action-line table ---
700 blReqActionId% = 100 : blReqLine& = 1010 : GOSUB blApiSetActionLine
710 blReqActionId% = 200 : blReqLine& = 2010 : GOSUB blApiSetActionLine
720 blReqActionId% = 150 : blReqLine& = 3010 : GOSUB blApiSetActionLine
730 blReqActionId% = 160 : blReqLine& = 4010 : GOSUB blApiSetActionLine
740 REM --- main loop ---
750 GOSUB blApiPollInput
760 WHILE blEventCount% > 0
770   GOSUB blApiNextEvent
780   IF blEventAction% <> blActionNone% THEN GOSUB blActionLine&(blEventAction%)
790 WEND
800 K$ = INKEY$ : IF K$ = CHR$(27) THEN END
810 IF blNeedsPaint% THEN GOSUB blApiPaintAll
820 SLEEP 16
830 GOTO 750
1000 REM --- button action ---
1010 blReqHandle% = blReadout%
1020 blReqText$ = "Button clicked" + CHR$(13) + "at " + TIME$
1030 GOSUB blApiSetText
1040 GOSUB blApiPaintAll
1050 RETURN
2000 REM --- click-target action ---
2010 blReqHandle% = blReadout%
2020 blReqText$ = "Target clicked" + CHR$(13) + "at " + TIME$
2030 GOSUB blApiSetText
2040 GOSUB blApiPaintAll
2050 RETURN
3000 REM --- slider action ---
3010 blReqHandle% = blSlider% : GOSUB blApiGetValue : blSliderV# = blRetValue#
3020 GOSUB 8000
3030 RETURN
4000 REM --- dial action ---
4010 blReqHandle% = blDial% : GOSUB blApiGetValue : blDialV# = blRetValue#
4020 GOSUB 8000
4030 RETURN
8000 REM --- refresh the value readout ---
8010 blReqHandle% = blSlider% : GOSUB blApiGetValue : blSliderV# = blRetValue#
8020 blReqHandle% = blGauge% : GOSUB blApiGetValue : blGaugeV# = blRetValue#
8030 blReqHandle% = blDial% : GOSUB blApiGetValue : blDialV# = blRetValue#
8040 blReqHandle% = blReadout%
8050 blReqText$ = "Slider " + STR$(INT(blSliderV#)) + CHR$(13) + "Gauge " + STR$(INT(blGaugeV#)) + CHR$(13) + "Dial " + STR$(INT(blDialV#))
8060 GOSUB blApiSetText
8070 GOSUB blApiPaintAll
8080 RETURN
9000 PRINT "Control gallery failed at status"; blStatus% : END
