20000 REM Windows-specific subroutines
20010	REM Usage: Call these subroutines for Windows-specific operations
20100	REM windowsInitialize subroutine: Call this to initialize Windows-specific settings
20110	REM Initialize Windows-specific settings here
20120	REM you can override windowsControlMax before calling this subroutine
20125	REM If not overridden, it will default to 100
20130	if windowsControlMax = 0 then windowsControlMax = 100
20140	REM Dimension the windows control array based on the maximum number of controls
20145	REM The array has 6 columns for storing control properties
20148	REM Each row represents a control and each column stores a specific property of the control
20150	REM The array will be used to store properties of each control

20160	REM Column 1: wcaControlIndex (control index)
20170	REM Column 2: wcaControlClass (control class)
20175	REM Column 3: wcaControlPosX (control position X)
20176	REM Column 4: wcaControlPosY (control position Y)
20177	REM Column 5: wcaControlWidth (control width)
20178	REM Column 6: wcaControlHeight (control height)
20179	REM Column 7: wcaControlVisibility (control visibility)
20180	REM Column 8: wcaControlEnabled (control enabled state)
20181	REM Column 9: wcaControlParent (parent control reference)
20182	REM Column 10: wcaControlReturnParam (return parameter index)

20184	REM Define some constants for control fields

20185	wcaControlIndex = 1
20186	wcaControlClass = 2
20187	wcaControlPosX = 3
20188	wcaControlPosY = 4
20189	wcaControlWidth = 5
20190	wcaControlHeight = 6
20191	wcaControlVisibility = 7
20192	wcaControlEnabled = 8
20193	wcaControlParent = 9
20194	wcaControlReturnParam = 10

20200	DIM windowsControlArray(windowsControlMax,10)
20210	REM Initialize the windows control array with default values
20220	FOR i = 1 TO windowsControlMax
20230		FOR j = 1 TO 10
20240			windowsControlArray(i,j) = 0
20250		NEXT j
20260	NEXT i
20270	windowsControlNextPtr = 0
