<!-- Chunked from GWBASIC.txt, lines 16013-17552. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: SCREEN (statement) - TAB

SCREEN Statement 


Purpose: 

To set the specifications for the display screen. 

Syntax: 

SCREEN [mode] [, [colorswitch] ] [, [apage]] [, [vpage]] 

Comments: 

The SCREEN statement is chiefly used to select a screen mode appropriate for a particular display- 
hardware configuration. Supported hardware configurations and screen modes are described below. 

MDPA with Monochrome Display: Mode 0 

The IBM Monochrome Display and Printer Adapter (MDPA) is used to connect only to a 
monochrome display. Programs written for this configuration must be text mode only. 

CGA with Color Display: Modes 0,1, and 2 

The IBM Color Graphics Adapter (CGA) and Color Display are typically paired with each other. This 
hardware configuration permits the running of text mode programs, and both medium-resolution and 
high-resolution graphics programs. 

EGA with Color Display: Modes 0,1, 2, 7, and 8 

The five screen modes 0, 1,2, 7, and 8 allow you to interface to the IBM Color Display when it is 
connected to an IBM Enhanced Graphics Adapter (EGA). If EGA switches are set for CGA 
compatibility, programs written for modes 1 and 2 will run just as they would with the CGA. Modes 7 
and 8 are similar to modes 1 and 2, except that a wider range of colors is available in modes 7 and 8. 

EGA with Enhanced Color Display: Modes 0,1, 2, 7, and 8 

With the EGA/IBM Enhanced Display configuration, modes 0, 1,2, 7, and 8 are virtually identical to 
their EGA/Color Display counterparts. Two possible differences are as follows: 

• In mode 0, the border color cannot be the same as for the EGA/Color Display because the 
border cannot be set on an Enhanced Color Display when it is in 640 x 350 text mode. 

• The quality of the text is better on the Enhanced Color Display (an 8^14 character box for 
Enhanced Color Display versus an 8 x 8 character box for color display). 

EGA with Enhanced Color Display: Mode 9 

The full capability of the Enhanced Color Display is taken advantage of in this mode. Mode 9 allows 
the highest resolution possible for the EGA/Enhanced Color Display configuration. Programs written 
for this mode will not work for any other hardware configuration. 

EGA with Monochrome Display: Mode 10 

The IBM Monochrome Display can be used to display monochrome graphics at a very high resolution 
in this mode. Programs written for this mode will not work for any other hardware configuration. 


Arguments 

The mode argument is an integer expression with legal values 0, 1,2, 7, 8, 9, and 10. All other values 
are illegal. Selection of a mode argument depends primarily on your program's anticipated display 
hardware, as described above. 

Each of the SCREEN modes is described individually in the following paragraphs. 

SCREEN 0 

• Text mode only 

• Either 40 x 25 or 80 x 25 text format with character-box size of 8 x 8 (8 x 14 with EGA) 

• Assignment of 16 colors to any of 2 attributes 

• Assignment of 16 colors to any of 16 attributes (with EGA) 

SCREEN 1 

• 320 X 200 pixel medium-resolution graphics 

• 80 X 25 text format with character-box size of 8 x 8 

• Assignment of 16 colors to any of 4 attributes 

• Supports both EGA and CGA 

• 2 bits per pixel 

SCREEN 2 

• 640 X 200 pixel high-resolution graphics 

• 40 X 25 text format with character-box size of 8 x 8 

• Assignment of 16 colors to any of 2 attributes 

• Supports both EGA and CGA 

• 1 bit per pixel 

SCREEN 7 

• 320 X 200 pixel medium-resolution graphics 

• 40 X 25 text format with character-box size of 8 x 8 

• 2, 4, or 8 memory pages with 64K, 128K, or 256K of memory, respectively, installed on the 
EGA 

• Assignment of any of 16 colors to 16 attributes 

• EGA required 

• 4 bit per pixel 

SCREEN 8 

• 640 X 200 pixel high-resolution graphics 

• 80 X 25 text format with character-box size of 8 x 8 


• 1, 2, or 4 memory pages with 64K, 128K, or 256K of memory, respeetively, installed on the 
EGA 

• Assignment of any of 16 colors to 16 attributes 

• EGA required 

• 4 bits per pixel 

SCREEN 9 

• 640 X 350 pixel enhanced-resolution graphics 

• 80 X 25 text format with character-box size of 8 x 14 

• Assignment of either 64 colors to 16 attributes (more than 64K of EGA memory), or 16 colors 
to 4 attributes (64K of EGA memory) 

• Two display pages if 256K of EGA memory installed 

• EGA required 

• 2 bits per pixel (64K EGA memory) 4 bits per pixel (more than 64K EGA memory) 

SCREEN 10 

• 640 X 350 enhanced-resolution graphics 

• 80 X 25 text format with character-box size of 8 x 14 

• Two display pages if 256K of EGA memory installed 

• Assignment of up to 9 pseudo-colors to 4 attributes 

• EGA required 

• 2 bits per pixel 

The following are default attributes for SCREEN 10, monochrome display: 

Attribute Value Displayed Pseudo-Color 

0 Off 

1 On, normal intensity 

2 Bli nk 

3 On, high intensity 

The following are color values for SCREEN 10 , monochrome display: 

Color Value Displayed Pseudo-Color 

0 Off 

1 Blink, off to on 

2 Blink, off to high intensity 

3 Blink, on to off 

4 On 

5 Blink, on to high intensity 

6 Blink, high intensity to off 

7 Blink, high intensity to on 

8 High intensity 


For both composite monitors and TVs, the colorswi tch is a numeric expression that is either true 
(non-zero) or false (zero). A value of zero disables eolor and permits display of blaek and white 
images only. A nonzero value permits eolor. The meaning of the colorswitch argument is inverted 
in SCREEN mode 0. 

For hardware eonfigurations that inelude an EGA and enough memory to support multiple-screen 
pages, two arguments are available. These apage and vpage arguments determine the "aetive" and 
"visual" memory pages. The aetive page is the area in memory where graphies statements are written; 
the visual page is the area of memory that is displayed on the sereen. 

Animation ean be aehieved by alternating the display of graphies pages. The goal is to display the 
visual page with eompleted graphies output, while exeeuting graphics statements in one or more 
aetive pages. Apage is displayed only when graphies output to that page is eomplete. Thus, the 
following program fragment is typieal; 

SCREEN 7,, 1, 2 'work in page 1, show page 2 

. Graphics output to page 1 
. while viewing page 2 

SCREEN 7,, 2,1 'work in page 2, show page 1 

. Graphics output to page 2 
. while viewing page 1 

The number of pages available depends on the screen mode and the amount of available memory, as 
deseribed in the following table: 

Table 2 

SCREEN Mode Specifications 




Attribute 

Color 

EGA 



Mode 

Resolution 

Range 

Range 

Memory Pages 

Page Size 

0 

40-column textNA 

0-15^^ 

NA 

1 

2K 

80-column textNA 

0-15^^ 

NA 

1 

4K 

1 

320x200 

0 - 3 !^ 

0-3 

NA 

1 

16K 

2 

640x200 

O-li" 

0-1 

NA 

1 

16K 





64K 

2 


7 

320x200 

0-15 

0-15 

128K 

4 

32K 





256K 

8 






64K 

1 


8 

640x200 

0-15 

0-15 

128K 

2 

64K 





256K 

4 




0-3 

0-15 

64K 

1 

64K 

9 

640x350 

0-15 

0-63 

128K 

1 

128K 



0-15 

0-63 

256K 

2 

10 

640x350 

0-3 

0-8 

128K 

256K 

1 

2 

128K 


Numbers in the range 16-31 are blinking versions of the eolors 0-15. 
Attributes applicable only with EGA. 


Attributes and Colors 


For various screen modes and display hardware eonfigurations, different attribute and eolor settings 
exist. (See the PALETTE statement for a discussion of attribute and color number.) The majority of 
these attribute and color configurations are summarized in the following table: 


Table 3 

Default Attributes and Colors for Most Screen Modes 


Attributes for Mode 

Color Display 

Monochrome Display 

1,9 2 

0,7,8,9*’ 

Number‘d 

Color 

Number‘d 

Color 

0 0 

0 

0 

Black 

0 

Off 


1 

1 

Blue 


(Underlined)^* 


2 

2 

Green 

1 

On** 


3 

3 

Cyan 

1 

On** 


4 

4 

Red 

1 

On** 


5 

5 

Magenta 

1 

On** 


6 

6 

Brown 

1 

On** 


7 

7 

White 

1 

On** 


8 

8 

Gray 

0 

Off 


9 

9 

Eight Blue 


High intensity (underlined) 


10 

10 

Light Green 

2 

High intensity 

1 

11 

11 

Light Cyan 

2 

High intensity 


12 

12 

Light Red 

2 

High intensity 

2 

13 

13 

Light Magenta 

2 

High intensity 


14 

14 

Yellow 

2 

High intensity 

3 1 

15 

15 

High-intensity White 

0 

Off 


Off when used for background. 

With EGA memory &gt; 64K. 

Only for mode 0 monochrome. 

The default foreground eolors for the various modes are given in the following table: 

Table 4 

Default Foreground Colors 

Default foreground attribute Default foreground color 
Screen Color/ExtendedMonocbrome Color/ExtendedMonocbrome 


mode 

Display** 

Display 

Display® 

Display 

0 

7 

7 

7 

1 

1 

3 

NA 

15 

NA 

2 

1 

NA 

15 

NA 

7 

15 

NA 

15 

NA 

8 

15 

NA 

15 

NA 

9 

3^ 

NA 

63 

NA 

10 

NA 

3 

NA 

8 


IBM Enhanced Color Display 
15 if greater than 64K of EGA memory 
NA=Not Applicable 



SGN Function 


Purpose: 

To return the sign of x. 

Syntax: 

SGN(x) 

Comments: 

X is any numeric expression. 

If X is positive, SGN (x) returns 1. 

If X is 0, SGN (x) returns 0. 

Ifx is negative, SGN (x) returns - I. 

This statement is similar to, but not the same as SIN (x) , which returns a trigonometric function 
radians, rather than in ones and zeros. 

Examples: 

10 INPUT "Enter value", x 

20 ON SGN(X)+2 GOTO 100, 200, 300 

GW-BASIC branches to 100 if X is negative, 200 if X is 0, and 300 if X is positive. 


SHELL Statement 


Purpose: 

To load and execute another program or batch file. When the program finishes, control returns to the 
GW-BASIC program at the statement following the shell statement. A program executed under 
control of GW-BASIC is referred to as a child process. 

Syntax: 

SHELL [string] 

Comments: 

string is a valid string expression containing the name of a program to run and (optionally) 
command arguments. 

The program name in string may have any extension that MS-DOS COMMAND.COM supports. If 
no extension is supplied, COMMAND will look for a .COM file, then an .EXE file, and finally, a 
.BAT file. If none is found, SHELL will issue a "File not found" error. 

Any text separated from the program name by at least one blank space will be processed by 
COMMAND as program parameters. 

GW-BASIC remains in memory while the child process is running. When the child process finishes, 
GW-BASIC continues at the statement following the SHELL statement. 

SHELL with no string will go to MS-DOS. You may now do anything that COMMAND allows. When 
ready to return to GW-BASIC, type the MS-DOS command EXIT. 

Examples: 

SHELL 

A&gt;DIR 

A&gt;EXIT 

Write some data to be sorted, use shell SORT to sort it, then read the sorted data to write a report. 

10 OPEN "SORTIN.DAT" FOR OUTPUT AS #1 
20 'write data to be sorted 


1000 

1010 

1020 

1030 


CLOSE 1 

SHELL "SORT &lt;SORTIN.DAT &gt;SORTOUT.DAT" 
OPEN "SORTOUT.DAT" FOR INPUT AS #1 
'Process the sorted data 


SIN Function 


Purpose: 

To calculate the trigonometrie sine of x, in radians. 

Syntax: 

SIN(x) 

Comments: 

SIN (x) is calculated in single-precision unless the /d switch is used when GW-BASIC is executed. 
To obtain SIN (x) when x is in degrees, use SIN (x*n/I80) . 

Examples: 

PRINT SIN(I.5) 

.9974951 

The sine of 1.5 radians is .9974951 (single-precision). 


SOUND Statement 


Purpose: 

To generate sound through the speaker. 

Syntax: 

SOUND freq, duration 

Comments: 

freq is the desired frequency in Hertz (cycles per second), freq is a numeric expression within the 
range of 37 to 32767. 

duration is the desired duration in clock ticks. Clock ticks occur 18.2 times per second, duration 
must be a numeric expression within the range of 0 to 65535. 

Values below .022 produce an infinite sound until the next SOUND or play statement is executed. 

If duration is zero, any active SOUND statement is turned off. If no SOUND statement is running, a 
duration of zero has no effect. 

The sound is executed in foreground or background depending on the play statement. 

Examples: 

The following example creates random sounds of short duration; 

2500 SOUND RND*1000+37, 2 
2600 GOTO 2500 

The following table shows the relationship of notes and their frequencies in the two octaves adjacent 
to middle C. 

Table 5 


Relationships of Notes and Frequencies 


Note 

Frequency 

Note 

Frequency 

C 

130.810 

C* 

523.250 

D 

146.830 

D 

587.330 

E 

164.810 

E 

659.260 

F 

174.610 

F 

698.460 

G 

196.000 

G 

783.990 

A 

220.000 

A 

880.000 

B 

246.940 

B 

987.770 

C 

261.630 

C 

1046.500 

D 

293.660 

D 

1174.700 

E 

329.630 

E 

1318.500 

F 

349.230 

F 

1396.900 

G 

392.000 

G 

1568.000 

A 

440.000 

A 

1760.000 

B 

493.880 

B 

1975.500 


^Middle C. 


By doubling or halving the frequency, the coinciding note values can be estimated for the preceding 
and following octaves. 

To produce periods of silence, use the following statement; 

SOUND 32767, duration 

To calculate the duration of one beat, divide beats per minute into the number of clock ticks in a 
minute (1092). 

The following table illustrates tempos requested by clock ticks: 

Table 6 

Tempos Requested by Clock Ticks 




Beats/ 


Tempo 

Notation 

Minute 

Ticks/Beat 


Larghissimo 

Largo 

40-66 

27.3-18.2 

very slow 

Larghetto 

Grave 

Lento 

60-66 

18.2-16.55 

slow 

Adagio 

Adagietto 

66-76 

16.55-14.37 

Andante 

Andantino 

76-108 

14.37-10.11 

medium 

Moderate 

108-120 

10.11-9.1 


Allegretto 

Allegro 

120-168 

9.1-6.5 

fast 

Vivace 

Veloce 

Presto 

168-208 

6.5-5.25 

very fast 

Prestissimo 




SPACES Function 


Purpose: 

To return a string of x spaces. 

Syntax: 

SPACE$(x) 

Comments: 

X is rounded to an integer and must be within the range of 0 to 255 (see SPC function). 
Examples: 

10 FOR N=1 TO 5 
20 X$=SPACE$(N) 

30 PRINT X$; N 
40 NEXT N 
RUN 
1 

2 

3 

4 

5 

Line 20 adds one space for each loop execution. 



SPC Function 


Purpose: 

To skip a specified number of spaces in a print or an lprint statement. 

Syntax: 

SPC(n) 

Comments: 

n must be within the range of 0 to 255. 

If n is greater than the defined width of the printer or the screen, the value used will be n mod width. 
A semicolon is assumed to follow the SPC (n) command. 

SPC may only be used with print, lprint and print# statements (see the SPACES function). 
Examples: 

PRINT "OVER" SPC (15) "THERE" 

OVER THERE 



SQR Function 

Purpose: 

Returns the square root of x. 

Syntax: 

SQR(x) 

Comments: 

X must be greater than or equal to 0. 

SQR (x) is computed in single-precision unless the /d switch is used when GW-BASIC is executed. 
Examples: 

10 FOR X=10 TO 25 STEP 5 
20 PRINT X; SQR(X) 

30 NEXT 
RUN 

10 3.162278 
15 3.872984 
20 4.472136 
25 5 


STICK Function 


Purpose: 

To return the x and y coordinates of two joysticks. 

Syntax: 

x=STICK(n) 

Comments: 

X is a numeric variable for storing the result, 
u is a valid numeric expression within the range of 0 to 3. 

Value of n Coordinate Returned 


X coordinate of joystick A. Stores the x and y values for both joysticks for the 
following three function calls. 


1 

2 

3 


y coordinate of joystick A. 
X coordinate of joystick B. 
y coordinate of joystick B. 


STOP Statement 


Purpose: 

To terminate program execution and return to command level. 

Syntax: 

STOP 

Comments: 

STOP statements may be used anywhere in a program to terminate execution. When a STOP is 
encountered, the following message is printed; 

Break in line nnnnn 

Unlike the end statement, the stop statement does not close files. 

GW-BASIC always returns to command level after a STOP is executed. Execution is resumed by 
issuing a CONT command. 

Examples: 

10 INPUT A, B, C 
20 K=A"'2*5.3: L=B"'3/.26 
30 STOP 

40 M=C*K+100: PRINT M 
RUN 

? 1, 2, 3 
BREAK IN 30 
PRINT L 
30.76923 
CONT 
115.9 


STR$ Function 


Purpose: 

To return a string representation of the value of x. 

Syntax: 

STR$(x) 

Comments: 

STR$ (x) is the complementary function to VAL (x$) (see the VAL function). 
Examples: 

5 REM ARITHMETIC FOR KIDS 

10 INPUT "TYPE A NUMBER"; N 

20 ON LEN(STR$(N)) GOSUB 30, 40, 50 


This program branches to various subroutines, depending on the number of characters typed before 
the RETURN key is pressed. 



STRIG Statement and Function 


Purpose: 

To return the status of the joystick triggers. 

Syntax: 

As a statement: 

STRIG ON 
STRIG OFF 

As a function: 

x=STRIG(ii) 

Comments: 

STRIG ON must be executed before any STRIG (u) function calls may be made. Once SIRIG ON 
executed, GW-BASIC will check to see if a button has been pressed before every statement is 
executed, strig off disables the checking. 

X is a numeric variable for storing the result. 

u is a numeric expression within the range of 0 to 7 that returns the following values: 

Value of n Returns 

0 -1 if trigger A1 was pressed since the last SIRIG ( 0 ) statement; returns 0, if not. 

1 -1 if trigger A1 is currently pressed; returns 0, if not. 

2 -1 if trigger B1 was pressed since the last strig ( 2 ) statement; returns 0, if not. 

3 -1 if trigger B1 is currently pressed; returns 0, if not. 

4 -1 if trigger A2 was pressed since the last stri G ( 4 ) statement; returns 0 if not. 

5 -1 if trigger A2 is currently pressed; returns 0, if not. 


STRIG(n) Statement 

Purpose: 

To allow the use of a joystiek by enabling or disabling the trapping of its buttons. 

Syntax: 

STRIG(n) ON 
STRIG(ii) OFF 
STRIG(n) STOP 

Comments: 

n is 0, 2, 4, or 6, corresponding to the buttons on the joystick, where: 

0 is button A1 
2 is button B1 
4 is button A2 
6 is button B2 

Examples: 

STRIG(n) ON 

Enables trapping of the joystick buttons. After this statement is executed, GW-BASIC checks to see 
this button has been pressed before executing following statements. 

STRIG(n) OFF 

Disables GW-BASIC from checking the state of the button. 

STRIG(n) STOP 

Disables trapping of a given button through the ON STRIG {n) statement. But, any pressings are 
remembered so that trapping may take place once it is reenabled. 


STRINGS Function 

Purpose: 

To return 

• a string of length n whose eharaeters all have ASCII eode j, or 

• the first eharaeter of 

Syntax: 

STRING$ (n,j) 

STRING$ (n,x$) 

Comments: 

STRING$ is also useful for printing top and bottom borders on the sereen or the printer. 
n and j are integer expressions in the range 0 to 255. 

Examples: 

10 X$ = STRING$(10, 45) 

20 PRINT X$ "MONTHLY REPORT" X$ 

RUN 

-MONTHLY REPORT- 

45 is the deeimal equivalent of the ASCII symbol for the minus (-) sign. 

Appendix C in the GW-BASIC User's Guide lists ASCII eharaeter eodes. 





SWAP Statement 

Purpose: 

To exchange the values of two variables. 

Syntax: 

SWAP variablel,variable2 

Comments: 

Any type variable may be swapped (integer, single-precision, double-precision, string), but the two 
variables must be of the same type or a "Type mismatch" error results. 

Examples: 

LIST 

10 A$="ONE ": B$="ALL ": C$="FOR " 

20 PRINT A$ C$ B$ 

30 SWAP A$, B$ 

40 PRINT A$ C$ B$ 

RUN 

ONE FOR ALL 
ALL FOR ONE 

Line 30 swaps the values in the A$ and B$ strings. 


SYSTEM Command 


Purpose: 

To return to MS-DOS. 

Syntax: 

SYSTEM 

Comments: 

Save your program before pressing return, or the program will be lost. 

The SYSTEM command closes all the files before it returns to MS-DOS. If you entered GW-BASIC 
through a batch file from MS-DOS, the SYSTEM command returns you to the batch file, which 
continues executing at the point it left off. 

Examples: 

SYSTEM 

A&gt; 


TAB Function 


Purpose: 

Spaces to position n on the screen. 

Syntax: 

TAB (n) 

Comments: 

If the eurrent print position is already beyond space n, tab goes to that position on the next line. 
Space 1 is the leftmost position. The rightmost position is the screen width. 
n must be within the range of 1 to 255. 

If the TAB function is at the end of a list of data items, GW-BASIC will not return the cursor to the 
next line. It is as though the tab function has an implied semicolon after it. 

TAB may be used only in print, lprint, or print# statements (see the SPC function). 

Examples: 

10 PRINT "NAME" TAB(25) "AMOUNT": PRINT 
20 READ A$,B$ 

30 PRINT A$ TAB(25) B$ 

40 DATA "G. T. JONES","$25.00" 

RUN 

NAME AMOUNT 

G. T. JONES $25.00 



