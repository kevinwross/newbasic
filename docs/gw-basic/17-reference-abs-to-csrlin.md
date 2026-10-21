<!-- Chunked from GWBASIC.txt, lines 8127-9410. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: ABS - CSRLIN

ABS Function 


Purpose: 

To return the absolute value of the expression 

Syntax: 

ABS in) 

Comments: 

n must be a numeric expression. 

Examples: 

PRINT ABS(7* (-5) ) 

35 

Prints 35 as the result of the action. 


ASC Function 


Purpose: 

To return a numeric value that is the ASCII code for the first character of the string x$. 

Syntax: 

ASC(x$) 

Comments: 

If x$ is null, an "Illegal Function Call" error is returned. 

If x$ begins with an uppercase letter, the value returned will be within the range of 65 to 90. 
If x$ begins with a lowercase letter, the range is 97 to 122. 

Numbers 0 to 9 return 48 to 57, sequentially. 

See the CHR$ function for ASCII-to-string conversion. 

See Appendix C in the GW-BASIC User's Guide for ASCII codes. 

Examples: 

10 X$="TEN" 

20 PRINT ASC(X$) 

RUN 

84 

84 is the ASCII code for the letter T. 




ATN Function 


Purpose: 

To return the aretangent of x, when x is expressed in radians. 

Syntax: 

ATN(x) 

Comments: 

The result is within the range of -7i/2 to %I2. 

The expression x may be any numerie type. The evaluation of ATN is performed in single preeision 
unless the /d switch is used when GW-BASIC is executed. 

To convert from degrees to radians, multiply by 71/I8O. 

Examples: 

10 INPUT X 
20 PRINT ATN(X) 

RUN 
? 3 

1.249046 

Prints the arctangent of 3 radians (1.249046). 


AUTO Command 


Purpose: 

To generate and increment line numbers automatically each time you press the return key. 

Syntax: 

AUTO [line number] [, [increment]] 

AUTO .[, [increment]] 

Comments: 

AUTO is useful for program entry because it makes typing line numbers unnecessary. 

AUTO begins numbering at line number and increments each subsequent line number by 
increment. The default for both values is 10. 

The period ( . ) can be used as a substitute for line number to indicate the current line. 

If line number is followed by a comma, and increment is not specified, the last increment 
specified in an AUTO command is assumed. 

If AUTO generates a line number that is already being used, an asterisk appears after the number to 
warn that any input will replace the existing line. However, pressing return immediately after the 
asterisk saves the line and generates the next line number. 

auto is terminated by entering ctrl-break or ctrl-c. GW-BASIC will then return to command level. 


Note 

The line in which ctrl-break or ctrl-c is entered is not saved. To be sure that you save all desired 
text, use CTRL-BREAK and ctrl-c only on lines by themselves. 


Examples: 

AUTO 100, 50 

Generates line numbers 100, 150, 200, and so on. 

auto 


Generates line numbers 10, 20, 30, 40, and so on. 




BEEP Statement 


Purpose: 

To sound the speaker at 800 Hz (800 eycles per second) for one-quarter of a second. 

Syntax: 

BEEP 

Comments: 

BEEP, CTRL-G, and PRINT CHR$ ( 7 ) have the same effect. 

Examples: 

2340 IF X&gt;20 THEN BEEP 

If X is out of range, the computer beeps. 


BLOAD Command 


Purpose: 

To load an image file anywhere in user memory. 

Syntax: 

BLOAD filenameifOffset] 

Comments: 

filename is a valid string expression containing the device and filename. 

offset is a valid numeric expression within the range of 0 to 65535. This is the offset into the 
segment, declared by the last def seg statement, where loading is to start. 

If offset is omitted, the offset specified at bsave is assumed; that is, the file is loaded into the same 
location it was saved from. 


Note 

BLOAD does not perform an address range check. It is possible to BLOAD anywhere in memory. You 
must not BLOAD over the GW-BASIC stack space, a GW-BASIC program, or the GW-BASIC 
variable area. 


While BLOAD and bsave are useful for loading and saving machine language programs, they are not 
restricted to them. The def seg statement lets you specify any segment as the source or target for 
BLOAD and BSAVE. For example, this allows the video screen buffer to be read from or written to the 
diskette, bload and bsave are useful in saving and displaying graphic images. 

Examples: 

10 DEF SEG=&amp;HB800 
20 BLOAD"PICTURE", 0 

The DEF SEG Statement in line 10 points the segment at the screen buffer. 

The DEF SEG statement in line 10 and the offset of 0 in line 20 guarantee that the correct address is 
used. 

The BLOAD command in line 20 loads the file named picture into the screen buffer. 


Note 

The BSAVE example in the next section illustrates how the file named picture is saved. 







BSAVE Command 


Purpose: 

To save portions of user memory on the speeified device. 

Syntax: 

BSAVE filename, offset, length 

Comments: 

filename is a valid string expression containing the filename. 

offset is a valid numeric expression within the range of 0 to 65535. This is the offset into the 
segment, declared by the last def seg, where saving is to start. 

length is a valid numeric expression within the range of 0 to 65535, specifying the length of the 
memory image to be saved. 

If filename is less than one character, a "Bad File Number" error is issued and the load is 
aborted. 

Execute a def seg statement before the bsave. The last known def seg address is always used for 
the save. 

The DEF SEG statement must be used to set up the segment address to the start of the screen buffer. 
An offset of 0 and a length of 16384 specify that the entire 16K screen buffer is to be saved. 

Examples: 

10 DEF SEG=&amp;HB800 

20 BSAVE"PICTURE", 0, 16384 

The DEF SEG Statement in line 10 points the segment at the screen buffer. 

The BSAVE command in line 20 saves the screen buffer in the file named picture. 


CALL Statement 


Purpose: 

To call an assembly (or machine) language subroutine. 

Syntax: 

CALL numvar[{variables) ] 

Comments: 

numvar is the starting point in memory of the subroutine being called as an offset into the current 

segment. 

variables are the variables or constants, separated by commas and enclosed in parentheses, that are 

to be passed to the routine. 

The CALL statement is recommended for interfacing assembly language programs with GW-BASIC. 

Although the USR funetion may also be used, call is compatible with more languages, produees a 

more readable source eode, and ean pass multiple arguments. 

Invocation of the call statement eauses the following to oeeur: 

• Each parameter loeation in the variable is pushed onto the staek. The parameter location is a 2- 
byte offset into GW-BASIC's data segment. 

• The return address code segment (CS) and the offset are pushed onto the staek. 

• Control is transferred to the user routine by the segment address given in the last def sec 
statement and the offset given in the variable name. 

• The user routine now has eontrol. Parameters may be refereneed by moving the staek pointer 
(SP) to the base pointer (BP) and adding a positive offset to BP. 

• The called routine may destroy the eontents of any registers. 

• The called program must know how many parameters were passed. Parameters are referenced 
by adding a positive offset to BP, assuming the ealled routine moved the eurrent staek pointer 
into BP (that is, MOV bp,sp). 

• The called program must know the variable type for numerie parameters passed. 

• The called routine must do a ret n, where n is the number of parameters in the variable times 
2. This is neeessary in order to adjust the staek to the point at the start of the ealling sequenee. 

• Values are returned to GW-BASIC by including in the argument list the name of the variable 
that is to reeeive the result. 

• If the argument is a string, the parameter offset points to three bytes called the string descriptor. 
Byte 0 of the string deseriptor eontains the length of the string (0 to 255). Bytes 1 and 2, 
respeetively, are the lower- and upper-eight bits of the string starting address in the string spaee. 

• If the argument is a string literal in the program, the string descriptor points to program text. Be 
careful not to alter or destroy a program this way. To avoid unpredictable results, add +"" to the 
string literal in the program, as in the following: 

20 A$="BASIC"+"" 



This forces the string literal to be eopied into the string spaee. Now the string may be modified 
without affecting the program. 


Note 

Strings may be altered by user routines, but their length must not be changed. GW-BASIC cannot 
correctly erase strings if their lengths are modified by external routines. 

For more information on the CALL statement and USR funetion, see Appendix D in the GW-BASIC 
User's Guide . 

Example 1: 

100 DEF SEG=&amp;H2000 
110 ARK=0 

120 CALL ARK(A, B$, C) 


Line 100 sets the segment to hex 2000. ARK is set to zero so that the call to ARK executes the 
subroutine at location 2000:0. 


Example 2: 


The following sequenee of 8086 Assembly Language demonstrates aecess of the parameters passed 
and stored in variable C: 


PUSH BP 

MOV BP, SP ; Gets current stack position in BP. 

MOV BX, 8[BP] ; Gets address of B$ descriptor. 

MOV CL, [BX] ; Gets length of B$ in CL. 

MOV DX, 1[BX] ; Gets address of B$ text in DX. 


10[BP] ; Gets address of A in SI. 

6[BP] ; Gets pointer to C in DI. 

; Stores variable A in C. 

; Restores stack and returns. 


MOV SI 
MOV DI 
MOVSW 
RET 6 


MOVSW eopies only two bytes. This is suffieient if variables A and C are integer. Four bytes must be 
copied if they are single precision; eight bytes, if they are double precision. 

Example 3: 

100 DEF SEG=&amp;H2000 

110 ACC=&amp;H7FA 

120 CALL ACC(A, B$, C) 


Line 100 sets the segment to hex 2000. The value of variable ACC is added into the address as the 
low word after the DEF SEG value is shifted four bits to the left (this is a funetion of the 






microprocessor, not of GW-BASIC). Here, ACC is set to &amp;H7FA, so that the eall to ACC exeeutes the 
subroutine at the loeation hex 2000:7FA (absolute address hex 207FA). 



CDBL Function 


Purpose: 

To convert x to a double-precision number. 

Syntax: 

CDBL(x) 

Comments: 

X must be a numeric expression. 

Example: 

10 A=454.67 
20 PRINT A; CDBL(A) 

RUN 

454.67 454.6700134277344 

Prints a double-precision version of the single-precision value stored in the variable named A. 

The last 11 numbers in the double-precision number have no meaning in this example, since A was 
previously defined to only two-decimal place accuracy. 


Note 

See the CINT and CSNG functions for converting numbers to integer and single precision, 
respectively. 






CHAIN Statement 


Purpose: 

To transfer control to the specified program and pass (chain) variables to it from the current program. 
Syntax: 

CHAIN[MERGE] filename[, [line] [, [ALL] [,DELETE range]]] 

Comments: 

MERGE overlays the current program with the called program. 


Note 

The called program must be an ASCII file (previously saved with the a option) if it is to be merged 
(see the MERGE command). 


filename is the name of the program that is called to be chained to. The .BAS extension is assumed 
unless another is specified. 

line is a line number or an expression that corresponds to a line number in the called program. It is 
the starting point for execution of the called program. Eor example the following begins execution of 
PROGl. BAS at line 1000; 

10 CHAIN "PROGl", 1000 
If line is omitted, execution begins at the first line. 

line is not affected by a renum command. However, the line numbers in the specified range are 
affected by a renum command. 

ALL specifies that every variable in the current program is chained to the called program. Eor 
example; 

20 CHAIN "PROGl", 1000, ALL 

If the ALL option is omitted, the current program must contain a COMMON statement to list the 
variables that are passed. 

CHAIN executes a restore before it runs the program that it is to be chained to. The read statement 
then gets the first item in the data statement. Reading will not resume where it left off in the program 
that is being chained. 

After an overlay is executed and used for a specific purpose, it is usually desirable to delete it so that a 
new overlay may be brought in. To do this, use the delete command. 

The CHAIN statement with the merge command leaves the files open and preserves the current option 
base setting. 

If the MERGE command is omitted, the OPTION BASE setting is preserved, and chain preserves no 
variable types or user-defined functions for use by the chained program. That is, any defint, 

DEFSNG, DEFDBL, DEFSTR, or DEF FN Statement containing shared variables must be restated in the 
chained program. 





When using the merge command, place user-defined functions before any chain merge statements 
in the program. Otherwise, they will be undefined after the merge is complete. 


CHDIR Command 


Purpose: 

To change from one working directory to another. 

Syntax: 

CHDIR pathname 
Comments: 

pathname is a string expression of up to 63 characters. 

To make sales the working direetory on Drive A; and inventory the working direetory on Drive B 
(assume A; is the default drive), type the following commands: 

CHDIR "SALES" 

CHDIR "B:INVENTORY" 


CHR$ Function 


Purpose: 

To convert an ASCII code to its equivalent charaeter. 

Syntax: 

CHR$( n) 

Comments: 

n is a value from 0 to 255. 

CHR$ is eommonly used to send a special character to the terminal or printer. For example, you eould 
send CHR$ ( 7 ) to sound a beep through the speaker as a preface to an error message, or you could 
send a form feed, CHR$ ( 12 ) , to the printer. 

See the ASC function for ASCII-to-numeric conversion. 

ASCII Codes are listed in Appendix C of the GW-BASIC User's Guide . 

Examples: 

PRINT CHR$(66); 

B 

This prints the ASCII eharaeter eode 66, which is the upperease letter B. 

PRINT CHR$(13); 

This command prints a carriage return. 




CINT Function 


Purpose: 

To round numbers with fractional portions to the next whole number or integer. 

Syntax: 

CINT(x) 

Comments: 

If X is not within the range of -32768 to 32767, an "Overflow" error occurs. 

See the FIX and INT functions, both of which return integers. 

Examples: 

PRINT CINT(45.67) 

46 

45.67 is rounded up to 46. 

Note 

See the CDBL and CSNG functions for converting numbers to the double-precision and single¬ 
precision data types, respectively. 








CIRCLE Statement 


Purpose: 

To draw a circle, ellipse, and angles on the screen during use of the Graphies mode. 

Syntax: 

CIRCLE (xcenter, ycenter), radius [, [ color] [ , [start] , [end] [ , aspect]]] 

Comments: 

xcenter and ycenter are the x- and y- coordinates of the center of the ellipse, and radius is the 
radius (measured along the major axis) of the ellipse. The quantities xcenter and ycenter can be 
expressions. The center attributes ean use either absolute or relative eoordinates. 

color specifies the color of the ellipse. Its value depends on the current screen mode. See the 
COLOR and SCREEN statements for more information on using eolors in different sereen modes. In 
the high-resolution mode, 0 indicates black and 1 indicates white. The default for the high resolution 
mode is 1. 

The start and end angle parameters are radian arguments between -In and 2n which specify where 
the drawing of the ellipse is to begin and end. If start or end is negative, the ellipse is conneeted to 
the eenter point with a line, and the angles are treated as if they are positive (note that this is different 
from adding 2n). 

aspect deseribes the ratio of the x radius to the y radius (x:y). The default aspeet ratio depends on 
the sereen mode, but gives a visual cirele in either graphies mode, assuming a standard monitor sereen 
aspect ratio of 4:3. If the aspect ratio is less than 1, then the radius is given in x-pixels. If it is greater 
than I, the radius is given in y-pixels. In many cases, an aspect ratio of I gives better ellipses in the 
medium-resolution mode. This also eauses the ellipse to be drawn faster. The start angle may be less 
than the end angle. 

Example 1: 

10 SCREENl: CIRCLE{100,100), 50 

Draws a eirele of radius 50, eentered at graphics points lOOx and lOOy. 

Example 2: 

1 ' This will draw 17 ellipses 
10 CLS 
20 SCREEN 1 

30 FOR R=160 TO 0 STEP-10 
40 CIRCLE (160,100),R,,,,5/18 
50 NEXT 

Example 3: 

10 'This will draw 5 spheres 
20 GOTO 160 
50 IF VERT GOTO 100 
60 CIRCLE (X,Y),R,C,,,.07 
70 FOR I = 1 TO 5 
80 CIRCLE (X,Y),R,C,,, 


I*.2:NEXT I 




90 IF VERT THEN RETURN 
100 CIRCLE (X,Y) ,R,C, ,,1.3 
110 CIRCLE (X,Y) ,R,C,,, 1 . 9 
120 CIRCLE (X,Y) ,R,C, ,,3.6 
130 CIRCLE (X,Y) ,R,C, ,,9.8 
140 IF VERT GOTO 60 
150 RETURN 

160 CLS: SCREEN 1: COLOR 0,1: KEY OFF: VERT=0 

170 X=160: Y=100: C=1: R=50: GOSUB 50 

180 X=30: Y=30: C=2: R=30: GOSUB 50 

190 X=30: Y=169: GOSUB 50 

200 X=289: Y=30: GOSUB 50 

210 X=289: Y=169: GOSUB 50 

220 LINE (30,30)-(289,169),1 

230 LINE (30,169)-(289,30) ,1 

240 LINE (30,169)-(289,30) , 1,B 

250 Z$=INKEY$: IF Z$="" THEN 250 


CLEAR Command 


Purpose: 

To set all numeric variables to zero, all string variables to null, and to close all open files. Options set 
the end of memory and reserve the amount of string and stack space available for use by GW-BASIC. 

Syntax: 

CLEAR[, [expression!] [,expression2]] 

Comments: 

expression! is a memory location that, if specified, sets the maximum number of bytes available 
for use by GW-BASIC. 

expression2 sets aside stack space for GW-BASIC. The default is the previous stack space size. 
When GW-BASIC is first executed, the stack space is set to 512 bytes, or one-eighth of the available 
memory, whichever is smaller. GW-BASIC allocates string space dynamically. An "Out of String 
Space" error occurs only if there is no free memory left for GW-BASIC to use. 

The CLEAR command; 

• Closes all files 

• Clears all COMMON and user variables 

• Resets the stack and string space 

• Releases all disk buffers 

• Turns off any sound 

• Resets sound to music foreground 

• Resets pen to off 

• Resets strig to off 

• Disables ON error trapping 

Examples: 

CLEAR 

Zeroes variables and nulls all strings. 

CLEAR 32768 

Zeroes variables, nulls strings, protects memory above 32768, does not change the stack space. 

CLEAR , , 20 00 

Zeroes variables, nulls strings, allocates 2000 bytes for stack space, and uses all available memory in 
the segment. 

CLEAR ,32768,2000 

Zeroes variables, nulls strings, protects memory above 32768, and allocates 2000 bytes for stack 
space. 



CLOSE Statement 


Purpose: 

To terminate input/output to a disk file or a deviee. 

Syntax: 

CLOSE [[#] filenumber [, [#] filenumber] .. .] 

Comments: 

filenumber is the number under which the file was opened. The association between a particular 
file or device and file number terminates upon execution of a CLOSE statement. The file or device 
may then be reopened using the same or a different file number. A CLOSE statement with no file 
number specified closes all open files and devices. 

A CLOSE statement sent to a file or device opened for sequential output writes the final buffer of 
output to that file or device. 

The END, NEW, RESET, SYSTEM, or RUN and LOAD (without r option) statements always close all files 
or devices automatically. STOP does not close files. 

Examples: 

250 CLOSE 

This closes all open devices and files. 

300 CLOSE 1, #2, #3 

Closes all files and devices associated with file numbers 1,2, and 3. 


CLS Statement 


Purpose: 

To clear the screen. 

Syntax: 

CLS [n] 

Comments: 

n is one of the following values: 

Value of n Effect 

0 Clears the screen of all text and graphics 

1 Clears only the graphics viewport 

2 Clears only the text window 

If the graphics viewport is active, CLS without argument clears only the viewport. If the graphics 
viewport is inaetive, CLS elears the text window. 

If the screen is in alpha mode, the active page is cleared to the currently seleeted background color 
(see the SCREEN and COLOR statements). 

If the screen is in graphics mode, the entire screen buffer is cleared to background color. 

The screen may also be cleared by pressing ctrl-home, or by changing the screen mode with the 
SCREEN or WIDTH Statements. 

CLS returns the cursor to the upper-left comer of the screen, and sets the last point referenced to the 
center of the screen. 

If the VIEW statement has been used, CLS clears only the last viewport specified. 

Examples: 

I CLS 


This clears the screen. 




COLOR Statement 


Purpose: 

To select display colors 
Syntax: 

COLOR [foreground] [, [background] [,border]] 

COLOR [background] [, [palette]] 

COLOR [foreground] [, [background]] 

Comments: 

In general, COLOR allows you to select the foreground and background colors for the display. In 
SCREEN 0 a border color can also be selected. In screen 1 no foreground color can be selected, but 
one of two four-color palettes can be selected for use with graphics statements. The different syntaxes 
and effects that apply to the various screen modes are described below; 

Mode Effect 

SCREEN 0 Modifies the current default text foreground and background colors, and the screen 
border. The foreground color must be an integer expression in the range 0-31. It is 
used to determine the "foreground" color in text mode, which is the default color of 
text. Sixteen colors can be selected with the integers 0-15. A blinking version of 
each color can be selected by adding 16 to the color number; for example, a blinking 
color 7 is equal to 7 + 16, or 23. Thus, the legal integer range for foreground is 0- 
31. 

The background color must be an integer expression in the range 0-7, and is the 
color of the background for each text character. Blinking colors are not permitted. 

The border color is an integer expression in the range 0-15, and is the color used 
when drawing the screen border. Blinking colors are not permitted. 

If no arguments are provided to COLOR, then the default color for background and 
border is black (color 0), and for foreground, is as described in the SCREEN 
statement reference pages. 

SCREEN 1 In mode 1, the color statement has a unique syntax that includes a palette 

argument, which is an odd or even integer expression. This argument determines the 
set of display colors to use when displaying particular color numbers. 

For hardware configurations that do not have an IBM® Enhanced Graphics Adapter 
(EGA), the default color settings for the palette parameter are equivalent to the 
following; 

COLOR ,0 'Same as the next three PALETTE statements 
'1 = green, 2 = red, 3 = yellow 

COLOR ,1 'Same as the next three PALETTE statements 
'1 = cyan, 2 = magenta, 3 = white 

With the EGA, the default color settings for the palette parameter are equivalent to 
the following; 



COLOR ,0 'Same as the next three PALETTE statements 

PALETTE 1,2 'Attribute 1 = color 3 (green) 

PALETTE 2,4 'Attribute 2 = color 5 (red) 

PALETTE 3,6 'Attribute 3 = color 6 (brown) 

COLOR ,1 'Same as the next three PALETTE statements 

PALETTE 1,3 'Attribute 1 = color 3 (cyan) 

PALETTE 2,5 'Attribute 2 = color 5 (magenta) 

PALETTE 3,7 'Attribute 3 = color 15 (white) 

Note that a color statement will override previous palette statements. 

No effect. An "illegal function call" error message occurs if COLOR is 
used in this mode. 

In these modes, no border color can be specified. The graphics background is 
given by the background color number, which must be in the valid range of 
color numbers appropriate to the screen mode. See the SCREEN statement 
reference pages for more details. The foreground color argument is the default 
line drawing color. 

Arguments outside valid numeric ranges result in "illegal function call" errors. 

The foreground color may be the same as the background color, making displayed characters 
invisible. The default background color is black, or color number 0, for all display hardware 
configurations and all screen modes. 

With the Enhanced Graphics Adapter (EGA) installed, the palette statement gives you flexibility in 
assigning different display colors to the actual color-number ranges for the foreground, 
background, and border colors discussed above. See the PALETTE statement reference pages for 
more details. 

For more information, see CIRCLE . DRAW . LINE. PAINT. PALETTE. PRESET . PSET. and 
SCREEN 


SCREEN 2 

SCREEN 7- 
SCREEN 10 


Examples: 

The following series of examples show COLOR statements and their effects in the various screen 
modes: 


SCREEN 0 
COLOR 1, 

2, 3 

'foreground=l, 

background=2, border=3 

SCREEN 1 
COLOR 1, 

0 

'foreground=l, 

even palette number 

COLOR 2, 

1 

'foreground=2, 

odd palette number 

SCREEN 7 
COLOR 3, 

5 

'foreground=3, 

background=5 

SCREEN 8 
COLOR 6, 

7 

'foreground=6, 

background=7 

SCREEN 9 
COLOR 1, 

2 

'foreground=l, 

background=2 












COM(n) Statement 

Purpose: 

To enable or disable trapping of oommunieations aetivity to the speeified eommunieations adapter. 

Syntax: 

COM(n) ON 
COM(n) OFF 
COM(n) STOP 

Comments: 

n is the number of the eommunieations adapter 1 or 2. 

Execute a COM {n) on statement before ON COM ( n) statement to allow trapping. After COM (n) on, 
if a nonzero number is specified in the ON COM {n) statement, BASIC checks every new statement to 
see if any characters have come in the communication adapter. 

With COM (n) OFF, no trapping takes place, and all communications activity will be lost. 

With COM ( n) STOP, no trapping takes place. However, any communication that takes place will be 
remembered so that immediate trapping will occur when COM {n) ON is executed. 


COMMON Statement 


Purpose: 

To pass variables to a chained program. 

Syntax: 

COMMON variables 
Comments: 

variables are one or more variables, separated by commas, that you want to pass to the chained 
program. 

The COMMON statement is used in conjunction with the chain statement. 

COMMON statements may appear anywhere in a program, although it is recommended that they appear 
at the beginning. 

Any number of COMMON statements may appear in a program, but the same variable cannot appear in 
more than one COMMON statement. To pass all variables using the chain statement, use the all option, 
and omit the COMMON statement. 

Place parentheses after the variable name to indicate array variables. 

Examples: 

100 COMMON A, B, C, D(),G$ 
no CHAIN "A:PROG3" 

This example chains to program PROG3.BAS on disk drive A;, and passes the array D along with the 
variables A, B, C, and string G$. 


CONT Command 


Purpose: 

To continue program execution after a break. 

Syntax: 

CONT 

Comments: 

Resumes program execution after ctrl-break, stop, or end halts a program. Execution continues at 
the point where the break happened. If the break took place during an input statement, execution 
continues after reprinting the prompt. 

CONT is useful in debugging, in that it lets you set break points with the STOP statement, modify 
variables using direct statements, continue program execution, or use GOTO to resume execution at a 
particular line number. If a program line is modified, CONT will be invalid. 


cos Function 


Purpose: 

To return the eosine of the range of x. 

Syntax: 

COS(x) 

Comments: 

X must be the radians. COS is the trigonometric cosine function. To convert from degrees to radians, 
multiply by 71/I8O. 

COS (x) is calculated in single precision unless the /d switch is used when GW-BASIC is executed. 

Example 1: 

10 X=2*COS ( .4) 

20 PRINT X 
RUN 

1.842122 

Example 2: 

10 PI=3.141593 
20 PRINT COS (PI) 

30 DEGREES=180 
40 RADIANS=DEGREES*PI/180 
50 PRINT COS(RADIANS) 

RUN 

-1 

-1 


CSNG Function 


Purpose: 

To convert x to a single-precision number. 

Syntax: 

CSNG(x) 

Comments: 

X must be a numeric expression (see the CINT and CDBL functions). 
Examples: 

10 A#=975.3421222# 

20 PRINT A#; CSNG(A#) 

RUN 

975.3421222 975.3421 




CSRLIN Variable 


Purpose: 

To return the eurrent line (row) position of the eursor. 

Syntax: 

y=CSRLIN 

Comments: 

y is a numeric variable receiving the value returned. The value returned is within the range of 1 to 25. 

The CSRLIN variable returns the vertical coordinate of the cursor on the active page (see the 
SCREEN statement). 

x=POS ( 0 ) returns the column location of the cursor. The value returned is within the range of 1 to 40, 
or 1 to 80, depending on the current screen width (see the POS function). 

Examples: 

10 Y=CSRLIN 
20 X=POS(0) 

30 LOCATE 24, 1 
40 PRINT "HELLO" 

50 LOCATE Y, X 
RUN 
HELLO 

The CSRLIN variable in line 10 records the current line. 

The POS function in line 20 records the current column. 

In line 40, the print statement displays the comment "hello" on the 24* line of the screen. 

The LOCATE statement in line 50 restores the position of the cursor to the original line and column. 




CVI, CVS, CVD Functions 

Purpose: 

To convert string values to numerie values. 

Syntax: 

CVl{2-byte string) 

CVS{4-byte string) 

CVD{8-byte string) 

Comments: 

Numeric values read in from a random-aeeess disk file must be converted from strings back into 
numbers if they are to be arithmetically manipulated. 

CVI eonverts a 2-byte string to an integer. MKI$ is its complement. 

CVS converts a 4-byte string to a single-precision number. MKS$ is its eomplement. 

CVD converts an 8-byte string to a double-precision number. mkd$ is its eomplement. 

(See MKI$, MKS$. and MKD$). 

Examples: 


70 FIELD #1, 4 AS N$, 12 AS B$... 
80 GET #I 
90 Y=CVS(N$) 


Line 80 reads a field from file #1 (the field read is defined in line 70), and converts the first four bytes 
(N$) into a single-preeision number assigned to the variable Y. 

Sinee a single-preeision number can contain as many as seven ASCII characters (seven bytes), writing 
a file using MKS$ conversion, and reading with the CVS conversion, as many as three bytes per 
number recorded are saved on the storage medium. Even more may be saved if double-preeision 
numbers are required. mkd$ and CVD eonversions would be used in this ease. 





