<!-- Chunked from GWBASIC.txt, lines 14828-16012. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: PRINT - SCREEN (function)

PRINT Statement 


Purpose: 

To output a display to the screen. 

Syntax: 

PRINT [list of expressions] [; ] 

?[list of expressions] [; ] 

Comments: 

If list of expressions is omitted, a blank line is displayed. 

If list of expressions is included, the values of the expressions are displayed. Expressions in 
the list may be numeric and/or string expressions, separated by commas, spaces, or semicolons. String 
constants in the list must be enclosed in double quotation marks. 

For more information about strings, see the STRINGS function. 

A question mark (?) may be used in place of the word print when using the GW-BASIC program 
editor. 

Print Positions 

GW-BASIC divides the line into print zones of 14 spaces. The position of each item printed is 
determined by the punctuation used to separate the items in the list: 

Separator Print Position 

r Beginning of next zone 

; Immediately after last value 

space(s) Immediately after last value 

If a comma, semicolon, or SPC or tab function ends an expression list, the next print statement 
begins printing on the same line, accordingly spaced. If the expression list ends without a comma, 
semicolon, or SPC or tab function, a carriage return is placed at the end of the lines (GW-BASIC 
places the cursor at the beginning of the next line). 

A carriage retum/line feed is automatically inserted after printing width characters, where width is 
40 or 80, as defined in the width statement. This results in two lines being skipped when you print 
exactly 40 (or 80) characters, unless the print statement ends in a semicolon. 

When numbers are printed on the screen, the numbers are always followed by a space. Positive 
number are preceded by a space. Negative numbers are preceded by a minus (-) sign. Single¬ 
precision numbers are represented with seven or fewer digits in a fixed-point or integer format. 

See the LPRINT and LPRINT USING statements for information on sending data to be printed on a 
printer. 

Examples: 

10 X$= STRING$(10,45) 

20 PRINT X$"MONTHLY REPORT" X$ 

RUN 

-MONTHLY REPORT- 

45 is the decimal equivalent of the ASCII symbol for the minus (-) sign. 







PRINT USING Statement 


Purpose: 

To print strings or numbers using a specified format. 

Syntax: 

PRINT USING string expressions;list of expressions[;] 


Comments: 

string expressions is a string literal or variable consisting of special formatting characters. The 
formatting characters determine the field and the format of printed strings or numbers. 

list of expressions consists of the string or numeric expressions separated by semicolons. 


String Fields 

The following three characters may be used to format the string field; 

! Specifies that only the first character in the string is to be printed. 

\n spaces\ Specifies that 2+n characters from the string are to be printed. 

If the backslashes are typed with no spaces, two characters are printed; if the 
backslashes are typed with one space, three characters are printed, and so on. 


If the string is longer than the field, the extra characters are ignored. If the field is longer 
than the string, the string is left-justified in the field and padded with spaces on the right. 
For example: 


10 

A$="LOOK": 

B$ = 

"OUT" 

30 

PRINT 

USING 

" ! 

A$; B$ 

40 

PRINT 

USING 

"\ 

\"; A$; B$ 

50 

PRINT 

USING 

"\ 

\"; A$; B$;" 

RUN 





LO 

LOOKOUT 
LOOK OUT!! 


I " 


&amp; Specifies a variable length string field. When the field is specified with &amp;, the string is 

output exactly as input. For example; 

10 A$="LOOK": B$="OUT" 

20 PRINT USING " ! "; A$ 

30 PRINT USING B$ 

RUN 

LOUT 


Numeric Fields 

The following special characters may be used to format the numeric field; 

# A pound sign is used to represent each digit position. Digit positions are always filled. If 

the number to be printed has fewer digits than positions specified, the number is right- 
justified (preceded by spaces) in the field. 


+ 


■k -k 


$$ 


**$ 


A decimal point may be inserted at any position in the field. If the format string 
specifies that a digit is to precede the decimal point, the digit always is printed (as 0 if 
necessary). Numbers are rounded as necessary. For example: 

PRINT USING 78 

0.78 


PRINT USING "###.## 
987.65 

PRINT USING "##.##" 
10.20 5.30 66.79 0 


;987.654 

;I0.2,5.3,66.789,.234 
23 


In the last example, three spaces were inserted at the end of the format string to 
separate the printed values on the line. 

A plus sign at the beginning or end of the format string causes the sign of the number 
(plus or minus) to be printed before or after the number. 

A minus sign at the end of the format field causes negative numbers to be printed with 
a trailing minus sign. For example: 

PRINT USING"!##.##";-68.95,2.4,55.6,-9 
-68.95 +2.40 +55.60 -0.90 

PRINT USING"##.##-";-68.95,22.449,-7.01 
68.95 22.45 7.01- 


A double asterisk at the beginning of the format string causes leading spaces in the 
numeric field to be filled with asterisks. The * * also specifies two more digit 
positions. For example: 

PRINT USING "**#.#";I2.39,-0.9,765.I 
*12.4* -09765.1 

A double dollar sign at the beginning of the format string causes a dollar sign to be 
printed to the immediate left of the formatted number. The $ $ specifies two more 
digit positions, one of which is the dollar sign. The exponential format cannot be used 
with $ $ . Negative numbers cannot be used unless the minus sign trails to the right. 
For example: 

PRINT USING "$$###.##"; 456.78 
$456.78 

The * * $ at the beginning of a format string combines the effects of the above two 
symbols. Leading spaces are filled with asterisks, and a dollar sign is printed before 
the number. * * $ specifies three more digit positions, one of which is the dollar sign. 
For example: 

PRINT USING "**$##.##";2.34 
***$2.34 

A comma to the left of the decimal point in the format string causes a comma to be 
printed to the left of every third digit to the left of the decimal point. A comma at the 
end of the format string is printed as part of the string. 

PRINT USING "####.##";I234.5 
1234.50 


Four carets may be placed after the digit position characters to specify exponential 
format. The four carets allow space for E+xx to be printed. Any decimal point 


position may be specified. The significant digits are left-justified, and the exponent 
is adjusted. Unless a leading + or trailing + or - is specified, one digit position is 
used to the left of the decimal point to print a space or a minus sign. For example; 


PRINT USING 

2.35E + 02 


234.56 

PRINT 

OK 

USING 

"_####AAAA 

-";888888 

PRINT 

USING 

fl_l_ ^^AAAAfl 

;I23 


+.12E+03 

Note that in the above examples the comma is not used as a delimiter with the 
exponential format. 

An underscore in the format string causes the next character to be output as a literal 
character. For example: 

PRINT USING "_!##.##_!";I2.34 
! 12.34! 

The literal character itself may be an underscore by placing " " in the format 
string. 

A percent sign is printed in front of the number if the number to be printed is larger 
than the specified numeric field. If rounding causes the number to exceed the field, 
a percent sign is printed in front of the rounded number. For example: 

PRINT USING "##.##";III.22 
%III.22 

PRINT USING -.999 

% 1.00 

If the number of digits specified exceeds 24, an "illegal function call" 
error results. 


PRINT# and PRINT# USING Statements 


Purpose: 

To write data to a sequential disk file. 

Syntax: 

PRINT#rile number, [VSlNGstring expressions;]list of expressions 

Comments: 

file n umber is the number used when the file was opened for outpuf. 

string expressions consists of the formatting characters described in the PRINT USING statement. 

list of expressions consists of the numeric and/or string expressions to be written to the file. 

Double quotation marks are used as delimiters for numeric and/or string expressions. The first double quotation mark 
opens the line for input; the second double quotation mark closes it. 

If numeric or string expressions are to be printed as they are input, they must be surrounded by double quotation 
marks. If the double quotation marks are omitted, the value assigned to the numeric or string expression is printed. If 
no value has been assigned, 0 is assumed. The double quotation marks do not appear on the screen. For example: 

10 PRINT#I, A 
0 

10 A=26 
20 PRINT#I, A 
26 

10 A=26 

20 PRINT#I, "A" 

A 

If double quotation marks are required within a string, use CHR$ ( 34 ) (the ASCII character for double quotation 
marks). For example: 

100 PRINT#I,"He said,"Hello", I think" 

He said, 0, I think 

because the machine assigns the value 0 the variable "Hello." 

100 PRINT#1, "He said, "CHR$(34) "Hello,"CHR$(34) " I think." 

He said, "Hello," I think 

If the strings contain commas, semicolons, or significant leading blanks, surround them with double quotation marks. 
The following example will input "CAMERA" to A$, and "AUTOMATIC 93604-1" to B$: 

10 A$="CAMERA, AUTOMATIC": B$="93604-l" 

20 PRINT#1, A$; B$ 

30 INPUT#1, A$, B$ 

To separate these strings properly, write successive double quotation marks using CHR$(34). For example: 

40 PRINT#1,CHR$(34); A$; CHR$(34); CHR$(34); B$; CHR$(34) "CAMERA,AUTOMATIC""93604-1" 
The PRINT# statement may also be used with the USING option to control the format of the disk file. For example: 

PRINT#1, USING"$$###.J; K; L 

PRINT# does not compress data on the diskette. An image of the data is written to the diskette, just as it would be 
displayed on the terminal screen with a PRINT statement. For this reason, be sure to delimit the data on the diskette 
so that it is input correctly from the diskette. 

In list of expressions, numeric expressions must be delimited by semicolons. For example: 

PRINT#1, A; B; C; X; Y; Z 


If commas are used as delimiters, the extra blanks inserted between print fields will also be written to the diskette. 
Commas have no effect, however, if used with the exponential format. 

String expressions must be separated by semicolons in the list. To format the string expressions correctly on the 
diskette, use explicit delimiters in list of expressions. For example, the following: 

10 A$="CAMERA": B$="93604-l" 

20 PRINT#1, A$, B$ 

gives a diskette image of: 

CAMERA93604-1 

Because there are no delimiters, this would not be input as two separate strings. To correct the problem, insert explicit 
delimiters into the PRINT# statement as follows: 

30 PRINT#1, A$; B$ 

This gives the following diskette image, which can be read back into two string variables: 

CAMERA, 93604-1 


PUT Statement (Files) 

Purpose: 

To write a record from a random buffer to a random disk file. 

Syntax: 

PUT [#]file number[,record number] 

Comments: 

file number is the number under which the file was opened. 

record number is the number of the record. If it is omitted, the record has the next available record 
number (after the last put). 

The largest possible record number is 2^^ -1. This will allow you to have large files with short record 
lengths. The smallest possible record number is 1. 

The PRINT#, PRINT# USING, LSET, RSET, or WRITE# statements may be used to put characters in 
the random file buffer before a put statement. 

In the case of write#, GW-BASIC pads the buffer with spaces up to an enter. 

Any attempt to read or write past the end of the buffer causes a "Field overflow" error. 

PUT can be used for communications files. Here record number is the number of bytes written to the 
file. Record number must be less than or equal to the length of the buffer set in the open " COM ( n) 
statement. 


PUT Statement (Graphics) 

Purpose: 

To transfer graphics images to the screen. 

Syntax: 

PUT (x,y),array, [faction verb] 

Comments: 

action verb may be PSET, PRESET, AND, OR, XOR. 

The (x, y) are the coordinates of the top-left comer of the image to be transferred. 

The PUT and get statements transfer graphics images to and from the screen, put and GET make 
possible animation and high-speed object motion in either graphics mode. 

The PUT statement transfers the image stored in the array onto the screen. The specified point is the 
coordinate of the upper-left comer of the image. An "illegal function call" error results if the 
image to be transferred is too large to fit onto the screen. 

The action verb is used to interact the transferred image with the image already on the screen. PSET 
transfers the data onto the screen verbatim. 

PRESET is the same as PSET except that an inverse image (black on white) is produced. 

AND transfers the image only if an image already exists under the transferred image. 

OR superimposes the image onto the existing image. 

XOR is a special mode often used for animation. XOR causes the points on the screen to be inverted 
where a point exists in the array image. This behavior is exactly like the cursor on the screen. XOR is 
especially useful for animation. When an image is put against a complex background twice, the 
background is restored unchanged. An object can be moved around the screen without obliterating the 
background. The default action mode is xOR. 

For more information about effects within the different modes, see the COLOR, PALETTE, and 
SCREEN statements. 

Animation of an object is usually performed as follows: 

1. Put the object(s) on the screen. 

2. Recalculate the new position of the object(s). 

3. Put the object(s) on the screen a second time at the old location(s) to remove the old image(s). 

4. Return to Step 1, this time putting the object(s) at the new location. 

Movement done this way leaves the background unchanged. Elicker can be cut down by minimizing 
the time between Steps 4 and 1, and by making sure that there is enough time delay between Steps 1 
and 3. If more than one object is being animated, process every object at once, one step at a time. 

If it is not important to preserve the background, animation can be performed using the PSET action 
verb. 





Leave a border around the image (when it is first gotten) as large or larger than the maximum distanee 
the objeet will move. Thus, when an object is moved, this border effectively erases any points. This 
method may be somewhat faster than the method using XOR described above since only one PUT is 
required to move an object. However, the image to be PUT must be larger than the existing image. 


Examples: 

10 CLS: SCREEN 1 
20 PSET (130, 120) 

30 DRAW "U25; E7; R20; D32; L6; U12; L14" 

40 DRAW "D12; L6": PSET(137, 102) 

50 DRAW "U4; E4; R8; D8; L12" 

60 PSET (137, 88) 

70 DRAW "E4; R20; D32; G4": PAINT (131,119) 
80 DIM A (500) 

90 GET (125, 130)-(170, 80), A 
100 FOR 1= 1 TO 1000: NEXT I 
110 PUT (20, 20), A, PSET 
120 FOR 1= 1 TO 1000: NEXT i 
130 GET (125, 130)-(170, 80), A 
140 FOR 1= 1 TO 1000: NEXT I 
150 PUT (220, 130), A, PRESET 


RANDOMIZE Statement 


Purpose: 

To reseed the random number generator. 

Syntax: 

RANDOMIZE [expression] 

RANDOMIZE TIMER 

Comments: 

If expression is omitted, GW-BASIC suspends program execution and asks for a value by displaying 
the following line; 

Random number seed (-32768 to 32767)? 

If the random number generator is not reseeded, the RND function returns the same sequence of 
random numbers each time the program is run. 

To change the sequence of random numbers every time the program is run, place a randomi ze 
statement at the beginning of the program, and change the argument with each run (see RND 
function). 

RANDOMIZE with no arguments will prompt you for a new seed, randomi ze [ express!on ] will 
not force floating-point values to integer, expression may be any numeric formula. 

To get a new random seed without prompting, use the new numeric timer function as follows: 

RANDOMIZE TIMER 

Example 1: 

The internal clock can be set at intervals. 

10 RANDOMIZE TIMER 
20 FOR I=I to 5 
30 PRINT RND; 

40 NEXT I 
RUN 

.88598 .484668 .586328 .119426 .709225 
RUN 

.803506 .162462 .929364 .292443 .322921 

Example 2: 

The internal clock can be used for random number seed. 

5 N=VAL(MID$(TIME$, 7, 2)) 'get seconds for seed 
10 RANDOMIZE N 'install number 

20 PRINT N 'print seconds 

30 PRINT RND 'print random number generated 

RUN 

36 

.2466638 

RUN 

37 



.6530511 
RUN 
38 

5.943847E+02 
RUN 
40 

.8722131 


READ Statement 


Purpose: 

To read values from a data statement and assign them to variables. 

Syntax: 

READ list of variables 

Comments: 

A READ Statement must always be used with a data statement. 

READ statements assign variables to data statement values on a one-to-one basis. 

READ statement variables may be numeric or string, and the values read must agree with the variable 
types specified. If they do not agree, a "Syntax error" results. 

A single read statement may access one or more data statements. They are accessed in order. 
Several read statements may access the same data statement. 

If the number of variables in list of variables exceeds the number of elements in the data 
statement(s), an "Out of data" message is printed. 

If the number of variables specified is fewer than the number of elements in the data statement(s), 
subsequent read statements begin reading data at the first unread element. If there are no subsequent 
READ statements, the extra data is ignored. 

To reread data statements from the start, use the restore statement. 

Examples: 


80 FOR 1=1 TO 10 
90 READ A(I) 

100 NEXT I 

110 DATA 3.08, 5.19, 3.12, 3.98, 4.24 
120 DATA 5.08, 5.55, 4.00, 3.16, 3.37 


This program segment reads the values from the data statements into array A. After execution, the 
value of A(l) is 3.08, and so on. The data statement (lines 110-120) may be placed anywhere in the 
program; they may even be placed ahead of the read statement. 

5 PRINT 

10 PRINT "CITY", "STATE", "ZIP" 

20 READ C$, S$, Z 

30 DATA "DENVER,", "COLORADO", 80211 

40 PRINT C$, S$, Z 

RUN 

CITY STATE ZIP 
DENVER, COLORADO 80211 


This program reads string and numeric data from the data statement in line 30. 


REM Statement 


Purpose: 

To allow explanatory remarks to be inserted in a program. 

Syntax: 

REM[ comment] 

’ [ comment] 

Comments: 

REM statements are not executed, but are output exactly as entered when the program is listed. 

Once a REM or its abbreviation, an apostrophe ( ' )&gt; is encountered, the program ignores everything else 
until the next line number or program end is encountered. 

REM statements may be branched into from a GOTO or GOSUB statement, and execution continues with 
the first executable statement after the rem statement. However, the program runs faster if the branch 
is made to the first statement. 

Remarks may be added to the end of a line by preceding the remark with an apostrophe ( ' ) instead of 
REM. 


Note 

Do not use rem in a data statement because it will be considered to be legal data. 


Examples: 


120 REM CALCULATE AVERAGE VELOCITY 
130 FOR 1=1 TO 20 
440 SUM=SUM+V(I) 

450 NEXT I 


129 FOR 1=1 TO 20 'CALCULATED AVERAGE VELOCITY 

130 SUM=SUM+V(I) 

140 NEXT I 




RENUM Command 


Purpose: 

To renumber program lines. 

Syntax: 

RENUM[r!eu^ number], [old number] [, incrementR] ] 

Comments: 

new number is the first line number to be used in the new sequence. The default is 10. 

ol d number is the line in the current program where renumbering is to begin. The default is the first 
line of the program. 

increment is the increment to be used in the new sequence. The default is 10. 

RENUM also changes all line number references following ELSE, GOTO, GOSUB, then, on . . . goto, 

ON. . . GOSUB, RESTORE, RESUME, and ERL statements to reflect the new line numbers. If a 
nonexistent line number appears after one of these statements, the error message, "Undefined line 
X in y" appears. The incorrect line number reference x is not changed by RENUM, but line number y 
may be changed. 

RENUM cannot be used to change the order of program lines (for example, RENUM 15,30 when the 
program has three lines numbered 10, 20 and 30) or to create line numbers greater than 65529. An 

"illegal function call" error results. 

Examples: 

RENUM 

Renumbers the entire program. The first new line number will be 10. Lines increment by 10. 

RENUM 300,,50 

Renumbers the entire program. The first new line number will be 300. Lines increment by 50. 

RENUM 1000,900,20 

Renumbers the lines from 900 up so they start with line number 1000 and are incremented by 20. 


RESET Command 


Purpose: 

To close all disk files and write the directory information to a diskette before it is removed from a disk 
drive. 

Syntax: 

RESET 

Comments: 

Always execute a reset command before removing a diskette from a disk drive. Otherwise, when 
the diskette is used again, it will not have the current directory information written on the directory 
track. 

RESET closes all open files on all drives and writes the directory track to every diskette with open 
files. 


RESTORE Statement 


Purpose: 

To allow DATA statements to be reread from a speeified line. 

Syntax: 

RESTORE [line number] 

Comments: 

If line number is specified, the next read statement accesses the first item in the specified data 
statement. 

If line number is omitted, the next READ statement accesses the first item in the first data 
statement. 

Examples: 


10 

READ 

A, 

B, C, 

20 

RESTORE 


30 

READ 

D, 

E, F 

40 

DATA 

57, 

68, 79 


Assigns the value 57 to both A and D variables, 68 to B and E, and so on. 


RESUME Statement 


Purpose: 

To continue program execution after an error-recovery procedure has been performed. 

Syntax: 

RESUME 
RESUME 0 
RESUME NEXT 
RESUME line number 

Comments: 

Any one of the four formats shown above may be used, depending upon where execution is to 


resume: 


Syntax 

RESUME or RESUME 0 
RESUME NEXT 


Result 

Execution resumes at the statement that caused an error. 

Execution resumes at the statement immediately following the one 
that caused an error. 


RESUME line number Execution resumes at the specified line number. 

A RESUME statement that is not in an error trapping routine causes a "resume without error" 
message to be printed. 

Examples: 

10 ON ERROR GOTO 900 


900 IF (ERR=230) AND (ERL=90) THEN PRINT "TRY AGAIN": RESUME 80 


If an error occurs after line 10 is executed, the action indicated in line 900 is taken and the program 
continues at line 80. 


RETURN Statement 


Purpose: 

To return from a subroutine. 

Syntax: 

RETURN [line number] 

Comments: 

The RETURN statement causes GW-BASIC to branch back to the statement following the most recent 
GOSUB statement. A subroutine may contain more than one return statement to return from different 
points in the subroutine. Subroutines may appear anywhere in the program. 

RETURN line number is primarily intended for use with event trapping. It sends the event-trapping 
routine back into the GW-BASIC program at a fixed line number while still eliminating the GOSUB 
entry that the trap created. 

When a trap is made for a particular event, the trap automatically causes a STOP on that event so that 
recursive traps can never take place. The return from the trap routine automatically does an ON 
unless an explicit OFF has been performed inside the trap routine. 

The non-local return must be used with care. Any GOSUB, while, or for statement active at the 
time of the trap remains active. 


RIGHTS Function 


Purpose: 

To return the rightmost n eharaeters of string x$. 

Syntax: 

RIGHT$ (x$, n) 

Comments: 

If n is equal to or greater than len (x$) , right$ returns x$. If n equals zero, the null string (length 
zero) is returned (see the MIPS and LEFTS functions). 

Examples: 

10 A$="DISK BASIC" 

20 PRINT RIGHT$(A$, 5) 

RUN 

BASIC 

Prints the rightmost five characters in the A$ string. 




RMDIR Command 


Purpose: 

To delete a subdireetory. 

Syntax: 

RMDIR pathname 
Comments: 

pathname is a string expression, not exceeding 63 characters, identifying the subdirectory to be 
removed from its parent. 

The subdirectory to be deleted must be empty of all files except " . " and " . . " or a "Path 
file/access error" is given. 

Examples: 

Referring to the sample directory structure illustrated in CHDIR, the following command deletes the 
subdirectory report: 

RMDIR "SALES\JOHN\REPORT" 


RND Function 


Purpose: 

To return a random number between 0 and 1. 

Syntax: 

RND[ (x) ] 

Comments: 

The same sequence of random numbers is generated each time the program is run unless the random 
number generator is reseeded (see RANDOMIZE statement). If x is equal to zero, then the last 
number is repeated. 

If X is greater than 0, or if x is omitted, the next random number in the sequence is generated. 

To get a random number within the range of zero through n, use the following formula: 

INT(RND* (n+1)) 

The random number generator may be seeded by using a negative value for x. 

Examples: 

10 FOR 1=1 TO 5 
20 PRINT INT(RND*101); 

30 NEXT 
RUN 

53 30 31 51 5 

Generates five pseudo-random numbers within the range of 0-100. 



RUN Command 


Purpose: 

To execute the program currently in memory, or to load a file from the diskette into memory and run 
it. 

Syntax: 

RUN [line number] [,r] 

RUN filename[fr] 

Comments: 

RUN or RUN line number runs the program currently in memory. 

If line number is specified, execution begins on that line. Otherwise, execution begins at the lower 
line number. 

If there is no program in memory when run is executed, GW-BASIC returns to command level. 

RUN filename closes all open files and deletes the current memory contents before loading the 
specified file from disk into memory and executing it. 

The r option keeps all data files open. 

If you are using the speaker on the computer, please note that executing the run command will turn 
off any sound that is currently running and will reset to Music Foreground. Also, the pen and STRIG 
Statements are reset to off. 

Examples: 

RUN "NEWFIL", R 

Runs NEWFIL.BAS without closing data files. 


SAVE Command 


Purpose: 

To save a program file on diskette. 

Syntax: 

SAVE filename, [,a] 

SAVE filename, [,p] 

Comments: 

filename is a quoted string that follows the normal MS-DOS naming eonventions. If filename 
already exists, the file will be written over. If the extension is omitted, .bas will be used. 

The a option saves the file in ASCII format. Otherwise, GW-BASIC saves the file in a compressed 
binary format. ASCII format takes more space on the diskette, but some diskette access commands 
(for example, the merge command and some MS-DOS commands, such as TYPE) may require an 
ASCII format file. 

The p option protects the file by saving it in an encoded binary format. When a protected file is later 
run or loaded, any attempt to list or edit it fails. When the p option is used, make an additional copy 
under another name or diskette to facilitate future program maintenance. 

Examples: 

The following command saves the file com2.bas in the ASCII format: 

SAVE "COM2", A 

The following command saves the file prog.bas in binary format, and protects access: 


SAVE "PROG", P 


SCREEN Function 


Purpose: 

To return the ASCII eode (0-255) for the eharaeter at the speeified row (line) and eolumn on the 
sereen. 

Syntax: 

x=SCREEN( row, col[, z]) 

Comments: 

X is a numerie variable receiving the ASCII code returned. 
row is a valid numeric expression within the range 1 to 25. 

col is a valid numeric expression 1 to 40, or 1 to 80, depending upon screen width setting. See the 
WIDTH statement. 

z is a valid numeric expression with a true or false value. It may be used only in alpha mode. 

The ordinal of the character at the specified coordinates is stored in the numeric variable. In alpha 
mode, if the optional parameter z is given and is true (nonzero), the color attribute for the character 
returned instead of the ASCII code for the character (see the COLOR statement). 

Any values entered outside of the range indicated result in an "illegal function call" error. 
Row 25 may be referenced only if the function key is off. 

Examples: 

100 X=SCREEN (10, 10) 

If the character at 10,10 is A, then X is 65. 

110 X=SCREEN (1, 1, 1) 

Returns the color attribute of the character in the upper-left comer of the screen. 




