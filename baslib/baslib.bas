500000 END
500010 CONST blVersion$ = "0.1"
500011 CONST blVersionNum# = 0.1
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
500100 LABEL blFramePush
500101 IF blFp% >= blMaxFrames% THEN blStatus% = blStatusFrameOverflow% : RETURN
500102 blFp% = blFp% + 1
500103 STACK PUSH blCallCtl%, blCallVerb%, blCallArgcN%, blCallArgcS%
500104 STACK PUSH blCallN#(1), blCallN#(2), blCallN#(3), blCallN#(4)
500105 STACK PUSH blCallN#(5), blCallN#(6), blCallN#(7), blCallN#(8)
500106 STACK PUSH blCallS$(1), blCallS$(2)
500107 blStatus% = blStatusOk%
500108 RETURN
500130 LABEL blFrameLoad
500131 STACK TOP blArgS$(blFp%,2), blArgS$(blFp%,1)
500132 STACK TOP blArgN#(blFp%,8), blArgN#(blFp%,7), blArgN#(blFp%,6), blArgN#(blFp%,5)
500133 STACK TOP blArgN#(blFp%,4), blArgN#(blFp%,3), blArgN#(blFp%,2), blArgN#(blFp%,1)
500134 STACK TOP blArgcS%(blFp%), blArgcN%(blFp%), blArgVerb%(blFp%), blArgCtl%(blFp%)
500135 RETURN
500160 LABEL blFrameClose
500161 STACK POP blTmpS2$, blTmpS1$, blTmpN8#, blTmpN7#, blTmpN6#, blTmpN5#, blTmpN4#
500162 STACK POP blTmpN3#, blTmpN2#, blTmpN1#, blTmpArgcS%, blTmpArgcN%, blTmpVerb%, blTmpCtl%
500163 blFp% = blFp% - 1
500164 STACK PUSH blRetS$, blRetN#
500165 blStatus% = blStatusOk%
500166 RETURN
500190 LABEL blFrameDrop
500191 STACK POP blTmpS2$, blTmpS1$, blTmpN8#, blTmpN7#, blTmpN6#, blTmpN5#, blTmpN4#
500192 STACK POP blTmpN3#, blTmpN2#, blTmpN1#, blTmpArgcS%, blTmpArgcN%, blTmpVerb%, blTmpCtl%
500193 blFp% = blFp% - 1
500194 STACK PUSH "", 0
500195 blStatus% = blStatusOk%
500196 RETURN
500300 LABEL blFail
500301 blStatus% = blFailCode%
500302 blError% = blFailCode%
500303 RETURN
500310 LABEL blClampW
500311 IF blClampV% < 0 THEN blClampV% = 0
500312 RETURN
500320 LABEL blIsVisible
500321 blYes% = 0
500322 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN RETURN
500323 IF blCtl%(blCheckH%,1) = 0 THEN RETURN
500324 IF (blCtl%(blCheckH%,13) AND blFlagVisible%) = 0 THEN RETURN
500325 blYes% = 1
500326 RETURN
500340 LABEL blIsEnabled
500341 blYes% = 0
500342 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN RETURN
500343 IF blCtl%(blCheckH%,1) = 0 THEN RETURN
500344 IF (blCtl%(blCheckH%,13) AND blFlagEnabled%) = 0 THEN RETURN
500345 blYes% = 1
500346 RETURN
500600 LABEL blInitTables
500601 IF blInitDone% <> 0 THEN GOTO blInitClear
500602 DIM blCtl%(255,17)
500603 DIM blCtlText$(255), blCtlName$(255)
500604 DIM blPanel%(255,8), blUser#(255,22), blClick%(255,4)
500605 DIM blCtlClipL%(255), blCtlClipT%(255), blCtlClipR%(255), blCtlClipB%(255)
500606 DIM blCtlZ%(255), blFreeNext%(255)
500607 DIM blMethod&(16,20), blUserMethod&(8,20), blPanelLayoutLine&(3), blActionLine&(255)
500608 DIM blClassName$(16), blClassFlags%(16), blClassFace%(16), blClassText%(16), blClassAccent%(16)
500609 DIM blCallN#(8), blCallS$(2)
500610 DIM blArgN#(32,8), blArgS$(32,2), blLocalN#(32,8), blLocalS$(32,2)
500611 DIM blArgCtl%(32), blArgVerb%(32), blArgcN%(32), blArgcS%(32)
500612 DIM blEvType%(63), blEvCtl%(63), blEvAction%(63), blEvX%(63), blEvY%(63)
500613 DIM blEvButton%(63), blEvWheelVal%(63)
500614 blInitDone% = 1
500615 LABEL blInitClear
500616 FOR blI% = 0 TO 255
500617   blCtl%(blI%,1) = 0
500618   blCtl%(blI%,13) = 0
500619   blCtlClipL%(blI%) = 0
500620   blCtlClipT%(blI%) = 0
500621   blCtlClipR%(blI%) = 0
500622   blCtlClipB%(blI%) = 0
500623   blCtlText$(blI%) = ""
500624   blCtlName$(blI%) = ""
500625   blCtlZ%(blI%) = 0
500626   blFreeNext%(blI%) = 0
500627 NEXT blI%
500628 FOR blC% = 1 TO 16
500629   blClassFlags%(blC%) = 0
500630   blClassFace%(blC%) = 7
500631   blClassText%(blC%) = 0
500632   blClassAccent%(blC%) = 4
500633   blClassName$(blC%) = ""
500634   FOR blV% = 1 TO 20
500635     blMethod&(blC%,blV%) = 500990
500636   NEXT blV%
500637 NEXT blC%
500638 FOR blV% = 1 TO 20
500639   blMethod&(1,blV%) = 506000
500640   blMethod&(2,blV%) = 507000
500641   blMethod&(3,blV%) = 508000
500642   blMethod&(4,blV%) = 509000
500643 NEXT blV%
500644 FOR blK% = 1 TO 8
500645   FOR blV% = 1 TO 20
500646     blUserMethod&(blK%,blV%) = 508100
500647   NEXT blV%
500648 NEXT blK%
500649 FOR blI% = 0 TO 3
500650   blPanelLayoutLine&(blI%) = 500990
500651 NEXT blI%
500652 FOR blI% = 0 TO 255
500653   blActionLine&(blI%) = 0
500654 NEXT blI%
500655 FOR blI% = 0 TO 63
500656   blEvType%(blI%) = 0
500657   blEvCtl%(blI%) = 0
500658   blEvAction%(blI%) = 0
500659   blEvX%(blI%) = 0
500660   blEvY%(blI%) = 0
500661   blEvButton%(blI%) = 0
500662   blEvWheelVal%(blI%) = 0
500663 NEXT blI%
500664 FOR blI% = 0 TO 32
500665   blArgCtl%(blI%) = 0
500666   blArgVerb%(blI%) = 0
500667   blArgcN%(blI%) = 0
500668   blArgcS%(blI%) = 0
500669 NEXT blI%
500670 blClassName$(1) = "Top"
500671 blClassName$(2) = "Panel"
500672 blClassName$(3) = "User"
500673 blClassName$(4) = "ClickTarget"
500674 RETURN
500900 LABEL blInit
500901 GOSUB blInitTables
500902 blRoot% = 1
500903 blFp% = 0
500904 blNeedsPaint% = 1
500905 blNeedsLayout% = 1
500906 blHover% = 0
500907 blPressed% = 0
500908 blCapture% = 0
500909 blEvHead% = 0
500910 blEvTail% = 0
500911 blEvCount% = 0
500912 blEventType% = 0
500913 blEventCtl% = 0
500914 blEventAction% = 0
500915 blEventX% = 0
500916 blEventY% = 0
500917 blEventButton% = 0
500918 blEventWheel% = 0
500919 blEventCount% = 0
500920 blRetHandle% = 0
500921 blRetN% = 0
500922 blRetN# = 0
500923 blRetS$ = ""
500924 blRetValue# = 0
500925 blStatus% = blStatusOk%
500926 blError% = 0
500927 blI% = 0
500928 blCtl%(1,1) = blClassTop%
500929 blCtl%(1,2) = 0
500930 blCtl%(1,7) = 0
500931 blCtl%(1,8) = 0
500932 blCtl%(1,9) = 640
500933 IF blReqW% > 0 THEN blCtl%(1,9) = blReqW% : GOTO blInitW
500934 IF SCREENW > 0 THEN blCtl%(1,9) = SCREENW
500935 LABEL blInitW
500936 blCtl%(1,10) = 400
500937 IF blReqH% > 0 THEN blCtl%(1,10) = blReqH% : GOTO blInitH
500938 IF SCREENH > 0 THEN blCtl%(1,10) = SCREENH
500939 LABEL blInitH
500940 blCtl%(1,11) = 0
500941 blCtl%(1,12) = 0
500942 blCtl%(1,13) = blFlagVisible% OR blFlagEnabled% OR blFlagClip%
500943 blCtl%(1,14) = 0
500944 blCtl%(1,15) = 0
500945 blCtl%(1,16) = 0
500946 blCtl%(1,17) = 0
500947 blCtlClipL%(1) = 0
500948 blCtlClipT%(1) = 0
500949 blCtlClipR%(1) = blCtl%(1,9) - 1
500950 blCtlClipB%(1) = blCtl%(1,10) - 1
500951 blCtlZ%(1) = 0
500952 blPanel%(1,1) = 0
500953 blPanel%(1,2) = 0
500954 blPanel%(1,3) = 0
500955 blPanel%(1,6) = 7
500956 blPanel%(1,7) = 8
500957 RETURN
500990 LABEL blNoop
500991 RETURN
501000 LABEL blAllocHandle
501001 blNewHandle% = 0
501002 FOR blH% = 1 TO blMaxControls%
501003   IF blCtl%(blH%,1) = 0 THEN blNewHandle% = blH% : blStatus% = blStatusOk% : RETURN
501004 NEXT blH%
501005 blStatus% = blStatusControlTableFull%
501006 RETURN
501050 LABEL blLinkHandle
501051 blP% = blCtl%(blLinkH%,2)
501052 IF blP% < 1 OR blP% > blMaxControls% THEN RETURN
501053 blLast% = blCtl%(blP%,4)
501054 IF blLast% = 0 THEN blCtl%(blP%,3) = blLinkH% : blCtl%(blLinkH%,5) = 0 : blCtl%(blLinkH%,6) = 0 : GOTO blLinkDone
501055 blCtl%(blLast%,6) = blLinkH%
501056 blCtl%(blLinkH%,5) = blLast%
501057 LABEL blLinkDone
501058 blCtl%(blP%,4) = blLinkH%
501059 RETURN
501080 LABEL blDetachHandle
501081 blP% = blCtl%(blDetachH%,2)
501082 IF blP% < 1 OR blP% > blMaxControls% THEN RETURN
501083 blPrev% = blCtl%(blDetachH%,5)
501084 blNext% = blCtl%(blDetachH%,6)
501085 IF blPrev% <> 0 THEN blCtl%(blPrev%,6) = blNext% ELSE blCtl%(blP%,3) = blNext%
501086 IF blNext% <> 0 THEN blCtl%(blNext%,5) = blPrev% ELSE blCtl%(blP%,4) = blPrev%
501087 blCtl%(blDetachH%,5) = 0
501088 blCtl%(blDetachH%,6) = 0
501089 RETURN
501100 LABEL blCreate
501101 blRetHandle% = 0
501102 blStatus% = blStatusOk%
501103 IF blReqClass% < 2 OR blReqClass% > 4 THEN blStatus% = blStatusInvalidClass% : RETURN
501104 IF blReqParent% < 1 OR blReqParent% > blMaxControls% THEN blStatus% = blStatusInvalidParent% : RETURN
501105 IF blCtl%(blReqParent%,1) = 0 THEN blStatus% = blStatusInvalidParent% : RETURN
501106 IF blReqClass% = blClassUser% AND (blReqKind% < 1 OR blReqKind% > 5) THEN blStatus% = blStatusInvalidValue% : RETURN
501107 GOSUB blAllocHandle
501108 IF blNewHandle% = 0 THEN RETURN
501109 blH% = blNewHandle%
501110 blCtl%(blH%,1) = blReqClass%
501111 blCtl%(blH%,2) = blReqParent%
501112 blCtl%(blH%,5) = 0
501113 blCtl%(blH%,6) = 0
501114 blCtl%(blH%,7) = blReqX%
501115 blCtl%(blH%,8) = blReqY%
501116 blCtl%(blH%,9) = blReqW%
501117 blCtl%(blH%,10) = blReqH%
501118 blCtl%(blH%,11) = blReqX%
501119 blCtl%(blH%,12) = blReqY%
501120 blCtl%(blH%,13) = blReqFlags% OR blFlagVisible% OR blFlagEnabled%
501121 blCtl%(blH%,14) = 0
501122 blCtl%(blH%,15) = blReqAction%
501123 blCtl%(blH%,16) = blReqTag%
501124 blCtl%(blH%,17) = 0
501125 blCtlText$(blH%) = blReqText$
501126 blCtlName$(blH%) = ""
501127 blCtlClipL%(blH%) = 0
501128 blCtlClipT%(blH%) = 0
501129 blCtlClipR%(blH%) = 0
501130 blCtlClipB%(blH%) = 0
501131 blCtlZ%(blH%) = blH%
501132 IF blReqClass% = 2 THEN GOSUB blInitPanel : GOTO blCreateLinked
501133 IF blReqClass% = 3 THEN GOSUB blInitUser : GOTO blCreateLinked
501134 IF blReqClass% = 4 THEN GOSUB blInitClick : GOTO blCreateLinked
501135 blStatus% = blStatusInvalidClass%
501136 RETURN
501137 LABEL blCreateLinked
501138 blLinkH% = blH%
501139 GOSUB blLinkHandle
501140 blRetHandle% = blH%
501141 blNeedsLayout% = 1
501142 blNeedsPaint% = 1
501143 blStatus% = blStatusOk%
501144 RETURN
501210 LABEL blInitPanel
501211 blCtl%(blH%,13) = blCtl%(blH%,13) OR blFlagClip%
501212 blPanel%(blH%,1) = blReqMode%
501213 blPanel%(blH%,2) = blReqPad%
501214 blPanel%(blH%,3) = blReqGap%
501215 blPanel%(blH%,4) = blReqCols%
501216 blPanel%(blH%,6) = 7
501217 blPanel%(blH%,7) = 8
501218 RETURN
501230 LABEL blInitUser
501231 blUser#(blH%,1) = blReqKind%
501232 blUser#(blH%,2) = blReqValue#
501233 blUser#(blH%,3) = 0
501234 blUser#(blH%,4) = 100
501235 blUser#(blH%,5) = 1
501236 blUser#(blH%,6) = 0
501237 blUser#(blH%,7) = 0
501238 blUser#(blH%,8) = 7
501239 blUser#(blH%,9) = 4
501240 blUser#(blH%,10) = 0
501241 blUser#(blH%,11) = 0
501242 blUser#(blH%,12) = 0
501243 blUser#(blH%,13) = blReqPad%
501244 blUser#(blH%,14) = 1
501245 blUser#(blH%,15) = 1
501246 blUser#(blH%,16) = 0
501247 blUser#(blH%,17) = 0
501248 blUser#(blH%,18) = 1
501249 blUser#(blH%,19) = 0
501250 blUser#(blH%,20) = 1
501251 blUser#(blH%,21) = 0
501252 blUser#(blH%,22) = 1
501253 RETURN
501270 LABEL blInitClick
501271 blClick%(blH%,1) = blReqButtonMask%
501272 RETURN
501300 LABEL blDestroy
501301 blStatus% = blStatusOk%
501302 blTargetH% = blDestroyH%
501303 IF blDestroyH% < 1 OR blDestroyH% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : RETURN
501304 IF blCtl%(blDestroyH%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
501305 IF blDestroyH% = blRoot% THEN blStatus% = blStatusCannotDestroyRoot% : RETURN
501306 STACK PUSH blTargetH%
501307 blChild% = blCtl%(blDestroyH%,3)
501308 WHILE blChild% <> 0
501309   blNext% = blCtl%(blChild%,6)
501310   blDestroyH% = blChild%
501311   GOSUB blDestroy
501312   blChild% = blNext%
501313 WEND
501314 STACK POP blTargetH%
501315 blDestroyH% = blTargetH%
501316 blDetachH% = blDestroyH%
501317 GOSUB blDetachHandle
501318 blCtl%(blDestroyH%,1) = 0
501319 blCtl%(blDestroyH%,2) = 0
501320 blCtl%(blDestroyH%,3) = 0
501321 blCtl%(blDestroyH%,4) = 0
501322 blCtl%(blDestroyH%,5) = 0
501323 blCtl%(blDestroyH%,6) = 0
501324 blCtl%(blDestroyH%,13) = 0
501325 blCtlText$(blDestroyH%) = ""
501326 blNeedsLayout% = 1
501327 blNeedsPaint% = 1
501328 blStatus% = blStatusOk%
501329 RETURN
501500 LABEL blValidateHandle
501501 blYes% = 0
501502 IF blCheckH% < 1 OR blCheckH% > blMaxControls% THEN RETURN
501503 IF blCtl%(blCheckH%,1) = 0 THEN RETURN
501504 blYes% = 1
501505 RETURN
501520 LABEL blSetDirty
501521 IF blDirtyH% < 1 OR blDirtyH% > blMaxControls% THEN RETURN
501522 blCtl%(blDirtyH%,13) = blCtl%(blDirtyH%,13) OR blFlagLayoutDirty%
501523 blNeedsLayout% = 1
501524 blNeedsPaint% = 1
501525 RETURN
502000 LABEL blDispatch
502001 blStatus% = blStatusOk%
502002 blDispCtl% = blCallCtl%
502003 blDispVerb% = blCallVerb%
502004 blCheckH% = blDispCtl%
502005 GOSUB blValidateHandle
502006 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : GOSUB blFrameDrop : RETURN
502007 blDispClass% = blCtl%(blDispCtl%,1)
502008 IF blDispClass% < 1 OR blDispClass% > blMaxClasses% THEN blStatus% = blStatusInvalidClass% : GOSUB blFrameDrop : RETURN
502009 IF blDispVerb% < 1 OR blDispVerb% > blMaxVerbs% THEN blStatus% = blStatusInvalidVerb% : GOSUB blFrameDrop : RETURN
502010 blMethodLine& = blMethod&(blDispClass%,blDispVerb%)
502011 IF blMethodLine& = blNoLine& THEN blStatus% = blStatusInvalidVerb% : GOSUB blFrameDrop : RETURN
502012 GOSUB blMethodLine&
502013 GOSUB blFrameClose
502014 RETURN
502900 LABEL blSend
502901 GOSUB blFramePush
502902 IF blStatus% <> blStatusOk% THEN RETURN
502903 GOSUB blDispatch
502904 STACK POP blRetN#, blRetS$
502905 RETURN
503000 LABEL blLayoutAll
503001 IF blRoot% < 1 THEN RETURN
503002 blNeedsLayout% = 0
503003 blCtl%(blRoot%,11) = 0
503004 blCtl%(blRoot%,12) = 0
503005 blCtlClipL%(blRoot%) = 0
503006 blCtlClipT%(blRoot%) = 0
503007 blCtlClipR%(blRoot%) = blCtl%(blRoot%,9) - 1
503008 blCtlClipB%(blRoot%) = blCtl%(blRoot%,10) - 1
503009 blLayoutH% = blRoot%
503010 GOSUB blLayoutControl
503011 RETURN
503100 LABEL blLayoutControl
503101 blH% = blLayoutH%
503102 IF blH% < 1 OR blH% > blMaxControls% THEN RETURN
503103 IF blCtl%(blH%,1) = 0 THEN RETURN
503104 IF (blCtl%(blH%,13) AND blFlagVisible%) = 0 THEN RETURN
503105 blC% = blCtl%(blH%,1)
503106 IF blC% = blClassTop% OR blC% = blClassPanel% THEN GOSUB blLayoutChildren
503107 RETURN
503200 LABEL blLayoutChildren
503201 blMode% = blPanel%(blH%,1)
503202 blPad% = blPanel%(blH%,2)
503203 blGap% = blPanel%(blH%,3)
503204 blPX% = blCtl%(blH%,11)
503205 blPY% = blCtl%(blH%,12)
503206 blCW% = blCtl%(blH%,9)
503207 blCH% = blCtl%(blH%,10)
503208 blCX% = blPX% + blPad%
503209 blCY% = blPY% + blPad%
503210 blChild% = blCtl%(blH%,3)
503211 WHILE blChild% <> 0
503212   IF (blCtl%(blChild%,13) AND blFlagVisible%) <> 0 THEN STACK PUSH blChild% : GOSUB blPlaceChild : STACK POP blChild%
503213   blChild% = blCtl%(blChild%,6)
503214 WEND
503215 RETURN
503300 LABEL blPlaceChild
503301 IF blMode% = 1 THEN GOSUB blPlaceV : GOTO blPlaceAbs
503302 IF blMode% = 2 THEN GOSUB blPlaceH
503303 LABEL blPlaceAbs
503304 blCtl%(blChild%,11) = blPX% + blCtl%(blChild%,7)
503305 blCtl%(blChild%,12) = blPY% + blCtl%(blChild%,8)
503306 blCl% = blCtlClipL%(blH%)
503307 blCt% = blCtlClipT%(blH%)
503308 blCr% = blCtlClipR%(blH%)
503309 blCb% = blCtlClipB%(blH%)
503310 IF blCtl%(blChild%,11) > blCl% THEN blCl% = blCtl%(blChild%,11)
503311 IF blCtl%(blChild%,12) > blCt% THEN blCt% = blCtl%(blChild%,12)
503312 blX2% = blCtl%(blChild%,11) + blCtl%(blChild%,9) - 1
503313 blY2% = blCtl%(blChild%,12) + blCtl%(blChild%,10) - 1
503314 IF blX2% < blCr% THEN blCr% = blX2%
503315 IF blY2% < blCb% THEN blCb% = blY2%
503316 blCtlClipL%(blChild%) = blCl%
503317 blCtlClipT%(blChild%) = blCt%
503318 blCtlClipR%(blChild%) = blCr%
503319 blCtlClipB%(blChild%) = blCb%
503320 STACK PUSH blChild%
503321 blLayoutH% = blChild%
503322 GOSUB blLayoutControl
503323 STACK POP blChild%
503324 RETURN
503400 LABEL blPlaceV
503401 blCtl%(blChild%,7) = blPad%
503402 blCtl%(blChild%,8) = blCY% - blPY%
503403 blCtl%(blChild%,9) = blCW% - 2 * blPad%
503404 blCY% = blCY% + blCtl%(blChild%,10) + blGap%
503405 RETURN
503420 LABEL blPlaceH
503421 blCtl%(blChild%,7) = blCX% - blPX%
503422 blCtl%(blChild%,8) = blPad%
503423 blCtl%(blChild%,10) = blCH% - 2 * blPad%
503424 blCX% = blCX% + blCtl%(blChild%,9) + blGap%
503425 RETURN
504000 LABEL blPaintAll
504001 IF blNeedsLayout% <> 0 THEN GOSUB blLayoutAll
504002 blNeedsPaint% = 0
504003 blPaintH% = blRoot%
504004 GOSUB blPaintTree
504005 RETURN
504100 LABEL blPaintTree
504101 blH% = blPaintH%
504102 IF blH% < 1 OR blH% > blMaxControls% THEN RETURN
504103 IF blCtl%(blH%,1) = 0 THEN RETURN
504104 IF (blCtl%(blH%,13) AND blFlagVisible%) = 0 THEN RETURN
504105 GOSUB blPaintOne
504106 blChild% = blCtl%(blH%,3)
504107 WHILE blChild% <> 0
504108   STACK PUSH blChild%
504109   blPaintH% = blChild%
504110   GOSUB blPaintTree
504111   STACK POP blChild%
504112   blChild% = blCtl%(blChild%,6)
504113 WEND
504114 RETURN
504200 LABEL blPaintOne
504201 blH% = blPaintH%
504202 blX% = blCtl%(blH%,11)
504203 blY% = blCtl%(blH%,12)
504204 blW% = blCtl%(blH%,9)
504205 blHH% = blCtl%(blH%,10)
504206 IF blW% <= 0 OR blHH% <= 0 THEN RETURN
504207 PUSH VIEW
504208 blCls% = blCtl%(blH%,1)
504209 IF blCls% = blClassTop% OR blCls% = blClassPanel% THEN GOSUB blPaintPanel : GOTO blPaintDone
504210 IF blCls% = blClassUser% THEN GOSUB blPaintUser
504211 LABEL blPaintDone
504212 POP VIEW
504213 RETURN
504300 LABEL blPaintPanel
504301 blFace% = blPanel%(blH%,6)
504302 blBorder% = blPanel%(blH%,7)
504303 LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
504304 IF blBorder% >= 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blBorder%, B
504305 RETURN
504320 LABEL blPaintUser
504321 blKind% = blUser#(blH%,1)
504322 IF blKind% = blKindButton% THEN GOSUB blPaintButton : RETURN
504323 IF blKind% = blKindLabel% THEN GOSUB blPaintLabel : RETURN
504324 IF blKind% = blKindSlider% THEN GOSUB blPaintSlider : RETURN
504325 IF blKind% = blKindGauge% THEN GOSUB blPaintGauge
504326 IF blKind% = blKindDial% THEN GOSUB blPaintDial
504327 RETURN
504340 LABEL blPaintButton
504341 blFace% = blUser#(blH%,8)
504342 blText% = blUser#(blH%,19)
504343 blBorder% = blUser#(blH%,17)
504344 IF (blCtl%(blH%,13) AND blFlagPressed%) <> 0 THEN blFace% = blUser#(blH%,9)
504345 LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
504346 IF blBorder% >= 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blBorder%, B
504347 blText$ = blCtlText$(blH%)
504348 blTx% = blX%
504349 blTy% = blY%
504350 blBoxW% = blW%
504351 blBoxH% = blHH%
504352 blColor% = blText%
504353 blScale% = blUser#(blH%,20)
504354 blAlign% = 1
504355 blVAlign% = 1
504356 blPad% = 2
504357 GOSUB blDrawText
504358 RETURN
504380 LABEL blPaintLabel
504381 blText$ = blCtlText$(blH%)
504382 IF blUser#(blH%,16) <> 0 THEN GOSUB blWrapText
504383 blFace% = blUser#(blH%,8)
504384 blText% = blUser#(blH%,19)
504385 blBorder% = blUser#(blH%,17)
504386 IF blUser#(blH%,18) = 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
504387 IF blBorder% >= 0 THEN LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blBorder%, B
504388 blTx% = blX%
504389 blTy% = blY%
504390 blBoxW% = blW%
504391 blBoxH% = blHH%
504392 blColor% = blText%
504393 blScale% = blUser#(blH%,20)
504394 blAlign% = blUser#(blH%,14)
504395 blVAlign% = blUser#(blH%,15)
504396 blPad% = blUser#(blH%,13)
504397 GOSUB blDrawText
504398 RETURN
504420 LABEL blPaintSlider
504421 blFace% = blUser#(blH%,8)
504422 blAccent% = blUser#(blH%,9)
504423 LINE (blX%,blY% + blHH% / 2)-(blX% + blW% - 1,blY% + blHH% / 2), blText%, BF
504424 blPos% = blX% + (blW% - 8) * (blUser#(blH%,2) - blUser#(blH%,3)) / (blUser#(blH%,4) - blUser#(blH%,3) + 0.000001)
504425 LINE (blPos%,blY% + 2)-(blPos% + 7,blY% + blHH% - 2), blAccent%, BF
504426 RETURN
504440 LABEL blPaintGauge
504441 blFace% = blUser#(blH%,8)
504442 blAccent% = blUser#(blH%,9)
504443 LINE (blX%,blY%)-(blX% + blW% - 1,blY% + blHH% - 1), blFace%, BF
504444 blPos% = blX% + (blW% - 2) * (blUser#(blH%,2) - blUser#(blH%,3)) / (blUser#(blH%,4) - blUser#(blH%,3) + 0.000001)
504445 LINE (blX% + 1,blY% + 1)-(blPos%,blY% + blHH% - 2), blAccent%, BF
504446 RETURN
504460 LABEL blPaintDial
504461 blFace% = blUser#(blH%,8)
504462 blAccent% = blUser#(blH%,9)
504463 blR% = blHH% / 2 - 2
504464 IF blW% < blHH% THEN blR% = blW% / 2 - 2
504465 blCx% = blX% + blW% / 2
504466 blCy% = blY% + blHH% / 2
504467 CIRCLE (blCx%,blCy%), blR%, blFace%, 0, 6.2831853, 1
504468 CIRCLE (blCx%,blCy%), blR% - 1, blAccent%, 0, 6.2831853, 1
504469 blA# = 3.14159265 * (1 - (blUser#(blH%,2) - blUser#(blH%,3)) / (blUser#(blH%,4) - blUser#(blH%,3) + 0.000001))
504470 blEx% = blCx% + COS(blA#) * (blR% - 4)
504471 blEy% = blCy% - SIN(blA#) * (blR% - 4)
504472 LINE (blCx%,blCy%)-(blEx%,blEy%), blAccent%
504473 RETURN
505000 LABEL blHitTest
505001 blHitHandle% = 0
505002 FOR blH% = blMaxControls% TO 1 STEP -1
505003   IF blCtl%(blH%,1) <> 0 AND (blCtl%(blH%,13) AND blFlagVisible%) <> 0 AND (blCtl%(blH%,13) AND blFlagEnabled%) <> 0 THEN GOSUB blHitOne
505004   IF blHitHandle% <> 0 THEN GOTO blHitDone
505005 NEXT blH%
505006 LABEL blHitDone
505007 RETURN
505050 LABEL blHitOne
505051 blX1% = blCtl%(blH%,11)
505052 blY1% = blCtl%(blH%,12)
505053 blX2% = blX1% + blCtl%(blH%,9) - 1
505054 blY2% = blY1% + blCtl%(blH%,10) - 1
505055 IF blHitX% >= blX1% AND blHitX% <= blX2% AND blHitY% >= blY1% AND blHitY% <= blY2% THEN blHitHandle% = blH%
505056 RETURN
505100 LABEL blClickAt
505101 blClickH% = 0
505102 blHitX% = blClickX%
505103 blHitY% = blClickY%
505104 GOSUB blHitTest
505105 blClickH% = blHitHandle%
505106 IF blClickH% = 0 THEN RETURN
505107 IF blCtl%(blClickH%,1) = blClassUser% THEN GOSUB blPointerValue
505108 blAction% = blCtl%(blClickH%,15)
505109 IF blAction% = 0 THEN GOSUB blBubbleAction
505110 IF blAction% <> 0 THEN GOSUB blPostClick
505111 blRetN% = 1
505112 RETURN
505150 LABEL blBubbleAction
505151 blP% = blCtl%(blClickH%,2)
505152 WHILE blP% <> 0
505153   IF blCtl%(blP%,15) <> 0 THEN blAction% = blCtl%(blP%,15) : blClickH% = blP% : blP% = 0 : GOTO blBubbleDone
505154   blP% = blCtl%(blP%,2)
505155 WEND
505156 LABEL blBubbleDone
505157 RETURN
505180 LABEL blPostClick
505181 blEvType% = blEvClick%
505182 blEvCtl% = blClickH%
505183 blEvAction% = blAction%
505184 blEvX% = blClickX%
505185 blEvY% = blClickY%
505186 blEvButton% = blClickButton%
505187 blWheelValue% = 0
505188 GOSUB blPostEvent
505189 RETURN
505200 LABEL blPostEvent
505201 IF blEvCount% >= blMaxEvents% THEN blStatus% = blStatusEventQueueFull% : RETURN
505202 blEvType%(blEvTail%) = blEvType%
505203 blEvCtl%(blEvTail%) = blEvCtl%
505204 blEvAction%(blEvTail%) = blEvAction%
505205 blEvX%(blEvTail%) = blEvX%
505206 blEvY%(blEvTail%) = blEvY%
505207 blEvButton%(blEvTail%) = blEvButton%
505208 blEvWheelVal%(blEvTail%) = blWheelValue%
505209 blEvTail% = (blEvTail% + 1) MOD blMaxEvents%
505210 blEvCount% = blEvCount% + 1
505211 blEventCount% = blEvCount%
505212 RETURN
505300 LABEL blReadEvent
505301 IF blEvCount% = 0 THEN blStatus% = blStatusEventQueueEmpty% : blEventType% = 0 : blEventCtl% = 0 : blEventAction% = 0 : blEventX% = 0 : blEventY% = 0 : blEventButton% = 0 : blEventWheel% = 0 : blEventCount% = 0 : RETURN
505302 blEventType% = blEvType%(blEvHead%)
505303 blEventCtl% = blEvCtl%(blEvHead%)
505304 blEventAction% = blEvAction%(blEvHead%)
505305 blEventX% = blEvX%(blEvHead%)
505306 blEventY% = blEvY%(blEvHead%)
505307 blEventButton% = blEvButton%(blEvHead%)
505308 blEventWheel% = blEvWheelVal%(blEvHead%)
505309 blEvHead% = (blEvHead% + 1) MOD blMaxEvents%
505310 blEvCount% = blEvCount% - 1
505311 blEventCount% = blEvCount%
505312 blStatus% = blStatusOk%
505313 RETURN
505400 LABEL blTestClick
505401 blClickX% = blTestX%
505402 blClickY% = blTestY%
505403 blClickButton% = blTestButton%
505404 GOSUB blClickAt
505405 blRetHandle% = blClickH%
505406 RETURN
505500 LABEL blTestMove
505501 blTestOld% = blHover%
505502 blHitX% = blTestX%
505503 blHitY% = blTestY%
505504 GOSUB blHitTest
505505 blHover% = blHitHandle%
505506 IF blCapture% <> 0 AND blPressed% <> 0 THEN blClickH% = blCapture% : blClickX% = blTestX% : blClickY% = blTestY% : GOSUB blPointerValue
505507 IF blTestOld% <> blHover% THEN GOSUB blPostHover
505508 IF blHover% <> 0 THEN GOSUB blPostMove
505509 RETURN
505540 LABEL blPostHover
505541 IF blTestOld% <> 0 THEN blEvType% = blEvLeave% : blEvCtl% = blTestOld% : blEvAction% = 0 : blEvX% = blTestX% : blEvY% = blTestY% : blEvButton% = 0 : blWheelValue% = 0 : GOSUB blPostEvent
505542 IF blHover% <> 0 THEN blEvType% = blEvEnter% : blEvCtl% = blHover% : blEvAction% = 0 : blEvX% = blTestX% : blEvY% = blTestY% : blEvButton% = 0 : blWheelValue% = 0 : GOSUB blPostEvent
505543 RETURN
505560 LABEL blPostMove
505561 blEvType% = blEvMove%
505562 blEvCtl% = blHover%
505563 blEvAction% = 0
505564 blEvX% = blTestX%
505565 blEvY% = blTestY%
505566 blEvButton% = 0
505567 blWheelValue% = 0
505568 GOSUB blPostEvent
505569 RETURN
505600 LABEL blTestButton
505601 IF blTestDown% <> 0 THEN GOSUB blButtonDown : RETURN
505602 GOSUB blButtonUp
505603 RETURN
505650 LABEL blButtonDown
505651 blPressed% = blTestButton%
505652 blClickX% = blTestX% : blClickY% = blTestY%
505653 IF blHover% <> 0 THEN blCapture% = blHover%
505654 blEvType% = blEvDown%
505655 blEvCtl% = blHover%
505656 blEvAction% = 0
505657 blEvX% = blTestX%
505658 blEvY% = blTestY%
505659 blEvButton% = blTestButton%
505660 blWheelValue% = 0
505661 GOSUB blPostEvent
505662 RETURN
505670 LABEL blButtonUp
505671 blEvType% = blEvUp%
505672 blEvCtl% = blCapture%
505673 blEvAction% = 0
505674 blEvX% = blTestX%
505675 blEvY% = blTestY%
505676 blEvButton% = blTestButton%
505677 blWheelValue% = 0
505678 GOSUB blPostEvent
505679 blPressed% = 0
505680 blCapture% = 0
505681 RETURN
505700 LABEL blTestWheel
505701 IF blHover% <> 0 THEN GOSUB blPostWheel
505702 RETURN
505730 LABEL blPostWheel
505731 blEvType% = blEvWheel%
505732 blEvCtl% = blHover%
505733 blEvAction% = 0
505734 blEvX% = blTestX%
505735 blEvY% = blTestY%
505736 blEvButton% = 0
505737 blWheelValue% = blTestWheelDelta%
505738 GOSUB blPostEvent
505739 RETURN
505800 LABEL blPollInput
505801 blRetN% = 0
505802 blB% = MOUSEB
505803 blTestX% = MOUSEPOSX
505804 blTestY% = MOUSEPOSY
505805 IF blTestX% >= 0 THEN GOSUB blTestMove
505806 blBM% = MOUSEBUTTONS AND 1
505807 IF blBM% <> 0 AND blPressed% = 0 THEN blTestButton% = 1 : blTestDown% = 1 : GOSUB blTestButton
505808 IF blBM% = 0 AND blPressed% <> 0 THEN blTestButton% = blPressed% : blTestDown% = 0 : GOSUB blTestButton
505809 blWh% = MOUSEWHEEL
505810 IF blWh% <> 0 THEN blTestWheelDelta% = blWh% : GOSUB blTestWheel
505811 IF blB% <> 0 THEN blTestX% = MOUSEX : blTestY% = MOUSEY : blTestButton% = blB% : GOSUB blTestClick : blRetN% = 1
505812 RETURN
505900 LABEL blPointerValue
505901 blKind% = blUser#(blClickH%,1)
505902 IF blKind% <> blKindSlider% AND blKind% <> blKindDial% THEN RETURN
505903 IF blCtl%(blClickH%,9) <= 0 THEN RETURN
505904 blV# = blUser#(blClickH%,3) + (blClickX% - blCtl%(blClickH%,11)) * (blUser#(blClickH%,4) - blUser#(blClickH%,3)) / blCtl%(blClickH%,9)
505905 IF blV# < blUser#(blClickH%,3) THEN blV# = blUser#(blClickH%,3)
505906 IF blV# > blUser#(blClickH%,4) THEN blV# = blUser#(blClickH%,4)
505907 blUser#(blClickH%,2) = blV#
505908 blEvType% = blEvChange%
505909 blEvCtl% = blClickH%
505910 blEvAction% = blCtl%(blClickH%,15)
505911 blEvX% = blClickX%
505912 blEvY% = blClickY%
505913 blEvButton% = blClickButton%
505914 blWheelValue% = 0
505915 GOSUB blPostEvent
505916 blNeedsPaint% = 1
505917 RETURN
506000 LABEL blTopRouter
506001 RETURN
507000 LABEL blPanelRouter
507001 RETURN
508000 LABEL blUserRouter
508001 RETURN
508100 LABEL blUserKindRouter
508101 RETURN
509000 LABEL blClickRouter
509001 RETURN
509500 LABEL blDrawText
509501 IF blText$ = "" THEN RETURN
509502 GOSUB blFitText
509503 blTextW% = TEXTWIDTH(blText$, blScale%)
509504 blTextH% = TEXTHEIGHT(blText$, blScale%)
509505 blDX% = blTx% + blPad%
509506 blDY% = blTy% + blPad%
509507 IF blAlign% = 1 THEN blDX% = blTx% + (blBoxW% - blTextW%) / 2
509508 IF blAlign% = 2 THEN blDX% = blTx% + blBoxW% - blTextW% - blPad%
509509 IF blVAlign% = 1 THEN blDY% = blTy% + (blBoxH% - blTextH%) / 2
509510 IF blVAlign% = 2 THEN blDY% = blTy% + blBoxH% - blTextH% - blPad%
509511 DRAWTEXT (blDX%,blDY%), blText$, blColor%, blScale%
509512 RETURN
509600 LABEL blAutoSizeH
509601 blS% = blUser#(blCheckH%,20)
509602 blP% = blUser#(blCheckH%,13)
509603 blCtl%(blCheckH%,9) = TEXTWIDTH(blCtlText$(blCheckH%), blS%) + 2 * blP% + 2
509604 blCtl%(blCheckH%,10) = TEXTHEIGHT(blCtlText$(blCheckH%), blS%) + 2 * blP% + 2
509605 blNeedsLayout% = 1
509606 blNeedsPaint% = 1
509607 RETURN
509700 LABEL blWrapText
509701 blWrapOut$ = ""
509702 blWrapLine$ = ""
509703 FOR blI% = 1 TO LEN(blText$)
509704   blCh$ = MID$(blText$,blI%,1)
509705   IF blCh$ = CHR$(13) OR blCh$ = CHR$(10) THEN blWrapOut$ = blWrapOut$ + blWrapLine$ + CHR$(13) + CHR$(10) : blWrapLine$ = "" : GOTO blWrapNext
509706   IF LEN(blWrapLine$) > 0 AND TEXTWIDTH(blWrapLine$ + blCh$,blUser#(blH%,20)) > (blW% - 2 * blUser#(blH%,13)) THEN blWrapOut$ = blWrapOut$ + blWrapLine$ + CHR$(13) + CHR$(10) : blWrapLine$ = blCh$ : GOTO blWrapNext
509707   blWrapLine$ = blWrapLine$ + blCh$
509708 LABEL blWrapNext
509709 NEXT blI%
509710 blText$ = blWrapOut$ + blWrapLine$
509711 RETURN
509800 LABEL blFitText
509801 IF blText$ = "" THEN RETURN
509802 blFitMax% = blBoxW% - 2 * blPad%
509803 IF blFitMax% < 1 THEN blFitMax% = 1
509804 blFitIn$ = blText$ : blFitOut$ = "" : blFitLine$ = ""
509805 FOR blFitI% = 1 TO LEN(blFitIn$)
509806   blFitCh$ = MID$(blFitIn$,blFitI%,1)
509807   IF blFitCh$ = CHR$(13) OR blFitCh$ = CHR$(10) THEN GOSUB blFitTrim : blFitOut$ = blFitOut$ + blFitLine$ + CHR$(13) + CHR$(10) : blFitLine$ = "" : GOTO blFitNext
509808   blFitLine$ = blFitLine$ + blFitCh$
509809 LABEL blFitNext
509810 NEXT blFitI%
509811 GOSUB blFitTrim
509812 blText$ = blFitOut$ + blFitLine$
509813 RETURN
509870 LABEL blFitTrim
509871 WHILE LEN(blFitLine$) > 0 AND TEXTWIDTH(blFitLine$,blScale%) > blFitMax%
509872   blFitLine$ = LEFT$(blFitLine$,LEN(blFitLine$) - 1)
509873 WEND
509874 RETURN
510000 LABEL blApiInit
510001 GOSUB blInit
510002 blStatus% = blStatusOk%
510003 RETURN
510100 LABEL blApiCreate
510101 GOSUB blCreate
510102 RETURN
510200 LABEL blApiDestroy
510201 blDestroyH% = blReqHandle%
510202 GOSUB blDestroy
510203 RETURN
510300 LABEL blApiSetBounds
510301 blCheckH% = blReqHandle%
510302 GOSUB blValidateHandle
510303 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
510304 blCtl%(blReqHandle%,7) = blReqX%
510305 blCtl%(blReqHandle%,8) = blReqY%
510306 blCtl%(blReqHandle%,9) = blReqW%
510307 blCtl%(blReqHandle%,10) = blReqH%
510308 blDirtyH% = blReqHandle%
510309 GOSUB blSetDirty
510310 blStatus% = blStatusOk%
510311 RETURN
510400 LABEL blApiShow
510401 blCheckH% = blReqHandle%
510402 GOSUB blValidateHandle
510403 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
510404 blCtl%(blReqHandle%,13) = blCtl%(blReqHandle%,13) OR blFlagVisible%
510405 blNeedsLayout% = 1
510406 blNeedsPaint% = 1
510407 blStatus% = blStatusOk%
510408 RETURN
510500 LABEL blApiHide
510501 blCheckH% = blReqHandle%
510502 GOSUB blValidateHandle
510503 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
510504 blCtl%(blReqHandle%,13) = blCtl%(blReqHandle%,13) AND (blFlagVisible% XOR 32767)
510505 IF blCapture% = blReqHandle% THEN blCapture% = 0
510506 blNeedsLayout% = 1
510507 blNeedsPaint% = 1
510508 blStatus% = blStatusOk%
510509 RETURN
510600 LABEL blApiSetText
510601 blCheckH% = blReqHandle%
510602 GOSUB blValidateHandle
510603 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
510604 blCtlText$(blReqHandle%) = blReqText$
510605 blNeedsPaint% = 1
510606 blStatus% = blStatusOk%
510607 RETURN
510700 LABEL blApiSetValue
510701 blCheckH% = blReqHandle%
510702 GOSUB blValidateHandle
510703 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
510704 IF blCtl%(blReqHandle%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : RETURN
510705 blV# = blReqValue#
510706 IF blV# < blUser#(blReqHandle%,3) THEN blV# = blUser#(blReqHandle%,3)
510707 IF blV# > blUser#(blReqHandle%,4) THEN blV# = blUser#(blReqHandle%,4)
510708 blUser#(blReqHandle%,2) = blV#
510709 blNeedsPaint% = 1
510710 blStatus% = blStatusOk%
510711 RETURN
510800 LABEL blApiGetValue
510801 blCheckH% = blReqHandle%
510802 GOSUB blValidateHandle
510803 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
510804 IF blCtl%(blReqHandle%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : RETURN
510805 blRetValue# = blUser#(blReqHandle%,2)
510806 blStatus% = blStatusOk%
510807 RETURN
510900 LABEL blApiSetLayout
510901 blCheckH% = blReqHandle%
510902 GOSUB blValidateHandle
510903 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
510904 IF blCtl%(blReqHandle%,1) <> blClassPanel% AND blCtl%(blReqHandle%,1) <> blClassTop% THEN blStatus% = blStatusInvalidClass% : RETURN
510905 blPanel%(blReqHandle%,1) = blReqMode%
510906 blPanel%(blReqHandle%,2) = blReqPad%
510907 blPanel%(blReqHandle%,3) = blReqGap%
510908 blDirtyH% = blReqHandle%
510909 GOSUB blSetDirty
510910 blStatus% = blStatusOk%
510911 RETURN
511000 LABEL blApiSetAction
511001 blCheckH% = blReqHandle%
511002 GOSUB blValidateHandle
511003 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
511004 blCtl%(blReqHandle%,15) = blReqAction%
511005 blStatus% = blStatusOk%
511006 RETURN
511100 LABEL blApiInvalidate
511101 blCheckH% = blReqHandle%
511102 GOSUB blValidateHandle
511103 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
511104 blNeedsPaint% = 1
511105 blStatus% = blStatusOk%
511106 RETURN
511200 LABEL blApiLayoutAll
511201 GOSUB blLayoutAll
511202 blStatus% = blStatusOk%
511203 RETURN
511300 LABEL blApiPaintAll
511301 GOSUB blPaintAll
511302 blStatus% = blStatusOk%
511303 RETURN
511400 LABEL blApiUpdate
511401 blCheckH% = blReqHandle%
511402 GOSUB blValidateHandle
511403 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
511404 blPaintH% = blReqHandle%
511405 GOSUB blPaintTree
511406 blStatus% = blStatusOk%
511407 RETURN
511500 LABEL blApiPollInput
511501 GOSUB blPollInput
511502 blStatus% = blStatusOk%
511503 RETURN
511550 LABEL blApiPollMouse
511551 GOTO 511500
511600 LABEL blApiNextEvent
511601 GOSUB blReadEvent
511602 RETURN
511700 LABEL blApiFindAt
511701 blHitX% = blReqX%
511702 blHitY% = blReqY%
511703 GOSUB blHitTest
511704 blRetHandle% = blHitHandle%
511705 blStatus% = blStatusOk%
511706 RETURN
511800 LABEL blApiTestClickAt
511801 blTestX% = blReqX%
511802 blTestY% = blReqY%
511803 blTestButton% = blReqButton%
511804 GOSUB blTestClick
511805 blStatus% = blStatusOk%
511806 RETURN
511810 LABEL blApiTestMouseMove
511811 blTestX% = blReqX%
511812 blTestY% = blReqY%
511813 GOSUB blTestMove
511814 blStatus% = blStatusOk%
511815 RETURN
511820 LABEL blApiTestMouseButton
511821 blTestX% = blReqX%
511822 blTestY% = blReqY%
511823 blTestButton% = blReqButton%
511824 blTestDown% = blReqDown%
511825 GOSUB blTestButton
511826 blStatus% = blStatusOk%
511827 RETURN
511830 LABEL blApiTestWheel
511831 blTestX% = blReqX%
511832 blTestY% = blReqY%
511833 blTestWheelDelta% = blReqWheel%
511834 GOSUB blTestWheel
511835 blStatus% = blStatusOk%
511836 RETURN
511900 LABEL blApiDump
511901 FOR blH% = 1 TO blMaxControls%
511902   IF blCtl%(blH%,1) <> 0 THEN PRINT "H"; blH%; " C"; blCtl%(blH%,1); " P"; blCtl%(blH%,2); " X"; blCtl%(blH%,11); " Y"; blCtl%(blH%,12); " W"; blCtl%(blH%,9); " H"; blCtl%(blH%,10)
511903 NEXT blH%
511904 blStatus% = blStatusOk%
511905 RETURN
511910 LABEL blApiAutoSize
511911 blCheckH% = blReqHandle%
511912 GOSUB blValidateHandle
511913 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
511914 IF blCtl%(blReqHandle%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : RETURN
511915 GOSUB blAutoSizeH
511916 blStatus% = blStatusOk%
511917 RETURN
511920 LABEL blApiSetActionLine
511921 IF blReqActionId% < 0 OR blReqActionId% > 255 THEN blStatus% = blStatusInvalidValue% : RETURN
511922 IF blReqLine& < 0 OR blReqLine& > 2147483647 THEN blStatus% = blStatusInvalidValue% : RETURN
511923 blActionLine&(blReqActionId%) = blReqLine&
511924 blStatus% = blStatusOk%
511925 RETURN
511930 LABEL blApiRegisterClass
511931 IF blReqClass% < 5 OR blReqClass% > blMaxClasses% THEN blStatus% = blStatusInvalidClass% : RETURN
511932 IF blReqLine& <= 0 OR blReqLine& > 2147483647 THEN blStatus% = blStatusInvalidValue% : RETURN
511933 FOR blV% = 1 TO blMaxVerbs%
511934   blMethod&(blReqClass%,blV%) = blReqLine&
511935 NEXT blV%
511936 blClassName$(blReqClass%) = blReqText$
511937 blStatus% = blStatusOk%
511938 RETURN
511950 LABEL blApiSetEnabled
511951 blCheckH% = blReqHandle%
511952 GOSUB blValidateHandle
511953 IF blYes% = 0 THEN blStatus% = blStatusInvalidHandle% : RETURN
511954 IF blReqEnabled% <> 0 THEN blCtl%(blReqHandle%,13) = blCtl%(blReqHandle%,13) OR blFlagEnabled% : GOTO blEnabledDone
511955 blCtl%(blReqHandle%,13) = blCtl%(blReqHandle%,13) AND (blFlagEnabled% XOR 32767)
511956 LABEL blEnabledDone
511957 blNeedsPaint% = 1
511958 blStatus% = blStatusOk%
511959 RETURN
512000 REM debug/test helper area
512010 LABEL blTestErl
512020 ERROR 5
512030 RETURN
512031 END
