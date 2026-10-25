500000 END
500010 CONST blVersion$ = "0.2"
500011 CONST blVersionNum# = 0.2
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
500100 SUB blFail
500101 blStatus% = blFailCode%
500102 blError% = blFailCode%
500103 END SUB
500110 FUNCTION blValidHandle%
500111 blValidHandle% = 0
500112 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN EXIT FUNCTION
500113 IF blCtl%(blCheckH%,1) = 0 THEN EXIT FUNCTION
500114 blValidHandle% = 1
500115 END FUNCTION
500120 FUNCTION blVisible%
500121 blVisible% = 0
500122 IF blCtl%(blCheckH%,1) = 0 THEN EXIT FUNCTION
500123 IF (blCtl%(blCheckH%,13) AND blFlagVisible%) = 0 THEN EXIT FUNCTION
500124 blVisible% = 1
500125 END FUNCTION
500130 FUNCTION blEnabled%
500131 blEnabled% = 0
500132 IF blCtl%(blCheckH%,1) = 0 THEN EXIT FUNCTION
500133 IF (blCtl%(blCheckH%,13) AND blFlagEnabled%) = 0 THEN EXIT FUNCTION
500134 blEnabled% = 1
500135 END FUNCTION
500140 FUNCTION blClamp#
500141 blClamp# = blClampV#
500142 IF blClamp# < blClampLo# THEN blClamp# = blClampLo#
500143 IF blClamp# > blClampHi# THEN blClamp# = blClampHi#
500144 END FUNCTION
500150 SUB blSetDirty
500151 IF blDirtyH% < 1 OR blDirtyH% > blMaxControls% THEN EXIT SUB
500152 blCtl%(blDirtyH%,13) = blCtl%(blDirtyH%,13) OR blFlagLayoutDirty%
500153 blNeedsLayout% = 1
500154 blNeedsPaint% = 1
500155 END SUB
500160 FUNCTION blAllocHandle%
500161 LOCAL blI%
500162 blAllocHandle% = 0
500163 FOR blI% = 1 TO blMaxControls%
500164   IF blCtl%(blI%,1) = 0 THEN blAllocHandle% = blI% : blStatus% = blStatusOk% : EXIT FUNCTION
500165 NEXT blI%
500166 blStatus% = blStatusControlTableFull%
500167 END FUNCTION
500170 SUB blLinkHandle
500171 LOCAL blP%, blLast%
500172 blP% = blCtl%(blLinkH%,2)
500173 IF blP% < 1 OR blP% > blMaxControls% THEN EXIT SUB
500174 blLast% = blCtl%(blP%,4)
500175 IF blLast% = 0 THEN blCtl%(blP%,3) = blLinkH% : blCtl%(blLinkH%,5) = 0 : blCtl%(blLinkH%,6) = 0 : GOTO blLinkDone
500176 blCtl%(blLast%,6) = blLinkH%
500177 blCtl%(blLinkH%,5) = blLast%
500178 LABEL blLinkDone
500179 blCtl%(blP%,4) = blLinkH%
500184 END SUB
500190 SUB blDetachHandle
500191 LOCAL blP%, blPrev%, blNext%
500192 blP% = blCtl%(blDetachH%,2)
500193 IF blP% < 1 OR blP% > blMaxControls% THEN EXIT SUB
500194 blPrev% = blCtl%(blDetachH%,5)
500195 blNext% = blCtl%(blDetachH%,6)
500196 IF blPrev% <> 0 THEN blCtl%(blPrev%,6) = blNext% ELSE blCtl%(blP%,3) = blNext%
500197 IF blNext% <> 0 THEN blCtl%(blNext%,5) = blPrev% ELSE blCtl%(blP%,4) = blPrev%
500198 blCtl%(blDetachH%,5) = 0
500199 blCtl%(blDetachH%,6) = 0
500200 END SUB
500210 SUB blInitTables
500211 LOCAL blI%, blC%, blV%, blK%
500212 IF blInitDone% <> 0 THEN GOTO blInitClear
500213 DIM blCtl%(255,17)
500214 DIM blCtlText$(255), blCtlName$(255)
500215 DIM blPanel%(255,8), blUser#(255,22), blClick%(255,4)
500216 DIM blCtlClipL%(255), blCtlClipT%(255), blCtlClipR%(255), blCtlClipB%(255)
500217 DIM blCtlZ%(255)
500218 DIM blMethod&(16,20), blActionLine&(255)
500219 DIM blClassName$(16), blClassFlags%(16), blClassFace%(16), blClassText%(16), blClassAccent%(16)
500220 DIM blEvType%(63), blEvCtl%(63), blEvAction%(63), blEvX%(63), blEvY%(63)
500221 DIM blEvButton%(63), blEvWheelVal%(63)
500222 blInitDone% = 1
500223 LABEL blInitClear
500224 FOR blI% = 0 TO 255
500225   blCtl%(blI%,1) = 0
500226   blCtl%(blI%,2) = 0
500227   blCtl%(blI%,3) = 0
500228   blCtl%(blI%,4) = 0
500229   blCtl%(blI%,5) = 0
500230   blCtl%(blI%,6) = 0
500231   blCtl%(blI%,7) = 0
500232   blCtl%(blI%,8) = 0
500233   blCtl%(blI%,9) = 0
500234   blCtl%(blI%,10) = 0
500235   blCtl%(blI%,11) = 0
500236   blCtl%(blI%,12) = 0
500237   blCtl%(blI%,13) = 0
500238   blCtl%(blI%,14) = 0
500239   blCtl%(blI%,15) = 0
500240   blCtl%(blI%,16) = 0
500241   blCtl%(blI%,17) = 0
500242   blCtlText$(blI%) = ""
500243   blCtlName$(blI%) = ""
500244   blCtlClipL%(blI%) = 0
500245   blCtlClipT%(blI%) = 0
500246   blCtlClipR%(blI%) = 0
500247   blCtlClipB%(blI%) = 0
500248   blCtlZ%(blI%) = 0
500249 NEXT blI%
500250 FOR blC% = 1 TO 16
500251   blClassFlags%(blC%) = 0
500252   blClassFace%(blC%) = 7
500253   blClassText%(blC%) = 0
500254   blClassAccent%(blC%) = 4
500255   blClassName$(blC%) = ""
500256   FOR blV% = 1 TO 20
500257     blMethod&(blC%,blV%) = 502999
500258   NEXT blV%
500259 NEXT blC%
500260 blClassName$(1) = "Top"
500261 blClassName$(2) = "Panel"
500262 blClassName$(3) = "User"
500263 blClassName$(4) = "ClickTarget"
500264 FOR blI% = 0 TO 255
500265   blActionLine&(blI%) = 0
500266 NEXT blI%
500267 FOR blI% = 0 TO 63
500268   blEvType%(blI%) = 0
500269   blEvCtl%(blI%) = 0
500270   blEvAction%(blI%) = 0
500271   blEvX%(blI%) = 0
500272   blEvY%(blI%) = 0
500273   blEvButton%(blI%) = 0
500274   blEvWheelVal%(blI%) = 0
500275 NEXT blI%
500276 END SUB
500280 SUB blInit
500281 CALL blInitTables
500282 blRoot% = 1
500283 blNeedsPaint% = 1
500284 blNeedsLayout% = 1
500285 blHover% = 0
500286 blPressed% = 0
500287 blCapture% = 0
500288 blEvHead% = 0
500289 blEvTail% = 0
500290 blEvCount% = 0
500291 blEventType% = 0
500292 blEventCtl% = 0
500293 blEventAction% = 0
500294 blEventX% = 0
500295 blEventY% = 0
500296 blEventButton% = 0
500297 blEventWheel% = 0
500298 blEventCount% = 0
500299 blRetHandle% = 0
500300 blRetN% = 0
500301 blRetN# = 0
500302 blRetS$ = ""
500303 blRetValue# = 0
500304 blStatus% = blStatusOk%
500305 blError% = 0
500306 blCtl%(1,1) = blClassTop%
500307 blCtl%(1,2) = 0
500308 blCtl%(1,3) = 0
500309 blCtl%(1,4) = 0
500310 blCtl%(1,5) = 0
500311 blCtl%(1,6) = 0
500312 blCtl%(1,7) = 0
500313 blCtl%(1,8) = 0
500314 blCtl%(1,9) = 640
500315 IF blReqW% > 0 THEN blCtl%(1,9) = blReqW%
500316 IF SCREENW > 0 THEN blCtl%(1,9) = SCREENW
500317 IF blReqW% > 0 THEN blCtl%(1,9) = blReqW%
500318 blCtl%(1,10) = 400
500319 IF blReqH% > 0 THEN blCtl%(1,10) = blReqH%
500320 IF SCREENH > 0 THEN blCtl%(1,10) = SCREENH
500321 IF blReqH% > 0 THEN blCtl%(1,10) = blReqH%
500322 blCtl%(1,11) = 0
500323 blCtl%(1,12) = 0
500324 blCtl%(1,13) = blFlagVisible% OR blFlagEnabled% OR blFlagClip%
500325 blCtl%(1,14) = 0
500326 blCtl%(1,15) = 0
500327 blCtl%(1,16) = 0
500328 blCtl%(1,17) = 0
500329 blCtlClipL%(1) = 0
500330 blCtlClipT%(1) = 0
500331 blCtlClipR%(1) = blCtl%(1,9) - 1
500332 blCtlClipB%(1) = blCtl%(1,10) - 1
500333 blCtlZ%(1) = 1
500334 blPanel%(1,1) = 0
500335 blPanel%(1,2) = 0
500336 blPanel%(1,3) = 0
500337 blPanel%(1,4) = 0
500338 blPanel%(1,6) = 7
500339 blPanel%(1,7) = 8
500340 END SUB
500350 SUB blInitPanel
500351 LOCAL blH%
500352 blH% = blInitH%
500353 blCtl%(blH%,13) = blCtl%(blH%,13) OR blFlagClip%
500354 blPanel%(blH%,1) = blReqMode%
500355 blPanel%(blH%,2) = blReqPad%
500356 blPanel%(blH%,3) = blReqGap%
500357 blPanel%(blH%,4) = blReqCols%
500358 blPanel%(blH%,6) = 7
500359 blPanel%(blH%,7) = 8
500360 END SUB
500370 SUB blInitUser
500371 LOCAL blH%
500372 blH% = blInitH%
500373 blUser#(blH%,1) = blReqKind%
500374 blUser#(blH%,2) = blReqValue#
500375 blUser#(blH%,3) = 0
500376 blUser#(blH%,4) = 100
500377 blUser#(blH%,5) = 1
500378 blUser#(blH%,6) = 0
500379 blUser#(blH%,7) = 0
500380 blUser#(blH%,8) = 7
500381 blUser#(blH%,9) = 4
500382 blUser#(blH%,10) = 0
500383 blUser#(blH%,11) = 0
500384 blUser#(blH%,12) = 0
500385 blUser#(blH%,13) = blReqPad%
500386 blUser#(blH%,14) = 1
500387 blUser#(blH%,15) = 1
500388 blUser#(blH%,16) = 0
500389 blUser#(blH%,17) = 0
500390 blUser#(blH%,18) = 1
500391 blUser#(blH%,19) = 0
500392 blUser#(blH%,20) = 1
500393 blUser#(blH%,21) = 0
500394 blUser#(blH%,22) = 1
500395 END SUB
500400 SUB blInitClick
500401 LOCAL blH%
500402 blH% = blInitH%
500403 blClick%(blH%,1) = blReqButtonMask%
500404 blClick%(blH%,2) = 0
500405 blClick%(blH%,3) = 0
500406 blClick%(blH%,4) = 0
500407 END SUB
500420 SUB blCreate
500421 LOCAL blH%
500422 blRetHandle% = 0
500423 blStatus% = blStatusOk%
500424 IF blReqClass% < 2 OR blReqClass% > 4 THEN blStatus% = blStatusInvalidClass% : EXIT SUB
500425 IF blReqParent% < 1 OR blReqParent% > blMaxControls% THEN blStatus% = blStatusInvalidParent% : EXIT SUB
500426 IF blCtl%(blReqParent%,1) = 0 THEN blStatus% = blStatusInvalidParent% : EXIT SUB
500427 IF blReqClass% = blClassUser% AND (blReqKind% < 1 OR blReqKind% > 5) THEN blStatus% = blStatusInvalidValue% : EXIT SUB
500428 blH% = blAllocHandle%()
500429 IF blH% = 0 THEN EXIT SUB
500430 blCtl%(blH%,1) = blReqClass%
500431 blCtl%(blH%,2) = blReqParent%
500432 blCtl%(blH%,3) = 0
500433 blCtl%(blH%,4) = 0
500434 blCtl%(blH%,5) = 0
500435 blCtl%(blH%,6) = 0
500436 blCtl%(blH%,7) = blReqX%
500437 blCtl%(blH%,8) = blReqY%
500438 blCtl%(blH%,9) = blReqW%
500439 blCtl%(blH%,10) = blReqH%
500440 blCtl%(blH%,11) = blReqX%
500441 blCtl%(blH%,12) = blReqY%
500442 blCtl%(blH%,13) = blReqFlags% OR blFlagVisible% OR blFlagEnabled%
500443 blCtl%(blH%,14) = 0
500444 blCtl%(blH%,15) = blReqAction%
500445 blCtl%(blH%,16) = blReqTag%
500446 blCtl%(blH%,17) = 0
500447 blCtlText$(blH%) = blReqText$
500448 blCtlName$(blH%) = ""
500449 blCtlClipL%(blH%) = 0
500450 blCtlClipT%(blH%) = 0
500451 blCtlClipR%(blH%) = 0
500452 blCtlClipB%(blH%) = 0
500453 blCtlZ%(blH%) = blH%
500454 IF blReqClass% = blClassPanel% THEN blInitH% = blH% : CALL blInitPanel : GOTO blCreateLinked
500455 IF blReqClass% = blClassUser% THEN blInitH% = blH% : CALL blInitUser : GOTO blCreateLinked
500456 IF blReqClass% = blClassClickTarget% THEN blInitH% = blH% : CALL blInitClick : GOTO blCreateLinked
500457 blStatus% = blStatusInvalidClass%
500458 EXIT SUB
500459 LABEL blCreateLinked
500460 blLinkH% = blH%
500461 CALL blLinkHandle
500462 blRetHandle% = blH%
500463 blNeedsLayout% = 1
500464 blNeedsPaint% = 1
500465 blStatus% = blStatusOk%
500466 END SUB
500480 SUB blDestroy(blDestroyIn%)
500481 LOCAL blH%, blChild%, blNext%
500482 blH% = blDestroyIn%
500483 blStatus% = blStatusOk%
500484 IF blH% < 1 OR blH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500485 IF blCtl%(blH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500486 IF blH% = blRoot% THEN blStatus% = blStatusCannotDestroyRoot% : EXIT SUB
500487 blChild% = blCtl%(blH%,3)
500488 WHILE blChild% <> 0
500489   blNext% = blCtl%(blChild%,6)
500490   CALL blDestroy(blChild%)
500491   blChild% = blNext%
500492 WEND
500493 blDetachH% = blH%
500494 CALL blDetachHandle
500495 IF blHover% = blH% THEN blHover% = 0
500496 IF blPressed% = blH% THEN blPressed% = 0
500497 IF blCapture% = blH% THEN blCapture% = 0
500498 blCtl%(blH%,1) = 0
500499 blCtl%(blH%,2) = 0
500500 blCtl%(blH%,3) = 0
500501 blCtl%(blH%,4) = 0
500502 blCtl%(blH%,5) = 0
500503 blCtl%(blH%,6) = 0
500504 blCtl%(blH%,13) = 0
500505 blCtlText$(blH%) = ""
500506 blNeedsLayout% = 1
500507 blNeedsPaint% = 1
500508 blStatus% = blStatusOk%
500509 END SUB
500520 SUB blSetBounds
500521 blCheckH% = blSetH%
500522 blCheckH% = blSetH%
500523 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500524 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500525 blCtl%(blCheckH%,7) = blSetX%
500526 blCtl%(blCheckH%,8) = blSetY%
500527 blCtl%(blCheckH%,9) = blSetW%
500528 blCtl%(blCheckH%,10) = blSetHgt%
500529 blDirtyH% = blCheckH%
500530 CALL blSetDirty
500531 blStatus% = blStatusOk%
500532 END SUB
500540 SUB blShow
500541 blCheckH% = blShowH%
500542 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500543 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500544 blCtl%(blCheckH%,13) = blCtl%(blCheckH%,13) OR blFlagVisible%
500545 blNeedsLayout% = 1
500546 blNeedsPaint% = 1
500547 blStatus% = blStatusOk%
500548 END SUB
500550 SUB blHide
500551 blCheckH% = blHideH%
500552 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500553 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500554 blCtl%(blCheckH%,13) = blCtl%(blCheckH%,13) AND (blFlagVisible% XOR 32767)
500555 IF blHover% = blCheckH% THEN blHover% = 0
500556 IF blPressed% = blCheckH% THEN blPressed% = 0
500557 IF blCapture% = blCheckH% THEN blCapture% = 0
500558 blNeedsLayout% = 1
500559 blNeedsPaint% = 1
500560 blStatus% = blStatusOk%
500561 END SUB
500570 SUB blSetEnabled
500571 blCheckH% = blEnabledH%
500572 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500573 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500574 IF blReqEnabled% <> 0 THEN blCtl%(blCheckH%,13) = blCtl%(blCheckH%,13) OR blFlagEnabled% : GOTO blEnabledDone
500575 blCtl%(blCheckH%,13) = blCtl%(blCheckH%,13) AND (blFlagEnabled% XOR 32767)
500576 LABEL blEnabledDone
500577 blNeedsPaint% = 1
500578 blStatus% = blStatusOk%
500579 END SUB
500590 SUB blSetText
500591 blCheckH% = blTextH%
500592 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500593 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500594 blCtlText$(blCheckH%) = blTextIn$
500595 blNeedsPaint% = 1
500596 blStatus% = blStatusOk%
500597 END SUB
500600 SUB blSetValue
500601 blCheckH% = blValueH%
500602 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500603 IF blCtl%(blCheckH%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT SUB
500604 blClampV# = blValueIn#
500605 blClampLo# = blUser#(blCheckH%,3)
500606 blClampHi# = blUser#(blCheckH%,4)
500607 blValueOut# = blClamp#()
500608 blUser#(blCheckH%,2) = blValueOut#
500609 blNeedsPaint% = 1
500610 blStatus% = blStatusOk%
500611 END SUB
500620 SUB blGetValue
500621 blCheckH% = blValueH%
500622 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500623 IF blCtl%(blCheckH%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT SUB
500624 blRetValue# = blUser#(blCheckH%,2)
500625 blStatus% = blStatusOk%
500626 END SUB
500640 SUB blSetLayout
500641 blCheckH% = blLayoutH%
500642 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500643 IF blCtl%(blCheckH%,1) <> blClassPanel% AND blCtl%(blCheckH%,1) <> blClassTop% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
500644 blPanel%(blCheckH%,1) = blReqMode%
500645 blPanel%(blCheckH%,2) = blReqPad%
500646 blPanel%(blCheckH%,3) = blReqGap%
500647 blDirtyH% = blCheckH%
500648 CALL blSetDirty
500649 blStatus% = blStatusOk%
500650 END SUB
500660 SUB blSetAction
500661 blCheckH% = blActionH%
500662 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500663 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
500664 blCtl%(blCheckH%,15) = blActionIn%
500665 blStatus% = blStatusOk%
500666 END SUB
500680 SUB blSetActionLine
500681 IF blReqActionId% < 0 OR blReqActionId% > 255 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
500682 IF blReqLine& < 0 OR blReqLine& > 2147483647 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
500683 blActionLine&(blReqActionId%) = blReqLine&
500684 blStatus% = blStatusOk%
500685 END SUB
500700 SUB blLayoutAll
500701 IF blRoot% < 1 THEN EXIT SUB
500702 blNeedsLayout% = 0
500703 blCtl%(blRoot%,11) = 0
500704 blCtl%(blRoot%,12) = 0
500705 blCtlClipL%(blRoot%) = 0
500706 blCtlClipT%(blRoot%) = 0
500707 blCtlClipR%(blRoot%) = blCtl%(blRoot%,9) - 1
500708 blCtlClipB%(blRoot%) = blCtl%(blRoot%,10) - 1
500709 CALL blLayoutControl(blRoot%)
500710 END SUB
500720 SUB blLayoutControl(blLayoutCtlH%)
500721 LOCAL blC%
500722 blCheckH% = blLayoutCtlH%
500723 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN EXIT SUB
500724 IF blCtl%(blCheckH%,1) = 0 THEN EXIT SUB
500725 IF (blCtl%(blCheckH%,13) AND blFlagVisible%) = 0 THEN EXIT SUB
500726 blC% = blCtl%(blCheckH%,1)
500727 IF blC% = blClassTop% OR blC% = blClassPanel% THEN CALL blLayoutChildren(blCheckH%)
500728 END SUB
500740 SUB blLayoutChildren(blLayoutCtlH%)
500741 LOCAL blMode%, blPad%, blGap%, blPX%, blPY%, blCW%, blCH%, blCX%, blCY%
500742 LOCAL blChild%, blNext%, blX2%, blY2%, blL%, blT%, blR%, blB%
500743 blCheckH% = blLayoutCtlH%
500744 blMode% = blPanel%(blCheckH%,1)
500745 blPad% = blPanel%(blCheckH%,2)
500746 blGap% = blPanel%(blCheckH%,3)
500747 blPX% = blCtl%(blCheckH%,11)
500748 blPY% = blCtl%(blCheckH%,12)
500749 blCW% = blCtl%(blCheckH%,9)
500750 blCH% = blCtl%(blCheckH%,10)
500751 blCX% = blPX% + blPad%
500752 blCY% = blPY% + blPad%
500753 blChild% = blCtl%(blCheckH%,3)
500754 WHILE blChild% <> 0
500755   IF (blCtl%(blChild%,13) AND blFlagVisible%) = 0 THEN GOTO blLayoutNext
500756     SELECT CASE blMode%
500757     CASE 1
500758       blCtl%(blChild%,7) = blPad%
500759       blCtl%(blChild%,8) = blCY% - blPY%
500760       blCtl%(blChild%,9) = blCW% - 2 * blPad%
500761       blCY% = blCY% + blCtl%(blChild%,10) + blGap%
500762     CASE 2
500763       blCtl%(blChild%,7) = blCX% - blPX%
500764       blCtl%(blChild%,8) = blPad%
500765       blCtl%(blChild%,10) = blCH% - 2 * blPad%
500766       blCX% = blCX% + blCtl%(blChild%,9) + blGap%
500767     CASE ELSE
500768       blCtl%(blChild%,7) = blCtl%(blChild%,7)
500769       blCtl%(blChild%,8) = blCtl%(blChild%,8)
500770     END SELECT
500771     blCtl%(blChild%,11) = blPX% + blCtl%(blChild%,7)
500772     blCtl%(blChild%,12) = blPY% + blCtl%(blChild%,8)
500773     blL% = blCtlClipL%(blCheckH%)
500774     blT% = blCtlClipT%(blCheckH%)
500775     blR% = blCtlClipR%(blCheckH%)
500776     blB% = blCtlClipB%(blCheckH%)
500777     IF blCtl%(blChild%,11) > blL% THEN blL% = blCtl%(blChild%,11)
500778     IF blCtl%(blChild%,12) > blT% THEN blT% = blCtl%(blChild%,12)
500779     blX2% = blCtl%(blChild%,11) + blCtl%(blChild%,9) - 1
500780     blY2% = blCtl%(blChild%,12) + blCtl%(blChild%,10) - 1
500781     IF blX2% < blR% THEN blR% = blX2%
500782     IF blY2% < blB% THEN blB% = blY2%
500783     blCtlClipL%(blChild%) = blL%
500784     blCtlClipT%(blChild%) = blT%
500785     blCtlClipR%(blChild%) = blR%
500786     blCtlClipB%(blChild%) = blB%
500787     blNext% = blCtl%(blChild%,6)
500788     CALL blLayoutControl(blChild%)
500789     blChild% = blNext%
500790     GOTO blLayoutLoopEnd
500791   LABEL blLayoutNext
500792     blChild% = blCtl%(blChild%,6)
500793   LABEL blLayoutLoopEnd
500794 WEND
500795 END SUB
500800 SUB blPaintAll
500801 IF blNeedsLayout% <> 0 THEN CALL blLayoutAll
500802 blNeedsPaint% = 0
500803 CALL blPaintTree(blRoot%)
500804 END SUB
500810 SUB blPaintTree(blPaintH%)
500811 LOCAL blChild%, blNext%
500812 blCheckH% = blPaintH%
500813 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN EXIT SUB
500814 IF blCtl%(blCheckH%,1) = 0 THEN EXIT SUB
500815 IF (blCtl%(blCheckH%,13) AND blFlagVisible%) = 0 THEN EXIT SUB
500816 CALL blPaintOne(blCheckH%)
500817 blChild% = blCtl%(blCheckH%,3)
500818 WHILE blChild% <> 0
500819   blNext% = blCtl%(blChild%,6)
500820   CALL blPaintTree(blChild%)
500821   blChild% = blNext%
500822 WEND
500823 END SUB
500830 SUB blPaintOne(blPaintOneH%)
500831 LOCAL blW%, blHH%, blCls%
500832 blCheckH% = blPaintOneH%
500833 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN EXIT SUB
500834 IF blCtl%(blCheckH%,1) = 0 THEN EXIT SUB
500835 blW% = blCtl%(blCheckH%,9)
500836 blHH% = blCtl%(blCheckH%,10)
500837 IF blW% <= 0 OR blHH% <= 0 THEN EXIT SUB
500838 PUSH VIEW
500839 blCls% = blCtl%(blCheckH%,1)
500840 IF blCls% = blClassTop% OR blCls% = blClassPanel% THEN CALL blPaintPanel(blCheckH%) : GOTO blPaintOneDone
500841 IF blCls% = blClassUser% THEN CALL blPaintUser(blCheckH%)
500842 LABEL blPaintOneDone
500843 POP VIEW
500844 END SUB
500860 SUB blPaintPanel(blPaintCtlH%)
500861 LOCAL blX%, blY%, blW%, blHH%, blFace%, blBorder%
500862 blCheckH% = blPaintCtlH%
500863 blX% = blCtl%(blCheckH%,11)
500864 blY% = blCtl%(blCheckH%,12)
500865 blW% = blCtl%(blCheckH%,9)
500866 blHH% = blCtl%(blCheckH%,10)
500867 blFace% = blPanel%(blCheckH%,6)
500868 blBorder% = blPanel%(blCheckH%,7)
500869 LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
500870 IF blBorder% >= 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blBorder%, B
500871 END SUB
500880 SUB blPaintUser(blPaintCtlH%)
500881 LOCAL blKind%
500882 blCheckH% = blPaintCtlH%
500883 blKind% = blUser#(blCheckH%,1)
500884 SELECT CASE blKind%
500885 CASE blKindButton%
500886   CALL blPaintButton(blCheckH%)
500887 CASE blKindLabel%
500888   CALL blPaintLabel(blCheckH%)
500889 CASE blKindSlider%
500890   CALL blPaintSlider(blCheckH%)
500891 CASE blKindGauge%
500892   CALL blPaintGauge(blCheckH%)
500893 CASE blKindDial%
500894   CALL blPaintDial(blCheckH%)
500895 END SELECT
500896 END SUB
500910 SUB blPaintButton(blPaintCtlH%)
500911 LOCAL blX%, blY%, blW%, blHH%, blFace%, blCol%, blBorder%, blScale%
500912 blCheckH% = blPaintCtlH%
500913 blX% = blCtl%(blCheckH%,11)
500914 blY% = blCtl%(blCheckH%,12)
500915 blW% = blCtl%(blCheckH%,9)
500916 blHH% = blCtl%(blCheckH%,10)
500917 blFace% = blUser#(blCheckH%,8)
500918 blCol% = blUser#(blCheckH%,19)
500919 blBorder% = blUser#(blCheckH%,17)
500920 blScale% = blUser#(blCheckH%,20)
500921 IF (blCtl%(blCheckH%,13) AND blFlagPressed%) <> 0 THEN blFace% = blUser#(blCheckH%,9)
500922 LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
500923 IF blBorder% >= 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blBorder%, B
500924 CALL blDrawText(blX%,blY%,blW%,blHH%,blCtlText$(blCheckH%),blCol%,blScale%,1,1,2)
500925 END SUB
500930 SUB blPaintLabel(blPaintCtlH%)
500931 LOCAL blX%, blY%, blW%, blHH%, blFace%, blCol%, blBorder%, blScale%, blAlign%, blVAlign%, blPad%, blT$
500932 blCheckH% = blPaintCtlH%
500933 blX% = blCtl%(blCheckH%,11)
500934 blY% = blCtl%(blCheckH%,12)
500935 blW% = blCtl%(blCheckH%,9)
500936 blHH% = blCtl%(blCheckH%,10)
500937 blFace% = blUser#(blCheckH%,8)
500938 blCol% = blUser#(blCheckH%,19)
500939 blBorder% = blUser#(blCheckH%,17)
500940 blScale% = blUser#(blCheckH%,20)
500941 blAlign% = blUser#(blCheckH%,14)
500942 blVAlign% = blUser#(blCheckH%,15)
500943 blPad% = blUser#(blCheckH%,13)
500944 blT$ = blCtlText$(blCheckH%)
500945 IF blUser#(blCheckH%,16) <> 0 THEN blT$ = blWrapText$(blT$,blW% - 2 * blPad%,blScale%)
500946 IF blUser#(blCheckH%,18) = 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
500947 IF blBorder% >= 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blBorder%, B
500948 CALL blDrawText(blX%,blY%,blW%,blHH%,blT$,blCol%,blScale%,blAlign%,blVAlign%,blPad%)
500949 END SUB
500960 SUB blPaintSlider(blPaintCtlH%)
500961 LOCAL blX%, blY%, blW%, blHH%, blPos%, blAccent%
500962 blCheckH% = blPaintCtlH%
500963 blX% = blCtl%(blCheckH%,11)
500964 blY% = blCtl%(blCheckH%,12)
500965 blW% = blCtl%(blCheckH%,9)
500966 blHH% = blCtl%(blCheckH%,10)
500967 blAccent% = blUser#(blCheckH%,9)
500968 IF blW% <= 0 THEN EXIT SUB
500969 LINE (blX%,blY% + blHH% / 2)-(blX% + blW% - 1,blY% + blHH% / 2), 8, BF
500970 blPos% = blX% + (blW% - 8) * (blUser#(blCheckH%,2) - blUser#(blCheckH%,3)) / (blUser#(blCheckH%,4) - blUser#(blCheckH%,3) + 0.000001)
500971 LINE (blPos%,blY% + 2)-(blPos% + 7,blY% + blHH% - 2), blAccent%, BF
500972 END SUB
500980 SUB blPaintGauge(blPaintCtlH%)
500981 LOCAL blX%, blY%, blW%, blHH%, blFace%, blAccent%, blPos%
500982 blCheckH% = blPaintCtlH%
500983 blX% = blCtl%(blCheckH%,11)
500984 blY% = blCtl%(blCheckH%,12)
500985 blW% = blCtl%(blCheckH%,9)
500986 blHH% = blCtl%(blCheckH%,10)
500987 blFace% = blUser#(blCheckH%,8)
500988 blAccent% = blUser#(blCheckH%,9)
500989 LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
500990 blPos% = blX% + (blW% - 2) * (blUser#(blCheckH%,2) - blUser#(blCheckH%,3)) / (blUser#(blCheckH%,4) - blUser#(blCheckH%,3) + 0.000001)
500991 LINE (blX% + 1,blY% + 1)-(blPos%,blY% + blHH% - 2), blAccent%, BF
500992 END SUB
501000 SUB blPaintDial(blPaintCtlH%)
501001 LOCAL blX%, blY%, blW%, blHH%, blFace%, blAccent%, blR%, blCX%, blCY%, blA#, blEX%, blEY%
501002 blCheckH% = blPaintCtlH%
501003 blX% = blCtl%(blCheckH%,11)
501004 blY% = blCtl%(blCheckH%,12)
501005 blW% = blCtl%(blCheckH%,9)
501006 blHH% = blCtl%(blCheckH%,10)
501007 blFace% = blUser#(blCheckH%,8)
501008 blAccent% = blUser#(blCheckH%,9)
501009 blR% = blHH% / 2 - 2
501010 IF blW% < blHH% THEN blR% = blW% / 2 - 2
501011 blCX% = blX% + blW% / 2
501012 blCY% = blY% + blHH% / 2
501013 CIRCLE (blCX%,blCY%), blR%, blFace%, 0, 6.2831853, 1
501014 CIRCLE (blCX%,blCY%), blR% - 1, blAccent%, 0, 6.2831853, 1
501015 blA# = 3.14159265 * (1 - (blUser#(blCheckH%,2) - blUser#(blCheckH%,3)) / (blUser#(blCheckH%,4) - blUser#(blCheckH%,3) + 0.000001))
501016 blEX% = blCX% + COS(blA#) * (blR% - 4)
501017 blEY% = blCY% - SIN(blA#) * (blR% - 4)
501018 LINE (blCX%,blCY%)-(blEX%,blEY%), blAccent%
501019 END SUB
501030 SUB blDrawText(blTextBoxX%,blTextBoxY%,blTextBoxW%,blTextBoxH%,blTextIn$,blTextColor%,blTextScale%,blTextAlign%,blTextVAlign%,blTextPad%)
501031 LOCAL blT$, blTW%, blTH%, blDX%, blDY%, blFitW%
501032 IF blTextIn$ = "" THEN EXIT SUB
501033 blFitW% = blTextBoxW% - 2 * blTextPad%
501034 IF blFitW% < 1 THEN blFitW% = 1
501035 blT$ = blFitText$(blTextIn$,blFitW%,blTextScale%)
501036 blTW% = TEXTWIDTH(blT$,blTextScale%)
501037 blTH% = TEXTHEIGHT(blT$,blTextScale%)
501038 blDX% = blTextBoxX% + blTextPad%
501039 blDY% = blTextBoxY% + blTextPad%
501040 IF blTextAlign% = 1 THEN blDX% = blTextBoxX% + (blTextBoxW% - blTW%) / 2
501041 IF blTextAlign% = 2 THEN blDX% = blTextBoxX% + blTextBoxW% - blTW% - blTextPad%
501042 IF blTextVAlign% = 1 THEN blDY% = blTextBoxY% + (blTextBoxH% - blTH%) / 2
501043 IF blTextVAlign% = 2 THEN blDY% = blTextBoxY% + blTextBoxH% - blTH% - blTextPad%
501044 DRAWTEXT (blDX%,blDY%), blT$, blTextColor%, blTextScale%
501045 END SUB
501060 FUNCTION blFitText$(blFitSource$,blFitMax%,blFitScale%)
501061 LOCAL blIn$, blOut$, blLine$, blCh$, blI%
501062 blFitText$ = ""
501063 IF blFitMax% < 1 THEN blFitMax% = 1
501064 IF blFitSource$ = "" THEN EXIT FUNCTION
501065 blIn$ = blFitSource$
501066 blOut$ = ""
501067 blLine$ = ""
501068 FOR blI% = 1 TO LEN(blIn$)
501069   blCh$ = MID$(blIn$,blI%,1)
501070   IF blCh$ = CHR$(13) OR blCh$ = CHR$(10) THEN GOTO blFitNewline
501071   blLine$ = blLine$ + blCh$
501072   GOTO blFitNext
501073   LABEL blFitNewline
501074   WHILE LEN(blLine$) > 0 AND TEXTWIDTH(blLine$,blFitScale%) > blFitMax%
501075     blLine$ = LEFT$(blLine$,LEN(blLine$) - 1)
501076   WEND
501077   blOut$ = blOut$ + blLine$ + CHR$(13) + CHR$(10)
501078   blLine$ = ""
501079   LABEL blFitNext
501080 NEXT blI%
501081 WHILE LEN(blLine$) > 0 AND TEXTWIDTH(blLine$,blFitScale%) > blFitMax%
501082   blLine$ = LEFT$(blLine$,LEN(blLine$) - 1)
501083 WEND
501084 blFitText$ = blOut$ + blLine$
501085 END FUNCTION
501100 FUNCTION blWrapText$(blWrapSource$,blWrapMax%,blWrapScale%)
501101 LOCAL blIn$, blOut$, blLine$, blCh$, blI%, blMax%
501102 blWrapText$ = ""
501103 blIn$ = blWrapSource$
501104 blOut$ = ""
501105 blLine$ = ""
501106 blMax% = blWrapMax%
501107 IF blMax% < 1 THEN blMax% = 1
501108 FOR blI% = 1 TO LEN(blIn$)
501109   blCh$ = MID$(blIn$,blI%,1)
501110   IF blCh$ = CHR$(13) OR blCh$ = CHR$(10) THEN GOTO blWrapNewline
501111   IF LEN(blLine$) > 0 AND TEXTWIDTH(blLine$ + blCh$,blWrapScale%) > blMax% THEN GOTO blWrapBreak
501112   blLine$ = blLine$ + blCh$
501113   GOTO blWrapNext
501114   LABEL blWrapBreak
501115   blOut$ = blOut$ + blLine$ + CHR$(13) + CHR$(10)
501116   blLine$ = blCh$
501117   GOTO blWrapNext
501118   LABEL blWrapNewline
501119   blOut$ = blOut$ + blLine$ + CHR$(13) + CHR$(10)
501120   blLine$ = ""
501121   LABEL blWrapNext
501122 NEXT blI%
501123 blWrapText$ = blOut$ + blLine$
501124 END FUNCTION
501140 SUB blAutoSize
501150 LOCAL blScale%, blPad%
501151 blCheckH% = blAutoH%
501152 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
501153 IF blCtl%(blCheckH%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT SUB
501154 blScale% = blUser#(blCheckH%,20)
501155 blPad% = blUser#(blCheckH%,13)
501156 blCtl%(blCheckH%,9) = TEXTWIDTH(blCtlText$(blCheckH%),blScale%) + 2 * blPad% + 2
501157 blCtl%(blCheckH%,10) = TEXTHEIGHT(blCtlText$(blCheckH%),blScale%) + 2 * blPad% + 2
501158 blNeedsLayout% = 1
501159 blNeedsPaint% = 1
501160 blStatus% = blStatusOk%
501161 END SUB
501180 FUNCTION blHitTest%
501181 LOCAL blH%, blX1%, blY1%, blX2%, blY2%
501182 blHitTest% = 0
501183 FOR blH% = blMaxControls% TO 1 STEP -1
501184   IF blCtl%(blH%,1) = 0 OR (blCtl%(blH%,13) AND blFlagVisible%) = 0 OR (blCtl%(blH%,13) AND blFlagEnabled%) = 0 THEN GOTO blHitNext
501185   blX1% = blCtl%(blH%,11)
501186   blY1% = blCtl%(blH%,12)
501187   blX2% = blX1% + blCtl%(blH%,9) - 1
501188   blY2% = blY1% + blCtl%(blH%,10) - 1
501189   IF blHitX% >= blX1% AND blHitX% <= blX2% AND blHitY% >= blY1% AND blHitY% <= blY2% THEN blHitTest% = blH% : EXIT FUNCTION
501190   LABEL blHitNext
501191 NEXT blH%
501192 END FUNCTION
501200 SUB blPostEvent
501201 IF blEvCount% >= blMaxEvents% THEN blStatus% = blStatusEventQueueFull% : EXIT SUB
501202 blEvType%(blEvTail%) = blPostType%
501203 blEvCtl%(blEvTail%) = blPostCtl%
501204 blEvAction%(blEvTail%) = blPostAction%
501205 blEvX%(blEvTail%) = blPostX%
501206 blEvY%(blEvTail%) = blPostY%
501207 blEvButton%(blEvTail%) = blPostButton%
501208 blEvWheelVal%(blEvTail%) = blPostWheel%
501209 blEvTail% = (blEvTail% + 1) MOD blMaxEvents%
501210 blEvCount% = blEvCount% + 1
501211 blEventCount% = blEvCount%
501212 END SUB
501220 SUB blReadEvent
501221 IF blEvCount% = 0 THEN blStatus% = blStatusEventQueueEmpty% : blEventType% = 0 : blEventCtl% = 0 : blEventAction% = 0 : blEventX% = 0 : blEventY% = 0 : blEventButton% = 0 : blEventWheel% = 0 : blEventCount% = 0 : EXIT SUB
501222 blEventType% = blEvType%(blEvHead%)
501223 blEventCtl% = blEvCtl%(blEvHead%)
501224 blEventAction% = blEvAction%(blEvHead%)
501225 blEventX% = blEvX%(blEvHead%)
501226 blEventY% = blEvY%(blEvHead%)
501227 blEventButton% = blEvButton%(blEvHead%)
501228 blEventWheel% = blEvWheelVal%(blEvHead%)
501229 blEvHead% = (blEvHead% + 1) MOD blMaxEvents%
501230 blEvCount% = blEvCount% - 1
501231 blEventCount% = blEvCount%
501232 blStatus% = blStatusOk%
501233 END SUB
501240 SUB blPostClick
501241 blPostType% = blEvClick%
501242 blPostCtl% = blClickOwner%
501243 blPostAction% = blClickAction%
501244 blPostX% = blClickPosX%
501245 blPostY% = blClickPosY%
501246 blPostButton% = blClickBtn%
501247 blPostWheel% = 0
501248 CALL blPostEvent
501249 END SUB
501260 SUB blPointerValue
501261 LOCAL blKind%, blV#, blCtlH%, blW%
501262 blCtlH% = blPtrH%
501263 blKind% = blUser#(blCtlH%,1)
501264 IF blKind% <> blKindSlider% AND blKind% <> blKindDial% THEN EXIT SUB
501265 blW% = blCtl%(blCtlH%,9)
501266 IF blW% <= 0 THEN EXIT SUB
501267 blV# = blUser#(blCtlH%,3) + (blPtrX% - blCtl%(blCtlH%,11)) * (blUser#(blCtlH%,4) - blUser#(blCtlH%,3)) / blW%
501268 blClampV# = blV#
501269 blClampLo# = blUser#(blCtlH%,3)
501270 blClampHi# = blUser#(blCtlH%,4)
501271 blV# = blClamp#()
501272 blUser#(blCtlH%,2) = blV#
501273 blPostType% = blEvChange%
501274 blPostCtl% = blCtlH%
501275 blPostAction% = blCtl%(blCtlH%,15)
501276 blPostX% = blPtrX%
501277 blPostY% = blPtrY%
501278 blPostButton% = blPtrButton%
501279 blPostWheel% = 0
501280 CALL blPostEvent
501281 blNeedsPaint% = 1
501282 END SUB
501300 SUB blClickAt
501301 LOCAL blH%, blAction%, blP%
501302 blHitX% = blClickX%
501303 blHitY% = blClickY%
501304 blH% = blHitTest%()
501305 blClickH% = blH%
501306 IF blH% = 0 THEN EXIT SUB
501307 IF blCtl%(blH%,1) = blClassUser% THEN blPtrH% = blH% : blPtrX% = blClickX% : blPtrY% = blClickY% : blPtrButton% = blClickButton% : CALL blPointerValue
501314 blAction% = blCtl%(blH%,15)
501315 blP% = blCtl%(blH%,2)
501316 WHILE blAction% = 0 AND blP% <> 0
501317   IF blCtl%(blP%,15) <> 0 THEN blAction% = blCtl%(blP%,15) : blH% = blP% : blP% = 0 ELSE blP% = blCtl%(blP%,2)
501324 WEND
501325 blClickH% = blH%
501326 IF blAction% <> 0 THEN blClickOwner% = blH% : blClickAction% = blAction% : blClickPosX% = blClickX% : blClickPosY% = blClickY% : blClickBtn% = blClickButton% : CALL blPostClick
501334 blRetN% = 1
501335 END SUB
501340 SUB blPostHover
501341 IF blHoverOld% <> 0 THEN blPostType% = blEvLeave% : blPostCtl% = blHoverOld% : blPostAction% = 0 : blPostX% = blHoverX% : blPostY% = blHoverY% : blPostButton% = 0 : blPostWheel% = 0 : CALL blPostEvent
501351 IF blHoverNew% <> 0 THEN blPostType% = blEvEnter% : blPostCtl% = blHoverNew% : blPostAction% = 0 : blPostX% = blHoverX% : blPostY% = blHoverY% : blPostButton% = 0 : blPostWheel% = 0 : CALL blPostEvent
501361 END SUB
501370 SUB blPostMove
501371 blPostType% = blEvMove%
501372 blPostCtl% = blMoveH%
501373 blPostAction% = 0
501374 blPostX% = blMoveX%
501375 blPostY% = blMoveY%
501376 blPostButton% = 0
501377 blPostWheel% = 0
501378 CALL blPostEvent
501379 END SUB
501390 SUB blTestMove
501391 LOCAL blOld%, blH%
501392 blOld% = blHover%
501393 blHitX% = blTestX%
501394 blHitY% = blTestY%
501395 blH% = blHitTest%()
501396 blHover% = blH%
501397 IF blCapture% <> 0 AND blPressed% <> 0 THEN blPtrH% = blCapture% : blPtrX% = blTestX% : blPtrY% = blTestY% : blPtrButton% = blPressed% : CALL blPointerValue
501404 IF blOld% <> blHover% THEN blHoverOld% = blOld% : blHoverNew% = blHover% : blHoverX% = blTestX% : blHoverY% = blTestY% : CALL blPostHover
501411 IF blHover% <> 0 THEN blMoveH% = blHover% : blMoveX% = blTestX% : blMoveY% = blTestY% : CALL blPostMove
501417 END SUB
501430 SUB blTestButton
501431 IF blTestDown% = 0 THEN GOTO blButtonUp
501432 blPressed% = blTestBtn%
501433 blClickX% = blTestX%
501434 blClickY% = blTestY%
501435 IF blHover% <> 0 THEN blCapture% = blHover%
501436 blPostType% = blEvDown%
501437 blPostCtl% = blHover%
501438 blPostAction% = 0
501439 blPostX% = blTestX%
501440 blPostY% = blTestY%
501441 blPostButton% = blTestBtn%
501442 blPostWheel% = 0
501443 CALL blPostEvent
501444 GOTO blButtonDone
501445 LABEL blButtonUp
501446 blPostType% = blEvUp%
501447 blPostCtl% = blCapture%
501448 blPostAction% = 0
501449 blPostX% = blTestX%
501450 blPostY% = blTestY%
501451 blPostButton% = blTestBtn%
501452 blPostWheel% = 0
501453 CALL blPostEvent
501454 blPressed% = 0
501455 blCapture% = 0
501456 LABEL blButtonDone
501457 END SUB
501470 SUB blTestWheel
501471 IF blHover% = 0 THEN EXIT SUB
501472 blPostType% = blEvWheel%
501473 blPostCtl% = blHover%
501474 blPostAction% = 0
501475 blPostX% = blTestX%
501476 blPostY% = blTestY%
501477 blPostButton% = 0
501478 blPostWheel% = blTestWheelDelta%
501479 CALL blPostEvent
501480 END SUB
501490 SUB blPollInput
501491 LOCAL blB%, blBM%, blWh%
501492 blRetN% = 0
501493 blB% = MOUSEB
501494 blTestX% = MOUSEPOSX
501495 blTestY% = MOUSEPOSY
501496 IF blTestX% >= 0 THEN CALL blTestMove
501497 blBM% = MOUSEBUTTONS AND 1
501498 IF blBM% <> 0 AND blPressed% = 0 THEN blTestBtn% = 1 : blTestDown% = 1 : CALL blTestButton
501502 IF blBM% = 0 AND blPressed% <> 0 THEN blTestBtn% = blPressed% : blTestDown% = 0 : CALL blTestButton
501507 blWh% = MOUSEWHEEL
501508 IF blWh% <> 0 THEN blTestWheelDelta% = blWh% : CALL blTestWheel
501512 IF blB% <> 0 THEN blTestX% = MOUSEX : blTestY% = MOUSEY : blClickX% = blTestX% : blClickY% = blTestY% : blClickButton% = blB% : CALL blClickAt : blRetN% = 1
501521 END SUB
502000 SUB blSetActionLineApi
502001 CALL blSetActionLine
502002 END SUB
502010 SUB blRegisterClass
502011 LOCAL blV%
502012 IF blReqClass% < 5 OR blReqClass% > blMaxClasses% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
502013 IF blReqLine& <= 0 OR blReqLine& > 2147483647 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
502014 FOR blV% = 1 TO blMaxVerbs%
502015   blMethod&(blReqClass%,blV%) = blReqLine&
502016 NEXT blV%
502017 blClassName$(blReqClass%) = blReqText$
502018 blStatus% = blStatusOk%
502019 END SUB
502030 SUB blDump
502031 LOCAL blI%
502032 FOR blI% = 1 TO blMaxControls%
502033   IF blCtl%(blI%,1) <> 0 THEN PRINT "H"; blI%; " C"; blCtl%(blI%,1); " P"; blCtl%(blI%,2); " X"; blCtl%(blI%,11); " Y"; blCtl%(blI%,12); " W"; blCtl%(blI%,9); " H"; blCtl%(blI%,10)
502034 NEXT blI%
502035 blStatus% = blStatusOk%
502036 END SUB
502040 SUB blUpdate
502041 blCheckH% = blUpdateH%
502042 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
502043 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
502044 blPaintH% = blCheckH%
502045 CALL blPaintTree(blPaintH%)
502046 blStatus% = blStatusOk%
502047 END SUB
502050 SUB blInvalidate
502051 blCheckH% = blInvalidateH%
502052 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
502053 IF blCtl%(blCheckH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
502054 blNeedsPaint% = 1
502055 blStatus% = blStatusOk%
502056 END SUB
502999 LABEL blNoop
503000 RETURN
510000 LABEL blApiInit
510001 CALL blInit
510002 RETURN
510100 LABEL blApiCreate
510101 CALL blCreate
510102 RETURN
510200 LABEL blApiDestroy
510201 blDestroyIn% = blReqHandle%
510202 CALL blDestroy(blDestroyIn%)
510203 RETURN
510300 LABEL blApiSetBounds
510301 blSetH% = blReqHandle% : blSetX% = blReqX% : blSetY% = blReqY% : blSetW% = blReqW% : blSetHgt% = blReqH%
510302 CALL blSetBounds
510303 RETURN
510400 LABEL blApiShow
510401 blShowH% = blReqHandle%
510402 CALL blShow
510403 RETURN
510500 LABEL blApiHide
510501 blHideH% = blReqHandle%
510502 CALL blHide
510503 RETURN
510600 LABEL blApiSetText
510601 blTextH% = blReqHandle% : blTextIn$ = blReqText$
510602 CALL blSetText
510603 RETURN
510700 LABEL blApiSetValue
510701 blValueH% = blReqHandle% : blValueIn# = blReqValue#
510702 CALL blSetValue
510703 RETURN
510800 LABEL blApiGetValue
510801 blValueH% = blReqHandle%
510802 CALL blGetValue
510803 RETURN
510900 LABEL blApiSetLayout
510901 blLayoutH% = blReqHandle%
510902 CALL blSetLayout
510903 RETURN
511000 LABEL blApiSetAction
511001 blActionH% = blReqHandle% : blActionIn% = blReqAction%
511002 CALL blSetAction
511003 RETURN
511100 LABEL blApiInvalidate
511101 blInvalidateH% = blReqHandle%
511102 CALL blInvalidate
511103 RETURN
511200 LABEL blApiLayoutAll
511201 CALL blLayoutAll
511202 blStatus% = blStatusOk%
511203 RETURN
511300 LABEL blApiPaintAll
511301 CALL blPaintAll
511302 blStatus% = blStatusOk%
511303 RETURN
511400 LABEL blApiUpdate
511401 blUpdateH% = blReqHandle%
511402 CALL blUpdate
511403 RETURN
511500 LABEL blApiPollInput
511501 CALL blPollInput
511502 blStatus% = blStatusOk%
511503 RETURN
511550 LABEL blApiPollMouse
511551 GOTO blApiPollInput
511600 LABEL blApiNextEvent
511601 CALL blReadEvent
511602 RETURN
511700 LABEL blApiFindAt
511701 blHitX% = blReqX% : blHitY% = blReqY%
511702 blRetHandle% = blHitTest%()
511703 blStatus% = blStatusOk%
511704 RETURN
511800 LABEL blApiTestClickAt
511801 blClickX% = blReqX% : blClickY% = blReqY% : blClickButton% = blReqButton%
511802 CALL blClickAt
511803 blRetHandle% = blClickH%
511804 blStatus% = blStatusOk%
511805 RETURN
511810 LABEL blApiTestMouseMove
511811 blTestX% = blReqX% : blTestY% = blReqY%
511812 CALL blTestMove
511813 blStatus% = blStatusOk%
511814 RETURN
511820 LABEL blApiTestMouseButton
511821 blTestX% = blReqX% : blTestY% = blReqY% : blTestBtn% = blReqButton% : blTestDown% = blReqDown%
511822 CALL blTestButton
511823 blStatus% = blStatusOk%
511824 RETURN
511830 LABEL blApiTestWheel
511831 blTestX% = blReqX% : blTestY% = blReqY% : blTestWheelDelta% = blReqWheel%
511832 CALL blTestWheel
511833 blStatus% = blStatusOk%
511834 RETURN
511900 LABEL blApiDump
511901 CALL blDump
511902 RETURN
511910 LABEL blApiAutoSize
511911 blAutoH% = blReqHandle%
511912 CALL blAutoSize
511913 RETURN
511920 LABEL blApiSetActionLine
511921 CALL blSetActionLineApi
511922 RETURN
511930 LABEL blApiRegisterClass
511931 CALL blRegisterClass
511932 RETURN
511950 LABEL blApiSetEnabled
511951 blEnabledH% = blReqHandle%
511952 CALL blSetEnabled
511953 RETURN
512000 REM debug/test helper area
512010 LABEL blTestErl
512020 ERROR 5
512030 RETURN
512031 END
