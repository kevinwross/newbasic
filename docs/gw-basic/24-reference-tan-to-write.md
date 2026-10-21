<!-- Chunked from GWBASIC.txt, lines 17553-18729. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: TAN - WRITE#

TAN Function 


Purpose: 

To calculate the trigonometrie tangent of x, in radians. 

Syntax: 

TAN(x) 

Comments: 

TAN (x) is calculated in single-precision unless the /d switch is used when GW-BASIC is executed. 

If TAN overflows, the "Overflow" error message is displayed; maehine infinity with the appropriate 
sign is supplied as the result, and execution continues. 

To obtain tan ( x) when x is in degrees, use tan (x*pi/180) . 

Examples: 

10 Y = TAN(X) 

When executed, Y will contain the value of the tangent of X radians. 


TIMES Statement and Variable 


Purpose: 

To set or retrieve the eurrent time. 

Syntax: 

As a statement: 

TIME$ = string exp 
As a variable: 

string exp=TIME$ 

Comments: 

string exp is a valid string literal or variable that lets you set hours (hh), hours and minutes 
{hh :mm), or hours, minutes, and seconds {hh :mm: ss). 

hh sets the hour (0-23). Minutes and seconds default to 00. 

hh :mm sets the hour and minutes (0-59). Seconds default to 00. 

hh :mm: ss sets the hour, minutes, and seconds (0-59). 

If string exp is not a valid string, a "Type mismatch" error results. 

As you enter any of the above values, you may omit the leading zero, if any. You must, however, enter 
at least one digit. If you wanted to set the time as a half hour after midnight, you could enter TIME$= 
"0:30",butnotTIME$= ":30". 

If any of the values are out of range, an "illegal function call" error is issued. The previous 
time is retained. 

The current time is stored if time $ is the target of a string assignment. 

The current time is fetched and assigned to the string variable if time$ is the expression in a let or 
PRINT statement. 

If string exp = TIME$, TIME$ returns an 8-character string in the form hh :mm: ss. 

Examples: 

The following example sets the time at 8:00 A.M.: 

TIME$ = "08:00" 

PRINT TIME$ 

08:00:05 

The following program displays the current date and time on the 25th line of the screen and will 
sound on the minute and half minute. 

10 KEY OFF: SCREEN 0: WIDTH 80: CLS 
20 LOCATE 25, 5 
30 PRINT DATE$, TIME$; 

40 SEC=VAL(MID$(TIME$, 7, 2)) 

50 IF SEC=SSEC THEN 20 ELSE SSEC=SEC 
60 IF SEC=0 THEN 1010 


70 IF SEC=30 THEN 1020 
80 IF SEC&lt;57 THEN 20 
1000 SOUND 1000, 2: GOTO 20 
1010 SOUND 2000, 8: GOTO 20 
1020 SOUND 400, 4: GOTO 20 


TIMER Function 


Purpose: 

To return single-precision floating-point numbers representing the elapsed number of seconds since 
midnight or system reset. 

Syntax: 

v=TIMER 

Comments: 

Fractions of seconds are calculated to the nearest degree possible. Timer is read-only. 


TRON/TROFF Commands 


Purpose: 

To trace the exeeution of program statements. 

Syntax: 

TRON 

TROFF 

Comments: 

As an aid in debugging, the TRON (trace on) command enables a trace flag that prints eaeh line 
number of the program as it is executed. The numbers appear enclosed in square braekets. 

TRON may be executed in either the direct or indirect mode. 

The trace flag is disabled with the TROFF (traee off) command, or when a new eommand is executed. 

Examples: 

TRON 
10 K=10 

20 FOR J=1 TO 2 

30 L=K + 10 

40 PRINT J; K; L 

50 K=K+10 

60 NEXT 

7 0 END 

RUN 

[10][20][30][40] 1 10 20 
[50] [60] [30] [40] 2 20 30 
[50][60][70] 

TROFF 


UNLOCK Statement 


Purpose: 

To release loeks that have been applied to an opened file. This is used in a multi-deviee environment, 
often referred to as a network or network environment. 


Syntax: 

UNLOCK [#]r! [, [record number] [TO record number]] 


Comments: 


n is the number that was assigned the file as it was originally numbered in the program. 


record number is the number of the individual reeord that is to be unloeked. Or, if a range of 
reeords are to be unloeked, reeord number designates the beginning and ending reeord of the speeified 
range. 

The range of legal reeord numbers is 1 to 2^^-l. The limit on reeord size is 32767 bytes. 

The reeord range speeified must be from lower to (the same or) higher reeord numbers. 

If a starting reeord number is not speeified, the reeord number 1 is assumed. 


If an ending reeord number is not speeified, then only the speeified reeord is unloeked. 

The following are legal unlock statements: 

unloeks the entire file n 
unloeks reeord x only 
unloeks reeords 1 through Y 
Y unloeks reeords x through Y 
The loeked file or reeord range should be unloeked before the file is elosed. 


UNLOCK 

#n 


UNLOCK 

#n, 

X 

UNLOCK 

#n, 

TO K 

UNLOCK 

#n, 

X TO 


Failure to exeeute the unlock statement can jeopardize future access to that file in a network 
environment. 


In the case of files opened in random mode, if a range of record numbers is specified, this range must 
match exactly the record number range given in the LOCK statement. 

The "Permission denied" message will appear if a syntactically correct UNLOCK request cannot 
be granted. The unlock statement must match exactly the paired lock statement. 

It is expected that the time in which files or regions within files are locked will be short, and thus the 
suggested usage of the lock statement is within shortterm paired lock/unlock statements. 


Examples: 

The following demonstrates how the lock/unlock statements should be used: 

LOCK #1, 1 TO 4 
LOCK #1, 5 TO 8 
UNLOCK #1, 1 TO 4 
UNLOCK #1, 5 TO 8 

The following example is illegal: 


LOCK #1, 
LOCK #1, 
UNLOCK # 


1 TO 4 
5 TO 8 
, 1 TO 8 


USR Function 


Purpose: 

To call an assembly language subroutine. 

Syntax: 

v=USR[r!] {argument) 

Comments: 

n specifies which uSR routine is being called. 
argument ean be any numerie or string expression. 

Although the CALL statement is reeommended for ealling assembly language subroutines, the USR 
function call may also be used. See Appendix D in the GW-BASIC User's Guide for a comparison of 
CALL and USR and for a detailed discussion of calling assembly language subroutines. 

Only values 0-9 are valid for n. If n is omitted, USRO is assumed (see DBF USR for the rules 
governing n). 

If a segment other than the default segment (GW-BASIC data segment, DS) is used, a def sec 
statement must be executed prior to a uSR call. This ensures that the code segment points to the 
subroutine being called. 

The segment address given in the def sec statement determines the starting segment of the 
subroutine. 

For eaeh USR function, a corresponding def USR statement must have been executed to define the 
USR call offset. This offset and the eurrently aetive def sec segment address determine the starting 
address of the subroutine. 

If more than 10 user routines are required, the value(s) of def usr may be redefined for the other 
starting addresses as many times as needed. 

The type (numerie or string) of the variable receiving the function call must be eonsistent with the 
argument passed. If no argument is required by the assembly language routine, then a dummy 
argument must be supplied. 




VAL Function 


Purpose: 

Returns the numerical value of string x$. 

Syntax: 

VAL {x$) 

Comments: 

The VAL function also strips leading blanks, tabs, and line feeds from the argument string. For example, the 
following line returns -3: 

VAL(" -3") 

The STR$ function (for numeric to string conversion) is the complement to the VAL {x$) function. 

If the first character of x$ is not numeric, the VAL {x$) will return zero. 

Examples: 

10 READ NAMES, CITY$, STATES, ZIPS 

20 IF VAL(ZIPS)&lt;90000 OR VAL(ZIPS)&gt;96699 THEN PRINT NAMES TAB(25) "OUT OF STATE" 
30 IF VAL(ZIPS)&gt;=90801 AND VAL(ZIPS)&lt;=90815 THEN PRINT NAMES TAB(25) "LONG BEACH" 


VARPTR Function 


Purpose: 

To return the address in memory of the variable or file eontrol block (FCB). 

Syntax: 

VARPTR (variaijie name) 

VARPTR (#file number) 

Comments: 

VARPTR is usually used to obtain the address of a variable or array so it can be passed to an assembly 
language subroutine. A function call of the following form; 

VARPTR(A(0)) 

is usually specified when passing an array, so that the lowest-addressed element of the array is 
returned. 

All simple variables should be assigned before calling varptr for an array, because the addresses of 
the arrays change whenever a new simple variable is assigned. 

VARPTR i#file number) returns the starting address of the GW-BASIC File Control Block 
assigned to Tile number. 

VARPTR ( variable name) returns the address of the first byte of data identified with the variable 
name. 

A value must be assigned to variable name prior to execution of varptr, otherwise, an " 11 legal 
function call" error results. 

Any type variable name may be used (numeric, string, or array), and the address returned will be an 
integer within the range of 32767 to -32768. If a negative address is returned, it is added to 65536 to 
obtain the actual address. 

Offsets to information in the FCB from the address returned by varptr are shown in the following 
table; 


Table 7 


Offsets to FCB Information 


Offset Length Name 

0 1 Mode 


1 38 FCB 

39 2 CURLOC 


Description 

The mode in which the file was opened; 

1 Input only 

2 Output only 
4 Random I/O 
16 Append only 
32 Internal use 
64 Future use 
128 Internal use 
Diskette file control block. 

Number of sectors read or written for sequential access. The last 
record number +1 read or written for random files. 


41 

42 

43 
46 


47 

48 

49 

50 

51 

179 

181 

183 

185 

186 
188 


1 

1 

3 

1 


1 

1 

1 

1 

128 

2 

2 

2 

1 

2 

n 


ORNOFS 
NMLOFS 
* * ^ 

DEVICE 


WIDTH 

POS 

EEAGS 

OUTPOS 

BUFEER 

VRECE 

PHYREC 

LOGREC 

^ :jc ^ 

OUTPOS 

EIEED 


Number of bytes in sector when read or written. 

Number of bytes left in input buffer. 

Reserved for future expansion. 

Device Number: 

0-9 Disks A: through J: 

255 kybd: 

254 SCRN: 

253 LPTl: 

252 CAST: 

251 COMl: 

250 COM2 : 

249 LPT2 : 

248 LPT3: 

Device width. 

Position in buffer for print. 

Internal use during bload/bsave. Not used for data files. 

Output position used during tab expansion. 

Physical data buffer. Used to transfer data between DOS and 
BASIC. Use this offset to examine data in sequential I/O mode. 
Variable length record size. Default is 128. Set by length option in 
OPEN statement. 

Current physical record number. 

Current logical record number. 

Euture use. 

Disk files only. Output position for print, input, and write. 
Actual FIELD data buffer. Size is determined by Siswitch. VRECE 
bytes are transferred between BUEEER and EIEED on I/O 
operations. Use this offset to examine file data in random I/O 
mode. 


Example 1: 

100 X=VARPTR(Y) 

When executed, the variable X will contain an address that points to the storage space assigned to the 
variable Y. 


Example 2: 

10 OPEN "DATA.FIL" AS #I 
20 FCBADR = VARPTR(#I) 

30 DATADR = FCBADR+I88 
40 A$ = PEEK(DATADR) 

In line 20, ECBADR contains the start of ECB. 

In line 30, DATADR contains the address of the data buffer. 

In line 40, A$ contains the first byte in the data buffer. 



VARPTRS Function 


Purpose: 

To return a character form of the offset of a variable in memory. 

Syntax: 

VARPTR$ {variable) 

Comments: 

variable is the name of a variable that exists in the program. 

Note 

Assign all simple variables before calling varptr$ for an array element, because the array addresses 
change when a new simple variable is assigned. 

VARPTR$ returns a three-byte string of the following form; 

1 Byte 0 I Byte 1 | Byte 2 | 

Byte 0 contains one of the following variable types: 

2 integer 

3 string 

4 single-precision 
8 double precision 

Byte 1 contains the 8086 address format, and is the least significant byte. 

Byte 2 contains the 8086 address format, and is the most significant byte. 

Examples: 

100 X = USR(VARPTR$(Y)) 




VIEW Statement 


Purpose: 

To define a physical viewport limit from xl, yl (upper-left x,y coordinates) to x2,y2 (lower-right x,y 
coordinates). 

Syntax: 

VIEW [[SCREEN] [(xi,yi)-(x2,y2) [,[fill] [, [border]]]] 

Comments: 

RUN or VIEW with no arguments define the entire screen as the viewport. 

{xl,yl) are the upper-left coordinates. 

{x2, y2) are the lower-right coordinates. 

The fill attribute lets you fill the view area with color. 

The border attribute lets you draw a line surrounding the viewport if space for a border is available. 
If border is omitted, no border is drawn. 

The X and y coordinates must be within the physical bounds of the screen and define the rectangle 
within the screen that graphics map into. The x and y coordinate pairs will be sorted, with the smallest 
values placed first. 

Points are plotted relative to the viewpoint if the screen argument is omitted; that is, xl and yl are 
added to the x and y coordinates before the point is plotted. 

It is possible to have a varied number of pairs of x and y. The only restriction is that xl cannot equal 
x2, and yi cannot equal y2. 

Points are plotted absolutely if the screen argument is present. Only points within the current 
viewpoint will be plotted. 

When using view, the CLS statement clears only the current viewport. To clear the entire screen, you 
must use view to disable the viewports. Then use CLS to clear the screen. CLS does not move the 
cursor to home. Press ctrl-home to send the cursor home, and clear the screen. 

Examples: 

The following defines a viewport such that the statement pset (0,0) , 3 would set down a point at 
the physical screen location 10,10. 

VIEW (10, I0)-(200, 100) 

The following defines a viewport such that the point designated by the statement pset (0,0) , 3 
would not appear because 0,0 is outside of the viewport. pset(10,10) ,3 would be within the 
viewport. 

VIEW SCREEN (10, 10)-(200, 100) 


VIEW PRINT Statement 


Purpose: 

To set the boundaries of the screen text window. 

Syntax: 

VIEW PRINT [topline TO bottomline] 

Comments: 

VIEW PRINT without topline and bottomline parameters initializes the whole screen area as the 
text window. The whole screen area consists of lines 1 to 24; by default, line 25 is not used. 

Statements and functions that operate within the defined text window include CLS, LOCATE, print, 
and SCREEN. 

The screen editor will limit functions such as scroll and cursor movement to the text window. 

For more information, see VIEW . 



WAIT Statement 


Purpose: 

To suspend program execution while monitoring the status of a machine input port. 

Syntax: 

WAIT port number, n[,j] 

Comments: 

port number represents a valid machine port number within the range of 0 to 65535. 
n and j are integer expressions in the range of 0 to 255. 

The WAIT statement causes execution to be suspended until a specified machine input port develops a 
specified bit pattern. 

The data read at the port is xORed with the integer expression j, and then ANDed with n. 

If the result is zero, GW-BASIC loops back and reads the data at the port again. 

If the result is nonzero, execution continues with the next statement. 

When executed, the wait statement tests the byte n for set bits. If any of the bits is set, then the 
program continues with the next statement in the program, wait does not wait for an entire pattern of 
bits to appear, but only for one of them to occur. 

It is possible to enter an infinite loop with the wait statement. You can exit the loop by pressing ctrl- 
BREAK, or by resetting the system. 

If j is omitted, zero is assumed. 

Examples: 

100 WAIT 32,2 

Suspends machine operation until port 32 receives 2 as input. 


WHILE ... WEND Statement 


Purpose: 

To execute a series of statements in a loop as long as a given condition is true. 
Syntax: 

WHILE expression 


[loop statements] 


END 

Comments: 

If expression is nonzero (true), loop statements are executed until the wend statement is 
encountered. GW-BASIC then returns to the while statement and checks expression. If it is still true, 
the process is repeated. 

If it is not true, execution resumes with the statement following the wend statement. 

WHILE and WEND loops may be nested to any level. Each wend matches the most recent while. 

An unmatched while statement causes a "while without wend" error. An unmatched wend 
statement causes a "wend without while" error. 

Examples: 

90 'BUBBLE SORT ARRAY A$ 

100 FLIPS=I 
no WHILE FLIPS 
115 FLIPS=0 
120 FOR N=I TO J-I 

130 IF A$(N)&gt;A$(N+I) THEN SWAP A$(N), A$(N+I): FLIPS=I 
140 NEXT N 
150 WEND 


WIDTH Statement 


Purpose: 

To set the printed line width in number of charaeters for the sereen and line printer. 

Syntax: 

WIDTH size 

WIDTH file number, size 

WIDTH "dev", size 

Comments: 

size, an integer within the range of 0 to 255, is the new width. 
file number is the number of the file that is open. 

dev is a valid string expression identifying the deviee. Valid deviees are SCRN: , LPTl: , LPT2 : , 

LPT3: , COMl : , and COM2: . 

Changing Screen Width 

The following statements are used to set the screen width. Only a 40- or 80-column width is allowed. 
WIDTH size 
WIDTH "SCRN:", size 
See the SCREEN statement for more information. 

Changing SCREEN mode affects screen width only when moving between SCREEN 2 and SCREEN 1 
or SCREEN 0. 

Note 

Changing the screen width clears the screen and sets the border screen color to black. 

Changing Line printer Width 

The following width statement is used as a deferred width assignment for the line printer. This 
statement stores the new width value without actually changing the current width setting; 

WIDTH "LPTl:", size 

A statement of the following form recognizes this stored width value; 

OPEN "LPTl:" FOR OUTPUT AS number 
and uses it while the file is open; 

WIDTH file number, size 

If the file is open to Iptl:, line printer width is immediately changed to the new size specified. This 
allows the width to be changed at will while the file is open. This form of width has meaning only 
for Iptl:. After outputting the indicated number of characters from the open file, GW-BASIC inserts a 
carriage return at the end of the line and wraps the output, if the width is less than the length of the 
record. 

Valid widths for the line printer are 1 through 255. 





Specifying width 25 5 for the line printer (Iptl:) enables line wrapping. This has the effect of 
infinite width. 

Any value entered outside of these ranges results in an "illegal function call" error. The 
previous value is retained. 

Using the width statement on a communications file causes a carriage return to be sent after the 
number of characters specified by the size attribute. It does not alter either the receive or transmit 
buffer. 

Examples: 

10 WIDTH "LPTI:", 75 

20 OPEN "LPTI:" FOR OUTPUT AS #I 


6020 WIDTH #1, 40 

Line 10 stores a line printer width of 75 characters per line. 

Line 20 opens file #1 to the line printer and sets the width to 75 for subsequent print #I, 
statements. 

Line 6020 changes the current line printer width to 40 characters per line. 


WINDOW Statement 


Purpose: 

To draw lines, graphics, and objects in space not bounded by the physical limits of the screen. 
Syntax: 

WINDOW[[SCREEN] {xl,yl)-{x2,y2)] 

Comments: 

{xl,yl) , (x2,y2) are the coordinates defined by the user. These coordinates, called the world 
coordinates, may be any single-precision, floating-point number. They define the world coordinate 
space that graphics statements map into the physical coordinate space, as defined by the view 
statement. 

WINDOW is the rectangular region in the world coordinate space. It allows zoom and pan. It allows the 
user to draw lines, graphics, and objects in space not bounded by the physical limits of the screen. To 
do this the user specifies the world coordinate pairs (xl,yl) and (x2,y2.) GW-BASIC then 
converts the world coordinate pairs into the appropriate physical coordinate pairs for subsequent 
display within screen space. 

Window inverts, with the screen attribute omitted, the y coordinate on subsequent graphics 
statements. This places the {xl,yl) coordinate in the lower-left and the (x2,y2) coordinate in the 
upper-right of the screen. This allows the screen to be viewed in true Cartesian coordinates. 

The coordinates are not inverted when the SCREEN attribute is included. This places the (xl, yl) 
coordinate in the upper-left and the {x2, y 2) coordinate in the lower-right corner of the screen. 

The WINDOW statement sorts the x and y argument pairs into ascending order. For example; 


WINDOW 

(50, 

o 
\— 1 

o 
\— 1 

1 

o 

LO 

becomes 

WINDOW 

(10, 

I0)-(50, 50) 

Or 



WINDOW 

(-2, 

2)-(2, -2) 

becomes 

WINDOW 

(-2, 

-2)-(2, 2) 


All coordinate pairs of x and y are valid, except that xl cannot equal x2 and yi cannot equal y2. 
WINDOW with no arguments disables previous window statements. 

Example 1: 

If you type: 

NEW: SCREEN 2 

the screen uses the standard coordinate attributes: 

(0, 0) (320, 0) (639, 0) 

i y increases 


(320, 100) 


( 0 , 199 ) 


( 320 , 199 ) 


( 639 , 199 ) 


Example 2: 

If you type; 

WINDOW (-1, -1)-(1, 1) 

the screen uses the Cartesian coordinates: 


(-1, 

1) 

(0, 

1) 

(1, 

1) 

T 

y increases 

(0, 

0) 



i 

y decreases 





(-1, 

-1) 

(0, 

-1) 

(1, 

-1) 


Example 3: 

If you type; 

WINDOW SCREEN (-1, -1)-(1, 1) 

the screen uses the non-inverted coordinate; 


(-1, 

-1) 

(0, 

-1) 

(1, 

-1) 

T y 

decreases 

(0, 

0) 



i y 

increases 





(-1, 

1) 

(0, 

1) 

(1, 

1) 


RUN, SCREEN, and WINDOW with no attributes disable any window definitions and return the screen to 
its normal physical coordinates. 


WRITE Statement 


Purpose: 

To output data to the screen. 

Syntax: 

WRITE [list of expressions] 

Comments: 

If list of expressions is omitted, a blank line is output. If list of expressions is included, the 
values of the expressions are output at the terminal. The expressions in the list may be numeric and/or 
string expressions, and must be separated by commas or semicolons. 

When printed items are output, each item will be separated from the last by a comma. Printed strings 
are delimited by double quotation marks. After the last item in the list is printed, GW-BASIC inserts a 
carriage retum/line feed. 

The difference between write and print is that write inserts commas between displayed items 
and delimits strings with double quotation marks. Positive numbers are not preceded by blank spaces. 

WRITE outputs numeric values using the same format as the print statement. 

Examples: 

10 A=80: B=90: C$="THAT'S ALL" 

20 WRITE A, B, C$ 

RUN 

80, 90, " 


THAT'S ALL 


WRITE# Statement 


Purpose: 

To write data to a sequential file. 

Syntax: 

WRITE #filenum, list of expressions 

Comments: 

filenum is the number under whieh the file was opened for output. 

list of expressions is a list of string and/or numeric expressions separated by commas or 
semicolons. 

The WRITE# and print# statements differ in that write# inserts commas between the items as they 
are written and delimits strings with quotation marks, making explicit delimiters in the list 
unnecessary. Another difference is that write# does not put a blank in front of a positive number. 
After the last item in the list is written, a carriage retum/line feed sequence is inserted. 

Examples: 

Let A$ = "CAMERA" and B$ = "93604-1". The following statement: 

WRITE#I, A$, B$ 
writes the following image to disk: 

"CAMERA", "93604-1" 

A subsequent input $ statement, such as the following, would input "CAMERA" to A$ and "93604- 
1" to B$: 


INPUT#I, A$, B$ 

    
