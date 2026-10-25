500000 END
500010 CONST blVersion$ = "0.3"
500011 CONST blVersionNum# = 0.3
500012 CONST blMaxControls% = 255
500013 CONST blMaxFrames% = 32
500014 CONST blMaxArgs% = 8
500015 CONST blMaxStrArgs% = 2
500016 CONST blMaxClasses% = 16
500017 CONST blMaxVerbs% = 20
500018 CONST blMaxKinds% = 8
500019 CONST blMaxEvents% = 64
500020 CONST blHandleRoot% = 1
500021 CONST blActionNone% = 0
500022 CONST blStatusOk% = 0
500023 CONST blStatusNotInit% = 1
500024 CONST blStatusControlTableFull% = 2
500025 CONST blStatusInvalidHandle% = 3
500026 CONST blStatusInvalidClass% = 4
500027 CONST blStatusInvalidVerb% = 5
500028 CONST blStatusFrameOverflow% = 6
500029 CONST blStatusInvalidParent% = 7
500030 CONST blStatusCannotDestroyRoot% = 8
500031 CONST blStatusInvalidGeometry% = 9
500032 CONST blStatusInvalidValue% = 10
500033 CONST blStatusInvalidLayout% = 11
500034 CONST blStatusEventQueueFull% = 12
500035 CONST blStatusEventQueueEmpty% = 13
500036 CONST blStatusNoGraphics% = 14
500037 CONST blStatusFrameProtocol% = 15
500038 CONST blStatusMissingActionHandler% = 16
500039 CONST blStatusClassTableFull% = 17
500040 CONST blClassTop% = 1
500041 CONST blClassPanel% = 2
500042 CONST blClassUser% = 3
500043 CONST blClassClickTarget% = 4
500044 CONST blVerbPaint% = 1
500045 CONST blVerbUpdate% = 2
500046 CONST blVerbShow% = 3
500047 CONST blVerbHide% = 4
500048 CONST blVerbLayout% = 5
500049 CONST blVerbClick% = 6
500050 CONST blVerbHitTest% = 7
500051 CONST blVerbMouseDown% = 8
500052 CONST blVerbMouseUp% = 9
500053 CONST blVerbMouseMove% = 10
500054 CONST blVerbMouseEnter% = 11
500055 CONST blVerbMouseLeave% = 12
500056 CONST blVerbWheel% = 13
500057 CONST blVerbCaptureLost% = 14
500058 CONST blVerbDestroy% = 15
500059 CONST blVerbKey% = 16
500060 CONST blVerbFocus% = 17
500061 CONST blKindButton% = 1
500062 CONST blKindSlider% = 2
500063 CONST blKindGauge% = 3
500064 CONST blKindDial% = 4
500065 CONST blKindLabel% = 5
500066 CONST blEvClick% = 1
500067 CONST blEvChange% = 2
500068 CONST blEvMove% = 3
500069 CONST blEvEnter% = 4
500070 CONST blEvLeave% = 5
500071 CONST blEvDown% = 6
500072 CONST blEvUp% = 7
500073 CONST blEvWheel% = 8
500074 CONST blEvKey% = 9
500075 CONST blEvFocus% = 10
500076 CONST blEvCaptureLost% = 11
500077 CONST blFlagVisible% = 1
500078 CONST blFlagEnabled% = 2
500079 CONST blFlagClip% = 4
500080 CONST blFlagDirty% = 8
500081 CONST blFlagHot% = 16
500082 CONST blFlagPressed% = 32
500083 CONST blFlagClickable% = 64
500084 CONST blFlagFocusable% = 128
500085 CONST blFlagTransparent% = 256
500086 CONST blFlagLayoutDirty% = 512
500087 CONST blFlagCaptured% = 1024
500088 CONST blNoLine& = 0
500100 FUNCTION blClamp#(V#, Lo#, Hi#)
500101 blClamp# = V#
500102 IF blClamp# < Lo# THEN blClamp# = Lo#
500103 IF blClamp# > Hi# THEN blClamp# = Hi#
500104 END FUNCTION
500110 FUNCTION blAllocHandle%()
500111 LOCAL I%
500112 blAllocHandle% = 0
500113 FOR I% = 1 TO blMaxControls%
500114   IF blCtl%(I%,1) = 0 THEN blAllocHandle% = I% : blStatus% = blStatusOk% : EXIT FUNCTION
500115 NEXT I%
500116 blStatus% = blStatusControlTableFull%
500117 END FUNCTION
500120 SUB blLinkHandle(H%)
500121 LOCAL P%, Last%
500122 P% = blCtl%(H%,2)
500123 IF P% < 1 OR P% > blMaxControls% THEN EXIT SUB
500124 Last% = blCtl%(P%,4)
500125 IF Last% = 0 THEN blCtl%(P%,3) = H% : blCtl%(H%,5) = 0 : blCtl%(H%,6) = 0 : GOTO blLinkDone
500126 blCtl%(Last%,6) = H%
500127 blCtl%(H%,5) = Last%
500128 LABEL blLinkDone
500129 blCtl%(P%,4) = H%
500130 END SUB
500140 SUB blDetachHandle(H%)
500141 LOCAL P%, Prev%, Next%
500142 P% = blCtl%(H%,2)
500143 IF P% < 1 OR P% > blMaxControls% THEN EXIT SUB
500144 Prev% = blCtl%(H%,5)
500145 Next% = blCtl%(H%,6)
500146 IF Prev% <> 0 THEN blCtl%(Prev%,6) = Next% ELSE blCtl%(P%,3) = Next%
500147 IF Next% <> 0 THEN blCtl%(Next%,5) = Prev% ELSE blCtl%(P%,4) = Prev%
500148 blCtl%(H%,5) = 0
500149 blCtl%(H%,6) = 0
500150 END SUB
500160 SUB blInitTables
500161 LOCAL I%, C%, V%
500162 IF blInitDone% <> 0 THEN GOTO blInitClear
500163 DIM blCtl%(255,17)
500164 DIM blCtlText$(255)
500165 DIM blPanel%(255,8), blUser#(255,22), blClick%(255,4)
500166 DIM blCtlClipL%(255), blCtlClipT%(255), blCtlClipR%(255), blCtlClipB%(255)
500167 DIM blCtlZ%(255)
500168 DIM blMethod&(16,20), blActionLine&(255)
500169 DIM blClassName$(16)
500170 DIM blEvType%(63), blEvCtl%(63), blEvAction%(63), blEvX%(63), blEvY%(63)
500171 DIM blEvButton%(63), blEvWheelVal%(63)
500172 blInitDone% = 1
500173 LABEL blInitClear
500174 FOR I% = 0 TO 255
500175   blCtl%(I%,1) = 0
500176   blCtl%(I%,2) = 0
500177   blCtl%(I%,3) = 0
500178   blCtl%(I%,4) = 0
500179   blCtl%(I%,5) = 0
500180   blCtl%(I%,6) = 0
500181   blCtl%(I%,7) = 0
500182   blCtl%(I%,8) = 0
500183   blCtl%(I%,9) = 0
500184   blCtl%(I%,10) = 0
500185   blCtl%(I%,11) = 0
500186   blCtl%(I%,12) = 0
500187   blCtl%(I%,13) = 0
500188   blCtl%(I%,14) = 0
500189   blCtl%(I%,15) = 0
500190   blCtl%(I%,16) = 0
500191   blCtl%(I%,17) = 0
500192   blCtlText$(I%) = ""
500193   blCtlClipL%(I%) = 0
500194   blCtlClipT%(I%) = 0
500195   blCtlClipR%(I%) = 0
500196   blCtlClipB%(I%) = 0
500197   blCtlZ%(I%) = 0
500198 NEXT I%
500199 FOR C% = 1 TO 16
500200   blClassName$(C%) = ""
500201   FOR V% = 1 TO 20
500202     blMethod&(C%,V%) = 502999
500203   NEXT V%
500204 NEXT C%
500205 blClassName$(1) = "Top"
500206 blClassName$(2) = "Panel"
500207 blClassName$(3) = "User"
500208 blClassName$(4) = "ClickTarget"
500209 FOR I% = 0 TO 255
500210   blActionLine&(I%) = 0
500211 NEXT I%
500212 FOR I% = 0 TO 63
500213   blEvType%(I%) = 0
500214   blEvCtl%(I%) = 0
500215   blEvAction%(I%) = 0
500216   blEvX%(I%) = 0
500217   blEvY%(I%) = 0
500218   blEvButton%(I%) = 0
500219   blEvWheelVal%(I%) = 0
500220 NEXT I%
500221 END SUB
500230 SUB blLayoutControl(H%)
500231 LOCAL C%
500232 IF H% < 1 OR H% > blMaxControls% THEN EXIT SUB
500233 IF blCtl%(H%,1) = 0 THEN EXIT SUB
500234 IF (blCtl%(H%,13) AND blFlagVisible%) = 0 THEN EXIT SUB
500235 C% = blCtl%(H%,1)
500236 IF C% = blClassTop% OR C% = blClassPanel% THEN CALL blLayoutChildren(H%)
500237 END SUB
500240 SUB blLayoutChildren(H%)
500241 LOCAL Mode%, Pad%, Gap%, PX%, PY%, CW%, CH%, CX%, CY%
500242 LOCAL Child%, Next%, X2%, Y2%, L%, T%, R%, B%
500243 Mode% = blPanel%(H%,1)
500244 Pad% = blPanel%(H%,2)
500245 Gap% = blPanel%(H%,3)
500246 PX% = blCtl%(H%,11)
500247 PY% = blCtl%(H%,12)
500248 CW% = blCtl%(H%,9)
500249 CH% = blCtl%(H%,10)
500250 CX% = PX% + Pad%
500251 CY% = PY% + Pad%
500252 Child% = blCtl%(H%,3)
500253 WHILE Child% <> 0
500254   IF (blCtl%(Child%,13) AND blFlagVisible%) = 0 THEN GOTO blLayoutNext
500255   SELECT CASE Mode%
500256   CASE 1
500257     blCtl%(Child%,7) = Pad%
500258     blCtl%(Child%,8) = CY% - PY%
500259     blCtl%(Child%,9) = CW% - 2 * Pad%
500260     CY% = CY% + blCtl%(Child%,10) + Gap%
500261   CASE 2
500262     blCtl%(Child%,7) = CX% - PX%
500263     blCtl%(Child%,8) = Pad%
500264     blCtl%(Child%,10) = CH% - 2 * Pad%
500265     CX% = CX% + blCtl%(Child%,9) + Gap%
500266   END SELECT
500267   blCtl%(Child%,11) = PX% + blCtl%(Child%,7)
500268   blCtl%(Child%,12) = PY% + blCtl%(Child%,8)
500269   L% = blCtlClipL%(H%)
500270   T% = blCtlClipT%(H%)
500271   R% = blCtlClipR%(H%)
500272   B% = blCtlClipB%(H%)
500273   IF blCtl%(Child%,11) > L% THEN L% = blCtl%(Child%,11)
500274   IF blCtl%(Child%,12) > T% THEN T% = blCtl%(Child%,12)
500275   X2% = blCtl%(Child%,11) + blCtl%(Child%,9) - 1
500276   Y2% = blCtl%(Child%,12) + blCtl%(Child%,10) - 1
500277   IF X2% < R% THEN R% = X2%
500278   IF Y2% < B% THEN B% = Y2%
500279   blCtlClipL%(Child%) = L%
500280   blCtlClipT%(Child%) = T%
500281   blCtlClipR%(Child%) = R%
500282   blCtlClipB%(Child%) = B%
500283   Next% = blCtl%(Child%,6)
500284   CALL blLayoutControl(Child%)
500285   Child% = Next%
500286   GOTO blLayoutLoopEnd
500287   LABEL blLayoutNext
500288   Child% = blCtl%(Child%,6)
500289   LABEL blLayoutLoopEnd
500290 WEND
500291 END SUB
500300 SUB blPaintTree(H%)
500301 LOCAL Child%, Next%
500302 IF H% < 1 OR H% > blMaxControls% THEN EXIT SUB
500303 IF blCtl%(H%,1) = 0 THEN EXIT SUB
500304 IF (blCtl%(H%,13) AND blFlagVisible%) = 0 THEN EXIT SUB
500305 CALL blPaintOne(H%)
500306 Child% = blCtl%(H%,3)
500307 WHILE Child% <> 0
500308   Next% = blCtl%(Child%,6)
500309   CALL blPaintTree(Child%)
500310   Child% = Next%
500311 WEND
500312 END SUB
500320 SUB blPaintOne(H%)
500321 LOCAL W%, HH%, Cls%
500322 IF H% < 1 OR H% > blMaxControls% THEN EXIT SUB
500323 IF blCtl%(H%,1) = 0 THEN EXIT SUB
500324 W% = blCtl%(H%,9)
500325 HH% = blCtl%(H%,10)
500326 IF W% <= 0 OR HH% <= 0 THEN EXIT SUB
500327 PUSH VIEW
500328 Cls% = blCtl%(H%,1)
500329 IF Cls% = blClassTop% OR Cls% = blClassPanel% THEN CALL blPaintPanel(H%) : GOTO blPaintOneDone
500330 IF Cls% = blClassUser% THEN CALL blPaintUser(H%)
500331 LABEL blPaintOneDone
500332 POP VIEW
500333 END SUB
500340 SUB blPaintPanel(H%)
500341 LOCAL X%, Y%, W%, HH%, Face%, Border%
500342 X% = blCtl%(H%,11)
500343 Y% = blCtl%(H%,12)
500344 W% = blCtl%(H%,9)
500345 HH% = blCtl%(H%,10)
500346 Face% = blPanel%(H%,6)
500347 Border% = blPanel%(H%,7)
500348 LINE (X%,Y%)-(X% + W% - 1,Y% + HH% - 1), Face%, BF
500349 IF Border% >= 0 THEN LINE (X%,Y%)-(X% + W% - 1,Y% + HH% - 1), Border%, B
500350 END SUB
500360 SUB blPaintUser(H%)
500361 LOCAL Kind%
500362 Kind% = blUser#(H%,1)
500363 SELECT CASE Kind%
500364 CASE blKindButton%
500365   CALL blPaintButton(H%)
500366 CASE blKindLabel%
500367   CALL blPaintLabel(H%)
500368 CASE blKindSlider%
500369   CALL blPaintSlider(H%)
500370 CASE blKindGauge%
500371   CALL blPaintGauge(H%)
500372 CASE blKindDial%
500373   CALL blPaintDial(H%)
500374 END SELECT
500375 END SUB
500380 SUB blPaintButton(H%)
500381 LOCAL X%, Y%, W%, HH%, Face%, Col%, Border%, Scale%
500382 X% = blCtl%(H%,11)
500383 Y% = blCtl%(H%,12)
500384 W% = blCtl%(H%,9)
500385 HH% = blCtl%(H%,10)
500386 Face% = blUser#(H%,8)
500387 Col% = blUser#(H%,19)
500388 Border% = blUser#(H%,17)
500389 Scale% = blUser#(H%,20)
500390 IF (blCtl%(H%,13) AND blFlagPressed%) <> 0 THEN Face% = blUser#(H%,9)
500391 LINE (X%,Y%)-(X% + W% - 1,Y% + HH% - 1), Face%, BF
500392 IF Border% >= 0 THEN LINE (X%,Y%)-(X% + W% - 1,Y% + HH% - 1), Border%, B
500393 CALL blDrawText(X%,Y%,W%,HH%,blCtlText$(H%),Col%,Scale%,1,1,2)
500394 END SUB
500400 SUB blPaintLabel(H%)
500401 LOCAL X%, Y%, W%, HH%, Face%, Col%, Border%, Scale%, Align%, VAlign%, Pad%, T$
500402 X% = blCtl%(H%,11)
500403 Y% = blCtl%(H%,12)
500404 W% = blCtl%(H%,9)
500405 HH% = blCtl%(H%,10)
500406 Face% = blUser#(H%,8)
500407 Col% = blUser#(H%,19)
500408 Border% = blUser#(H%,17)
500409 Scale% = blUser#(H%,20)
500410 Align% = blUser#(H%,14)
500411 VAlign% = blUser#(H%,15)
500412 Pad% = blUser#(H%,13)
500413 T$ = blCtlText$(H%)
500414 IF blUser#(H%,16) <> 0 THEN T$ = blWrapText$(T$,W% - 2 * Pad%,Scale%)
500415 IF blUser#(H%,18) = 0 THEN LINE (X%,Y%)-(X% + W% - 1,Y% + HH% - 1), Face%, BF
500416 IF Border% >= 0 THEN LINE (X%,Y%)-(X% + W% - 1,Y% + HH% - 1), Border%, B
500417 CALL blDrawText(X%,Y%,W%,HH%,T$,Col%,Scale%,Align%,VAlign%,Pad%)
500418 END SUB
500430 SUB blPaintSlider(H%)
500431 LOCAL X%, Y%, W%, HH%, Pos%, Accent%
500432 X% = blCtl%(H%,11)
500433 Y% = blCtl%(H%,12)
500434 W% = blCtl%(H%,9)
500435 HH% = blCtl%(H%,10)
500436 Accent% = blUser#(H%,9)
500437 IF W% <= 0 THEN EXIT SUB
500438 LINE (X%,Y% + HH% / 2)-(X% + W% - 1,Y% + HH% / 2), 8, BF
500439 Pos% = X% + (W% - 8) * (blUser#(H%,2) - blUser#(H%,3)) / (blUser#(H%,4) - blUser#(H%,3) + 0.000001)
500440 LINE (Pos%,Y% + 2)-(Pos% + 7,Y% + HH% - 2), Accent%, BF
500441 END SUB
500450 SUB blPaintGauge(H%)
500451 LOCAL X%, Y%, W%, HH%, Face%, Accent%, Pos%
500452 X% = blCtl%(H%,11)
500453 Y% = blCtl%(H%,12)
500454 W% = blCtl%(H%,9)
500455 HH% = blCtl%(H%,10)
500456 Face% = blUser#(H%,8)
500457 Accent% = blUser#(H%,9)
500458 LINE (X%,Y%)-(X% + W% - 1,Y% + HH% - 1), Face%, BF
500459 Pos% = X% + (W% - 2) * (blUser#(H%,2) - blUser#(H%,3)) / (blUser#(H%,4) - blUser#(H%,3) + 0.000001)
500460 LINE (X% + 1,Y% + 1)-(Pos%,Y% + HH% - 2), Accent%, BF
500461 END SUB
500470 SUB blPaintDial(H%)
500471 LOCAL X%, Y%, W%, HH%, Face%, Accent%, R%, CX%, CY%, A#, EX%, EY%
500472 X% = blCtl%(H%,11)
500473 Y% = blCtl%(H%,12)
500474 W% = blCtl%(H%,9)
500475 HH% = blCtl%(H%,10)
500476 Face% = blUser#(H%,8)
500477 Accent% = blUser#(H%,9)
500478 R% = HH% / 2 - 2
500479 IF W% < HH% THEN R% = W% / 2 - 2
500480 CX% = X% + W% / 2
500481 CY% = Y% + HH% / 2
500482 CIRCLE (CX%,CY%), R%, Face%, 0, 6.2831853, 1
500483 CIRCLE (CX%,CY%), R% - 1, Accent%, 0, 6.2831853, 1
500484 A# = 3.14159265 * (1 - (blUser#(H%,2) - blUser#(H%,3)) / (blUser#(H%,4) - blUser#(H%,3) + 0.000001))
500485 EX% = CX% + COS(A#) * (R% - 4)
500486 EY% = CY% - SIN(A#) * (R% - 4)
500487 LINE (CX%,CY%)-(EX%,EY%), Accent%
500488 END SUB
500500 SUB blDrawText(X%, Y%, W%, HH%, Text$, Color%, Scale%, Align%, VAlign%, Pad%)
500501 LOCAL T$, TW%, TH%, DX%, DY%, FitW%
500502 IF Text$ = "" THEN EXIT SUB
500503 FitW% = W% - 2 * Pad%
500504 IF FitW% < 1 THEN FitW% = 1
500505 T$ = blFitText$(Text$,FitW%,Scale%)
500506 TW% = TEXTWIDTH(T$,Scale%)
500507 TH% = TEXTHEIGHT(T$,Scale%)
500508 DX% = X% + Pad%
500509 DY% = Y% + Pad%
500510 IF Align% = 1 THEN DX% = X% + (W% - TW%) / 2
500511 IF Align% = 2 THEN DX% = X% + W% - TW% - Pad%
500512 IF VAlign% = 1 THEN DY% = Y% + (HH% - TH%) / 2
500513 IF VAlign% = 2 THEN DY% = Y% + HH% - TH% - Pad%
500514 DRAWTEXT (DX%,DY%), T$, Color%, Scale%
500515 END SUB
500520 FUNCTION blFitText$(Source$, MaxW%, Scale%)
500521 LOCAL In$, Out$, Line$, Ch$, I%
500522 blFitText$ = ""
500523 IF MaxW% < 1 THEN MaxW% = 1
500524 IF Source$ = "" THEN EXIT FUNCTION
500525 In$ = Source$
500526 Out$ = ""
500527 Line$ = ""
500528 FOR I% = 1 TO LEN(In$)
500529   Ch$ = MID$(In$,I%,1)
500530   IF Ch$ = CHR$(13) OR Ch$ = CHR$(10) THEN GOTO blFitNewline
500531   Line$ = Line$ + Ch$
500532   GOTO blFitNext
500533   LABEL blFitNewline
500534   WHILE LEN(Line$) > 0 AND TEXTWIDTH(Line$,Scale%) > MaxW%
500535     Line$ = LEFT$(Line$,LEN(Line$) - 1)
500536   WEND
500537   Out$ = Out$ + Line$ + CHR$(13) + CHR$(10)
500538   Line$ = ""
500539   LABEL blFitNext
500540 NEXT I%
500541 WHILE LEN(Line$) > 0 AND TEXTWIDTH(Line$,Scale%) > MaxW%
500542   Line$ = LEFT$(Line$,LEN(Line$) - 1)
500543 WEND
500544 blFitText$ = Out$ + Line$
500545 END FUNCTION
500550 FUNCTION blWrapText$(Source$, MaxW%, Scale%)
500551 LOCAL In$, Out$, Line$, Ch$, I%, Max%
500552 blWrapText$ = ""
500553 In$ = Source$
500554 Out$ = ""
500555 Line$ = ""
500556 Max% = MaxW%
500557 IF Max% < 1 THEN Max% = 1
500558 FOR I% = 1 TO LEN(In$)
500559   Ch$ = MID$(In$,I%,1)
500560   IF Ch$ = CHR$(13) OR Ch$ = CHR$(10) THEN GOTO blWrapNewline
500561   IF LEN(Line$) > 0 AND TEXTWIDTH(Line$ + Ch$,Scale%) > Max% THEN GOTO blWrapBreak
500562   Line$ = Line$ + Ch$
500563   GOTO blWrapNext
500564   LABEL blWrapBreak
500565   Out$ = Out$ + Line$ + CHR$(13) + CHR$(10)
500566   Line$ = Ch$
500567   GOTO blWrapNext
500568   LABEL blWrapNewline
500569   Out$ = Out$ + Line$ + CHR$(13) + CHR$(10)
500570   Line$ = ""
500571   LABEL blWrapNext
500572 NEXT I%
500573 blWrapText$ = Out$ + Line$
500574 END FUNCTION
500580 FUNCTION blHitTest%(X%, Y%)
500581 LOCAL H%, X1%, Y1%, X2%, Y2%
500582 blHitTest% = 0
500583 FOR H% = blMaxControls% TO 1 STEP -1
500584   IF blCtl%(H%,1) = 0 OR (blCtl%(H%,13) AND blFlagVisible%) = 0 OR (blCtl%(H%,13) AND blFlagEnabled%) = 0 THEN GOTO blHitNext
500585   X1% = blCtl%(H%,11)
500586   Y1% = blCtl%(H%,12)
500587   X2% = X1% + blCtl%(H%,9) - 1
500588   Y2% = Y1% + blCtl%(H%,10) - 1
500589   IF X% >= X1% AND X% <= X2% AND Y% >= Y1% AND Y% <= Y2% THEN blHitTest% = H% : EXIT FUNCTION
500590   LABEL blHitNext
500591 NEXT H%
500592 END FUNCTION
500600 SUB blPostEvent(Type%, Ctl%, Action%, X%, Y%, Button%, Wheel%)
500601 IF blEvCount% >= blMaxEvents% THEN blStatus% = blStatusEventQueueFull% : EXIT SUB
500602 blEvType%(blEvTail%) = Type%
500603 blEvCtl%(blEvTail%) = Ctl%
500604 blEvAction%(blEvTail%) = Action%
500605 blEvX%(blEvTail%) = X%
500606 blEvY%(blEvTail%) = Y%
500607 blEvButton%(blEvTail%) = Button%
500608 blEvWheelVal%(blEvTail%) = Wheel%
500609 blEvTail% = (blEvTail% + 1) MOD blMaxEvents%
500610 blEvCount% = blEvCount% + 1
500611 END SUB
500620 SUB blPostClick(Owner%, Action%, X%, Y%, Button%)
500621 CALL blPostEvent(blEvClick%,Owner%,Action%,X%,Y%,Button%,0)
500622 END SUB
500630 SUB blPointerValue(H%, X%, Y%, Button%)
500631 LOCAL Kind%, V#, W%
500632 Kind% = blUser#(H%,1)
500633 IF Kind% <> blKindSlider% AND Kind% <> blKindDial% THEN EXIT SUB
500634 W% = blCtl%(H%,9)
500635 IF W% <= 0 THEN EXIT SUB
500636 V# = blUser#(H%,3) + (X% - blCtl%(H%,11)) * (blUser#(H%,4) - blUser#(H%,3)) / W%
500637 V# = blClamp#(V#,blUser#(H%,3),blUser#(H%,4))
500638 blUser#(H%,2) = V#
500639 CALL blPostEvent(blEvChange%,H%,blCtl%(H%,15),X%,Y%,Button%,0)
500640 blNeedsPaint% = 1
500641 END SUB
500650 FUNCTION blClickAt%(X%, Y%, Button%)
500651 LOCAL H%, Action%, P%
500652 blClickAt% = 0
500653 H% = blHitTest%(X%,Y%)
500654 IF H% = 0 THEN EXIT FUNCTION
500655 IF blCtl%(H%,1) = blClassUser% THEN CALL blPointerValue(H%,X%,Y%,Button%)
500656 Action% = blCtl%(H%,15)
500657 P% = blCtl%(H%,2)
500658 WHILE Action% = 0 AND P% <> 0
500659   IF blCtl%(P%,15) <> 0 THEN Action% = blCtl%(P%,15) : H% = P% : P% = 0 ELSE P% = blCtl%(P%,2)
500660 WEND
500661 blClickAt% = H%
500662 IF Action% <> 0 THEN CALL blPostClick(H%,Action%,X%,Y%,Button%)
500663 END FUNCTION
500670 SUB blPostHover(Old%, New%, X%, Y%)
500671 IF Old% <> 0 THEN CALL blPostEvent(blEvLeave%,Old%,0,X%,Y%,0,0)
500672 IF New% <> 0 THEN CALL blPostEvent(blEvEnter%,New%,0,X%,Y%,0,0)
500673 END SUB
500680 SUB blPostMove(H%, X%, Y%)
500681 CALL blPostEvent(blEvMove%,H%,0,X%,Y%,0,0)
500682 END SUB
500690 SUB blTestMove(X%, Y%)
500691 LOCAL Old%, H%
500692 Old% = blHover%
500693 H% = blHitTest%(X%,Y%)
500694 blHover% = H%
500695 IF blCapture% <> 0 AND blPressed% <> 0 THEN CALL blPointerValue(blCapture%,X%,Y%,blPressed%)
500696 IF Old% <> blHover% THEN CALL blPostHover(Old%,blHover%,X%,Y%)
500697 IF blHover% <> 0 THEN CALL blPostMove(blHover%,X%,Y)
500698 END SUB
500700 SUB blTestButton(Button%, Down%, X%, Y%)
500701 IF Down% = 0 THEN GOTO blButtonUp
500702 blPressed% = Button%
500703 IF blHover% <> 0 THEN blCapture% = blHover%
500704 CALL blPostEvent(blEvDown%,blHover%,0,X%,Y%,Button%,0)
500705 GOTO blButtonDone
500706 LABEL blButtonUp
500707 CALL blPostEvent(blEvUp%,blCapture%,0,X%,Y%,Button%,0)
500708 blPressed% = 0
500709 blCapture% = 0
500710 LABEL blButtonDone
500711 END SUB
500720 SUB blWheelAt(X%, Y%, Delta%)
500721 IF blHover% = 0 THEN EXIT SUB
500722 CALL blPostEvent(blEvWheel%,blHover%,0,X%,Y%,0,Delta%)
500723 END SUB
502999 LABEL blNoop
503000 RETURN
510000 SUB blInit(W%, H%)
510001 CALL blInitTables
510002 blRoot% = 1
510003 blNeedsPaint% = 1
510004 blNeedsLayout% = 1
510005 blHover% = 0
510006 blPressed% = 0
510007 blCapture% = 0
510008 blEvHead% = 0
510009 blEvTail% = 0
510010 blEvCount% = 0
510011 blCurType% = 0
510012 blCurCtl% = 0
510013 blCurAction% = 0
510014 blCurX% = 0
510015 blCurY% = 0
510016 blCurButton% = 0
510017 blCurWheel% = 0
510018 blStatus% = blStatusOk%
510019 blCtl%(1,1) = blClassTop%
510020 blCtl%(1,2) = 0
510021 blCtl%(1,3) = 0
510022 blCtl%(1,4) = 0
510023 blCtl%(1,5) = 0
510024 blCtl%(1,6) = 0
510025 blCtl%(1,7) = 0
510026 blCtl%(1,8) = 0
510027 blCtl%(1,9) = 640
510028 IF W% > 0 THEN blCtl%(1,9) = W%
510029 IF W% <= 0 AND SCREENW > 0 THEN blCtl%(1,9) = SCREENW
510030 blCtl%(1,10) = 400
510031 IF H% > 0 THEN blCtl%(1,10) = H%
510032 IF H% <= 0 AND SCREENH > 0 THEN blCtl%(1,10) = SCREENH
510033 blCtl%(1,11) = 0
510034 blCtl%(1,12) = 0
510035 blCtl%(1,13) = blFlagVisible% OR blFlagEnabled% OR blFlagClip%
510036 blCtl%(1,14) = 0
510037 blCtl%(1,15) = 0
510038 blCtl%(1,16) = 0
510039 blCtl%(1,17) = 0
510040 blCtlClipL%(1) = 0
510041 blCtlClipT%(1) = 0
510042 blCtlClipR%(1) = blCtl%(1,9) - 1
510043 blCtlClipB%(1) = blCtl%(1,10) - 1
510044 blCtlZ%(1) = 1
510045 blPanel%(1,1) = 0
510046 blPanel%(1,2) = 0
510047 blPanel%(1,3) = 0
510048 blPanel%(1,4) = 0
510049 blPanel%(1,6) = 7
510050 blPanel%(1,7) = 8
510051 END SUB
510060 FUNCTION blCreate%(Parent%, Class%, Kind%, X%, Y%, W%, Hgt%, Text$, Action%, Flags%, Tag%)
510061 LOCAL H%
510062 blCreate% = 0
510063 blStatus% = blStatusOk%
510064 IF Parent% < 1 OR Parent% > blMaxControls% THEN blStatus% = blStatusInvalidParent% : EXIT FUNCTION
510065 IF blCtl%(Parent%,1) = 0 THEN blStatus% = blStatusInvalidParent% : EXIT FUNCTION
510066 IF Class% < 2 OR Class% > 4 THEN blStatus% = blStatusInvalidClass% : EXIT FUNCTION
510067 IF Class% = blClassUser% AND (Kind% < 1 OR Kind% > 5) THEN blStatus% = blStatusInvalidValue% : EXIT FUNCTION
510068 H% = blAllocHandle%()
510069 IF H% = 0 THEN EXIT FUNCTION
510070 blCtl%(H%,1) = Class%
510071 blCtl%(H%,2) = Parent%
510072 blCtl%(H%,3) = 0
510073 blCtl%(H%,4) = 0
510074 blCtl%(H%,5) = 0
510075 blCtl%(H%,6) = 0
510076 blCtl%(H%,7) = X%
510077 blCtl%(H%,8) = Y%
510078 blCtl%(H%,9) = W%
510079 blCtl%(H%,10) = Hgt%
510080 blCtl%(H%,11) = X%
510081 blCtl%(H%,12) = Y%
510082 blCtl%(H%,13) = Flags% OR blFlagVisible% OR blFlagEnabled%
510083 blCtl%(H%,14) = 0
510084 blCtl%(H%,15) = Action%
510085 blCtl%(H%,16) = Tag%
510086 blCtl%(H%,17) = 0
510087 blCtlText$(H%) = Text$
510088 blCtlZ%(H%) = H%
510089 IF Class% = blClassPanel% THEN blPanel%(H%,1) = 0 : blPanel%(H%,2) = 0 : blPanel%(H%,3) = 0 : blPanel%(H%,4) = 0 : blPanel%(H%,6) = 7 : blPanel%(H%,7) = 8 : blCtl%(H%,13) = blCtl%(H%,13) OR blFlagClip%
510090 IF Class% = blClassUser% THEN blUser#(H%,1) = Kind% : blUser#(H%,2) = 0 : blUser#(H%,3) = 0 : blUser#(H%,4) = 100 : blUser#(H%,5) = 1 : blUser#(H%,6) = 0 : blUser#(H%,7) = 0 : blUser#(H%,8) = 7 : blUser#(H%,9) = 4 : blUser#(H%,10) = 0 : blUser#(H%,11) = 0
510091 IF Class% = blClassUser% THEN blUser#(H%,12) = 0 : blUser#(H%,13) = 2 : blUser#(H%,14) = 1 : blUser#(H%,15) = 1 : blUser#(H%,16) = 0 : blUser#(H%,17) = 0 : blUser#(H%,18) = 1 : blUser#(H%,19) = 0 : blUser#(H%,20) = 1 : blUser#(H%,21) = 0 : blUser#(H%,22) = 1
510092 IF Class% = blClassClickTarget% THEN blClick%(H%,1) = 1 : blClick%(H%,2) = 0 : blClick%(H%,3) = 0 : blClick%(H%,4) = 0
510093 CALL blLinkHandle(H%)
510094 blCreate% = H%
510095 blNeedsLayout% = 1
510096 blNeedsPaint% = 1
510097 END FUNCTION
510110 SUB blDestroy(H%)
510111 LOCAL Child%, Next%
510112 blStatus% = blStatusOk%
510113 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510114 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510115 IF H% = blRoot% THEN blStatus% = blStatusCannotDestroyRoot% : EXIT SUB
510116 Child% = blCtl%(H%,3)
510117 WHILE Child% <> 0
510118   Next% = blCtl%(Child%,6)
510119   CALL blDestroy(Child%)
510120   Child% = Next%
510121 WEND
510122 CALL blDetachHandle(H%)
510123 IF blHover% = H% THEN blHover% = 0
510124 IF blPressed% = H% THEN blPressed% = 0
510125 IF blCapture% = H% THEN blCapture% = 0
510126 blCtl%(H%,1) = 0
510127 blCtl%(H%,2) = 0
510128 blCtl%(H%,3) = 0
510129 blCtl%(H%,4) = 0
510130 blCtl%(H%,5) = 0
510131 blCtl%(H%,6) = 0
510132 blCtl%(H%,13) = 0
510133 blCtlText$(H%) = ""
510134 blNeedsLayout% = 1
510135 blNeedsPaint% = 1
510136 END SUB
510150 SUB blSetBounds(H%, X%, Y%, W%, Hgt%)
510151 blStatus% = blStatusOk%
510152 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510153 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510154 blCtl%(H%,7) = X%
510155 blCtl%(H%,8) = Y%
510156 blCtl%(H%,9) = W%
510157 blCtl%(H%,10) = Hgt%
510158 blCtl%(H%,13) = blCtl%(H%,13) OR blFlagLayoutDirty%
510159 blNeedsLayout% = 1
510160 blNeedsPaint% = 1
510161 END SUB
510170 SUB blSetLayout(H%, Mode%, Pad%, Gap%)
510171 blStatus% = blStatusOk%
510172 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510173 IF blCtl%(H%,1) <> blClassPanel% AND blCtl%(H%,1) <> blClassTop% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
510174 blPanel%(H%,1) = Mode%
510175 blPanel%(H%,2) = Pad%
510176 blPanel%(H%,3) = Gap%
510177 blCtl%(H%,13) = blCtl%(H%,13) OR blFlagLayoutDirty%
510178 blNeedsLayout% = 1
510179 blNeedsPaint% = 1
510180 END SUB
510190 SUB blShow(H%)
510191 blStatus% = blStatusOk%
510192 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510193 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510194 blCtl%(H%,13) = blCtl%(H%,13) OR blFlagVisible%
510195 blNeedsLayout% = 1
510196 blNeedsPaint% = 1
510197 END SUB
510200 SUB blHide(H%)
510201 blStatus% = blStatusOk%
510202 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510203 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510204 blCtl%(H%,13) = blCtl%(H%,13) AND (blFlagVisible% XOR 32767)
510205 IF blHover% = H% THEN blHover% = 0
510206 IF blPressed% = H% THEN blPressed% = 0
510207 IF blCapture% = H% THEN blCapture% = 0
510208 blNeedsLayout% = 1
510209 blNeedsPaint% = 1
510210 END SUB
510220 SUB blSetEnabled(H%, Enabled%)
510221 blStatus% = blStatusOk%
510222 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510223 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510224 IF Enabled% <> 0 THEN blCtl%(H%,13) = blCtl%(H%,13) OR blFlagEnabled% : GOTO blEnabledDone
510225 blCtl%(H%,13) = blCtl%(H%,13) AND (blFlagEnabled% XOR 32767)
510226 LABEL blEnabledDone
510227 blNeedsPaint% = 1
510228 END SUB
510240 SUB blSetText(H%, Text$)
510241 blStatus% = blStatusOk%
510242 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510243 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510244 blCtlText$(H%) = Text$
510245 blNeedsPaint% = 1
510246 END SUB
510250 FUNCTION blGetText$(H%)
510251 blStatus% = blStatusOk%
510252 blGetText$ = ""
510253 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT FUNCTION
510254 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT FUNCTION
510255 blGetText$ = blCtlText$(H%)
510256 END FUNCTION
510260 SUB blSetValue(H%, Value#)
510261 blStatus% = blStatusOk%
510262 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510263 IF blCtl%(H%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510264 blUser#(H%,2) = blClamp#(Value#,blUser#(H%,3),blUser#(H%,4))
510265 blNeedsPaint% = 1
510266 END SUB
510270 FUNCTION blGetValue#(H%)
510271 blStatus% = blStatusOk%
510272 blGetValue# = 0
510273 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT FUNCTION
510274 IF blCtl%(H%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT FUNCTION
510275 blGetValue# = blUser#(H%,2)
510276 END FUNCTION
510280 SUB blSetAction(H%, Action%)
510281 blStatus% = blStatusOk%
510282 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510283 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510284 blCtl%(H%,15) = Action%
510285 END SUB
510290 SUB blSetActionLine(ActionId%, Line&)
510291 blStatus% = blStatusOk%
510292 IF ActionId% < 0 OR ActionId% > 255 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510293 IF Line& < 0 OR Line& > 2147483647 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510294 blActionLine&(ActionId%) = Line&
510295 END SUB
510300 SUB blSetClickMask(H%, Mask%)
510301 blStatus% = blStatusOk%
510302 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510303 IF blCtl%(H%,1) <> blClassClickTarget% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
510304 blClick%(H%,1) = Mask%
510305 END SUB
510310 SUB blInvalidate(H%)
510311 blStatus% = blStatusOk%
510312 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510313 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510314 blNeedsPaint% = 1
510315 END SUB
510320 SUB blLayoutAll
510321 blNeedsLayout% = 0
510322 blCtl%(1,11) = 0
510323 blCtl%(1,12) = 0
510324 blCtlClipL%(1) = 0
510325 blCtlClipT%(1) = 0
510326 blCtlClipR%(1) = blCtl%(1,9) - 1
510327 blCtlClipB%(1) = blCtl%(1,10) - 1
510328 CALL blLayoutControl(1)
510329 END SUB
510330 SUB blPaintAll
510331 IF blNeedsLayout% <> 0 THEN CALL blLayoutAll
510332 blNeedsPaint% = 0
510333 CALL blPaintTree(1)
510334 END SUB
510340 SUB blUpdate(H%)
510341 blStatus% = blStatusOk%
510342 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510343 IF blCtl%(H%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510344 CALL blPaintTree(H%)
510345 END SUB
510350 SUB blAutoSize(H%)
510351 LOCAL Scale%, Pad%
510352 blStatus% = blStatusOk%
510353 IF H% < 1 OR H% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510354 IF blCtl%(H%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510355 Scale% = blUser#(H%,20)
510356 Pad% = blUser#(H%,13)
510357 blCtl%(H%,9) = TEXTWIDTH(blCtlText$(H%),Scale%) + 2 * Pad% + 2
510358 blCtl%(H%,10) = TEXTHEIGHT(blCtlText$(H%),Scale%) + 2 * Pad% + 2
510359 blNeedsLayout% = 1
510360 blNeedsPaint% = 1
510361 END SUB
510370 FUNCTION blPollInput%()
510371 LOCAL B%, BM%, Wh%, X%, Y%, R%
510372 R% = 0
510373 B% = MOUSEB
510374 X% = MOUSEPOSX
510375 Y% = MOUSEPOSY
510376 IF X% >= 0 THEN CALL blTestMove(X%,Y%)
510377 BM% = MOUSEBUTTONS AND 1
510378 IF BM% <> 0 AND blPressed% = 0 THEN CALL blTestButton(1,1,X%,Y%)
510379 IF BM% = 0 AND blPressed% <> 0 THEN CALL blTestButton(blPressed%,0,X%,Y%)
510380 Wh% = MOUSEWHEEL
510381 IF Wh% <> 0 THEN CALL blWheelAt(X%,Y%,Wh%)
510382 IF B% <> 0 THEN X% = MOUSEX : Y% = MOUSEY : R% = blClickAt%(X%,Y%,B%)
510383 blPollInput% = R%
510384 END FUNCTION
510390 FUNCTION blNextEvent%()
510391 blStatus% = blStatusOk%
510392 IF blEvCount% = 0 THEN blStatus% = blStatusEventQueueEmpty% : blCurType% = 0 : blNextEvent% = 0 : EXIT FUNCTION
510393 blCurType% = blEvType%(blEvHead%)
510394 blCurCtl% = blEvCtl%(blEvHead%)
510395 blCurAction% = blEvAction%(blEvHead%)
510396 blCurX% = blEvX%(blEvHead%)
510397 blCurY% = blEvY%(blEvHead%)
510398 blCurButton% = blEvButton%(blEvHead%)
510399 blCurWheel% = blEvWheelVal%(blEvHead%)
510400 blEvHead% = (blEvHead% + 1) MOD blMaxEvents%
510401 blEvCount% = blEvCount% - 1
510402 blNextEvent% = blCurType%
510403 END FUNCTION
510410 FUNCTION blEventCtl%()
510411 blEventCtl% = blCurCtl%
510412 END FUNCTION
510420 FUNCTION blEventAction%()
510421 blEventAction% = blCurAction%
510422 END FUNCTION
510430 FUNCTION blEventX%()
510431 blEventX% = blCurX%
510432 END FUNCTION
510440 FUNCTION blEventY%()
510441 blEventY% = blCurY%
510442 END FUNCTION
510450 FUNCTION blEventButton%()
510451 blEventButton% = blCurButton%
510452 END FUNCTION
510460 FUNCTION blEventWheel%()
510461 blEventWheel% = blCurWheel%
510462 END FUNCTION
510470 FUNCTION blEventCount%()
510471 blEventCount% = blEvCount%
510472 END FUNCTION
510480 FUNCTION blFindAt%(X%, Y%)
510481 blFindAt% = blHitTest%(X%,Y%)
510482 END FUNCTION
510490 SUB blDump
510491 LOCAL I%
510492 FOR I% = 1 TO blMaxControls%
510493   IF blCtl%(I%,1) <> 0 THEN PRINT "H"; I%; " C"; blCtl%(I%,1); " P"; blCtl%(I%,2); " X"; blCtl%(I%,11); " Y"; blCtl%(I%,12); " W"; blCtl%(I%,9); " H"; blCtl%(I%,10)
510494 NEXT I%
510495 blStatus% = blStatusOk%
510496 END SUB
510500 SUB blRegisterClass(Class%, Line&, Name$)
510501 LOCAL V%
510502 blStatus% = blStatusOk%
510503 IF Class% < 5 OR Class% > blMaxClasses% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
510504 IF Line& <= 0 OR Line& > 2147483647 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510505 FOR V% = 1 TO blMaxVerbs%
510506   blMethod&(Class%,V%) = Line&
510507 NEXT V%
510508 blClassName$(Class%) = Name$
510509 END SUB
510520 FUNCTION blTestClickAt%(X%, Y%, Button%)
510521 blTestClickAt% = blClickAt%(X%,Y%,Button%)
510522 END FUNCTION
510530 SUB blTestMouseMove(X%, Y%)
510531 CALL blTestMove(X%,Y%)
510532 END SUB
510540 SUB blTestMouseButton(Button%, Down%, X%, Y%)
510541 CALL blTestButton(Button%,Down%,X%,Y%)
510542 END SUB
510550 SUB blTestWheel(X%, Y%, Delta%)
510551 CALL blWheelAt(X%,Y%,Delta%)
510552 END SUB
512000 REM debug/test helper area
512010 LABEL blTestErl
512020 ERROR 5
512030 RETURN
512031 END
