10 REM SUB/FUNCTION/LOCAL sample: procedures, parameters, recursion.
20 CALL Welcome
30 PRINT "Clamp:"; Clamp%(150, 0, 100)
40 PRINT "Fact:"; Fact%(5)
50 END
100 SUB Welcome
110 LOCAL N%
120 FOR N% = 1 TO 3
130 PRINT "Tick"; N%
140 NEXT N%
150 END SUB
200 FUNCTION Clamp%(V%, LO%, HI%)
210 LOCAL R%
220 R% = V%
230 IF R% < LO% THEN R% = LO%
240 IF R% > HI% THEN R% = HI%
250 Clamp% = R%
260 END FUNCTION
300 FUNCTION Fact%(N%)
310 IF N% <= 1 THEN Fact% = 1 : EXIT FUNCTION
320 Fact% = N% * Fact%(N% - 1)
330 END FUNCTION
