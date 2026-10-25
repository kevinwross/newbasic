10 REM baslib complete host sample: one of every control, mouse + keyboard loop.
20 REM Host lines stay below 500000. Merge baslib.bas before RUN:
30 REM   LOAD "baslib/samplehost.bas" : MERGE "baslib/baslib.bas" : RUN
40 REM No baslib source is included here; only public blApi... calls and bl constants.
50 CONST ActButton% = 100
60 CONST ActSlider% = 150
70 CONST ActDial% = 160
80 CONST ActTarget% = 200
90 ButtonCount% = 0 : TargetCount% = 0 : ClockFlag% = 0
100 SCREEN 720
110 CLS
120 ON TIMER(1) GOSUB 6010
130 blReqW% = 0 : blReqH% = 0
140 GOSUB blApiInit
150 IF blStatus% <> blStatusOk% THEN GOTO 9000
200 GOSUB ResetReq
210 blReqClass% = blClassPanel% : blReqParent% = blRoot%
220 blReqX% = 40 : blReqY% = 24 : blReqW% = 640 : blReqH% = 480
230 blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
240 GOSUB blApiCreate : MainH% = blRetHandle%
250 IF blStatus% <> blStatusOk% THEN GOTO 9000
300 GOSUB ResetReq
310 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindLabel%
320 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 24
330 blReqText$ = "baslib sample host - one of every control" : blReqPad% = 4
340 GOSUB blApiCreate : HeaderH% = blRetHandle%
350 IF blStatus% <> blStatusOk% THEN GOTO 9000
400 GOSUB ResetReq
410 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindLabel%
420 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
430 blReqText$ = "Clock: " + TIME$ : blReqPad% = 4
440 GOSUB blApiCreate : ClockH% = blRetHandle%
450 IF blStatus% <> blStatusOk% THEN GOTO 9000
500 GOSUB ResetReq
510 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindLabel%
520 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 60
530 blReqText$ = "Button clicks: 0" + CHR$(13) + "Target hits: 0" + CHR$(13) + "Slider: 25   Dial: 35" : blReqPad% = 4
540 GOSUB blApiCreate : StatusH% = blRetHandle%
550 IF blStatus% <> blStatusOk% THEN GOTO 9000
700 GOSUB ResetReq
710 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindButton%
720 blReqX% = 0 : blReqY% = 0 : blReqW% = 180 : blReqH% = 28
730 blReqText$ = "Click me" : blReqAction% = ActButton% : blReqPad% = 6
740 GOSUB blApiCreate : ButtonH% = blRetHandle%
750 IF blStatus% <> blStatusOk% THEN GOTO 9000
800 GOSUB ResetReq
810 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindSlider%
820 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
830 blReqValue# = 25 : blReqAction% = ActSlider% : blReqPad% = 6
840 GOSUB blApiCreate : SliderH% = blRetHandle%
850 IF blStatus% <> blStatusOk% THEN GOTO 9000
900 GOSUB ResetReq
910 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindGauge%
920 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 28
930 blReqValue# = 60 : blReqPad% = 6
940 GOSUB blApiCreate : GaugeH% = blRetHandle%
950 IF blStatus% <> blStatusOk% THEN GOTO 9000
1000 GOSUB ResetReq
1010 blReqClass% = blClassUser% : blReqParent% = MainH% : blReqKind% = blKindDial%
1020 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 96
1030 blReqValue# = 35 : blReqAction% = ActDial% : blReqPad% = 6
1040 GOSUB blApiCreate : DialH% = blRetHandle%
1050 IF blStatus% <> blStatusOk% THEN GOTO 9000
1100 GOSUB ResetReq
1110 blReqClass% = blClassClickTarget% : blReqParent% = MainH%
1120 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 40
1130 blReqAction% = ActTarget% : blReqButtonMask% = 1
1140 GOSUB blApiCreate : TargetH% = blRetHandle%
1150 IF blStatus% <> blStatusOk% THEN GOTO 9000
1200 GOSUB blApiPaintAll
1210 IF blStatus% <> blStatusOk% THEN GOTO 9000
1220 blReqActionId% = ActButton% : blReqLine& = 2010 : GOSUB blApiSetActionLine
1230 IF blStatus% <> blStatusOk% THEN GOTO 9000
1240 blReqActionId% = ActSlider% : blReqLine& = 2210 : GOSUB blApiSetActionLine
1250 IF blStatus% <> blStatusOk% THEN GOTO 9000
1260 blReqActionId% = ActDial% : blReqLine& = 2310 : GOSUB blApiSetActionLine
1270 IF blStatus% <> blStatusOk% THEN GOTO 9000
1280 blReqActionId% = ActTarget% : blReqLine& = 2110 : GOSUB blApiSetActionLine
1290 IF blStatus% <> blStatusOk% THEN GOTO 9000
1295 TIMER ON
1298 GOTO MainLoop
1300 LABEL MainLoop
1310 IF ClockFlag% <> 0 THEN ClockFlag% = 0 : GOSUB UpdateClock
1320 GOSUB blApiPollInput
1330 WHILE blEventCount% > 0
1340   GOSUB blApiNextEvent
1350   GOSUB DispatchAction
1360 WEND
1370 K$ = INKEY$
1380 IF K$ <> "" THEN GOSUB HandleKey
1390 IF blNeedsPaint% <> 0 THEN GOSUB blApiPaintAll
1395 SLEEP 16
1398 GOTO MainLoop
2000 REM action 100: button click
2010 ButtonCount% = ButtonCount% + 1
2020 GOSUB RefreshStatus
2030 RETURN
2100 REM action 200: invisible click target
2110 TargetCount% = TargetCount% + 1
2120 GOSUB RefreshStatus
2130 RETURN
2200 REM action 150: slider value changed
2210 GOSUB RefreshStatus
2220 RETURN
2300 REM action 160: dial value changed
2310 GOSUB RefreshStatus
2320 RETURN
6000 REM 1-second timer tick: set a flag and return. Do not call baslib here.
6010 ClockFlag% = 1
6020 RETURN
7000 LABEL ResetReq
7010 blReqClass% = 0 : blReqParent% = 0 : blReqHandle% = 0
7020 blReqX% = 0 : blReqY% = 0 : blReqW% = 0 : blReqH% = 0
7030 blReqFlags% = 0 : blReqAction% = 0 : blReqTag% = 0
7040 blReqKind% = 0 : blReqMode% = 0 : blReqPad% = 0 : blReqGap% = 0 : blReqCols% = 0
7050 blReqButtonMask% = 1 : blReqText$ = "" : blReqValue# = 0 : blReqEnabled% = 1
7060 RETURN
7100 LABEL RefreshStatus
7110 blReqHandle% = SliderH% : GOSUB blApiGetValue : SliderV# = blRetValue#
7120 blReqHandle% = DialH% : GOSUB blApiGetValue : DialV# = blRetValue#
7130 blReqHandle% = StatusH%
7140 StatusText$ = "Button clicks: " + MID$(STR$(ButtonCount%),2)
7142 StatusText$ = StatusText$ + CHR$(13) + "Target hits: " + MID$(STR$(TargetCount%),2)
7144 StatusText$ = StatusText$ + CHR$(13) + "Slider: " + MID$(STR$(INT(SliderV#)),2)
7146 StatusText$ = StatusText$ + "   Dial: " + MID$(STR$(INT(DialV#)),2)
7148 blReqText$ = StatusText$
7150 GOSUB blApiSetText
7160 GOSUB blApiPaintAll
7170 RETURN
7200 LABEL UpdateClock
7210 blReqHandle% = ClockH%
7220 blReqText$ = "Clock: " + TIME$
7230 GOSUB blApiSetText
7240 GOSUB blApiPaintAll
7250 RETURN
7300 LABEL DispatchAction
7310 IF blEventAction% = blActionNone% THEN RETURN
7320 ActLine& = blActionLine&(blEventAction%)
7330 IF ActLine& = blNoLine& THEN RETURN
7340 GOSUB ActLine&
7350 RETURN
7400 LABEL HandleKey
7410 IF K$ = CHR$(27) THEN TIMER OFF : END
7420 IF K$ = "R" OR K$ = "r" THEN ButtonCount% = 0 : TargetCount% = 0 : GOSUB RefreshStatus
7430 IF K$ = "+" OR K$ = "=" THEN GOSUB NudgeUp
7440 IF K$ = "-" OR K$ = "_" THEN GOSUB NudgeDown
7450 IF K$ = "C" OR K$ = "c" THEN GOSUB UpdateClock
7460 RETURN
7500 LABEL NudgeUp
7510 blReqHandle% = SliderH% : GOSUB blApiGetValue
7520 NewV# = blRetValue# + 5
7530 IF NewV# > 100 THEN NewV# = 100
7540 blReqValue# = NewV# : GOSUB blApiSetValue
7550 GOSUB RefreshStatus
7560 RETURN
7600 LABEL NudgeDown
7610 blReqHandle% = SliderH% : GOSUB blApiGetValue
7620 NewV# = blRetValue# - 5
7630 IF NewV# < 0 THEN NewV# = 0
7640 blReqValue# = NewV# : GOSUB blApiSetValue
7650 GOSUB RefreshStatus
7660 RETURN
9000 LABEL Fail
9010 TIMER OFF
9020 PRINT "baslib sample host failed; status ="; blStatus%
9030 END
9040 END
