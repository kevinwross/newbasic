<!-- Chunked from GWBASIC.txt, lines 9411-10819. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: DATA - FIX (incl. FOR... NEXT)

DATA Statement 


Purpose: 

To store the numeric and string constants that are accessed by the program read statement(s). 

Syntax: 

DATA constants 
Comments: 

constants are numeric constants in any format (fixed point, floating-point, or integer), separated by 
commas. No expressions are allowed in the list. 

String constants in data statements must be surrounded by double quotation marks only if they 
contain commas, colons, or significant leading or trailing spaces. Otherwise, quotation marks are not 
needed. 

DATA statements are not executable and may be placed anywhere in the program. A data statement 
can contain as many constants that will fit on a line (separated by commas), and any number of data 
statements may be used in a program. 

READ statements access the data statements in order (by line number). The data contained therein 
may be thought of as one continuous list of items, regardless of how many items are on a line or 
where the lines are placed in the program. The variable type (numeric or string) given in the read 
statement must agree with the corresponding constant in the data statement, or a "Type Mismatch" 
error occurs. 

DATA statements may be reread from the beginning by use of the RESTORE statement. 

For further information and examples, see the RESTORE statement and the READ statement. 

Example 1: 


80 FOR 1=1 TO 10 
90 READ A(I) 

100 NEXT I 

110 DATA 3.08,5.19,3.12,3.98,4.24 
120 DATA 5.08,5.55,4.00,3.16,3.37 


This program segment reads the values from the data statements into array A. After execution, the 
value of A(l) is 3.08, and so on. The data statements (lines 110-120) may be placed anywhere in the 
program; they may even be placed ahead of the read statement. 

Example 2: 

5 PRINT 

10 PRINT "CITY","STATE","ZIP" 

2 0 READ C$, S$, Z 

30 DATA "DENVER,","COLORADO", 80211 




40 PRINT C$,S$,Z 
RUN 

CITY STATE ZIP 
DENVER, COLORADO 802II 

This program reads string and numeric data from the data statement in line 30. 


DATE$ Statement and Variable 


Purpose: 

To set or retrieve the eurrent date. 

Syntax: 

As a statement: 

DATE$=v$ 

As a variable: 

v.$=DATE$ 

Comments: 

v$ is a valid string literal or variable. 

v$ can be any of the following formats when assigning the date: 

mm-dd-yy 

mm/dd/yy 

mm-dd-yyyy 

mm/dd/yyyy 

If v$ is not a valid string, a "Type Mismatch" error results. Previous values are retained. 

If any of the values are out of range or missing, an "illegal Function Call" error is issued. Any 
previous date is retained. 

The current date (as assigned when the operating system was initialized) is fetched and assigned to the 
string variable if date$ is the expression in a let or print statement. 

The current date is stored if date$ is the target of a string assignment. 

With v$=DATE$, DATE$ returns a 10-character string in the form mm-dd-yyyy. mm is the month (01 
to 12), dd is the day (01 to 31), and yyyy is the year (1980 to 2099). 

Examples: 

v$=DATE$ 

PRINT V$ 

01-01-1985 


DEF FN Statement 


Purpose: 

To define and name a funetion written by the user. 

Syntax: 

DEF FNname[arguments] expression 

Comments: 

name must be a legal variable name. This name, preceded by FN, becomes the name of the function. 

arguments consists of those variable names in the function definition that are to be replaced when 
the function is called. The items in the list are separated by commas. 

expression is an expression that performs the operation of the function. It is limited to one 
statement. 

In the DEF FN statement, arguments serve only to define the function; they do not affect program 
variables that have the same name. A variable name used in a function definition may or may not 
appear in the argument. If it does, the value of the parameter is supplied when the function is called. 
Otherwise, the current value of the variable is used. 

The variables in the argument represent, on a one-to-one basis, the argument variables or values that 
are to be given in the function call. 

User-defined functions may be numeric or string. If a type is specified in the function name, the value 
of the expression is forced to that type before it is returned to the calling statement. If a type is 
specified in the function name and the argument type does not match, a "Type Mismatch" error 
occurs. 

A user-defined function may be defined more than once in a program by repeating the DEF FN 
statement. 

A DEF FN statement must be executed before the function it defines may be called. If a function is 
called before it has been defined, an "undefined User Function" error occurs. 

DEF FN is illegal in the direct mode. 

Recursive functions are not supported in the DEF FN statement. 

Examples: 


400 

R=1: 

: S=2 

410 

DEF 

FNAB(X 

420 

T=FNAB(R, 


Y) =X''3/Y^2 


Line 410 defines the user-defined function FNAB. The function is called in line 420. When executed, 
the variable T will contain the value divided by S^, or .25. 


DEFINT/SNG/DBL/STR Statements 


Purpose: 

To declare variable types as integer, single-precision, double-precision, or string. 

Syntax: 

DEFtype letters 

Comments: 

type is INT (integer), SNG (single-precision number), dbl (double-precision number), or STR (string 
of 0-255 characters). 

letters are letters (separated by commas) or range of letters of the alphabet. 

A DEFtype statement declares that variable names beginning with the letter(s) specify that type of 
variable. However, a type declaration character (%,!,#,$) always takes precedence over a DEFtype 
statement in the typing of a variable. 

If no type declaration statements are encountered, BASIC assumes all variables are single-precision. 
Single-precision is the default value. 

Examples: 

10 DEFDBL L-P 

All variables beginning with the letters L, M, N, O, and P will be double-precision variables. 

10 DEFSTR A 
20 A="120#" 

All variables beginning with the letter A will be string variables. The $ declaration is unnecessary in 
this example. 

10 DEFINE I-N, W-Z 
20 W$="120#" 

All variables beginning with the letters I, J, K, L, M, N, W, X, Y, Z will be integer variables. W$ in 
Line 20 establishes a string variable beginning with the letter W. However, the variable W will remain 
an integer elsewhere in the program. 


DEF SEG Statement 


Purpose: 

To assign the current segment address to be referenced by a subsequent bload, bsave, call, peek, 
POKE, or USR. 

Syntax: 

DEF SEG [=address] 

Comments: 

address is a numeric expression within the range of 0 to 65535. 

The address specified is saved for use as the segment required by bload, bsave, peek, poke, and 
CALL statements. 

Entry of any value outside the address range (0-65535) results in an "illegal Function Call" 
error, and the previous value is retained. 

If the address option is omitted, the segment to be used is set to GW-BASIC's data segment (DS). This 
is the initial default value. 

If you specify the address option, base it on a 16-byte boundary. 

Segment addresses are shifted 4 bits to the left; so to get the segment address, divide the memory 
location by 16. 

For BLOAD, BSAVE, PEEK, POKE, or CALL statements, the value is shifted left four bits (this is done by 
the microprocessor, not by GW-BASIC) to form the code segment address for the subsequent call 
instruction (see the BLOAD, BSAVE, CALL , PEEK, and POKE statements). 

GW-BASIC does not perform additional checking to assure that the resultant segment address is 
valid. 

Examples: 

10 DEF SEG=&amp;HB800 

Sets segment to screen buffer. 

20 DEF SEG 

Restores segment to BASIC DS. 

Note 

DEF and SEG must be separated by a space. Otherwise, GW-BASIC will interpret the statement 
DEFSEG=100 to mean, "assign the value 100 to the variable DEFSEG." 









DEF USR Statement 


Purpose: 

To specify the starting address of an assembly language subroutine to be called from memory by the 
USR function. 

Syntax: 

DEF USR [n] =ir!teger 

Comments: 

n may be any digit from 0 to 9. The digit corresponds to the USR routine address being specified. If n 
is omitted, DEF USRO is assumed. 

integer is the offset address of the USR routine. If more than 10 USR routines are required, DEF 
USR [ n ] may appear in the program as many times as necessary to redefine the USR [ n ] starting 
address. 

Add the current segment value to the integer to get the starting address of the user routine. 

When an Assembly Language Subroutine is called, the GW-B ASIC program execution is paused, and 
control is transferred to the Assembly Language program. When that program is executed, control is 
returned to the GW-BASIC program at the point of interruption. 

Examples: 


190 DEF SEG=0 

200 DEF USR0=24000 

210 X=USR0 (Y^2/2 .82) 


Lines 190 and 200 set the absolute address. 

Line 210 calls the USR routine located at that address, and passes the integer value of the expression 
contained within the parentheses to the user program (see USR). 


Note 

This statement is given here primarily to provide compatibility with other GW-BASIC 
implementations. The more versatile CALL statement should be used if this downward compatibility is 
unimportant. 





DELETE Command 


Purpose: 

To delete program lines or line ranges. 

Syntax: 

DELETE [line numberl] [-line nuTnber2] 

DELETE line number 1- 

Comments: 

line numberl is the first line to be deleted. 
line number2 is the last line to be deleted. 

GW-BASIC always returns to eommand level after a delete eommand is exeeuted. Unless at least 
one line number is given, an "illegal Function Call" error oeeurs. 

The period ( . ) may be used to substitute for either line number to indieate the eurrent line. 

Examples: 

DELETE 40 

Deletes line 40. 

DELETE 40-100 

Deletes lines 40 through 100, inelusively. 

DELETE -40 

Deletes all lines up to and ineluding line 40. 

DELETE 40- 

Deletes all lines from line 40 to the end of the program. 


DIM Statement 


Purpose: 

To specify the maximum values for array variable subscripts and allocate storage accordingly. 
Syntax: 

DIM variable(subscripts) [, variable(subscripts )] ... 

Comments: 

If an array variable name is used without a dim statement, the maximum value of its subscript(s) is 
assumed to be 10. If a subscript greater than the maximum specified is used, a "Subscript out o 
range" error occurs. 

The maximum number of dimensions for an array is 255. 

The minimum value for a subscript is always 0, unless otherwise specified with the OPTION BASE 
statement. 

An array, once dimensioned, cannot be re-dimensioned within the program without first executing a 
CLEAR or ERASE Statement. 

The DIM statement sets all the elements of the specified arrays to an initial value of zero. 

Examples: 

10 DIM A(20) 

20 FOR 1=0 TO 20 
30 READ A(I) 

40 NEXT I 

This example reads 21 data statements elsewhere in the program and assigns their values to A(0) 
through A(20), sequentially and inclusively. If the A array is single precision (default accuracy) then 
line 10 will allocate 84 bytes of memory to this array (4 bytes times 21 elements). 


DRAW Statement 


Purpose: 

To draw a figure. 

Syntax: 

DRAW string expression 

Comments: 

The DRAW statement eombines most of the eapabilities of the other graphics statements into an object 
definition language called Graphics Macro Language (GML). A GML command is a single character 
within a string, optionally followed by one or more arguments. 

The DRAW statement is valid only in graphics mode. 

Movement Commands: 

Each of the following movement commands begins movement from the current graphics position. 

This is usually the coordinate of the last graphics point plotted with another GML command, line, or 
PSET. The current position defaults to the center of the screen (160,100 in medium resolution; 

320,100 in high resolution) when a program is run. Movement commands move for a distance of scale 
factor *n, where the default for n is 1; thus, they move one point if n is omitted and the default scale 
factor is used. 


Command 

Moves 

Un 

up 

Dn 

down 

Ln 

left 

Rn 

right 

En 

diagonally up and right 

Fn 

diagonally down and right 

Gn 

diagonally down and left 

En 

diagonally up and left 


This command moves as specified by the following argument: 

Mx, y Move absolute or relative. If x is preceded by a + or -, x and y are added to the 

current graphics position, and connected to the current position by a line. Otherwise, 
a line is drawn to point x, y from the current position. 

The following prefix commands may precede any of the above movement commands: 

B Move, but plot no points. 

N Move, but return to original position when done. 

The following commands are also available: 

An Set angle n. n may range from 0 to 3, where 0 is 0°, 1 is 90°, 2 is 180°, and 3 is 270°. 

Figures rotated 90° or 270° are scaled so that they will appear the same size as with 
0° or 180° on a monitor screen with the standard aspect ratio of 4:3. 

TAn Turn angle n. n can be any value from negative 360 to positive 360. If the value 

specified by n is positive, it turns the angle counter-clockwise. If the value specified 
by n is negative, it turns clockwise. 

Cn 


Sn 


xstring; 

variable 


Set color n. See the COLOR, PALETTE, and SCREEN statements for discussions 
of valid colors, numbers and attributes. 

Set scale factor, n may range from 1 to 255. n is divided by 4 to derive the scale 
factor. The scale factor is multiplied by the distances given with u, D, L, R, E, F, G, 
H, or relative M commands to get the actual distance traveled. The default for s is 

4. 

Execute substring. This command executes a second substring from a string, much 
like GO SUB. One string executes another, which executes a third, and so on. 


string is a variable assigned to a string of movement commands. 

Ppaint, Specifies the colors for a graphics figure and creates a filled-in figure. 

boundary 

pain t specifies what color you want the figure filled in with. 
boundary specifies the border color (outline). 

See the COLOR, PALETTE, and SCREEN statements for discussions of valid 
colors, numbers and attributes. 


You must specify values for both paint and boundary when used. 


This command {Ppaint , boundary) does not paint color tiling. 


Numeric Arguments: 

Numeric arguments can be constants like "123" or "^variable; ", where variable is the name of 
variable. 

When you use the second syntax, "^variable; ", the semicolon must be used. Otherwise, the 
semicolon is optional between commands. 

You can also specify variables using varptr$ ( variable) . 

Example 1: 

To draw a box in medium resolution; 

10 SCREEN 1 
20 A=20 

30 DRAW "U=A; R=A; D=A; L=A;" 


Example 2: 

The aspect ratio to draw a square on a standard screen is 4:3, as shown below: 

To draw a 96 pixel-wide square on a 640 x 200 pixel screen (SCREEN 2), do the following 
calculations: 

Horizontal value = 96 

Vertical value = 96* (200/640)*(4/3) 


or 

Vertical value = 40 

Horizontal value = 40* (640/200)*(3/4) 

The horizontal values equals 4/3 of the vertical values. 








Example 3: 

To draw a triangle in medium resolution: 

10 CLS 

20 SCREEN 1 

30 PSET (60, 125) 

40 DRAW "ElOO; FlOO; L199" 


EDIT Command 


Purpose: 

To display a specified line, and to position the cursor under the first digit of the line number, so that 
the line may be edited. 

Syntax: 

EDIT line number 

EDIT . 

Comments: 

line number is the number of a line existing in the program. 

A period ( . ) refers to the current line. The following command enters edit at the current line; 

EDIT . 

When a line is entered, it becomes the current line. 

The current line is always the last line referenced by an edit statement, list command, or error 
message. 

If line number refers to a line which does not exist in the program, an "Undefined Line 
Number" error occurs. 

Examples: 

EDIT 150 

Displays program line number 150 for editing. 


END Statement 


Purpose: 

To terminate program execution, close all files, and return to command level. 

Syntax: 

END 

Comments: 

END statements may be placed anywhere in the program to terminate execution. 

Unlike the STOP statement, END does not cause a "Break in line xxxx" message to be printed. 

An END statement at the end of a program is optional. GW-BASIC always returns to command level 
after an end is executed. 

END closes all files. 

Examples: 

520 IF K&gt;1000 THEN END ELSE GOTO 20 

Ends the program and returns to command level whenever the value of K exceeds 1000. 


ENVIRON Statement 


Purpose: 

To allow the user to modify parameters in GW-BASIC's environment string table. This may be to 
ehange the path parameter for a ehild process, (see ENVIRONS, SHELL, and the MS-DOS utilities 
PATH command), or to pass parameters to a child by inventing a new environment parameter. 

Syntax: 

ENVIRON string 
Comments: 

string is a valid string expression containing the new environment string parameter. 
string must be of the following form 

parmid=text 

where parmid is the name of the parameter such as PATH. 

parmid must be separated from text by an equal sign or a blank, environ takes everything to the left 
of the first blank or equal sign as the parmid; everything following is taken as text. 

text is the new parameter text. If text is a null string, or consists only of a single semicolon, then 
the parameter (including parmid=) is removed from the environment string table, and the table is 
compressed, text must not contain any embedded blanks. 

If parmid does not exist, then string is added at the end of the environment string table. 

If parmid does exist, it is deleted, the environment string table is compressed, and the new string is 
added at the end. 

Examples: 

Assuming the environment string table is empty, the following statement will create a default path to 
the root directory on Disk A; 

ENVIRON "PATH=A:\" 

If your work subdirectory were John, you would be able to get DEBUG from the root. 

A new parameter may be added; 

ENVIRON "COMSPEC=A:\COMMAND.COM" 

The environment string table now contains 
PATH=A:\; COMSPEC=A:\COMMAND.COM 
The path may be changed to a new value: 

ENVIRON "PATH=A:\SALES; A:\ACCOUNTING" 

The path parameter may be appended by using the environ$ function with the environ statement; 

ENVIRON "PATH="+ENVIRON$("PATH")+"; B:\SAMPLES" 

Finally, delete the parameter COMSPEC: 

ENVIRON "COMSPEC=;" 




The environment string table now contains 

PATH=A:\SALES; A:\ACCOUNTING; B:\SAMPLES 


ENVIRONS Function 


Purpose: 

To allow the user to retrieve the speeified environment string from the environment table. 

Syntax: 

v.?=ENVIRON$ iparmid) 
v.f=ENVIRON$ (nthparm) 

Comments: 

parmid is a valid string expression containing the parameter to search for. 
nthparm is an integer expression in the range of 1 to 255. 

If a string argument is used, environ$ returns a string containing the text following parmid= from 
the environment string table. 

If parmi d is not found, then a null string is returned. 

If a numeric argument is used, environ$ returns a string containing the nth parameter from the 
environment string table. 

If there is no n th parameter, then a null string is returned. 

The ENViRON$ function distinguishes between upper- and lowercase. 

Examples: 

The following lines; 

ENVIRON "PATH=A:\SALES; A:\ACOUNTING; B:\MKT:" 'Create entry 
PRINT ENVIRON$("PATH") 'Print entry 

will print the following string: 

A:\SALES; A:\ACCOUNTING; B:\MKT 

The following line will print the first string in the environment: 

PRINT ENVIRON$(I) 

The following program saves the environment string table in an array so that it can be modified for a 
child process. After the child process completes, the environment is restored. 

10 DIM ENVTBL$(10) 

20 NPARMS= I 

30 WHILE LEN(ENVIRON$(NPARMS))&gt;0 
40 ENVTBL$ (NPARMS) = ENVIRON$(NPARMS) 

50 NPARMS = NPARMS + I 
60 WEND 

70 NPARMS = NPARMS-I 

72 WHILE LEN(ENVIRON$(I))&gt;0 

73 A$=MID$(ENVIRON$(I), I, INSTR (ENVIRON$(I), "=")) 

74 ENVIRON A$+";" 

75 WEND 

90 ENVIRON "MYCHILDPARMI=SORT BY NAME" 

100 ENVIRON "MYCHILDPARM2=LIST BY NAME" 


1000 SHELL "MYCHILD"'RUNS "MYCHILD.EXE" 

1002 WHILE LEN(ENVIRON$ (1))&gt;0 

1003 A$=MID$(ENVIRON$ (1), 1, INSTR(ENVIRON$ (1), "=")) 

1004 ENVIRON A$+";" 

1005 WEND 

1010 FOR 1=1 TO NPARMS 
1020 ENVIRON ENVTBL$(I) 

1030 NEXT I 


The DIM statement in line 10 assumes no more than 10 parameters will be aecessed. 

In line 20, the initial number of parameters is established as 1. 

In lines 30 through 70, a series of statements are used to adjust and correct the parameter numbers. 
Line 71 deletes the present environment. 

Lines 72 through 80 create a new environment. Line 74 deletes the string. 

Lines 80 through 100 store the new environment. 

Lines 1000 through 1030 repeat the procedure by deleting the present environment and restore the 
parameters established in the first part of the program. 


EOF Function 


Purpose: 

To return -1 (true) when the end of a sequential or a oommunieations file has been reaehed, or to 
return 0 if end of file (EOF) has not been found. 

Syntax: 

v=EOF(file number) 

Comments: 

If a GET is done past the end of the file, EOF returns -1. This may be used to find the size of a file 
using a binary seareh or other algorithm. With oommunieations files, a -1 indioates that the buffer is 
empty. 

Use EOF to test for end of file while inputting to avoid "input Past End" errors. 

Examples: 


10 

OPEN "I", 

1, "DATA 

20 

o 

II 

O 


30 

IF EOF(l) 

THEN 100 

40 

INPUT#1, 

M(C) 

50 

C=C+1: GOTO 30 

100 

END 



The file named DATA is read into the M array until the end of the file is reaehed, then the program 
branohes to line 100. 


ERASE Statement 


Purpose: 

To eliminate arrays from a program. 

Syntax: 

ERASE list of array variables 

Comments: 

Arrays may be re-dimensioned after they are erased, or the memory space previously allocated to the 
array may be used for other purposes. 

If an attempt is made to re-dimension an array without first erasing it, an error occurs. 

Examples: 

200 DIM B (250) 

450 ERASE A, B 
460 DIM B(3, 4) 

Arrays A and B are eliminated from the program. The B array is re-dimensioned to a 3-column by 4- 
row array (12 elements), all of which are set to a zero value. 


ERDEV and ERDEV$ Variables 


Purpose: 

To return the aetual value (erdev) of a deviee error, and the name of the deviee (erdev$) eausing the 
error. 

Syntax: 

ERDEV 

ERDEV$ 

Comments: 

ERDEV will eontain the error code from interrupt 24H in the lower 8 bits. Bits 8 to 15 from the 
attribute word in the Device Header Block are mapped directly into the upper 8 bits. 

ERDEV$ will contain the 8-byte character device name if the error was on a character device. It will 
contain the 2 byte block device name (A;, B;, etc.) if the device was not a character device. 

Examples: 

Installed device driver lpt2: caused a "Printer out of paper" error via INT 24H. 

ERDEV contains the error number 9 in the lower 8 bits, while the upper 8 bits contain the upper byte of 
the Device Header word attributes. 

ERDEV$ contains "LPT2: 


ERR and ERL Variables 


Purpose: 

To return the error code (err) and line number (erl) associated with an error. 

Syntax: 

v=ERR 

v=ERL 

Comments: 

The variable err contains the error code for the last occurrence of an error. All the error codes and 
their definitions are listed in Appendix A of the GW-BASIC User's Guide . 

The variable erl contains the line number of the line in which the error was detected. 

The ERR and erl variables are usually used in if-then, or on error. . . GOTO, or gosub 
statements to direct program flow in error trapping. 

If the statement that caused the error was a direct mode statement, erl will contain 65535. To test if 
an error occurred in a direct mode statement, use a line of the following form; 

IF 65535=ERL THEN ... 

Otherwise, use the following; 

10 IF ERR=error code THEN...GOSUB 4000 
20 IF ERL=line number THEN...GOSUB 4010 

Note 

If the line number is not on the right side of the relational operator, it cannot be renumbered by 

RENUM. 


Because erl and err are reserved variables, neither may appear to the left of the equal sign in a let 
(assignment) statement. 





ERROR Statement 


Purpose: 

To simulate the oceurrence of an error, or to allow the user to define error eodes. 

Syntax: 

ERROR integer expression 

Comments: 

The value of integer expression must be greater than 0 and less than 255. 

If the value of integer expression equals an error code already in use by GW-BASIC, the 
ERROR statement simulates the occurrence of that error, and the corresponding error message is 
printed. 

A user-defined error code must use a value greater than any used by the GW- BASIC error codes. 
There are 76 GW-BASIC error codes at present. It is preferable to use a code number high enough to 
remain valid when more error codes are added to GW-BASIC. 

User-defined error codes may be used in an error-trapping routine. 

If an ERROR statement specifies a code for which no error message has been defined, GW-BASIC 
responds with the message "unprintable Error". 

Execution of an ERROR statement for which there is no error-trapping routine causes an error message 
to be printed and execution to halt. 

For a complete list of the error codes and messages already defined in GW-BASIC, refer to Appendix 
A in the GW-BASIC User's Guide. 


Examples: 

The following examples simulate error 15 (the code for "string too long"): 

10 S=10 
20 T=5 

30 ERROR S+T 

4 0 END 

RUN 

String too long in 30 

Or, in direct mode: 

ERROR 15 (you type this line) 

String too long (GW-BASIC types this line) 

The following example includes a user-defined error code message. 


110 ON ERROR GOTO 400 

120 INPUT "WHAT IS YOUR BET";B 

130 IF B&gt;5000 THEN ERROR 210 




400 IF ERR=210 THEN PRINT "HOUSE LIMIT IS $5000" 
410 IF ERL=I30 THEN RESUME 120 


EXP Function 


Purpose: 

To return e (the base of natural logarithms) to the power of x. 

Syntax: 

EXP (x) 

Comments: 

X must be less than 88.02969. 

If EXP overflows, the "Overflow" error message appears; maehine infinity with the appropriate sign 
is supplied as the result, and execution continues. 

EXP (x) is calculated in single precision, unless the /d switch is used when GW-BASIC is executed. 

Examples: 

10 X = 5 

20 PRINT EXP(X-l) 

RUN 

54.59815 

Prints the value of e to the 4th power. 


EXTERR Function 


Purpose: 

To return extended error information. 

Syntax: 

EXTERR (n) 

Comments: 

EXTERR returns "extended" error information provided by versions of DOS 3.0 and greater. For 
versions of DOS earlier than 3.0, EXTERR always returns zero. The single integer argument must be in 
the range 0-3 as follows: 

Value of uReturn Value 

0 Extended error code 

1 Extended error class 

2 Extended error suggested action 

3 Extended error locus 

The values returned are not defined by GW-BASIC, but by DOS. Refer to the MS-DOS Programmer's 
Reference (version 3.0 or later) for a description of the values returned by the DOS extended error 
function. 

The extended error code is actually retrieved and saved by GW-BASIC each time appropriate DOS 
functions are performed. Thus when an EXTERR function call is made, these saved values are 
returned. 


FIELD Statement 


Purpose: 

To allocate space for variables in a random file buffer. 

Syntax: 

FIELD [#] filenum, width AS stringvar [, width AS strlngvar] . .. 

Comments: 

filenum is the number under which the file was opened. 

width is the number of characters to be allocated to the string variable. 

string variable is a string variable which will be used for random file access. 

Afield statement must have been executed before you can 

• Get data out of a random buffer after a GET statement 

• Enter data before a PUT statement 

For example, the following line allocates the first 20 positions (bytes) in the random file buffer to the 
string variable N$, the next 10 positions to ID$, and the next 40 positions to ADD$: 

FIELD I, 20 AS N$, 10 AS ID$, 40 AS ADD$ 

FIELD only allocates space; it does not place any data in the random file buffer. 

The total number of bytes allocated in a field statement must not exceed the record length specified 
when the file was opened. Otherwise, a "Field overflow" error occurs (the default record length is 
128). 

Any number of field statements may be executed for the same file, and all field statements 
executed are in effect at the same time. 

Note 

Do not use a fielded variable name in an input or let statement. Once a variable name is fielded, it 
points to the correct place in the random file buffer. If a subsequent input or let statement with that 
variable name is executed, the variable's pointer is moved to string space (see LSET/RSET and GET 
statements). 






FILES Command 


Purpose: 

To print the names of the files residing on the speeified drive. 

Syntax: 

FILES [pathname] 

Comments: 

If pa thname is omitted, the eommand lists all files in the current directory of the selected drive. 
pathname may contain question marks (?) to match any character in the filename or extension. An 
asterisk (*) as the first character of the filename or extension will match any file or any extension. 

This syntax also displays the name of the directory and the number of bytes in the file. When a tree- 
structured directory is used, two special symbols also appear. 

Subdirectories are denoted by &lt;dir&gt; following the directory name. 

Examples: 

FILES 

FILES "*.BAS" 

FILES "B:*.*" 

FILES "TEST?.BAS" 

FILES now allows pathnames. The directory for the specified path is displayed. If an explicit path is 
not given, the current directory is assumed. 

FILES "ACCTS\" 

Lists all files in the directory named accts. 

FILES "B:ACCTS\*.PAY" 

Lists all files in the directory named accts that are on the diskette in drive B; and have the extension 

oi.PAY. 


FIX Function 


Purpose: 

To truncate x to a whole number. 

Syntax: 

FIX(x) 

Comments: 

FIX does not round off numbers, it simply eliminates the deeimal point and all eharacters to the right 
of the decimal point. 

FIX (x) is equivalent to SGN (x) *INT (ABS (x) ) . The major differenee between fix and int is that 
FIX does not return the next lower number for negative x. 

FIX is useful in modulus arithmetic. 

Examples: 

PRINT FIX(58.75) 

58 

PRINT FIX(-58.75) 

-58 


FOR... NEXT Statements 


Purpose: 

To execute a series of instructions a specified number of times in a loop. 
Syntax: 

FOR variable=x TO y [STEP z] 


NEXT [variable] [,variable...] 

Comments: 

variable is used as a counter. 
x,y, and z are numeric expressions. 

STEP z specifies the counter increment for each loop. 

The first numeric expression (x) is the initial value of the counter. The second numeric expression (y) 
is the final value of the counter. 

Program lines following the FOR statement are executed until the next statement is encountered. 
Then, the counter is incremented by the amount specified by STEP. 

If STEP is not specified, the increment is assumed to be 1. 

A check is performed to see if the value of the counter is now greater than the final value (y). If it is 
not greater, GW-BASIC branches back to the statement after the FOR statement, and the process is 
repeated. If it is greater, execution continues with the statement following the next statement. This is 

a FOR-NEXT loop. 

The body of the loop is skipped if the initial value of the loop times the sign of the step exceeds the 
final value times the sign of the step. 

If STEP is negative, the final value of the counter is set to be less than the initial value. The counter is 
decremented each time through the loop, and the loop is executed until the counter is less than the 
final value. 

Nested Loops: 

FOR-NEXT loops may be nested; that is, a for-next loop may be placed within the context of 
another for-next loop. When loops are nested, each loop must have a unique variable name as its 
counter. 

The NEXT statement for the inside loop must appear before that for the outside loop. 

If nested loops have the same end point, a single next statement may be used for all of them. 

The variable (s ) in the NEXT statement may be omitted, in which case the next statement will 
match the most recent FOR statement. 

If a NEXT statement is encountered before its corresponding for statement, a "next without for" 
error message is issued and execution is terminated. 


Examples: 

The following example prints integer values of the variable 1% from 1 to 10 in steps of z. For fastest 
exeeution, I is deelared as an integer by the % sign. 

10 K=10 

20 FOR I%=1 TO K STEP 2 
30 PRINT 1% 

40 NEXT 
RUN 
1 
3 
5 
7 
9 

In the following example, the loop does not execute because the initial value of the loop exceeds the 
final value. Nothing is printed by this example. 

10 R=0 

20 FOR S=1 TO R 
30 PRINT S 
40 NEXT S 

In the next example, the loop executes 10 times. The final value for the loop variable is always set 
before the initial value is set. 

10 S=5 

20 FOR S=1 TO S+5 
30 PRINT S; 

40 NEXT 
RUN 

123456789 10 


