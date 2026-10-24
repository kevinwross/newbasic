10000	REM subroutine initStack is here 
10005   REM Parameter stack functions. 
10010	REM Usage: During program startup, call initStack subroutine
10015	REM once. That gets it setup for subsequent stack operations
10020   REM stackXfer = 10 : gosub 10100 :REM Push 10 onto the stack
10025   REM gosub 10200 : REM Pop value from the stack into stackXfer
10030	REM Initialize stack pointer and array
10035	REM This subroutine sets up the stack for use
10040	REM Sets stackPtr to 1000 if not already set and dims stackArray accordingly
10060	if stackPtr = 0 then stackPtr = 1000
10070	DIM stackArray(stackPtr)
10080	return
10100   REM pushStack subroutine: stackXfer variable contains value to push onto the stack
10110	stackPtr = stackPtr - 1 :stackArray(stackPtr) = stackXfer
10120	return
10200	REM popStack subroutine: pops value from the stack into stackXfer variable
10210	stackXfer = stackArray(stackPtr) : stackPtr = stackPtr + 1 
10220	return
