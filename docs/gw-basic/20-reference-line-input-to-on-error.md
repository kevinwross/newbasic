<!-- Chunked from GWBASIC.txt, lines 12229-13420. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: LINE INPUT - ON ERROR GOTO

LINE INPUT Statement 


Purpose: 

To input an entire line (up to 255 characters) from the keyboard into a string variable, ignoring 
delimiters. 

Syntax: 

LINE INPUT [;][prompt string;]string variable 

Comments: 

prompt string is a string literal, displayed on the screen, that allows user input during program 
execution. 

A question mark is not printed unless it is part of prompt string. 

string variable accepts all input from the end of the prompt to the carriage return. Trailing 
blanks are ignored. 

LINE INPUT is almost the same as the input statement, except that it accepts special characters 
(such as commas) in operator input during program execution. 

If a line-feed/carriage return sequence (this order only) is encountered, both characters are input and 
echoed. Data input continues. 

If LINE INPUT is immediately followed by a semicolon, pressing the return key will not move the 
cursor to the next line. 

A LINE INPUT may be escaped by typing ctrl-break. GW-BASIC returns to command level and 
displays Ok. 

Typing CONT resumes execution at the line input line. 

Example: 

100 LINE INPUT A$ 

Program execution pauses at line 100, and all keyboard characters typed thereafter are input to string 
A$ until RETURN, CTRL-M, CTRL-C, or CTRL-BREAK is entered. 


LINE INPUT# Statement 


Purpose: 

To read an entire line (up to 255 eharaeters), without delimiters, from a sequential disk file to a string 
variable. 

Syntax: 

LINE INPUT# file number, string variable 

Comments: 

file number is the number under which the file was opened. 
string variable is the variable name to which the line will be assigned. 

LINE INPUT# reads all characters in the sequential file up to a carriage return. If a line feed/carriage 
return sequence (this order only) is encountered, it is input. 

LINE INPUT# is especially useful if each line of a data file has been broken into fields, or if a BASIC 
program saved in ASCII mode is being read as data by another program. 

Examples: 

10 OPEN "0", I, "INFO" 

20 LINE INPUT "CUSTOMER INFORMATION?"; C$ 

30 PRINT#I, C$ 

40 CLOSE I 

50 OPEN "I", 1, "INFO" 

60 LINE INPUT#I, C$ 

70 PRINT C$ 

80 CLOSE I 
RUN 

CUSTOMER INFORMATION? 

If the operator enters 

LINDA JONES 234, 4 MEMPHIS 
then the program continues with the following; 

LINDA JONES 234, 4 MEMPHIS 


LIST Command 


Purpose: 

To list all or part of a program to the screen, line printer, or file. 

Syntax: 

LIST [line number] [-line number] [, filename] 

LIST [line number-] [, filename] 

Comments: 

linenumber is a valid line number within the range of 0 to 65529. 

If filename is omitted, the specified lines are listed to the screen. 

Use the hyphen to specify a line range. If the line range is omitted, the entire program is listed. 
linenumber- lists that line and all higher numbered lines, -linenumber lists lines from the 
beginning of the program through the specified line. 

The period ( . ) can replace either linenumber to indicate the current line. 

Any listing may be interrupted by pressing ctrl-break. 

Examples: 

LIST 

Lists all lines in the program. 

LIST -20 

Lists lines 1 through 20. 

LIST 10-20 
Lists lines 10 through 20. 

LIST 20- 

Lists lines 20 through the end of the program. 


LLIST Command 


Purpose: 

To list all or part of the program currently in memory to the line printer. 

Syntax: 

LLIST [line number] [-line number] 

LLIST [line number-] 

Comments: 

GW-BASIC always returns to command level after a LLIST is executed. The line range options for 
LLIST are the same as for LIST. 

Examples: 

See the examples in the LIST statement. 



LOAD Command 


Purpose: 

To load a file from diskette into memory. 

Syntax: 

LOAD filenameif r] 

Comments: 

filename is the filename used when the file was saved. If the extension was omitted, .BAS will be 
used. 

LOAD closes all open files and deletes all variables and program lines currently residing in memory 
before it loads the designated program. 

If the r option is used with LOAD, the program runs after it is loaded, and all open data files are kept 
open. 

LOAD with the r option lets you chain several programs (or segments of the same program). 
Information can be passed between the programs using the disk data files. 

Examples: 

LOAD "STRTRK",R 

Loads the file strtrk.bas and runs it, retaining all open files and variables from a previous program 
intact. 


LOC Function 


Purpose: 

To return the eurrent position in the file. 

Syntax: 

LOC (Tile number) 

Comments: 

file number is the file number used when the file was opened. 

When transmitting or receiving a file through a communication port, LOC returns the number of 
characters in the input buffer waiting to be read. The default size for the input buffer is 256 characters, 
but can be changed with the /c: option on the GW-BASIC command line. If there are more than 255 
characters in the buffer, LOC returns 255. Since a string is limited to 255 characters, this practical limit 
alleviates the need to test for string size before reading data into it. If fewer than 255 characters 
remain in the buffer, then LOC returns the actual count. 

With random disk files, LOC returns the record number just read from, or written to, with a GET or 
PUT statement. 

With sequential files, LOC returns the number of 128-byte blocks read from, or written to, the file 
since it was opened. When the sequential file is opened for input, GW-BASIC initially reads the first 
sector of the file. In this case, the LOC function returns the character 1 before any input is allowed. 

If the fide was opened but no disk input/output was performed, LOC returns a zero. 

Examples: 

200 IF LOC(1)&gt;50 THEN STOP 

The program stops after 51 records are read or written. 


LOCATE Statement 


Purpose: 

To move the eursor to the speeified position on the active screen. Optional parameters cause the 
cursor to blink on and off, and define the start and stop raster lines for the cursor. A raster line is the 
vertical or horizontal distance between two adjacent, addressable points on your screen. 

Syntax: 

LOCATE [ row] [ , [ col ] [, [ cursor] [ , [ start] [ , stop] ]]] 

Comments: 

row is the screen line number, a numeric expression within the range of 1 to 25. 

col is the screen column number, a numeric expression within the range of 1 to 40, or 1 to 80, 
depending upon screen width. 

cursor is a Boolean value indicating whether the cursor is visible; zero is off, nonzero is on. 

start is the cursor start scan line, a numeric expression within the range of 0 to 31. 

stop is the cursor stop scan line, a numeric expression within the range of 0 to 31. 

When the cursor is moved to the specified position, subsequent print statements begin placing 
characters at this location. Optionally, the LOCATE statement may be used to start the cursor blinking 
on or off, or change the size of the blinking cursor. 

Any values entered outside of these ranges results in "illegal function call" errors. Previous 
values are retained. 

As you set up the parameters for the LOCATE statement, you may find that you do not wish to change 
one or more of the existing specifications. To omit a parameter from this LOCATE statement, insert a 
comma for the parameter that is being skipped. If the omitted parameter(s) occurs at the end of the 
statement, you do not have to type the comma. 

If the start scan line parameter is given and the stop scan line parameter is omitted, stop assumes the 
start value. 

Examples: 

10 LOCATE 1,1 

Moves cursor to the home position in the upper-left comer. 

20 LOCATE ,,1 

Makes the cursor visible. Its position remains unchanged. Notice that the first two parameters are not 
used. A comma has been inserted for each omitted parameter. 

30 LOCATE ,,,7 

Cursor position and visibility remain unchanged. Sets the cursor to appear at the bottom of the 
character starting and ending on scan line 7. 

40 LOCATE 5,1,1,0,7 

Moves the cursor to line 5, column 1, and turns the cursor on. The cursor covers an entire character 
cell, starting at scan line 0 and ending on scan line 7. 


LOCK Statement 


Purpose: 

To restrict the access to all or part of a file that has been opened by another process. This is used in a 
multi-device environment, often referred to as a network or network environment. 

Syntax: 

LOCK [#]r! [, [record number] [TO record number]] 

Comments: 

n is the number that was assigned to the file as it was originally numbered in the program. 

record number is the number of the individual record that is to be locked. Or, if a range of records 
are to be locked, record number designates the beginning and ending record of the specified range. 

The range of legal record numbers is 1 to 2^^ -1. The limit on record size is 32767 bytes. 

The record range specified must be from lower to (the same or) higher record numbers. 

If a starting record number is not specified, the record number 1 is assumed. 

If an ending record number is not specified, then only the specified record is locked. 

The following are examples of legal LOCK statements: 

LOCK #11 locks the entire file n 

LOCK #n, X locks record X only 
LOCK #n TO Y locks records 1 through K 
LOCK #n, X TO Klocks records X through K 

With a random-access file, the entire opened file, or a range of records within an opened file, may be 
locked, thus denying access to those records to any other process which has also opened the file. 

With a sequential access file that has been opened for input or output, the entire file is locked, 
regardless of any record range specified. This is not considered an error. The specification of a range 
in the LOCK statement regarding the sequential file will simply be disregarded. 

The LOCK statement should be executed on a file or record range within a file before attempting to 
read or write to that file. 

The locked file or record range should be unlocked before the file is closed. Failure to execute the 
UNLOCK statement can jeopardize future access to that file in a network environment. 

It is expected that the time in which files or regions within files are locked will be short, and thus the 
suggested usage of the lock statement is within short-term paired lock/unlock statements. 

Examples: 

The following sequence demonstrates how the lock/unlock statements should be used: 

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


LOF Function 


Purpose: 

To return the length (number of bytes) allocated to the file. 

Syntax: 

LOF (file number) 

Comments: 

file n umber is the number of the file that the file was opened under. 

With communications files, LOF returns the amount of free space in the input buffers. 

Examples: 

The following sequence gets the last record of the random-access file file.big, and assumes that 
file was created with a default record length of 128 bytes: 

10 OPEN "R",1,"FILE.BIG" 

20 GET #1,LOF(1)/128 


LOG Function 


Purpose: 

To return the natural logarithm of x. 

Syntax: 

LOG(x) 

Comments: 

X must be a number greater than zero. 

LOG (x) is caleulated in single preeision, unless the /d switeh is used when GW-BASIC is exeeuted. 

Examples: 

PRINT LOG(2) 

.6931471 

PRINT LOG(l) 

0 


LPOS Function 


Purpose: 

To return the eurrent position of the line printer print head within the line printer buffer. 

Syntax: 

LPOS(x) 

Comments: 

LPOS does not necessarily give the physical position of the print head. 

X is a dummy argument. 

If the printer has less than the 132 characters-per-line capability, it may issue internal line feeds and 
not inform the computer internal line printer buffer. If this has happened, the value returned by 
LPOS (x) may be incorrect. LPOS (x) simply counts the number of printable characters since the last 
line feed was issued. 

Examples: 

The following line causes a carriage return after the 60th character is printed on a line; 

100 IF LPOS(X)&gt;60 THEN LPRINT CHR${13) 


LPRINT and LPRINT USING Statements 


Purpose: 

To print data at the line printer. 

Syntax: 

LPRINT [list of expressions] [; ] 

LPRINT USING string exp; list of expressions[;] 

Comments: 

list of expressions eonsists of the string or numeric expression separated by semicolons. 

string expressions is a string literal or variable consisting of special formatting characters. The 
formatting characters determine the field and the format of printed strings or numbers. 

These statements are the same as print and print using, except that output goes to the line 
printer. For more information about string and numeric fields and the variables used in them, see the 
PRINT and PRINT USING statements. 

The LPRINT and LPRINT USING statements assume that your printer is an 80-character-wide printer. 

To reset the number of characters that you can print across the printed page (assuming that your 
printer is wider than 80 characters), see the WIDTH statement. 





LSET and RSET Statements 


Purpose: 

To move data from memory to a random-file buffer and left- or right-justify it in preparation for a put 
statement. 

Syntax: 

LSET string variable=string expression 
RSET string variable=string expression 

Comments: 

If string expression requires fewer bytes than were fielded to string variable, LSET left- 
justifies the string in the field, and RSET right-justifies the string (spaees are used to pad the extra 
positions). 

If the string is too long for the field, eharaeters are dropped from the right. 

To eonvert numerie values to strings before the LSET or RSET statement is used, see the MKI$, 

MKS$, and MKD$ funetions. 

LSET or RSET may also be used with a nonfielded string variable to left-justify or right-justify a string 
in a given field. 

Examples: 

110 A$=SPACE$(20) 

120 RSET A$=N$ 

These two statements right-justify the string N$ in a 20-eharacter field. This ean be valuable for 
formatting printed output. 





MERGE Command 


Purpose: 

To merge the lines from an ASCII program file into the program already in memory. 

Syntax: 

MERGE filename 
Comments: 

filename is a valid string expression containing the filename. If no extension is specified, then GW- 
BASIC assumes an extension of .BAS. 

The diskette is searched for the named file. If found, the program lines on the diskette are merged 
with the lines in memory. After the merge command, the merged program resides in memory, and 
GW-BASIC returns to the direct mode. 

If the program being merged was not saved in ASCII code with the a option to the SAVE command, a 
"Bad file mode" error is issued. The program in memory remains unchanged. 

If any line numbers in the file have the same number as lines in the program in memory, the lines 
from the file replace the corresponding lines in memory. 

Examples: 

MERGE "SUBRTN" 

Merges the file subrtn.bas with the program currently in memory, provided subrtn was previously 
saved with the a option. If some of the program lines are the same as those in the subrtn.bas file being 
merged, then the original program lines are replaced by the lines from subrtn.bas. 


MID$ Function 


Purpose: 

To return a string of m characters from x$ beginning with the nth character. 

Syntax: 

MID$ [,in] ) 

Comments: 

n must be within the range of 1 to 255. 
m must be within the range of 0 to 255. 

If m is omitted, or if there are fewer than m characters to the right of n, all rightmost characters 
beginning with n are returned. 

If n &gt; LEN (x.f ) , MID$ function returns a null string. 

If m equals 0, the mid$ function returns a null string. 

If either u or m is out of range, an "illegal function call error" is returned. 

For more information and examples, see the LEFTS and RIGHTS functions. 

Examples: 

10 A$="GOOD" 

20 B$="MORNING EVENING AFTERNOON" 

30 PRINT A$; MID$(B$, 8, 8) 

RUN 

GOOD EVENING 

Line 30 concatenates (joins) the AS string to another string with a length of eight characters, 
beginning at position 8 within the BS string. 




MID$ Statement 


Purpose: 

To replace a portion of one string with another string. 

Syntax: 

MID$ {stringexpl,n[,m])=stringexp2 

Comments: 

Both n and m are integer expressions. 

stringexpl and stringexp2 are string expressions. 

The characters in stringexpl, beginning at position n, are replaced by the characters in 

stringexp2. 

The optional m refers to the number of characters from stringexp2 that are used in the replacement. 
If m is omitted, all of stringexp2 is used. 

Whether m is omitted or included, the replacement of characters never goes beyond the original length 

of s tringexpl. 

Examples: 

10 A$="KANSAS CITY, MO" 

20 MID$(A$, 14)="KS" 

30 PRINT A$ 

RUN 

KANSAS CITY, KS 

Line 20 overwrites "MO" in the A$ string with "KS". 


MKDIR Command 


Purpose: 

To create a subdirectory. 

Syntax: 

MKDIR pathname 
Comments: 

pathname is a string expression, not exceeding 63 charaeters, identifying the subdirectory to be 
created. 

Examples: 

MKDIR "C:SALES\JOHN" 

Creates the subdireetory within the directory of SALES. 


MKI$, MKS$, MKD$ Functions 

Purpose: 

To convert numeric values to string values. 

Syntax: 

MKI$ (integer expression) 

ltKS$ {single-precision expression) 

ltKD$ {double-precision expression) 

Comments: 

MKI$ converts an integer to a 2-byte string. 

MKS$ converts a single-precision number to a 4-byte string. 

MKD$ converts a double-precision number to an 8-byte string. 

Any numeric value placed in a random file buffer with a lset or a rset statement must be converted 
to a string (see CVT CVS, CVD , for the complementary functions). 

These functions differ from STR$ because they change the interpretations of the bytes, not the bytes 
themselves. 

Examples: 

90 AMT=(K+T) 

100 FIELD #1, 8 AS D$, 20 AS N$ 

110 LSET D$=MKS$(AMT) 

120 LSET N$=A$ 

130 PUT #1 





NAME Command 


Purpose: 

To change the name of a disk file. 

Syntax: 

NAME old filename AS new filename 

Comments: 

old filename must exist and new filename must not exist; otherwise, an error results. 

After a name command, the file exists on the same diskette, in the same disk location, with the new 
name. 

Examples: 

NAME "ACCTS" AS "LEDGER" 

The file formerly named ^4Cells' will now be named LEDGER. The file content and physical location 
on the diskette is unchanged. 


NEW Command 


Purpose: 

To delete the program eurrently in memory and elear all variables. 

Syntax: 

NEW 

Comments: 

NEW is entered at command level to clear memory before entering a new program. GW-BASIC always 
returns to command level after a new is executed. 

Examples: 

NEW 


or 


980 PRINT "Do You Wish To Quit (Y/N) 
990 ANS$=INKEY$: IF ANS$=""THEN 990 
1000 IF ANS$="Y" THEN NEW 
1010 IF ANS$="N" THEN 980 
1020 GOTO 990 


OCT$ Function 


Purpose: 

To convert a decimal value to an octal value. 

Syntax: 

OCT$(x) 

Comments: 

X is rounded to an integer before OCT$ (x) is evaluated. 

This statement converts a decimal value within the range of -32768 to +65535 to an octal string 
expression. 

Octal numbers are numbers to the base 8 rather than base 10 (deeimal numbers). 

See the HEX$ function for hexadecimal conversion. 

Examples: 

10 PRINT OCT$ (18) 

RUN 

22 

Deeimal 18 equals octal 22. 



ON COM(n), ON KEY(n), ON PEN, ON PLAY(n), ON STRIG(n), and 
ON TIMER(n) Statements 

Purpose: 

To create an event trap line number for a specified event (such as communications, pressing function 
or cursor control keys, using the light pen, or using joysticks). 

Syntax: 

ON event specifier GOSUB line number 


Comments: 


The syntax shown sets up an event trap line number for the specified event. A line number of 0 
disables trapping for this event. 

Once trap line numbers have been set, event trapping itself can be controlled with the following 

When an event is ON, and a nonzero line number is specified for the trap, then 
ON every time BASIC starts a new statement, it checks to see if the specified event 
has occurred. If it has, BASIC performs a GOSUB to the line specified in the ON 
statement. 

When an event is OFF, no trapping occurs and the event is not remembered, even 
if it occurs. 

When an event is stopped, no trapping can occur, but if the event happens, it is 
remembered so an immediate trap occurs when an event specifier ON is executed. 

When a trap is made for a particular event, the trap automatically causes a stop on that event, so 
recursive traps can never take place. 

The return from the trap routine automatically does an ON unless an explicit OFF has been performed 
inside the trap routine. 

When an error trap takes place, this automatically disables all trapping. 

Trapping will never take place when BASIC is not executing a program. 

The following are valid values for event specifier: 


syntax lines: 

event 

specifier 

event 

specifier 

OFF 

event 

specifier 

STOP 


COM(n) 
KEY(n) 


PEN 

PLAY(n) 
STRIG(n) 


n is the number of the COM channel (1 or 2). 

n is a function key number 1-20. 1 through 10 are the function keys Fi 
through Fio. 11 through 14 are the cursor control keys as follows: 

11 = CURSOR UP 13= CURSOR RIGHT 
12= CURSOR LEFT 14= CURSOR DOWN 

15-20 are user-defined keys. 

Since there is only one pen, no number is given. 

n is an integer expression in the range of 1-32. Values outside this range 
result in "illegal function call" errors. 
n is 0, 2, 4 or 6. (0=trigger Al; 4=trigger A2; 2=trigger Bl; 6=trigger 
B2). 


n is a numeric expression within the range of 1 to 86,400. A value 
outside of this range results in an "illegal function call" error. 
This optional form of return is primarily intended for use with event 
trapping. The event-trapping routine may want to go back into the 
GW-BASIC program at a fixed line number while still eliminating the 
GO SUB entry that the trap created. 

Use of the nonlocal return must be done with care. Any other 
GOSUB, WHILE, or FOR that was active at the time of the trap remains 
active. 

If the trap comes out of a subroutine, any attempt to continue loops 
outside the subroutine results in a "next without for" error. 

Special Notes about Each Type of Trap 
COM Trapping 

Typically, the COM trap routine will read an entire message from the COM port before returning. 

It is recommended that you not use the COM trap for single character messages, since at high baud 
rates the overhead of trapping and reading for each individual character may allow the interrupt buffer 
for COM to overflow. 

KEY Trapping 

Trappable keys 15 to 20 are defined by the following statement: 

KEY(r!) ,CHR$ [hex code]+CHR$ [ scan code] 
n is an integer expression within the range of 15 to 20 defining the key to be trapped, 
hex code is the mask for the latched key: (caps lock, num lock, alt, Ctrl, left shift, right shift) 

scan code is the number identifying one of the 83 keys to trap. Refer to Appendix H in the GW- 
BASIC User's Guide for key scan codes. 

The appropriate bit in hex code must be set in order to trap a key that is shifted, control-shifted, or 
alt-shifted, hex code values are as follows: 


Mask 

Hex code 

Indicates that 

EXTENDED 

&amp;H80 

Key is extended 

CAPS LOCK 

&amp;H40 

CAPS LOCK is active 

NUM LOCK 

&amp;H20 

NUM LOCK is active 

ALT 

&amp;H08 

The ALT key is pressed 

CTRL 

&amp;H04 

The CTRL key is pressed 

LEFT SHIFT 

&amp;H02 

The left shift key is pressed 

RIGHT SHIFT 

&amp;H0I 

The right shift key is pressed 


For trapping shifted keys, you may use the value &amp;H01, &amp;H02, or &amp;H03. The left and right shift 
keys are coupled when &amp;H03 is used. 

Refer to the KEY(n) statement for more information. 

No type of trapping is activated when GW-BASIC is in direct mode. Function keys resume their 
standard expansion meaning during input. 


TIMER(n) 

RETURN line number 





A key that causes a trap is not available for examination with the input or inkey$ statement, so the 
trap routine for each key must be different if a different function is desired. 

If CTL-PRTSCR is trapped, the line printer echo toggle is processed first. Defining ctl-prtscr as a key 
trap does not prevent characters from being echoed to the printer if ctl-prtscr is pressed. 

Function keys 1 through 14 are predefined. Therefore, setting scan codes 59-68, 72, 75, 77, or 80 has 
no effect. 

PLAY Trapping 

A PLAY event trap is issued only when playing background music (play "MB. . ). play event music 
traps are not issued when running in MUSIC foreground (default case, or play"MF . . ). 

Choose conservative values for n. An ON play ( 32 ) . . statement will cause event traps so often that 
there will be little time to execute the rest of your program. 

The ON PLAY (n) statement causes an event trap when the background music queue goes from n to 
n-1 notes. 

STRIG Trapping 

Using STRIG (n) ON activates the interrupt routine that checks the trigger status. Down-strokes that 
cause trapping will not set STRIG (0) , STRIG (2) , STRIG (4) , or STRIG (6) functions. 

TIMER Trapping 

An ON TIMER (n) event trapping statement is used with applications needing an internal timer. The 
trap occurs when n seconds have elapsed since the timer on statement. 

Example 1: 

This is a very simple terminal program. 

10 REM "ON COM(n)" EXAMPLE 
20 OPEN "COM1:9600, 0, 7" AS #1 
30 ON COM(l) GOSUB 80 
40 COM(l) ON 

50 REM TRANSMIT CHARACTERS FROM KEYBOARD 

60 A$=INKEY$: IF A$=""THEN 50 

70 PRINT #1, A$;: GOTO 50 

80 REM DISPLAY RECEIVE CHARACTERS 

90 ALL=LOC(l): IF ALL&lt;1 THEN RETURN 

100 B$=INPUT$(ALL, #1): PRINT B$;:RETURN 

Example 2: 

Prevents a ctrl-break or system reset during a program. 

10 KEY 15, CHR$(4)+CHR$(70) REM Trap ^BREAK 

20 KEY 16, CHR$(12)+CHR$(83) REM Trap system reset 

30 ON KEY(15) GOSUB 1000 

40 ON KEY(16) GOSUB 2000 

50 KEY (15) ON 

60 KEY (16) ON 


1000 PRINT "I'm sorry, I can't let you do that" 
1010 RETURN 

2000 ATTEMPS=ATTEMPS+1 

2010 ON ATTEMPS GOTO 2100, 2200, 2300, 2400, 2500 
2100 PRINT "Mary had a little lamb": RETURN 
2200 PRINT "Its fleece was white as snow": RETURN 
2300 PRINT "And everywhere that Mary went": RETURN 
2400 PRINT "The lamb was sure to go": RETURN 
2500 KEY(16) OFF REM If they hit us once more... 
2510 RETURN REM then BASIC dies... 

Example 3: 

Displays the time of day on line 1 every minute. 

10 ON TIMER(60) GOSUB 10000 
20 TIMER ON 


10000 OLDROW=CSRLIN REM Saves the current row 
10010 OLDCOL=POS( 0 ) REM Saves the current column 
10020 LOCATE 1, 1: PRINT TIME$ 

10030 LOCATE OLDROW, OLDCOL REM Restores row and column 
10040 RETURN 


ON ERROR GOTO Statement 


Purpose: 

To enable error trapping and speeify the first line of the error-handling subroutine. 

Syntax: 

ON ERROR GOTO line number 
Comments: 

Once error trapping has been enabled, all errors detected by GW-BASIC, including direct mode 
errors, (for example, syntax errors) cause GW-BASIC to branch to the line in the program which 
begins the specified error-handling subroutine. 

GW-BASIC branches to the line specified by the ON error statement until a resume statement is 
found. 

If line nu/nUer does not exist, an "Undefined line" error results. 

To disable error trapping, execute the following statement; 

ON ERROR GOTO 0 

Subsequent errors print an error message and halt execution. 

An ON ERROR GOTO 0 statement in an error-trapping subroutine causes GW-BASIC to stop and 
print the error message for the error that caused the trap. It is recommended that all error-trapping 
subroutines execute an ON ERROR GOTO 0 if an error is encountered for which there is no recovery 
action. 

If an error occurs during execution of an error-handling subroutine, the GW-BASIC error message is 
printed and execution terminated. Error trapping does not occur within the error-handling subroutine. 

Examples: 

10 ON ERROR GOTO 1000 


1000 A=ERR: B=ERL 
1010 PRINT A, B 
1020 RESUME NEXT 

Line 1010 prints the type and location of the error on the screen (see the ERR and ERL variables). 
Line 1020 causes program execution to continue with the line following the error. 




ON ... GOSUB and ON ... GOTO Statements 


Purpose: 

To branch to one of several speeified line numbers, depending on the value returned when an 
expression is evaluated. 

Syntax: 

ON expression GOTO line numbers 
ON expression GOSUB line numbers 

Comments: 

In the ON ... GOTO statement, the value of expression determines which line number in the list 
will be used for branching. For example, if the value is 3, the third line number in the list will be 
destination of the braneh. If the value is a non-integer, the fraetional portion is rounded. 

In the ON ... GOSUB statement, each line number in the list must be the first line number of a 
subroutine. 

If the value of expression is zero or greater than the number of items in the list (but less than or 
equal to 255), GW-BASIC continues with the next executable statement. 

If the value of expression is negative, or greater than 255, an "illegal function call" error 
occurs. 

Examples: 

100 IF R&lt;1 or R&gt;4 then print "ERROR":END 
If the integer value of R is less than 1, or greater than 4, program exeeution ends. 

200 ON R GOTO 150,300,320,390 
If R=l, the program goes to line 150. 

If R=2, the program branches to line 300 and continues from there. If R=3, the branch will be to line 
320, and so on. 


