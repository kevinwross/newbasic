<!-- Chunked from GWBASIC.txt, lines 5879-5947. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# Appendix E: Converting BASIC Programs to GW-BASIC

Appendix E 

Converting BASIC Programs to GW-BASIC 

Programs written in a BASIC language other than GW-BASIC may require some minor adjustments 
before they ean be run. The following seetions deseribe these adjustments. 

E.l String Dimensions 

Delete all statements used to deelare the length of strings. A statement sueh as the following: 

DIM A$(I,J) 

whieh dimensions a string array for J elements of length I, should be eonverted to the following 
statement: 

DIM A$(J) 

Some GW-BASIC languages use a eomma or ampersand (&amp;) for string eoneatenation. Eaeh of these 
must be ehanged to a plus sign (+), whieh is the operator for GW-BASIC string eoneatenation. 

In GW-BASIC, the MID$, RIGHT$, and left$ functions are used to take substrings of strings. Forms 
such as A$(I) to access the Ith character in A$, or A$(I,J) to take a substring of A$ from position I to 
position J, must be changed as follows: 

Other BASICGW-BASIC 

X$=A$(I) X$=MID$(A$,I,I) 

X$=A$(I,J) X$=MID$(A$,I,J-I+I) 

If the substring reference is on the left side of an assignment, and X$ is used to replace characters in 
A$, convert as follows: 

Other BASICGW-BASIC 

A$(I)=X$ MID$ (A$, I,I)=X$ 

A$ (I,J)=X$ MID$ (A$, I, J-I + I)=X$ 


E.2 Multiple Assignments 

Some GW-BASIC languages allow statements of the following form to set B and C equal to zero: 

10 LET B=C=0 

GW-BASIC would interpret the second equal sign as a logical operator and set B equal to -1 if C 
equaled 0. Convert this statement to two assignment statements: 

10 C=0: B=0 

E.3 Multiple Statements 

Some GW-BASIC languages use a backslash (\) to separate multiple statements on a line. With GW- 
BASIC, be sure all elements on a line are separated by a colon ( : ). 

E.4 MAT Functions 

Programs using the mat functions available in some GW-BASIC languages must be rewritten using 
FOR-NEXT loops to execute properly. 


E.5 FOR-NEXT Loops 

Some GW-BASIC languages will always exeeute a for-next loop once, regardless of the limits. 
GW-BASIC checks the limits first and does not execute the loop if past limits. 


