10 REM baslib demo host: panel, button, multiline label, click target.
20 REM Host lines stay below 500000; baslib starts at 500000.
30 SCREEN 720
40 CALL blInit(0, 0)
50 IF blStatus% <> blStatusOk% THEN PRINT "init failed"; blStatus%: END
60 REM --- panel ---
70 P% = blCreate%(1, 2, 0, 40, 40, 520, 360, "", 0, 0, 0)
80 CALL blSetLayout(P%, 1, 12, 8)
90 REM --- button ---
100 B% = blCreate%(P%, 3, 1, 0, 0, 220, 32, "Click me", 100, 0, 0)
110 REM --- label ---
120 L% = blCreate%(P%, 3, 5, 0, 40, 460, 64, "Ready" + CHR$(13) + "second line", 0, 0, 0)
130 blUser#(L%,13) = 6
140 REM --- click target ---
150 T% = blCreate%(P%, 4, 0, 0, 120, 460, 80, "", 200, 0, 0)
160 CALL blSetClickMask(T%, 1)
170 CALL blPaintAll
180 REM --- action lines ---
190 CALL blSetActionLine(100, 1010)
200 CALL blSetActionLine(200, 2010)
210 REM --- main loop ---
220 R% = blPollInput%()
230 WHILE blEventCount%() > 0
240   E% = blNextEvent%()
250   A% = blEventAction%()
260   IF A% <> blActionNone% THEN GOSUB blActionLine&(A%)
270 WEND
280 K$ = INKEY$ : IF K$ = CHR$(27) THEN END
290 CALL blPaintAll
300 SLEEP 16
310 GOTO 220
1000 REM button action
1010 CALL blSetText(L%, "Button clicked" + CHR$(13) + "at " + TIME$)
1020 CALL blAutoSize(L%)
1030 CALL blPaintAll
1040 RETURN
2000 REM click-target action
2010 CALL blSetText(L%, "Target clicked" + CHR$(13) + "at " + TIME$)
2020 CALL blAutoSize(L%)
2030 CALL blPaintAll
2040 RETURN
