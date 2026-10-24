10 REM baslib demo host: panel, button, multiline label, click target.
20 REM Host lines stay below 500000; baslib starts at 500000.
30 SCREEN 720
40 blReqW% = 0 : blReqH% = 0
50 GOSUB blApiInit
60 IF blStatus% <> blStatusOk% THEN PRINT "init failed"; blStatus%: END
70 REM --- panel ---
80 blReqClass% = blClassPanel% : blReqParent% = blRoot%
90 blReqX% = 40 : blReqY% = 40 : blReqW% = 520 : blReqH% = 360
100 blReqMode% = 1 : blReqPad% = 12 : blReqGap% = 8
110 GOSUB blApiCreate : blPanel% = blRetHandle%
120 REM --- button ---
130 blReqClass% = blClassUser% : blReqParent% = blPanel%
140 blReqKind% = blKindButton% : blReqX% = 0 : blReqY% = 0 : blReqW% = 220 : blReqH% = 32
150 blReqText$ = "Click me" : blReqAction% = 100
160 GOSUB blApiCreate : blButton% = blRetHandle%
170 REM --- label ---
180 blReqClass% = blClassUser% : blReqParent% = blPanel%
190 blReqKind% = blKindLabel% : blReqX% = 0 : blReqY% = 40 : blReqW% = 460 : blReqH% = 64
200 blReqText$ = "Ready" + CHR$(13) + "second line"
210 blReqPad% = 6 : blReqBorderStyle% = 0
220 GOSUB blApiCreate : blLabel% = blRetHandle%
230 REM --- click target ---
240 blReqClass% = blClassClickTarget% : blReqParent% = blPanel%
250 blReqX% = 0 : blReqY% = 120 : blReqW% = 460 : blReqH% = 80
260 blReqText$ = "" : blReqAction% = 200 : blReqButtonMask% = 1
270 GOSUB blApiCreate : blTarget% = blRetHandle%
280 GOSUB blApiPaintAll
290 REM --- action lines ---
300 blReqActionId% = 100 : blReqLine& = 1000 : GOSUB blApiSetActionLine
310 blReqActionId% = 200 : blReqLine& = 2000 : GOSUB blApiSetActionLine
320 REM --- main loop ---
330 GOSUB blApiPollInput
340 WHILE blEventCount% > 0
350   GOSUB blApiNextEvent
360   IF blEventAction% <> blActionNone% THEN GOSUB blActionLine&(blEventAction%)
370 WEND
380 K$ = INKEY$ : IF K$ = CHR$(27) THEN END
390 IF blNeedsPaint% THEN GOSUB blApiPaintAll
400 SLEEP 16
410 GOTO 330
1000 REM button action
1010 blReqHandle% = blLabel%
1020 blReqText$ = "Button clicked" + CHR$(13) + "at " + TIME$
1030 GOSUB blApiSetText
1040 GOSUB blApiAutoSize
1050 GOSUB blApiPaintAll
1060 RETURN
2000 REM click-target action
2010 blReqHandle% = blLabel%
2020 blReqText$ = "Target clicked" + CHR$(13) + "at " + TIME$
2030 GOSUB blApiSetText
2040 GOSUB blApiAutoSize
2050 GOSUB blApiPaintAll
2060 RETURN
