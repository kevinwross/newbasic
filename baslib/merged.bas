10	REM baslib control gallery: panel, button, label, click target, slider, gauge, dial.
20	REM Host lines stay below 500000; merge baslib.bas before RUN.
30	SCREEN 720
40	CALL blInit(0, 0)
50	IF blStatus% <> blStatusOk% THEN GOTO 9000
60	REM --- panel ---
70	PanelH% = blCreate%(1, 2, 0, 40, 30, 640, 520, "", 0, 0, 0)
80	CALL blSetLayout(PanelH%, 1, 12, 8)
90	IF blStatus% <> blStatusOk% THEN GOTO 9000
100	REM --- header ---
110	HeaderH% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 24, "baslib control gallery", 0, 0, 0)
120	blUser#(HeaderH%,13) = 4
130	REM --- button ---
140	ButtonH% = blCreate%(PanelH%, 3, 1, 0, 0, 100, 32, "Click me", 100, 0, 0)
150	blUser#(ButtonH%,13) = 6
160	REM --- slider ---
170	SlideLbl% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 20, "Slider - click at a position", 0, 0, 0)
180	blUser#(SlideLbl%,13) = 2
190	SliderH% = blCreate%(PanelH%, 3, 2, 0, 0, 100, 28, "", 150, 0, 0)
200	CALL blSetValue(SliderH%, 25)
210	REM --- gauge ---
220	GaugeLbl% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 20, "Gauge - read only", 0, 0, 0)
230	blUser#(GaugeLbl%,13) = 2
240	GaugeH% = blCreate%(PanelH%, 3, 3, 0, 0, 100, 28, "", 0, 0, 0)
250	CALL blSetValue(GaugeH%, 60)
260	REM --- dial ---
270	DialLbl% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 20, "Dial - click at a position", 0, 0, 0)
280	blUser#(DialLbl%,13) = 2
290	DialH% = blCreate%(PanelH%, 3, 4, 0, 0, 100, 96, "", 160, 0, 0)
300	CALL blSetValue(DialH%, 35)
310	REM --- readout ---
320	ReadoutH% = blCreate%(PanelH%, 3, 5, 0, 0, 100, 76, "Slider 25" + CHR$(13) + "Gauge 60" + CHR$(13) + "Dial 35", 0, 0, 0)
330	blUser#(ReadoutH%,13) = 6
340	REM --- invisible click target ---
350	TargetH% = blCreate%(PanelH%, 4, 0, 0, 0, 100, 48, "", 200, 0, 0)
360	CALL blSetClickMask(TargetH%, 1)
370	CALL blPaintAll
380	REM --- action-line table ---
390	CALL blSetActionLine(100, 1010)
400	CALL blSetActionLine(200, 2010)
410	CALL blSetActionLine(150, 3010)
420	CALL blSetActionLine(160, 4010)
430	REM --- main loop ---
440	ClickH% = blPollInput%()
450	WHILE blEventCount%() > 0
460	EventTyp% = blNextEvent%()
470	ActionId% = blEventAction%()
480	IF ActionId% <> blActionNone% THEN GOSUB blActionLine&(ActionId%)
490	WEND
500	KeyChar$ = INKEY$ : IF KeyChar$ = CHR$(27) THEN END
510	CALL blUpdateAll
520	SLEEP 16
530	GOTO 440
1000	REM --- button action ---
1010	CALL blSetText(ReadoutH%, "Button clicked" + CHR$(13) + "at " + TIME$)
1020	CALL blUpdateAll
1030	RETURN
2000	REM --- click-target action ---
2010	CALL blSetText(ReadoutH%, "Target clicked" + CHR$(13) + "at " + TIME$)
2020	CALL blUpdateAll
2030	RETURN
3000	REM --- slider action ---
3010	GOSUB 8000
3020	RETURN
4000	REM --- dial action ---
4010	GOSUB 8000
4020	RETURN
8000	REM --- refresh the value readout ---
8010	SlideVal# = blGetValue#(SliderH%)
8020	GaugeVal# = blGetValue#(GaugeH%)
8030	DialVal# = blGetValue#(DialH%)
8040	CALL blSetText(ReadoutH%, "Slider " + STR$(INT(SlideVal#)) + CHR$(13) + "Gauge " + STR$(INT(GaugeVal#)) + CHR$(13) + "Dial " + STR$(INT(DialVal#)))
8050	CALL blUpdateAll
8060	RETURN
9000	PRINT "Control gallery failed at status"; blStatus% : END
500000	END
500010	CONST blVersion$ = "0.3"
500011	CONST blVersionNum# = 0.3
500012	CONST blMaxControls% = 255
500013	CONST blMaxFrames% = 32
500014	CONST blMaxArgs% = 8
500015	CONST blMaxStrArgs% = 2
500016	CONST blMaxClasses% = 16
500017	CONST blMaxVerbs% = 20
500018	CONST blMaxKinds% = 8
500019	CONST blMaxEvents% = 64
500020	CONST blHandleRoot% = 1
500021	CONST blActionNone% = 0
500022	CONST blStatusOk% = 0
500023	CONST blStatusNotInit% = 1
500024	CONST blStatusControlTableFull% = 2
500025	CONST blStatusInvalidHandle% = 3
500026	CONST blStatusInvalidClass% = 4
500027	CONST blStatusInvalidVerb% = 5
500028	CONST blStatusFrameOverflow% = 6
500029	CONST blStatusInvalidParent% = 7
500030	CONST blStatusCannotDestroyRoot% = 8
500031	CONST blStatusInvalidGeometry% = 9
500032	CONST blStatusInvalidValue% = 10
500033	CONST blStatusInvalidLayout% = 11
500034	CONST blStatusEventQueueFull% = 12
500035	CONST blStatusEventQueueEmpty% = 13
500036	CONST blStatusNoGraphics% = 14
500037	CONST blStatusFrameProtocol% = 15
500038	CONST blStatusMissingActionHandler% = 16
500039	CONST blStatusClassTableFull% = 17
500040	CONST blClassTop% = 1
500041	CONST blClassPanel% = 2
500042	CONST blClassUser% = 3
500043	CONST blClassClickTarget% = 4
500044	CONST blVerbPaint% = 1
500045	CONST blVerbUpdate% = 2
500046	CONST blVerbShow% = 3
500047	CONST blVerbHide% = 4
500048	CONST blVerbLayout% = 5
500049	CONST blVerbClick% = 6
500050	CONST blVerbHitTest% = 7
500051	CONST blVerbMouseDown% = 8
500052	CONST blVerbMouseUp% = 9
500053	CONST blVerbMouseMove% = 10
500054	CONST blVerbMouseEnter% = 11
500055	CONST blVerbMouseLeave% = 12
500056	CONST blVerbWheel% = 13
500057	CONST blVerbCaptureLost% = 14
500058	CONST blVerbDestroy% = 15
500059	CONST blVerbKey% = 16
500060	CONST blVerbFocus% = 17
500061	CONST blKindButton% = 1
500062	CONST blKindSlider% = 2
500063	CONST blKindGauge% = 3
500064	CONST blKindDial% = 4
500065	CONST blKindLabel% = 5
500066	CONST blEvClick% = 1
500067	CONST blEvChange% = 2
500068	CONST blEvMove% = 3
500069	CONST blEvEnter% = 4
500070	CONST blEvLeave% = 5
500071	CONST blEvDown% = 6
500072	CONST blEvUp% = 7
500073	CONST blEvWheel% = 8
500074	CONST blEvKey% = 9
500075	CONST blEvFocus% = 10
500076	CONST blEvCaptureLost% = 11
500077	CONST blFlagVisible% = 1
500078	CONST blFlagEnabled% = 2
500079	CONST blFlagClip% = 4
500080	CONST blFlagDirty% = 8
500081	CONST blFlagHot% = 16
500082	CONST blFlagPressed% = 32
500083	CONST blFlagClickable% = 64
500084	CONST blFlagFocusable% = 128
500085	CONST blFlagTransparent% = 256
500086	CONST blFlagLayoutDirty% = 512
500087	CONST blFlagCaptured% = 1024
500088	CONST blNoLine& = 0
500100	FUNCTION blClamp#(Value#, LowVal#, HighVal#)
500101	blClamp# = Value#
500102	IF blClamp# < LowVal# THEN blClamp# = LowVal#
500103	IF blClamp# > HighVal# THEN blClamp# = HighVal#
500104	END FUNCTION
500110	FUNCTION blAllocHandle%()
500111	LOCAL Index%
500112	blAllocHandle% = 0
500113	FOR Index% = 1 TO blMaxControls%
500114	IF blCtl%(Index%,1) = 0 THEN blAllocHandle% = Index% : blStatus% = blStatusOk% : EXIT FUNCTION
500115	NEXT Index%
500116	blStatus% = blStatusControlTableFull%
500117	END FUNCTION
500120	SUB blLinkHandle(Handle%)
500121	LOCAL Parent%, LastChld%
500122	Parent% = blCtl%(Handle%,2)
500123	IF Parent% < 1 OR Parent% > blMaxControls% THEN EXIT SUB
500124	LastChld% = blCtl%(Parent%,4)
500125	IF LastChld% = 0 THEN blCtl%(Parent%,3) = Handle% : blCtl%(Handle%,5) = 0 : blCtl%(Handle%,6) = 0 : GOTO blLinkDone
500126	blCtl%(LastChld%,6) = Handle%
500127	blCtl%(Handle%,5) = LastChld%
500128	LABEL blLinkDone
500129	blCtl%(Parent%,4) = Handle%
500130	END SUB
500140	SUB blDetachHandle(Handle%)
500141	LOCAL Parent%, PrevSib%, NextSib%
500142	Parent% = blCtl%(Handle%,2)
500143	IF Parent% < 1 OR Parent% > blMaxControls% THEN EXIT SUB
500144	PrevSib% = blCtl%(Handle%,5)
500145	NextSib% = blCtl%(Handle%,6)
500146	IF PrevSib% <> 0 THEN blCtl%(PrevSib%,6) = NextSib% ELSE blCtl%(Parent%,3) = NextSib%
500147	IF NextSib% <> 0 THEN blCtl%(NextSib%,5) = PrevSib% ELSE blCtl%(Parent%,4) = PrevSib%
500148	blCtl%(Handle%,5) = 0
500149	blCtl%(Handle%,6) = 0
500150	END SUB
500151	SUB blMarkDirty(Handle%)
500152	LOCAL Parent%
500153	IF Handle% < 1 OR Handle% > blMaxControls% THEN EXIT SUB
500154	IF blCtl%(Handle%,1) = 0 THEN EXIT SUB
500155	blCtl%(Handle%,13) = blCtl%(Handle%,13) OR blFlagDirty%
500156	Parent% = blCtl%(Handle%,2)
500157	IF Parent% >= 1 AND Parent% <= blMaxControls% THEN blCtl%(Parent%,13) = blCtl%(Parent%,13) OR blFlagDirty%
500158	blNeedsPaint% = 1
500159	END SUB
500160	SUB blInitTables
500161	LOCAL Index%, ClassId%, VerbId%
500162	IF blInitDone% <> 0 THEN GOTO blInitClear
500163	DIM blCtl%(255,17)
500164	DIM blCtlText$(255)
500165	DIM blPanel%(255,8), blUser#(255,22), blClick%(255,4)
500166	DIM blCtlClipL%(255), blCtlClipT%(255), blCtlClipR%(255), blCtlClipB%(255)
500167	DIM blCtlZ%(255)
500168	DIM blMethod&(16,20), blActionLine&(255)
500169	DIM blClassName$(16)
500170	DIM blEvType%(63), blEvCtl%(63), blEvAction%(63), blEvX%(63), blEvY%(63)
500171	DIM blEvButton%(63), blEvWheelVal%(63)
500172	blInitDone% = 1
500173	LABEL blInitClear
500174	FOR Index% = 0 TO 255
500175	blCtl%(Index%,1) = 0
500176	blCtl%(Index%,2) = 0
500177	blCtl%(Index%,3) = 0
500178	blCtl%(Index%,4) = 0
500179	blCtl%(Index%,5) = 0
500180	blCtl%(Index%,6) = 0
500181	blCtl%(Index%,7) = 0
500182	blCtl%(Index%,8) = 0
500183	blCtl%(Index%,9) = 0
500184	blCtl%(Index%,10) = 0
500185	blCtl%(Index%,11) = 0
500186	blCtl%(Index%,12) = 0
500187	blCtl%(Index%,13) = 0
500188	blCtl%(Index%,14) = 0
500189	blCtl%(Index%,15) = 0
500190	blCtl%(Index%,16) = 0
500191	blCtl%(Index%,17) = 0
500192	blCtlText$(Index%) = ""
500193	blCtlClipL%(Index%) = 0
500194	blCtlClipT%(Index%) = 0
500195	blCtlClipR%(Index%) = 0
500196	blCtlClipB%(Index%) = 0
500197	blCtlZ%(Index%) = 0
500198	NEXT Index%
500199	FOR ClassId% = 1 TO 16
500200	blClassName$(ClassId%) = ""
500201	FOR VerbId% = 1 TO 20
500202	blMethod&(ClassId%,VerbId%) = 502999
500203	NEXT VerbId%
500204	NEXT ClassId%
500205	blClassName$(1) = "Top"
500206	blClassName$(2) = "Panel"
500207	blClassName$(3) = "User"
500208	blClassName$(4) = "ClickTarget"
500209	FOR Index% = 0 TO 255
500210	blActionLine&(Index%) = 0
500211	NEXT Index%
500212	FOR Index% = 0 TO 63
500213	blEvType%(Index%) = 0
500214	blEvCtl%(Index%) = 0
500215	blEvAction%(Index%) = 0
500216	blEvX%(Index%) = 0
500217	blEvY%(Index%) = 0
500218	blEvButton%(Index%) = 0
500219	blEvWheelVal%(Index%) = 0
500220	NEXT Index%
500221	END SUB
500222	SUB blClearDirtyTree
500223	LOCAL Index%
500224	FOR Index% = 1 TO blMaxControls%
500225	blCtl%(Index%,13) = blCtl%(Index%,13) AND (blFlagDirty% XOR 32767)
500226	NEXT Index%
500227	END SUB
500230	SUB blLayoutControl(Handle%)
500231	LOCAL ClassId%
500232	IF Handle% < 1 OR Handle% > blMaxControls% THEN EXIT SUB
500233	IF blCtl%(Handle%,1) = 0 THEN EXIT SUB
500234	IF (blCtl%(Handle%,13) AND blFlagVisible%) = 0 THEN EXIT SUB
500235	ClassId% = blCtl%(Handle%,1)
500236	IF ClassId% = blClassTop% OR ClassId% = blClassPanel% THEN CALL blLayoutChildren(Handle%)
500237	END SUB
500240	SUB blLayoutChildren(Handle%)
500241	LOCAL Mode%, PadSize%, GapSize%, PanelX%, PanelY%, ClientW%, ClientH%, CursorX%, CursorY%
500242	LOCAL Child%, NextSib%, RightX%, BottomY%, ClipL%, ClipT%, ClipR%, ClipB%
500243	Mode% = blPanel%(Handle%,1)
500244	PadSize% = blPanel%(Handle%,2)
500245	GapSize% = blPanel%(Handle%,3)
500246	PanelX% = blCtl%(Handle%,11)
500247	PanelY% = blCtl%(Handle%,12)
500248	ClientW% = blCtl%(Handle%,9)
500249	ClientH% = blCtl%(Handle%,10)
500250	CursorX% = PanelX% + PadSize%
500251	CursorY% = PanelY% + PadSize%
500252	Child% = blCtl%(Handle%,3)
500253	WHILE Child% <> 0
500254	IF (blCtl%(Child%,13) AND blFlagVisible%) = 0 THEN GOTO blLayoutNext
500255	SELECT CASE Mode%
500256	CASE 1
500257	blCtl%(Child%,7) = PadSize%
500258	blCtl%(Child%,8) = CursorY% - PanelY%
500259	blCtl%(Child%,9) = ClientW% - 2 * PadSize%
500260	CursorY% = CursorY% + blCtl%(Child%,10) + GapSize%
500261	CASE 2
500262	blCtl%(Child%,7) = CursorX% - PanelX%
500263	blCtl%(Child%,8) = PadSize%
500264	blCtl%(Child%,10) = ClientH% - 2 * PadSize%
500265	CursorX% = CursorX% + blCtl%(Child%,9) + GapSize%
500266	END SELECT
500267	blCtl%(Child%,11) = PanelX% + blCtl%(Child%,7)
500268	blCtl%(Child%,12) = PanelY% + blCtl%(Child%,8)
500269	ClipL% = blCtlClipL%(Handle%)
500270	ClipT% = blCtlClipT%(Handle%)
500271	ClipR% = blCtlClipR%(Handle%)
500272	ClipB% = blCtlClipB%(Handle%)
500273	IF blCtl%(Child%,11) > ClipL% THEN ClipL% = blCtl%(Child%,11)
500274	IF blCtl%(Child%,12) > ClipT% THEN ClipT% = blCtl%(Child%,12)
500275	RightX% = blCtl%(Child%,11) + blCtl%(Child%,9) - 1
500276	BottomY% = blCtl%(Child%,12) + blCtl%(Child%,10) - 1
500277	IF RightX% < ClipR% THEN ClipR% = RightX%
500278	IF BottomY% < ClipB% THEN ClipB% = BottomY%
500279	blCtlClipL%(Child%) = ClipL%
500280	blCtlClipT%(Child%) = ClipT%
500281	blCtlClipR%(Child%) = ClipR%
500282	blCtlClipB%(Child%) = ClipB%
500283	NextSib% = blCtl%(Child%,6)
500284	CALL blLayoutControl(Child%)
500285	Child% = NextSib%
500286	GOTO blLayoutLoopEnd
500287	LABEL blLayoutNext
500288	Child% = blCtl%(Child%,6)
500289	LABEL blLayoutLoopEnd
500290	WEND
500291	END SUB
500300	SUB blPaintTree(Handle%)
500301	LOCAL Child%, NextSib%
500302	IF Handle% < 1 OR Handle% > blMaxControls% THEN EXIT SUB
500303	IF blCtl%(Handle%,1) = 0 THEN EXIT SUB
500304	IF (blCtl%(Handle%,13) AND blFlagVisible%) = 0 THEN EXIT SUB
500305	CALL blPaintOne(Handle%)
500306	Child% = blCtl%(Handle%,3)
500307	WHILE Child% <> 0
500308	NextSib% = blCtl%(Child%,6)
500309	CALL blPaintTree(Child%)
500310	Child% = NextSib%
500311	WEND
500312	END SUB
500320	SUB blPaintOne(Handle%)
500321	LOCAL Width%, Height%, ClassId%
500322	IF Handle% < 1 OR Handle% > blMaxControls% THEN EXIT SUB
500323	IF blCtl%(Handle%,1) = 0 THEN EXIT SUB
500324	Width% = blCtl%(Handle%,9)
500325	Height% = blCtl%(Handle%,10)
500326	IF Width% <= 0 OR Height% <= 0 THEN EXIT SUB
500327	PUSH VIEW
500328	ClassId% = blCtl%(Handle%,1)
500329	IF ClassId% = blClassTop% OR ClassId% = blClassPanel% THEN CALL blPaintPanel(Handle%) : GOTO blPaintOneDone
500330	IF ClassId% = blClassUser% THEN CALL blPaintUser(Handle%)
500331	LABEL blPaintOneDone
500332	POP VIEW
500333	END SUB
500340	SUB blPaintPanel(Handle%)
500341	LOCAL PosX%, PosY%, Width%, Height%, FaceCol%, Border%
500342	PosX% = blCtl%(Handle%,11)
500343	PosY% = blCtl%(Handle%,12)
500344	Width% = blCtl%(Handle%,9)
500345	Height% = blCtl%(Handle%,10)
500346	FaceCol% = blPanel%(Handle%,6)
500347	Border% = blPanel%(Handle%,7)
500348	LINE (PosX%,PosY%)-(PosX% + Width% - 1,PosY% + Height% - 1), FaceCol%, BF
500349	IF Border% >= 0 THEN LINE (PosX%,PosY%)-(PosX% + Width% - 1,PosY% + Height% - 1), Border%, B
500350	END SUB
500360	SUB blPaintUser(Handle%)
500361	LOCAL Kind%
500362	Kind% = blUser#(Handle%,1)
500363	SELECT CASE Kind%
500364	CASE blKindButton%
500365	CALL blPaintButton(Handle%)
500366	CASE blKindLabel%
500367	CALL blPaintLabel(Handle%)
500368	CASE blKindSlider%
500369	CALL blPaintSlider(Handle%)
500370	CASE blKindGauge%
500371	CALL blPaintGauge(Handle%)
500372	CASE blKindDial%
500373	CALL blPaintDial(Handle%)
500374	END SELECT
500375	END SUB
500380	SUB blPaintButton(Handle%)
500381	LOCAL PosX%, PosY%, Width%, Height%, FaceCol%, TextCol%, Border%, Scale%
500382	PosX% = blCtl%(Handle%,11)
500383	PosY% = blCtl%(Handle%,12)
500384	Width% = blCtl%(Handle%,9)
500385	Height% = blCtl%(Handle%,10)
500386	FaceCol% = blUser#(Handle%,8)
500387	TextCol% = blUser#(Handle%,19)
500388	Border% = blUser#(Handle%,17)
500389	Scale% = blUser#(Handle%,20)
500390	IF (blCtl%(Handle%,13) AND blFlagPressed%) <> 0 THEN FaceCol% = blUser#(Handle%,9)
500391	LINE (PosX%,PosY%)-(PosX% + Width% - 1,PosY% + Height% - 1), FaceCol%, BF
500392	IF Border% >= 0 THEN LINE (PosX%,PosY%)-(PosX% + Width% - 1,PosY% + Height% - 1), Border%, B
500393	CALL blDrawText(PosX%,PosY%,Width%,Height%,blCtlText$(Handle%),TextCol%,Scale%,1,1,2)
500394	END SUB
500400	SUB blPaintLabel(Handle%)
500401	LOCAL PosX%, PosY%, Width%, Height%, FaceCol%, TextCol%, Border%, Scale%, Align%, VAlign%, PadSize%, LabelText$
500402	PosX% = blCtl%(Handle%,11)
500403	PosY% = blCtl%(Handle%,12)
500404	Width% = blCtl%(Handle%,9)
500405	Height% = blCtl%(Handle%,10)
500406	FaceCol% = blUser#(Handle%,8)
500407	TextCol% = blUser#(Handle%,19)
500408	Border% = blUser#(Handle%,17)
500409	Scale% = blUser#(Handle%,20)
500410	Align% = blUser#(Handle%,14)
500411	VAlign% = blUser#(Handle%,15)
500412	PadSize% = blUser#(Handle%,13)
500413	LabelText$ = blCtlText$(Handle%)
500414	IF blUser#(Handle%,16) <> 0 THEN LabelText$ = blWrapText$(LabelText$,Width% - 2 * PadSize%,Scale%)
500415	IF blUser#(Handle%,18) = 0 THEN LINE (PosX%,PosY%)-(PosX% + Width% - 1,PosY% + Height% - 1), FaceCol%, BF
500416	IF Border% >= 0 THEN LINE (PosX%,PosY%)-(PosX% + Width% - 1,PosY% + Height% - 1), Border%, B
500417	CALL blDrawText(PosX%,PosY%,Width%,Height%,LabelText$,TextCol%,Scale%,Align%,VAlign%,PadSize%)
500418	END SUB
500430	SUB blPaintSlider(Handle%)
500431	LOCAL PosX%, PosY%, Width%, Height%, BarPos%, Accent%
500432	PosX% = blCtl%(Handle%,11)
500433	PosY% = blCtl%(Handle%,12)
500434	Width% = blCtl%(Handle%,9)
500435	Height% = blCtl%(Handle%,10)
500436	Accent% = blUser#(Handle%,9)
500437	IF Width% <= 0 THEN EXIT SUB
500438	LINE (PosX%,PosY% + Height% / 2)-(PosX% + Width% - 1,PosY% + Height% / 2), 8, BF
500439	BarPos% = PosX% + (Width% - 8) * (blUser#(Handle%,2) - blUser#(Handle%,3)) / (blUser#(Handle%,4) - blUser#(Handle%,3) + 0.000001)
500440	LINE (BarPos%,PosY% + 2)-(BarPos% + 7,PosY% + Height% - 2), Accent%, BF
500441	END SUB
500450	SUB blPaintGauge(Handle%)
500451	LOCAL PosX%, PosY%, Width%, Height%, FaceCol%, Accent%, BarPos%
500452	PosX% = blCtl%(Handle%,11)
500453	PosY% = blCtl%(Handle%,12)
500454	Width% = blCtl%(Handle%,9)
500455	Height% = blCtl%(Handle%,10)
500456	FaceCol% = blUser#(Handle%,8)
500457	Accent% = blUser#(Handle%,9)
500458	LINE (PosX%,PosY%)-(PosX% + Width% - 1,PosY% + Height% - 1), FaceCol%, BF
500459	BarPos% = PosX% + (Width% - 2) * (blUser#(Handle%,2) - blUser#(Handle%,3)) / (blUser#(Handle%,4) - blUser#(Handle%,3) + 0.000001)
500460	LINE (PosX% + 1,PosY% + 1)-(BarPos%,PosY% + Height% - 2), Accent%, BF
500461	END SUB
500470	SUB blPaintDial(Handle%)
500471	LOCAL PosX%, PosY%, Width%, Height%, FaceCol%, Accent%, Radius%, CenterX%, CenterY%, Angle#, EndX%, EndY%
500472	PosX% = blCtl%(Handle%,11)
500473	PosY% = blCtl%(Handle%,12)
500474	Width% = blCtl%(Handle%,9)
500475	Height% = blCtl%(Handle%,10)
500476	FaceCol% = blUser#(Handle%,8)
500477	Accent% = blUser#(Handle%,9)
500478	Radius% = Height% / 2 - 2
500479	IF Width% < Height% THEN Radius% = Width% / 2 - 2
500480	CenterX% = PosX% + Width% / 2
500481	CenterY% = PosY% + Height% / 2
500482	CIRCLE (CenterX%,CenterY%), Radius%, FaceCol%, 0, 6.2831853, 1
500483	CIRCLE (CenterX%,CenterY%), Radius% - 1, Accent%, 0, 6.2831853, 1
500484	Angle# = 3.14159265 * (1 - (blUser#(Handle%,2) - blUser#(Handle%,3)) / (blUser#(Handle%,4) - blUser#(Handle%,3) + 0.000001))
500485	EndX% = CenterX% + COS(Angle#) * (Radius% - 4)
500486	EndY% = CenterY% - SIN(Angle#) * (Radius% - 4)
500487	LINE (CenterX%,CenterY%)-(EndX%,EndY%), Accent%
500488	END SUB
500500	SUB blDrawText(PosX%, PosY%, Width%, Height%, Text$, Color%, Scale%, Align%, VAlign%, PadSize%)
500501	LOCAL LabelText$, TextW%, TextH%, DrawX%, DrawY%, FitWidth%
500502	IF Text$ = "" THEN EXIT SUB
500503	FitWidth% = Width% - 2 * PadSize%
500504	IF FitWidth% < 1 THEN FitWidth% = 1
500505	LabelText$ = blFitText$(Text$,FitWidth%,Scale%)
500506	TextW% = TEXTWIDTH(LabelText$,Scale%)
500507	TextH% = TEXTHEIGHT(LabelText$,Scale%)
500508	DrawX% = PosX% + PadSize%
500509	DrawY% = PosY% + PadSize%
500510	IF Align% = 1 THEN DrawX% = PosX% + (Width% - TextW%) / 2
500511	IF Align% = 2 THEN DrawX% = PosX% + Width% - TextW% - PadSize%
500512	IF VAlign% = 1 THEN DrawY% = PosY% + (Height% - TextH%) / 2
500513	IF VAlign% = 2 THEN DrawY% = PosY% + Height% - TextH% - PadSize%
500514	DRAWTEXT (DrawX%,DrawY%), LabelText$, Color%, Scale%
500515	END SUB
500520	FUNCTION blFitText$(Source$, MaxW%, Scale%)
500521	LOCAL SourceIn$, SourceOut$, Line$, Char$, Index%
500522	blFitText$ = ""
500523	IF MaxW% < 1 THEN MaxW% = 1
500524	IF Source$ = "" THEN EXIT FUNCTION
500525	SourceIn$ = Source$
500526	SourceOut$ = ""
500527	Line$ = ""
500528	FOR Index% = 1 TO LEN(SourceIn$)
500529	Char$ = MID$(SourceIn$,Index%,1)
500530	IF Char$ = CHR$(13) OR Char$ = CHR$(10) THEN GOTO blFitNewline
500531	Line$ = Line$ + Char$
500532	GOTO blFitNext
500533	LABEL blFitNewline
500534	WHILE LEN(Line$) > 0 AND TEXTWIDTH(Line$,Scale%) > MaxW%
500535	Line$ = LEFT$(Line$,LEN(Line$) - 1)
500536	WEND
500537	SourceOut$ = SourceOut$ + Line$ + CHR$(13) + CHR$(10)
500538	Line$ = ""
500539	LABEL blFitNext
500540	NEXT Index%
500541	WHILE LEN(Line$) > 0 AND TEXTWIDTH(Line$,Scale%) > MaxW%
500542	Line$ = LEFT$(Line$,LEN(Line$) - 1)
500543	WEND
500544	blFitText$ = SourceOut$ + Line$
500545	END FUNCTION
500550	FUNCTION blWrapText$(Source$, MaxW%, Scale%)
500551	LOCAL SourceIn$, SourceOut$, Line$, Char$, Index%, MaxWidth%
500552	blWrapText$ = ""
500553	SourceIn$ = Source$
500554	SourceOut$ = ""
500555	Line$ = ""
500556	MaxWidth% = MaxW%
500557	IF MaxWidth% < 1 THEN MaxWidth% = 1
500558	FOR Index% = 1 TO LEN(SourceIn$)
500559	Char$ = MID$(SourceIn$,Index%,1)
500560	IF Char$ = CHR$(13) OR Char$ = CHR$(10) THEN GOTO blWrapNewline
500561	IF LEN(Line$) > 0 AND TEXTWIDTH(Line$ + Char$,Scale%) > MaxWidth% THEN GOTO blWrapBreak
500562	Line$ = Line$ + Char$
500563	GOTO blWrapNext
500564	LABEL blWrapBreak
500565	SourceOut$ = SourceOut$ + Line$ + CHR$(13) + CHR$(10)
500566	Line$ = Char$
500567	GOTO blWrapNext
500568	LABEL blWrapNewline
500569	SourceOut$ = SourceOut$ + Line$ + CHR$(13) + CHR$(10)
500570	Line$ = ""
500571	LABEL blWrapNext
500572	NEXT Index%
500573	blWrapText$ = SourceOut$ + Line$
500574	END FUNCTION
500580	FUNCTION blHitTest%(PosX%, PosY%)
500581	LOCAL Handle%, LeftX%, TopY%, RightX%, BottomY%
500582	blHitTest% = 0
500583	FOR Handle% = blMaxControls% TO 1 STEP -1
500584	IF blCtl%(Handle%,1) = 0 OR (blCtl%(Handle%,13) AND blFlagVisible%) = 0 OR (blCtl%(Handle%,13) AND blFlagEnabled%) = 0 THEN GOTO blHitNext
500585	LeftX% = blCtl%(Handle%,11)
500586	TopY% = blCtl%(Handle%,12)
500587	RightX% = LeftX% + blCtl%(Handle%,9) - 1
500588	BottomY% = TopY% + blCtl%(Handle%,10) - 1
500589	IF PosX% >= LeftX% AND PosX% <= RightX% AND PosY% >= TopY% AND PosY% <= BottomY% THEN blHitTest% = Handle% : EXIT FUNCTION
500590	LABEL blHitNext
500591	NEXT Handle%
500592	END FUNCTION
500600	SUB blPostEvent(EventTyp%, CtlH%, Action%, PosX%, PosY%, Button%, Wheel%)
500601	IF blEvCount% >= blMaxEvents% THEN blStatus% = blStatusEventQueueFull% : EXIT SUB
500602	blEvType%(blEvTail%) = EventTyp%
500603	blEvCtl%(blEvTail%) = CtlH%
500604	blEvAction%(blEvTail%) = Action%
500605	blEvX%(blEvTail%) = PosX%
500606	blEvY%(blEvTail%) = PosY%
500607	blEvButton%(blEvTail%) = Button%
500608	blEvWheelVal%(blEvTail%) = Wheel%
500609	blEvTail% = (blEvTail% + 1) MOD blMaxEvents%
500610	blEvCount% = blEvCount% + 1
500611	END SUB
500620	SUB blPostClick(OwnerH%, Action%, PosX%, PosY%, Button%)
500621	CALL blPostEvent(blEvClick%,OwnerH%,Action%,PosX%,PosY%,Button%,0)
500622	END SUB
500630	SUB blPointerValue(Handle%, PosX%, PosY%, Button%)
500631	LOCAL Kind%, Value#, Width%
500632	Kind% = blUser#(Handle%,1)
500633	IF Kind% <> blKindSlider% AND Kind% <> blKindDial% THEN EXIT SUB
500634	Width% = blCtl%(Handle%,9)
500635	IF Width% <= 0 THEN EXIT SUB
500636	Value# = blUser#(Handle%,3) + (PosX% - blCtl%(Handle%,11)) * (blUser#(Handle%,4) - blUser#(Handle%,3)) / Width%
500637	Value# = blClamp#(Value#,blUser#(Handle%,3),blUser#(Handle%,4))
500638	blUser#(Handle%,2) = Value#
500639	CALL blPostEvent(blEvChange%,Handle%,blCtl%(Handle%,15),PosX%,PosY%,Button%,0)
500640	CALL blMarkDirty(Handle%)
500641	END SUB
500650	FUNCTION blClickAt%(PosX%, PosY%, Button%)
500651	LOCAL Handle%, Action%, Parent%
500652	blClickAt% = 0
500653	Handle% = blHitTest%(PosX%,PosY%)
500654	IF Handle% = 0 THEN EXIT FUNCTION
500655	IF blCtl%(Handle%,1) = blClassUser% THEN CALL blPointerValue(Handle%,PosX%,PosY%,Button%)
500656	Action% = blCtl%(Handle%,15)
500657	Parent% = blCtl%(Handle%,2)
500658	WHILE Action% = 0 AND Parent% <> 0
500659	IF blCtl%(Parent%,15) <> 0 THEN Action% = blCtl%(Parent%,15) : Handle% = Parent% : Parent% = 0 ELSE Parent% = blCtl%(Parent%,2)
500660	WEND
500661	blClickAt% = Handle%
500662	IF Action% <> 0 THEN CALL blPostClick(Handle%,Action%,PosX%,PosY%,Button%)
500663	END FUNCTION
500670	SUB blPostHover(OldHover%, NewHover%, PosX%, PosY%)
500671	IF OldHover% <> 0 THEN CALL blPostEvent(blEvLeave%,OldHover%,0,PosX%,PosY%,0,0)
500672	IF NewHover% <> 0 THEN CALL blPostEvent(blEvEnter%,NewHover%,0,PosX%,PosY%,0,0)
500673	END SUB
500680	SUB blPostMove(Handle%, PosX%, PosY%)
500681	CALL blPostEvent(blEvMove%,Handle%,0,PosX%,PosY%,0,0)
500682	END SUB
500690	SUB blTestMove(PosX%, PosY%)
500691	LOCAL OldHover%, Handle%
500692	OldHover% = blHover%
500693	Handle% = blHitTest%(PosX%,PosY%)
500694	blHover% = Handle%
500695	IF blCapture% <> 0 AND blPressed% <> 0 THEN CALL blPointerValue(blCapture%,PosX%,PosY%,blPressed%)
500696	IF OldHover% <> blHover% THEN CALL blPostHover(OldHover%,blHover%,PosX%,PosY%)
500697	IF blHover% <> 0 THEN CALL blPostMove(blHover%,PosX%,PosY%)
500698	END SUB
500700	SUB blTestButton(Button%, Down%, PosX%, PosY%)
500701	IF Down% = 0 THEN GOTO blButtonUp
500702	blPressed% = Button%
500703	IF blHover% <> 0 THEN blCapture% = blHover%
500704	CALL blPostEvent(blEvDown%,blHover%,0,PosX%,PosY%,Button%,0)
500705	GOTO blButtonDone
500706	LABEL blButtonUp
500707	CALL blPostEvent(blEvUp%,blCapture%,0,PosX%,PosY%,Button%,0)
500708	blPressed% = 0
500709	blCapture% = 0
500710	LABEL blButtonDone
500711	END SUB
500720	SUB blWheelAt(PosX%, PosY%, Delta%)
500721	IF blHover% = 0 THEN EXIT SUB
500722	CALL blPostEvent(blEvWheel%,blHover%,0,PosX%,PosY%,0,Delta%)
500723	END SUB
500730	SUB blRepaintDirty(Handle%)
500731	LOCAL Child%, NextSib%
500732	IF Handle% < 1 OR Handle% > blMaxControls% THEN EXIT SUB
500733	IF blCtl%(Handle%,1) = 0 THEN EXIT SUB
500734	IF (blCtl%(Handle%,13) AND blFlagVisible%) = 0 THEN EXIT SUB
500735	IF (blCtl%(Handle%,13) AND blFlagDirty%) <> 0 THEN CALL blPaintTree(Handle%) : EXIT SUB
500736	Child% = blCtl%(Handle%,3)
500737	WHILE Child% <> 0
500738	NextSib% = blCtl%(Child%,6)
500739	CALL blRepaintDirty(Child%)
500740	Child% = NextSib%
500741	WEND
500742	END SUB
502999	LABEL blNoop
503000	RETURN
510000	SUB blInit(Width%, Height%)
510001	CALL blInitTables
510002	blRoot% = 1
510003	blNeedsPaint% = 1
510004	blNeedsLayout% = 1
510005	blHover% = 0
510006	blPressed% = 0
510007	blCapture% = 0
510008	blEvHead% = 0
510009	blEvTail% = 0
510010	blEvCount% = 0
510011	blCurType% = 0
510012	blCurCtl% = 0
510013	blCurAction% = 0
510014	blCurX% = 0
510015	blCurY% = 0
510016	blCurButton% = 0
510017	blCurWheel% = 0
510018	blStatus% = blStatusOk%
510019	blCtl%(1,1) = blClassTop%
510020	blCtl%(1,2) = 0
510021	blCtl%(1,3) = 0
510022	blCtl%(1,4) = 0
510023	blCtl%(1,5) = 0
510024	blCtl%(1,6) = 0
510025	blCtl%(1,7) = 0
510026	blCtl%(1,8) = 0
510027	blCtl%(1,9) = 640
510028	IF Width% > 0 THEN blCtl%(1,9) = Width%
510029	IF Width% <= 0 AND SCREENW > 0 THEN blCtl%(1,9) = SCREENW
510030	blCtl%(1,10) = 400
510031	IF Height% > 0 THEN blCtl%(1,10) = Height%
510032	IF Height% <= 0 AND SCREENH > 0 THEN blCtl%(1,10) = SCREENH
510033	blCtl%(1,11) = 0
510034	blCtl%(1,12) = 0
510035	blCtl%(1,13) = blFlagVisible% OR blFlagEnabled% OR blFlagClip%
510036	blCtl%(1,14) = 0
510037	blCtl%(1,15) = 0
510038	blCtl%(1,16) = 0
510039	blCtl%(1,17) = 0
510040	blCtlClipL%(1) = 0
510041	blCtlClipT%(1) = 0
510042	blCtlClipR%(1) = blCtl%(1,9) - 1
510043	blCtlClipB%(1) = blCtl%(1,10) - 1
510044	blCtlZ%(1) = 1
510045	blPanel%(1,1) = 0
510046	blPanel%(1,2) = 0
510047	blPanel%(1,3) = 0
510048	blPanel%(1,4) = 0
510049	blPanel%(1,6) = 7
510050	blPanel%(1,7) = 8
510051	CALL blMarkDirty(1)
510052	END SUB
510060	FUNCTION blCreate%(Parent%, Class%, Kind%, PosX%, PosY%, Width%, Height%, Text$, Action%, Flags%, TagId%)
510061	LOCAL Handle%
510062	blCreate% = 0
510063	blStatus% = blStatusOk%
510064	IF Parent% < 1 OR Parent% > blMaxControls% THEN blStatus% = blStatusInvalidParent% : EXIT FUNCTION
510065	IF blCtl%(Parent%,1) = 0 THEN blStatus% = blStatusInvalidParent% : EXIT FUNCTION
510066	IF Class% < 2 OR Class% > 4 THEN blStatus% = blStatusInvalidClass% : EXIT FUNCTION
510067	IF Class% = blClassUser% AND (Kind% < 1 OR Kind% > 5) THEN blStatus% = blStatusInvalidValue% : EXIT FUNCTION
510068	Handle% = blAllocHandle%()
510069	IF Handle% = 0 THEN EXIT FUNCTION
510070	blCtl%(Handle%,1) = Class%
510071	blCtl%(Handle%,2) = Parent%
510072	blCtl%(Handle%,3) = 0
510073	blCtl%(Handle%,4) = 0
510074	blCtl%(Handle%,5) = 0
510075	blCtl%(Handle%,6) = 0
510076	blCtl%(Handle%,7) = PosX%
510077	blCtl%(Handle%,8) = PosY%
510078	blCtl%(Handle%,9) = Width%
510079	blCtl%(Handle%,10) = Height%
510080	blCtl%(Handle%,11) = PosX%
510081	blCtl%(Handle%,12) = PosY%
510082	blCtl%(Handle%,13) = Flags% OR blFlagVisible% OR blFlagEnabled%
510083	blCtl%(Handle%,14) = 0
510084	blCtl%(Handle%,15) = Action%
510085	blCtl%(Handle%,16) = TagId%
510086	blCtl%(Handle%,17) = 0
510087	blCtlText$(Handle%) = Text$
510088	blCtlZ%(Handle%) = Handle%
510089	IF Class% = blClassPanel% THEN blPanel%(Handle%,1) = 0 : blPanel%(Handle%,2) = 0 : blPanel%(Handle%,3) = 0 : blPanel%(Handle%,4) = 0 : blPanel%(Handle%,6) = 7 : blPanel%(Handle%,7) = 8 : blCtl%(Handle%,13) = blCtl%(Handle%,13) OR blFlagClip%
510090	IF Class% = blClassUser% THEN blUser#(Handle%,1) = Kind% : blUser#(Handle%,2) = 0 : blUser#(Handle%,3) = 0 : blUser#(Handle%,4) = 100 : blUser#(Handle%,5) = 1 : blUser#(Handle%,6) = 0 : blUser#(Handle%,7) = 0 : blUser#(Handle%,8) = 7 : blUser#(Handle%,9) = 4 : blUser#(Handle%,10) = 0 : blUser#(Handle%,11) = 0
510091	IF Class% = blClassUser% THEN blUser#(Handle%,12) = 0 : blUser#(Handle%,13) = 2 : blUser#(Handle%,14) = 1 : blUser#(Handle%,15) = 1 : blUser#(Handle%,16) = 0 : blUser#(Handle%,17) = 0 : blUser#(Handle%,18) = 1 : blUser#(Handle%,19) = 0 : blUser#(Handle%,20) = 1 : blUser#(Handle%,21) = 0 : blUser#(Handle%,22) = 1
510092	IF Class% = blClassClickTarget% THEN blClick%(Handle%,1) = 1 : blClick%(Handle%,2) = 0 : blClick%(Handle%,3) = 0 : blClick%(Handle%,4) = 0
510093	CALL blLinkHandle(Handle%)
510094	CALL blMarkDirty(Handle%)
510095	blCreate% = Handle%
510096	blNeedsLayout% = 1
510097	END FUNCTION
510110	SUB blDestroy(Handle%)
510111	LOCAL Child%, NextSib%, Parent%
510112	blStatus% = blStatusOk%
510113	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510114	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510115	IF Handle% = blRoot% THEN blStatus% = blStatusCannotDestroyRoot% : EXIT SUB
510116	Parent% = blCtl%(Handle%,2) : Child% = blCtl%(Handle%,3)
510117	WHILE Child% <> 0
510118	NextSib% = blCtl%(Child%,6)
510119	CALL blDestroy(Child%)
510120	Child% = NextSib%
510121	WEND
510122	CALL blDetachHandle(Handle%)
510123	IF blHover% = Handle% THEN blHover% = 0
510124	IF blPressed% = Handle% THEN blPressed% = 0
510125	IF blCapture% = Handle% THEN blCapture% = 0
510126	blCtl%(Handle%,1) = 0
510127	blCtl%(Handle%,2) = 0
510128	blCtl%(Handle%,3) = 0
510129	blCtl%(Handle%,4) = 0
510130	blCtl%(Handle%,5) = 0
510131	blCtl%(Handle%,6) = 0
510132	blCtl%(Handle%,13) = 0
510133	blCtlText$(Handle%) = ""
510134	blNeedsLayout% = 1
510135	CALL blMarkDirty(Parent%)
510136	END SUB
510150	SUB blSetBounds(Handle%, PosX%, PosY%, Width%, Height%)
510151	blStatus% = blStatusOk%
510152	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510153	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510154	blCtl%(Handle%,7) = PosX%
510155	blCtl%(Handle%,8) = PosY%
510156	blCtl%(Handle%,9) = Width%
510157	blCtl%(Handle%,10) = Height%
510158	blCtl%(Handle%,13) = blCtl%(Handle%,13) OR blFlagLayoutDirty%
510159	CALL blMarkDirty(Handle%)
510160	blNeedsLayout% = 1
510161	END SUB
510170	SUB blSetLayout(Handle%, Mode%, PadSize%, GapSize%)
510171	blStatus% = blStatusOk%
510172	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510173	IF blCtl%(Handle%,1) <> blClassPanel% AND blCtl%(Handle%,1) <> blClassTop% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
510174	blPanel%(Handle%,1) = Mode%
510175	blPanel%(Handle%,2) = PadSize%
510176	blPanel%(Handle%,3) = GapSize%
510177	blCtl%(Handle%,13) = blCtl%(Handle%,13) OR blFlagLayoutDirty%
510178	CALL blMarkDirty(Handle%)
510179	blNeedsLayout% = 1
510180	END SUB
510190	SUB blShow(Handle%)
510191	blStatus% = blStatusOk%
510192	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510193	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510194	blCtl%(Handle%,13) = blCtl%(Handle%,13) OR blFlagVisible%
510195	CALL blMarkDirty(Handle%)
510196	blNeedsLayout% = 1
510197	END SUB
510200	SUB blHide(Handle%)
510201	blStatus% = blStatusOk%
510202	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510203	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510204	blCtl%(Handle%,13) = blCtl%(Handle%,13) AND (blFlagVisible% XOR 32767)
510205	IF blHover% = Handle% THEN blHover% = 0
510206	IF blPressed% = Handle% THEN blPressed% = 0
510207	IF blCapture% = Handle% THEN blCapture% = 0
510208	CALL blMarkDirty(Handle%)
510209	blNeedsLayout% = 1
510210	END SUB
510220	SUB blSetEnabled(Handle%, Enabled%)
510221	blStatus% = blStatusOk%
510222	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510223	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510224	IF Enabled% <> 0 THEN blCtl%(Handle%,13) = blCtl%(Handle%,13) OR blFlagEnabled% : GOTO blEnabledDone
510225	blCtl%(Handle%,13) = blCtl%(Handle%,13) AND (blFlagEnabled% XOR 32767)
510226	LABEL blEnabledDone
510227	CALL blMarkDirty(Handle%)
510228	END SUB
510240	SUB blSetText(Handle%, Text$)
510241	blStatus% = blStatusOk%
510242	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510243	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510244	blCtlText$(Handle%) = Text$
510245	CALL blMarkDirty(Handle%)
510246	END SUB
510250	FUNCTION blGetText$(Handle%)
510251	blStatus% = blStatusOk%
510252	blGetText$ = ""
510253	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT FUNCTION
510254	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT FUNCTION
510255	blGetText$ = blCtlText$(Handle%)
510256	END FUNCTION
510260	SUB blSetValue(Handle%, Value#)
510261	blStatus% = blStatusOk%
510262	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510263	IF blCtl%(Handle%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510264	blUser#(Handle%,2) = blClamp#(Value#,blUser#(Handle%,3),blUser#(Handle%,4))
510265	CALL blMarkDirty(Handle%)
510266	END SUB
510270	FUNCTION blGetValue#(Handle%)
510271	blStatus% = blStatusOk%
510272	blGetValue# = 0
510273	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT FUNCTION
510274	IF blCtl%(Handle%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT FUNCTION
510275	blGetValue# = blUser#(Handle%,2)
510276	END FUNCTION
510280	SUB blSetAction(Handle%, Action%)
510281	blStatus% = blStatusOk%
510282	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510283	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510284	blCtl%(Handle%,15) = Action%
510285	END SUB
510290	SUB blSetActionLine(ActionId%, Line&)
510291	blStatus% = blStatusOk%
510292	IF ActionId% < 0 OR ActionId% > 255 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510293	IF Line& < 0 OR Line& > 2147483647 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510294	blActionLine&(ActionId%) = Line&
510295	END SUB
510300	SUB blSetClickMask(Handle%, Mask%)
510301	blStatus% = blStatusOk%
510302	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510303	IF blCtl%(Handle%,1) <> blClassClickTarget% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
510304	blClick%(Handle%,1) = Mask%
510305	END SUB
510310	SUB blInvalidate(Handle%)
510311	blStatus% = blStatusOk%
510312	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510313	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510314	CALL blMarkDirty(Handle%)
510315	END SUB
510320	SUB blLayoutAll
510321	blNeedsLayout% = 0
510322	blCtl%(1,11) = 0
510323	blCtl%(1,12) = 0
510324	blCtlClipL%(1) = 0
510325	blCtlClipT%(1) = 0
510326	blCtlClipR%(1) = blCtl%(1,9) - 1
510327	blCtlClipB%(1) = blCtl%(1,10) - 1
510328	CALL blLayoutControl(1)
510329	END SUB
510330	SUB blPaintAll
510331	IF blNeedsLayout% <> 0 THEN CALL blLayoutAll
510332	CALL blPaintTree(1)
510333	blNeedsPaint% = 0
510334	CALL blClearDirtyTree
510335	END SUB
510340	SUB blUpdate(Handle%)
510341	blStatus% = blStatusOk%
510342	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510343	IF blCtl%(Handle%,1) = 0 THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510344	CALL blPaintTree(Handle%)
510345	END SUB
510350	SUB blAutoSize(Handle%)
510351	LOCAL Scale%, PadSize%
510352	blStatus% = blStatusOk%
510353	IF Handle% < 1 OR Handle% > blMaxControls% THEN blStatus% = blStatusInvalidHandle% : EXIT SUB
510354	IF blCtl%(Handle%,1) <> blClassUser% THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510355	Scale% = blUser#(Handle%,20)
510356	PadSize% = blUser#(Handle%,13)
510357	blCtl%(Handle%,9) = TEXTWIDTH(blCtlText$(Handle%),Scale%) + 2 * PadSize% + 2
510358	blCtl%(Handle%,10) = TEXTHEIGHT(blCtlText$(Handle%),Scale%) + 2 * PadSize% + 2
510359	CALL blMarkDirty(Handle%)
510360	blNeedsLayout% = 1
510361	END SUB
510362	SUB blUpdateAll
510363	IF blNeedsPaint% = 0 THEN EXIT SUB
510364	IF blNeedsLayout% <> 0 THEN CALL blLayoutAll
510365	CALL blRepaintDirty(1)
510366	blNeedsPaint% = 0
510367	CALL blClearDirtyTree
510368	END SUB
510370	FUNCTION blPollInput%()
510371	LOCAL MouseB%, BtnMask%, WheelD%, PosX%, PosY%, ClickH%
510372	ClickH% = 0
510373	MouseB% = MOUSEB
510374	PosX% = MOUSEPOSX
510375	PosY% = MOUSEPOSY
510376	IF PosX% >= 0 THEN CALL blTestMove(PosX%,PosY%)
510377	BtnMask% = MOUSEBUTTONS AND 1
510378	IF BtnMask% <> 0 AND blPressed% = 0 THEN CALL blTestButton(1,1,PosX%,PosY%)
510379	IF BtnMask% = 0 AND blPressed% <> 0 THEN CALL blTestButton(blPressed%,0,PosX%,PosY%)
510380	WheelD% = MOUSEWHEEL
510381	IF WheelD% <> 0 THEN CALL blWheelAt(PosX%,PosY%,WheelD%)
510382	IF MouseB% <> 0 THEN PosX% = MOUSEX : PosY% = MOUSEY : ClickH% = blClickAt%(PosX%,PosY%,MouseB%)
510383	blPollInput% = ClickH%
510384	END FUNCTION
510390	FUNCTION blNextEvent%()
510391	blStatus% = blStatusOk%
510392	IF blEvCount% = 0 THEN blStatus% = blStatusEventQueueEmpty% : blCurType% = 0 : blNextEvent% = 0 : EXIT FUNCTION
510393	blCurType% = blEvType%(blEvHead%)
510394	blCurCtl% = blEvCtl%(blEvHead%)
510395	blCurAction% = blEvAction%(blEvHead%)
510396	blCurX% = blEvX%(blEvHead%)
510397	blCurY% = blEvY%(blEvHead%)
510398	blCurButton% = blEvButton%(blEvHead%)
510399	blCurWheel% = blEvWheelVal%(blEvHead%)
510400	blEvHead% = (blEvHead% + 1) MOD blMaxEvents%
510401	blEvCount% = blEvCount% - 1
510402	blNextEvent% = blCurType%
510403	END FUNCTION
510410	FUNCTION blEventCtl%()
510411	blEventCtl% = blCurCtl%
510412	END FUNCTION
510420	FUNCTION blEventAction%()
510421	blEventAction% = blCurAction%
510422	END FUNCTION
510430	FUNCTION blEventX%()
510431	blEventX% = blCurX%
510432	END FUNCTION
510440	FUNCTION blEventY%()
510441	blEventY% = blCurY%
510442	END FUNCTION
510450	FUNCTION blEventButton%()
510451	blEventButton% = blCurButton%
510452	END FUNCTION
510460	FUNCTION blEventWheel%()
510461	blEventWheel% = blCurWheel%
510462	END FUNCTION
510470	FUNCTION blEventCount%()
510471	blEventCount% = blEvCount%
510472	END FUNCTION
510480	FUNCTION blFindAt%(PosX%, PosY%)
510481	blFindAt% = blHitTest%(PosX%,PosY%)
510482	END FUNCTION
510490	SUB blDump
510491	LOCAL Index%
510492	FOR Index% = 1 TO blMaxControls%
510493	IF blCtl%(Index%,1) <> 0 THEN PRINT "H"; Index%; " C"; blCtl%(Index%,1); " P"; blCtl%(Index%,2); " X"; blCtl%(Index%,11); " Y"; blCtl%(Index%,12); " W"; blCtl%(Index%,9); " H"; blCtl%(Index%,10)
510494	NEXT Index%
510495	blStatus% = blStatusOk%
510496	END SUB
510500	SUB blRegisterClass(Class%, Line&, Name$)
510501	LOCAL VerbId%
510502	blStatus% = blStatusOk%
510503	IF Class% < 5 OR Class% > blMaxClasses% THEN blStatus% = blStatusInvalidClass% : EXIT SUB
510504	IF Line& <= 0 OR Line& > 2147483647 THEN blStatus% = blStatusInvalidValue% : EXIT SUB
510505	FOR VerbId% = 1 TO blMaxVerbs%
510506	blMethod&(Class%,VerbId%) = Line&
510507	NEXT VerbId%
510508	blClassName$(Class%) = Name$
510509	END SUB
510520	FUNCTION blTestClickAt%(PosX%, PosY%, Button%)
510521	blTestClickAt% = blClickAt%(PosX%,PosY%,Button%)
510522	END FUNCTION
510530	SUB blTestMouseMove(PosX%, PosY%)
510531	CALL blTestMove(PosX%,PosY%)
510532	END SUB
510540	SUB blTestMouseButton(Button%, Down%, PosX%, PosY%)
510541	CALL blTestButton(Button%,Down%,PosX%,PosY%)
510542	END SUB
510550	SUB blTestWheel(PosX%, PosY%, Delta%)
510551	CALL blWheelAt(PosX%,PosY%,Delta%)
510552	END SUB
512000	REM debug/test helper area
512010	LABEL blTestErl
512020	ERROR 5
512030	RETURN
512031	END
