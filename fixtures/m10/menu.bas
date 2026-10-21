10 REM A menu driven by INKEY$, so the corpus can script a key sequence (.keys).
20 CLS
30 FOR I = 1 TO 3
40 READ A$, B$
50 NEXT I
60 PRINT "CHOOSE A LETTER"
70 K$ = INKEY$: IF K$ = "" THEN 70
80 IF K$ = "A" THEN PRINT "ADD";: GOTO 110
90 IF K$ = "E" THEN PRINT "EDIT";: GOTO 110
100 IF K$ = "Q" THEN PRINT "QUIT";: GOTO 130
105 GOTO 70
110 PRINT " DONE"
120 GOTO 70
130 END
140 DATA "ADD", "x", "EDIT", "y", "QUIT", "z"
