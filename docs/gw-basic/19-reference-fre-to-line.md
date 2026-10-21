<!-- Chunked from GWBASIC.txt, lines 10820-12228. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: FRE - LINE (incl. GET Files/Graphics)

FRE Function 

Purpose: 

To return the number of available bytes in alloeated string memory. 

Syntax: 

FRE (x.f) 

FRE (x) 

Comments: 

Arguments (x$) and (x) are dummy arguments. 

Before fre (x.?) returns the amount of spaee available in allocated string memory, GW-BASIC 
initiates a "garbage collection" activity. Data in string memory space is collected and reorganized, and 
unused portions of fragmented strings are discarded to make room for new input. 

If FRE is not used, GW-BASIC initiates an automatic garbage collection activity when all string 
memory space is used up. GW-BASIC will not initiate garbage collection until all free memory has 
been used. Garbage collection may take 1 to 1.5 minutes. 

FRE (" " ) or any string forces a garbage collection before returning the number of free bytes. 
Therefore, using fre (" ") periodically will result in shorter delays for each garbage collection. 

It should be noted that the ctrl-break function cannot be used during this housecleaning process. 

Examples: 

PRINT FRE(O) 

14542 

Your computer may return a different value. 


GET Statement (Files) 

Purpose: 

To read a record from a random disk file into a random buffer. 

Syntax: 

GET [#]file number[,record number] 

Comments: 

file number is the number under which the file was opened. 

record number is the number of the record, within the range of 1 to 16,777,215. 

If record number is omitted, the next record (after the last get) is read into the buffer. 

After a GET statement, input# and line input# may be used to read characters from the random 
file buffer. 

GET may also be used for communications files, record number is the number of bytes to be read 
from the communications buffer, record number cannot exceed the buffer length set in the OPEN 
COM ( n ) statement. 

Examples: 

The following example opens the vendor file for random access, defines the fields, reads a record, 
then displays it: 

10 OPEN "R", 1, "A:VENDOR.FIL" 

20 FIELD 1, 30 AS VENDNAMES$, 20 AS ADDR$, 15 AS CITY$ 

30 GET 1 

40 PRINT VENDNAMES$, ADDR$, CITY$ 

50 CLOSE 1 

This example opens the file vendorfil for random access, with fields defined in line 20. 

In line 30, the GET statement reads a record into the file buffer. 

Line 40 displays the information from the record just read. 

Line 50 closes the file. 


GET Statement (Graphics) 

Purpose: 

To transfer graphics images from the screen. 

Syntax: 

GET (xlfyl)-(x2,y2) farray name 

Comments: 

The PUT and get statements are used to transfer graphics images to and from the screen, put and 
GET make animation and high-speed object motion possible in either graphics mode. 

The GET statement transfers the screen image bounded by the rectangle described by the specified 
points into the array. The rectangle is defined the same way as the rectangle drawn by the line 
statement using the , B option. 

The array is used simply as a place to hold the image, and can be of any type except string. It must be 
dimensioned large enough to hold the entire image. The contents of the array after a GET will be 
meaningless when interpreted directly (unless the array is of the type integer, as shown below). 

The storage format in the array is as follows: 

• 2 bytes given x dimension in bits 

• 2 bytes given y dimension in bits 

• the array data itself 

The data for each row of pixels is left-justified on a byte boundary. If less than a multiple of eight bits 
is stored, the rest of the byte will be filled out with zeros. The required array size in bytes is as 
follows: 

4TINT((x*bitsperpixel + 7) / 8) *y 

See the SCREEN statement for bits-per-pixel values for different screen modes. 

The bytes-per-element of an array are as follows: 

• 2 for integer 

• 4 for single-precision 

• 8 for double-precision 

The number of bytes required to get a 10 by 12 image into an integer array is 
4 + lNT ( (10*2 + 7) / 8) *12, or 40 bytes. An integer array with at least 20 elements is necessary. 

If OPTION BASE equals zero, an integer array can be used to examine the x and y dimensions and the 
data. The x dimension is in element 0 of the array, and the y dimension is in element 1. Integers are 
stored low byte first, then high byte, but data is transferred high byte first (left-most), then low byte. 

It is possible to get an image in one mode and put it in another, although the effect may be quite 
strange because of the way points are represented in each mode. 



Examples: 

10 CLS: SCREEN 1 
20 PSET(130, 120) 

30 DRAW "U25; E7; R20; D32; L6; U12; L14" 
40 DRAW "D12;L6":PSET(137,102) 

50 DRAW "U4; E4; R8; D8; L12" 

60 PSET(137, 88) 

70 DRAW "E4; R20; D32; G4": PAINT(139, 87) 
80 DIM A(500) 

90 GET (125, 130)-(170, 80),A 
100 FOR 1=1 TO 1000: NEXT I 
110 PUT (20,20), A, PSET 
120 FOR 1=1 TO 1000: NEXT I 
130 GET (125,130)-(170, 80),A 
140 FOR 1=1 TO 1000: NEXT I 
150 PUT (220,130), A, PRESET 


GOSUB ... RETURN Statement 


Purpose: 

To branch to, and return from, a subroutine. 
Syntax: 

GOSUB line number 


RETURN [line number] 

Comments: 

line number is the first line number of the subroutine. 

A subroutine may be called any number of times in a program, and a subroutine may be called from 
within another subroutine. Such nesting of subroutines is limited only by available memory. 

A RETURN statement in a subroutine causes GW-BASIC to return to the statement following the most 
recent GOSUB statement. A subroutine can contain more than one return statement, should logic 
dictate a return at different points in the subroutine. 

Subroutines can appear anywhere in the program, but must be readily distinguishable from the main 
program. 

To prevent inadvertent entry, preeede the subroutine by a STOP, end, or GOTO statement to direct 
program control around the subroutine. 

Examples: 


10 

GOSUB 

40 

20 

PRINT 

"BACK FROM SUBROUTINE 

30 

END 


40 

PRINT 

"SUBROUTINE"; 

50 

PRINT 

" IN"; 

60 

PRINT 

" PROGRESS" 

70 

RETURN 


RUN 



SUBROUTINE IN PROGRESS 
BACK FROM SUBROUTINE 


The END statement in line 30 prevents re-execution of the subroutine. 


GOTO Statement 


Purpose: 

To branch unconditionally out of the normal program sequence to a speeified line number. 

Syntax: 

GOTO line number 

Comments: 

line number is any valid line number within the program. 

If line number is an executable statement, that statement and those following are exeeuted. If it is a 
non-executable statement, execution proceeds at the first executable statement encountered after line 
number. 

Examples: 


10 

READ 

R 

20 

PRINT 

"R =" 

30 

A = 3 

&lt; 

\—1 

40 

PRINT 

"AREA 

50 

GOTO 

10 

60 

DATA 

5, 7, 

RUN 



R = 5 AREA = 78.5 
R = 7 AREA = 153.86 
R = 12 AREA = 452.16 
Out of data in 10 

The " out of data" advisory is generated when the program attempts to read a fourth data 
statement (which does not exist) in line 60. 


HEX$ Function 


Purpose: 

To return a string which represents the hexadecimal value of the numeric argument. 

Syntax: 

v.?=HEX$ (x) 

Comments: 

HEX$ converts decimal values within the range of -32768 to +65535 into a hexadecimal string 
expression within the range of 0 to FFFF. 

Hexadecimal numbers are numbers to the base 16, rather than base 10 (decimal numbers). Appendix 
C and Appendix G in the GW-BASIC User's Guide contain more information on hexadecimals and 
their equivalents. 

X is rounded to an integer before HEX$ (x) is evaluated. See the QCT$ function for octal conversions. 
If X is negative, 2's (binary) complement form is used. 

Examples: 

10 CLS: INPUT "INPUT DECIMAL NUMBER";X 
20 A$=HEX$(X) 

30 PRINT X "DECIMAL IS "A$" HEXADECIMAL" 

RUN 

INPUT DECIMAL NUMBER? 32 
32 DECIMAL IS 20 HEXADECIMAL 





IF ... THEN ... ELSE Statement 


Purpose: 

To make a decision regarding program flow based on the result returned by an expression. 

Syntax: 

IF expressionl,] THEN statement (s) [,] [ELSE statement (s)] 

IF expressionl,] GOTO line number[[,] ELSE statement (s)] 

Comments: 

If the result of expression is nonzero (logical true), the then or GOTO line number is executed. 

If the result of expression is zero (false), the then or GOTO line number is ignored and the ELSE 
line number, if present, is executed. Otherwise, execution continues with the next executable 
statement. A comma is allowed before then and else. 

THEN and ELSE may be followed by either a line number for branching, or one or more statements to 
be executed. 

GOTO is always followed by a line number. 

If the statement does not contain the same number of else's and then's line number, each ELSE is 
matched with the closest unmatched then. For example: 

IF A=B THEN IF B=C THEN PRINT "A=C" ELSE PRINT "A &lt; &gt; C" 

will not print "a &lt; &gt; c" when A &lt; &gt; B. 

If an IF. . . THEN statement is followed by a line number in the direct mode, an "Undefined line 
number" error results, unless a statement with the specifled line number was previously entered in the 
indirect mode. 

Because if . . then . . . else is all one statement, the else clause cannot be on a separate line. It 
must be all on one line. 

Nesting of IF Statements 

IF. . . THEN. . . ELSE statements may be nested. Nesting is limited only by the length of the line. For 
example, the following is a legal statement: 

100 IF X &gt; Y THEN PRINT "GREATER" ELSE IF Y &gt; X THEN&amp; 

110 PRINT "LESS THAN" 

200 ELSE PRINT "EQUAL" 

Testing Equality 

When using l F to test equality for a value that is the result of a floating-point computation, remember 
that the internal representation of the value may not be exact. Therefore, test against the range over 
which the accuracy of the value may vary. 

For example, to test a computed variable A against the value 1.0, use the following statement: 

IF ABS (A-1.0)&lt;1.OE-6 THEN ... 

This test returns true if the value of A is 1.0 with a relative error of less than l.OE-6. 


Examples: 

The following statement gets record number N, if N is not zero. 

200 IF N THEN GET#1, N 

In the following example, a test determines if N is greater than 10 and less than 20. If N is within this 
range, DB is calculated and execution branches to line 300. If N is not within this range, execution 
continues with line 110. 

100 IF(N&lt;20) and {N&gt;10) THEN DB=1979-1: GOTO 300 
110 PRINT "OUT OF RANGE" 

The next statement causes printed output to go either to the terminal or the line printer, depending on 
the value of a variable (lOFLAG). If lOFLAG is zero, output goes to the line printer; otherwise, 
output goes to the terminal. 

210 IF lOFLAG THEN PRINT A$ ELSE LPRINT A$ 


INKEYS Variable 


Purpose: 

To return one eharacter read from the keyboard. 

Syntax: 

v.?=INKEY$ 

Comments: 

If no charaeter is pending in the keyboard buffer, a null string (length zero) is returned. 

If several eharaeters are pending, only the first is returned. The string will be one or two eharaeters in 
length. 

Two eharaeter strings are used to return the extended eodes deseribed in Appendix C of the GW- 
BASIC User's Guide . The first character of a two character code is zero. 

No characters are displayed on the screen, and all characters except the following are passed to the 
program; 

CTRL-BREAK 
CTRL-NUM LOCK 
CTRL-ALT-DEL 
CTRL-PRTSCR 
PRTSCR 

Examples: 

10 CLS: PRINT"PRESS RETURN 
20 TIMELIMIT% = 1000 
30 GOSUB 1010 

40 IF TIMEOUT% THEN PRINT "TOO LONG" ELSE PRINT "GOOD SHOW" 

50 PRINT RESPONSE$ 

60 END 


1000 REM TIMED INPUT SUBROUTINE 
1010 RESPONSE$="" 

1020 FOR N%=1 TO TIMELIMIT% 

1030 A$=INKEY$:IF LEN(A$)=0 THEN 1060 
1040 IF ASC(A$)=13 THEN TIMEOUT%=0: RETURN 
1050 RESPONSE$=RESPONSE$+A$ 

1060 NEXT N% 

1070 TIMEOUT%=l: RETURN 

When this program is executed, and if the return key is pressed before 1000 loops are completed, 
then "GOOD show" is printed on the screen. Otherwise, "too long" is printed. 

Since an inkey$ statement scans the keyboard only once, place inkey$ statements within loops to 
provide adequate response times for the operator. 




INP Function 


Purpose: 

To return the byte read from maehine port n. 

Syntax: 

INP in) 

Comments: 

n represents a valid machine port number within the range of 0 to 65535. 

The INP function is one way in which a peripheral device may communicate with a GW-BASIC 
program. 

INP is the complementary function to the OUT statement. 

Examples: 

100 A=INP(56) 

Upon execution, variable A contains the value present on port 56. The number returned will be within 
the range of 0 to 255, decimal. 

The assembly language equivalent to this statement is 

MOV DX,56 
IN AL,DX 


INPUT Statement 


Purpose: 

To prepare the program for input from the terminal during program execution. 

Syntax: 

INPUT [;][prompt string;] list of variables 
INPUT [;][prompt string,] list of variables 

Comments: 

prompt string is a request for data to be supplied during program execution. 

list of variables contains the variable(s) that stores the data in the prompt string. 

Each data item in the prompt string must be surrounded by double quotation marks, followed by a 
semicolon or comma and the name of the variable to which it will be assigned. If more than one 
variable is given, data items must be separated by commas. 

The data entered is assigned to the variable list. The number of data items supplied must be the same 
as the number of variables in the list. 

The variable names in the list may be numeric or string variable names (including subscripted 
variables). The type of each data item input must agree with the type specified by the variable name. 

Too many or too few data items, or the wrong type of values (for example, numeric instead of string), 
causes the message "?Redo from start" to be printed. No assignment of input values is made until 
an acceptable response is given. 

A comma may be used instead of a semicolon after prompt string to suppress the question mark. For 
example, the following line prints the prompt with no question mark: 

INPUT "ENTER BIRTHDATE",B$ 

If the prompt string is preceded by a semicolon, the return key pressed by the operator is suppressed. 
During program execution, data on that line is displayed, and data from the next print statement is 
added to the line. 

When an input statement is encountered during program execution, the program halts, the prompt 
string is displayed, and the operator types in the requested data. Strings that input to an input 
statement need not be surrounded by quotation marks unless they contain commas or leading or 
trailing blanks. 

When the operator presses the return key, program execution continues. 

INPUT and LINE INPUT statements have built-in print statements. When an input statement with 
a quoted string is encountered during program execution, the quoted string is printed automatically 
(see the PRINT statement). 

The principal difference between the input and line input statements is that line input 
accepts special characters (such as commas) within a string, without requiring double quotation 
marks, while the input statement requires double quotation marks. 


Example 1: 

To find the square of a number: 



10 INPUT X 

20 PRINT X "SQUARED IS" X^2 

30 END 

RUN 


The operator types a number (5) in response to the question mark. 

5 SQUARED IS 25 

Example 2: 

To find the area of a circle when the radius is known: 

10 PI=3.14 

20 INPUT "WHAT IS THE RADIUS"; R 
30 A=PI*R^2 

40 PRINT "THE AREA OF THE CIRCLE IS"; A 
50 PRINT 
60 GOTO 20 
RUN 

WHAT IS THE RADIUS? 7.4 

THE AREA OF THE CIRCLE IS 171.9464 

Note that line 20 in the above example makes use of the built-in print statement contained within 

INPUT. 


INPUT# Statement 


Purpose: 

To read data items from a sequential file and assign them to program variables. 

Syntax: 

INPUT# file number, variable list 

Comments: 

file number is the number used when the file was opened for input. 

variable list eontains the variable names to be assigned to the items in the file. 

The data items in the fide appear just as they would if data were being typed on the keyboard in 
response to an input statement. 

The variable type must match the type specified by the variable name. 

With INPUT#, no question mark is printed, as it is with input. 

Numeric Values 

For numeric values, leading spaces and line feeds are ignored. The first character encountered (not a 
space or line feed) is assumed to be the start of a number. The number terminates on a space, carriage 
return, line feed, or comma. 

Strings 

If GW-BASIC is scanning the sequential data file for a string, leading spaces and line feeds are 
ignored. 

If the first character is a double quotation mark ("), the string will consist of all characters read 
between the first double quotation mark and the second. A quoted string may not contain a double 
quotation mark as a character. The second double quotation mark always terminates the string. 

If the first character of the string is not a double quotation mark, the string terminates on a comma, 
carriage return, line feed, or after 255 characters have been read. 

If end of the file is reached when a numeric or string item is being input, the item is terminated. 
INPUT# can also be used with random files. 


INPUTS Function 


Purpose: 

To return a string of x characters read from the keyboard, or from file number. 

Syntax: 

INPUT$(x[,[#]file number)] 

Comments: 

If the keyboard is used for input, no characters will appear on the screen. All control characters 
(except ctrl-break) are passed through, ctrl-break interrupts the execution of the input $ function. 

The INPUT$ function is preferred over input and line input statements for reading 
communications files, because all ASCII characters may be significant in communications, input is 
the least desirable because input stops when a comma or carriage return is seen, line input 
terminates when a carriage return is seen. 

INPUT$ allows all characters read to be assigned to a string. input$ will return x characters from the 
file number or keyboard. 

For more information about communications, refer to Appendix F in the GW-BASIC User’s Guide . 

Example 1: 

The following example lists the contents of a sequential file in hexadecimal. 

10 OPEN"I", 1, "DATA" 

20 IF EOF(l) THEN 50 

30 PRINT HEX$(ASC(INPUT$(1, #1))); 

40 GOTO 20 
50 PRINT 
60 END 

Example 2: 

In the following program, the program pauses, awaiting a keyboard entry of either P or S. Line 130 
continues to loop back to line 100 if the input is other than P or S. 


100 PRINT "TYPE P TO PROCEED OR S TO STOP" 
110 X$=INPUT$ (1) 

120 IF X$="P" THEN 500 

130 IF X$="S" THEN 700 ELSE 100 



INSTR Function 


Purpose: 

To search for the first occurrence of string y$ in x$, and return the position at which the string is 
found. 

Syntax: 

INSTR ( [n, 

Comments: 

Optional offset n sets the position for starting the search. The default value for n is 1. If n equals zero, 
the error message "illegal argument in line number" is returned, n must be within the range 
of 1 to 255. If n is out of this range, an "illegal Function Call" error is returned. 

INSTR returns 0 if; 

• n&gt; LEN(x$) 

• is null 

• y$ cannot be found 

If yf is null, INSTR returns n. 

x$ and y$ may be string variables, string expressions, or string literals. 

Examples: 

10 X$="ABCDEBXYZ" 

20 Y$="B" 

30 PRINT INSTR(X$, Y$); INSTR(4, X$, Y$) 

RUN 
2 6 

The interpreter searches the string "ABCDFBXYZ" and finds the first occurrence of the character B at 
position 2 in the string. It then starts another search at position 4 (D) and finds the second match at 
position 6 (B). The last three characters are ignored, since all conditions set out in line 30 were 
satisfied. 


INT Function 


Purpose: 

To truncate an expression to a whole number. 

Syntax: 

INT(x) 

Comments: 

Negative numbers return the next lowest number. 

The FIX and Cl NT funetions also return integer values. 

Examples: 

PRINT INT(98. 89) 

98 

PRINT INT(-I2. II) 

-13 


lOCTL Statement 


Purpose: 

To allow GW-BASIC to send a "control data" string to a character device driver anytime after the 
driver has been opened. 

Syntax: 

IOCTL[#]file number,string 

Comments: 

file number is the file number open to the device driver. 

string is a valid string expression containing characters that control the device. 

lOCTL commands are generally 2 to 3 characters followed by an optional alphanumeric argument. An 
lOCTL string may be up to 255 bytes long, with commands within the string separated by semicolons. 

Examples: 

If a user had installed a driver to replace Iptl, and that driver was able to set page length (the number 
of lines to print on a page before issuing a form feed), then the following lines would open the new 
Iptl driver and set the page length to 66 lines: 

OPEN "LPTl:" FOR OUTPUT AS #1 
lOCTL #1, "PL66" 

The following statements open Iptl with an initial page length of 56 lines: 

OPEN "\DEV\LPT1" FOR OUTPUT AS #1 
lOCTL #1, "PL56" 


IOCTL$ Function 


Purpose: 

To allow GW-BASIC to read a "control data" string from an open character device driver. 

Syntax: 

IOCTL$([#]file number) 

Comments: 

file number is the file number open to the device. 

The I0CTL$ function is generally used to get acknowledgement that an lOCTL statement succeeded 
or failed. It is also used to get device information, such as device width after an lOCTL statement 
requests it. 

Examples: 

10 'GW is a possible command 
20 'for get device width 
30 OPEN "\DEV\MYLPT" AS#1 
40 lOCTYL#!, "GW" 

50 'Save it in WID 
60 WID=VAL(IOCTL$(#1)) 


KEY Statement 


Purpose: 

To allow rapid entry of as many as 15 characters into a program with one keystroke by redefining 
GW-BASIC special function keys. 

Syntax: 

KEY key number,string expression 
KEY n,CHR$(hex code)+CRR${scan code) 

KEY ON 
KEY OFF 
KEY LIST 

Comments: 

key number is the number of the key to be redefined, key number may range from 1-20. 

string expression is the key assignment. Any valid string of 1 to 15 characters may be used. If a 
string is longer than 15 characters, only the first 15 will be assigned. Constants must be enclosed in 
double quotation marks. 

scan code is the variable defining the key you want to trap. Appendix H in the GW-BASIC User's 
Guide lists the scan codes for the keyboard keys. 

hex code is the hexadecimal code assigned to the key shown below; 

Key Hex code 

EXTENDED &amp;H80 
CAPS LOCK&amp;H40 
NUMLOCK&amp;H20 
ALT &amp;H08 

CTRL &amp;H04 

SHIFT &amp;H01,&amp;H02, &amp;H03 

Hex codes may be added together, such as &amp;H03, which is both shift keys. 

Initially, the function keys are assigned the following special functions; 


FILIST 

F2 RUNf 


F3LOAD" 

F4 SAVE" 


FSCONTf 

F6 , "LPTl : 

r 

FTTRONf 

F8 TROEFf 


F9KEY 

FIOSCREEN 

ooor 


Note 

( (arrow) means that you do not have to press return after each of these keys has been pressed. 

Any one or all of the 10 keys may be redefined. When the key is pressed, the data assigned to it will 
be input to the program. 

KEY key number, "string expression" 

Assigns the string expression to the specified key. 






KEY LIST 


List all 10 key values on the sereen. All 15 charaeters of each value are displayed. 

KEY ON 

Displays the first six characters of the key values on the 25th line of the screen. When the display 
width is set at 40, five of the 10 keys are displayed. When the width is set at 80, all 10 are displayed. 

KEY OFF 

Erases the key display from the 25th line, making that line available for program use. key off does 
not disable the function keys. 

If the value for key number is not within the range of 1 to 10, or 15 to 20, an "illegal function 
call" error occurs. The previous key assignment is retained. 

Assigning a null string (length 0) disables the key as a function key. 

When a function key is redefined, the inkey$ function returns one character of the assigned string 
per invocation. If the function key is disabled, inkey$ returns a string of two characters: the first is 
binary zero; the second is the key scan code. 

Examples: 

10 KEY 1, "MENU"+CHR$(13) 

Displays a menu selected by the operator each time key 1 is pressed. 

1 KEY OFF 

Turns off the key display. 

10 DATA KEYl, KEY2, KEYS, KEY4, KEYS 
20 FOR N=1 TO 5: READ SOFTKEYS$(n) 

30 KEY N, SOFTKEYS$(I) 

40 NEXT N 
50 KEY ON 

Displays new function keys on line 25 of the screen. 

20 KEY 1, "" 

Disables function key 1. 


KEY(n) Statement 

Purpose: 

To initiate and terminate key eapture in a GW-BASIC program. 

Syntax: 

KEY(n) ON 
KEY(n) OFF 
KEY(n) STOP 

Comments: 

n is a number from 1 to 20 that indieates whieh key is to be eaptured. Keys are numbered as follows: 

Key 

Number Key 

1-10 Funetion keys F i through F i o 

11 CURSOR UP 

12 CURSOR LEFT 

13 CURSOR RIGHT 

14 CURSOR DOWN 

15-20 Keys defined in the following format (see KEY statement): key n, CHR$ (hex 

code) + CHR$(scan code) 

Exeeution of the key (n) on statement is required to aetivate keystroke eapture from the funetion 
keys or eursor eontrol keys. When the key (n) ON statement is aetivated and enabled, GW-BASIC 
eheeks eaeh new statement to see if the speeified key is pressed. If so, GW-BASIC performs a GO SUB 
to the line number speeified in the ON KEY(n) statement. An on KEY(n) statement must preeede a 
KEY(n) statement. 

When KEY (n) OFF is exeeuted, no key eapture oeeurs and no keystrokes are retained. 

If KEY (n) STOP is exeeuted, no key eapture oeeurs, but if a speeified key is pressed, the keystroke is 
retained so that immediate keystroke eapture oeeurs when a key (n) on is exeeuted. 


Eor further information on key trapping, see the ON KEY(n) statement. 




KILL Command 


Purpose: 

To delete a file from a disk. 

Syntax: 

KILL filename 
Comments: 

filename can be a program file, sequential file, or random-aeeess data file. 

KILL is used for all types of disk files, ineluding program, random data, and sequential data files. 

Note 

You must speeify the filename's extension when using the kill eommand. Remember that files saved 
in GW-BASIC are given the default extension .BAS. 

If a KILL eommand is given for a file that is eurrently open, a "File already open" error oeeurs. 
Examples: 

The following eommand deletes the GW-BASIC file DATAl, and makes the spaee available for 
realloeation to another file; 

200 KILL "DATAl.BAS" 

The following eommand deletes the GW-BASIC file RAINING from the subdireetory dogs: 

KILL "CATS\DOGS\RAINING.BAS" 




LEFT$ Function 


Purpose: 

To return a string that comprises the left-most n characters of x$. 

Syntax: 

LEFT$ {x$,n) 

Comments: 

n must be within the range of 0 to 255. If n is greater than len {x$) , the entire string {x$) will be 
returned. If n equals zero, the null string (length zero) is returned (see the MIPS and RIGHTS 
substring functions). 

Example: 

10 A$="BASIC" 

20 B$=LEFT$(A$, 3) 

30 PRINT B$ 

RUN 

BAS 

The left-most three letters of the string "BASIC" are printed on the screen. 




LEN FUNCTION 


Purpose: 

To return the number of eharacters in x$. 

Syntax: 

LEN(x$) 

Comments: 

Nonprinting characters and blanks are counted. 

Example: 

x$ is any string expression. 

10 X$="PORTLAND, OREGON" 

20 PRINT LEN(X$) 

RUN 

16 

Note that the comma and space are included in the character count of 16. 


LET Statement 


Purpose: 

To assign the value of an expression to a variable. 

Syntax: 

[LET] variable=expression 

Comments: 

The word let is optional; that is, the equal sign is suffieient when assigning an expression to a 
variable name. 

The LET statement is seldom used. It is included here to ensure compatibility with previous versions 
of BASIC that require it. 

When using let, remember that the type of the variable and the type of the expression must match. If 
they don't, a "Type mismatch" error occurs. 

Example 1: 

The following example lets you have downward compatibility with an older system. If this downward 
compatibility is not required, use the second example, as it requires less memory. 


110 

LET 

CM 
\—1 

II 

Q 

120 

LET 

E=12^2 

130 

LET 

&lt; 

CM 
\—1 

II 

140 

LET 

SUM=D+E+F 


Example 2: 


110 

CM 
\—1 

II 

Q 


120 

E=12 

^'2 

130 

F=12 

^4 

140 

SUM= 

D+E+F 


LINE Statement 


Purpose: 

To draw lines and boxes on the screen. 

Syntax: 

LINE [ {xl,yl)]-(x2,y2) [, [attribute] [, B[F] ][, style]] 

Comments: 

xl , yl and x2, y2 specify the end points of a line. 

Resolution mode is determined by the screen statement. 

attribute specifies color or intensity of the displayed pixel (see the COLOR and PALETTE 
statements). 

B (box) draws a box with the points (xl , yl) and (x2,y2) at opposite corners. 

BF (filled box) draws a box (as ,B) and fills in the interior with points. 


Note 

If attribute is not specified, two commas must be used before B or BE. 


LINE supports the additional argument style, style is a 16-bit integer mask used when putting 
down pixels on the screen. This is called line-styling. 

Each time line stores a point on the screen, it uses the current circulating bit in style. If that bit is 0, 
no store will be done. If the bit is a I, then a normal store is done. After each point, the next bit 
position in style is selected. 

Since a 0 bit in style does not clear out the old contents, you may wish to draw a background line 
before a styled line, in order to force a known background. 

style is used for normal lines and boxes, but is illegal for filled boxes. 

If the BF parameter is used with the style parameter, a "Syntax" error will occur. 

When out-of-range values are given in the line statement, the coordinates that are out of range are 
not visible on the screen. This is called line-clipping. 

In the syntax shown here, the coordinate form STEP (x offset, y offset) is not shown. 
However, this form can be used wherever a coordinate is used. 

In a LINE statement, if the relative form is used on the second coordinate, it is relative to the first 
coordinate. 

After a line statement, the last referenced point is x2, y2. 

The simplest form of line is the following; 

LINE -{xz,yz) 

This draws a line from the last point referenced to the point (x2 , y 2) in the foreground color. 






Examples: 

LINE (0, 100)-(639, 100) 

Draws a horizontal line which divides the screen in half from top to bottom in SCREEN 2. 

LINE (160,0)-(160,199) 

Draws a vertical line which divides the screen in half from left to right in SCREEN 1; makes a one- 
quarter/three-quarter division in SCREEN 2. 

LINE (0,0)-(319,199) 

Draws a diagonal line from the top left to lower right corner of the screen in SCREEN 1, and from the 
upper left comer to the center bottom of the screen in screen 2. 

LINE (10,10)-(20,20),2 

Draws a line in color 2 if SCREEN 1 is previously specified (see the COLOR statement). 

10 CLS 

20 LINE -(RND*319,RND*199) ,RND*4 
30 GOTO 20 

Draw lines forever using random attributes. 

10 FOR X=0 TO 319 

20 LINE (X,0)-(X,199),X AND 1 

30 NEXT 

Draws an alternating pattern: line on, line off 

10 CLS 

20 LINE -(RND*639,RND*199),RND*2,BF 
30 GOTO 20 

Draws lines all over the screen. 

LINE (0,0)-(100, 175) , ,B 
Draws a square box in the upper left corner of the screen. 

LINE (0,0)-(100,175),,BF 
Draws the same box and fills it in. 

LINE (0,0)-(100,175),2,BF 
Draws the same filled box in magenta in SCREEN 1. 

LINE (0,0)-(100,350) , ,B 
Draws the same box if SCREEN 2 is specified. 

400 SCREEN 1 

410 LINE(160,100)-(160,199),,,&amp;HCCCC 
Draws a vertical dotted line down the center of the screen in screen 1. 

220 SCREEN 2 

230 LINE (300,100)-(400,50),,B,&amp;HAAAA 
Draws a rectangle with a dotted line in SCREEN 2. 

LINE (0,0)-(160,100) , 3,,&amp;HFF00 
Draws a dotted line from the upper left comer to the screen center. 



