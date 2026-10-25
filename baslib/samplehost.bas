10 REM baslib complete host sample: one of every control, mouse + keyboard loop.
20 REM Host lines stay below 500000. Merge baslib.bas before RUN:
30 REM   LOAD "baslib/samplehost.bas" : MERGE "baslib/baslib.bas" : RUN
40 REM Public calls are parameterized SUB/FUNCTION calls.
50 CONST ActButton% = 100
60 CONST ActSlider% = 150
70 CONST ActDial% = 160
80 CONST ActTarget% = 200
90 ButtonCount% = 0 : TargetCount% = 0 : ClockFlag% = 0
100 SCREEN 720
110 CLS
120 ON TIMER(1) GOSUB 6010
130 CALL blInit(0, 0)
140 IF blStatus% <> blStatusOk% THEN GOTO 9000
150 MainH% = blCreate%(1, 2, 0, 40, 24, 640, 480, "", 0, 0, 0)
160 CALL blSetLayout(MainH%, 1, 12, 8)
170 IF blStatus% <> blStatusOk% THEN GOTO 9000
180 HeaderH% = blCreate%(MainH%, 3, 5, 0, 0, 100, 24, "baslib sample host - one of every control", 0, 0, 0)
190 blUser#(HeaderH%,13) = 4
200 ClockH% = blCreate%(MainH%, 3, 5, 0, 0, 100, 28, "Clock: " + TIME$, 0, 0, 0)
210 blUser#(ClockH%,13) = 4
220 StatusH% = blCreate%(MainH%, 3, 5, 0, 0, 100, 60, "Button clicks: 0" + CHR$(13) + "Target hits: 0" + CHR$(13) + "Slider: 25   Dial: 35", 0, 0, 0)
230 blUser#(StatusH%,13) = 4
240 ButtonH% = blCreate%(MainH%, 3, 1, 0, 0, 180, 28, "Click me", ActButton%, 0, 0)
250 blUser#(ButtonH%,13) = 6
260 SliderH% = blCreate%(MainH%, 3, 2, 0, 0, 100, 28, "", ActSlider%, 0, 0)
270 CALL blSetValue(SliderH%, 25)
280 GaugeH% = blCreate%(MainH%, 3, 3, 0, 0, 100, 28, "", 0, 0, 0)
290 CALL blSetValue(GaugeH%, 60)
300 DialH% = blCreate%(MainH%, 3, 4, 0, 0, 100, 96, "", ActDial%, 0, 0)
310 CALL blSetValue(DialH%, 35)
320 TargetH% = blCreate%(MainH%, 4, 0, 0, 0, 100, 40, "", ActTarget%, 0, 0)
330 CALL blSetClickMask(TargetH%, 1)
340 IF blStatus% <> blStatusOk% THEN GOTO 9000
350 CALL blPaintAll
360 CALL blSetActionLine(ActButton%, 2010)
370 CALL blSetActionLine(ActSlider%, 2210)
380 CALL blSetActionLine(ActDial%, 2310)
390 CALL blSetActionLine(ActTarget%, 2110)
400 IF blStatus% <> blStatusOk% THEN GOTO 9000
410 TIMER ON
420 GOTO MainLoop
430 LABEL MainLoop
440 IF ClockFlag% <> 0 THEN ClockFlag% = 0 : GOSUB UpdateClock
450 R% = blPollInput%()
460 WHILE blEventCount%() > 0
470   E% = blNextEvent%()
480   GOSUB DispatchAction
490 WEND
500 K$ = INKEY$
510 IF K$ <> "" THEN GOSUB HandleKey
520 CALL blPaintAll
530 SLEEP 16
540 GOTO MainLoop
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
7000 LABEL RefreshStatus
7010 SliderV# = blGetValue#(SliderH%)
7020 DialV# = blGetValue#(DialH%)
7030 StatusText$ = "Button clicks: " + MID$(STR$(ButtonCount%),2)
7040 StatusText$ = StatusText$ + CHR$(13) + "Target hits: " + MID$(STR$(TargetCount%),2)
7050 StatusText$ = StatusText$ + CHR$(13) + "Slider: " + MID$(STR$(INT(SliderV#)),2)
7060 StatusText$ = StatusText$ + "   Dial: " + MID$(STR$(INT(DialV#)),2)
7070 CALL blSetText(StatusH%, StatusText$)
7080 CALL blPaintAll
7090 RETURN
7100 LABEL UpdateClock
7110 CALL blSetText(ClockH%, "Clock: " + TIME$)
7120 CALL blPaintAll
7130 RETURN
7200 LABEL DispatchAction
7210 A% = blEventAction%()
7220 IF A% = blActionNone% THEN RETURN
7230 ActLine& = blActionLine&(A%)
7240 IF ActLine& = blNoLine& THEN RETURN
7250 GOSUB ActLine&
7260 RETURN
7300 LABEL HandleKey
7310 IF K$ = CHR$(27) THEN TIMER OFF : END
7320 IF K$ = "R" OR K$ = "r" THEN ButtonCount% = 0 : TargetCount% = 0 : GOSUB RefreshStatus
7330 IF K$ = "+" OR K$ = "=" THEN GOSUB NudgeUp
7340 IF K$ = "-" OR K$ = "_" THEN GOSUB NudgeDown
7350 IF K$ = "C" OR K$ = "c" THEN GOSUB UpdateClock
7360 RETURN
7400 LABEL NudgeUp
7410 NewV# = blGetValue#(SliderH%) + 5
7420 IF NewV# > 100 THEN NewV# = 100
7430 CALL blSetValue(SliderH%, NewV#)
7440 GOSUB RefreshStatus
7450 RETURN
7500 LABEL NudgeDown
7510 NewV# = blGetValue#(SliderH%) - 5
7520 IF NewV# < 0 THEN NewV# = 0
7530 CALL blSetValue(SliderH%, NewV#)
7540 GOSUB RefreshStatus
7550 RETURN
9000 PRINT "sample host failed at status"; blStatus% : END
