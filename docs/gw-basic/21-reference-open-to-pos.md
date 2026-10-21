<!-- Chunked from GWBASIC.txt, lines 13421-14827. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Reference: OPEN - POS

OPEN Statement 


Purpose: 

To establish input/output (I/O) to a file or device. 

Syntax: 

OPEN mode, [#]file number, filename[, reclen] 

OPEN filename [FOR mode ][ACCESS access] [lock] AS [#]file number [LEN=recIen] 

Comments: 

filename is the name of the file. 

mode (first syntax) is a string expression with one of the following characters: 

Expression Specifies 

0 Sequential output mode 

I Sequential input mode 

R Random input/output mode 

A Position to end of file 

mode (second syntax) determines the initial positioning within the file, and the action to be taken if the 
file does not exist. If the FOR mode clause is omitted, the initial position is at the beginning of the file. If 
the file is not found, one is created. This is the random I/O mode. That is, records may be read or written 
at any position within the file. The valid modes and actions taken are as follows: 

INPUT Position to the beginning of the file. A "File not found" error is given if the file 

does not exist. 

OUTPUT Position to the beginning of the file. If the file does not exist, one is created. 

APPEND Position to the end of the file. If the file does not exist, one is created. 

RANDOM Specifies random input or output mode. 
mode must be a string constant. Do not enclose mode in double quotation marks, access can be one of 
the following: 

READ 

WRITE 

READ WRITE 

file n umber is a number between 1 and the maximum number of files allowed. The number associates 
an I/O buffer with a disk file or device. This association exists until a close or close file number 
statement is executed. 

reclen is an integer expression within the range of 1-32767 that sets the record length to be used for 
random files. If omitted, the record length defaults to 128-b3^e records. 

When reclen is used for sequential files, the default is 128 bytes, and reclen cannot exceed the value 
specified by the /s switch. 

A disk file must be opened before any disk I/O operation can be performed on that file, open allocates a 
buffer for I/O to the file and determines the mode of access that is used with the buffer. 

More than one file can be opened for input or random access at one time with different file numbers. For 
example, the following statements are allowed: 

OPEN "B:TEMP" FOR INPUT AS #1 
OPEN "B:TEMP" FOR INPUT AS #2 


However, a file may be opened only once for output or appending. For example, the following statements 
are illegal: 

OPEN "TEMP" FOR OUTPUT AS #1 
OPEN "TEMP" FOR OUTPUT AS #2 


Note 

Be sure to close all files before removing diskettes from the disk drives (see CLOSE and RESET) . 


A device may be one of the following: 

A:, B:, C: . . . Disk Drive 

KYBD : Keyboard (input only) 

SCRN: Screen (output only) 

LPTl: Line Printer 1 

LPT2: Line Printer 2 

LPT3 : Line Printer 3 

COMl : RS-232 Communications 1 

COM2 : RS-232 Communications 2 

For each device, the following open modes are allowed: 

KYBD : Input Only 

SCRN: Output Only 

LPTl: Output Only 

LPT2 : Output Only 

LPT3 : Output Only 

COMl : Input, Output, or Random Only 

COM2 : Input, Output, or Random Only 

Disk files allow all modes. 

When a disk file is opened for append, the position is initially at the end of the file, and the record 
number is set to the last record of the file (lof (x) /128). PRINT, WRITE, or PUT then extends the file. 
The program may position elsewhere in the file with a GET statement. If this is done, the mode is 
changed to random and the position moves to the record indicated. 

Once the position is moved from the end of the file, additional records may be appended to the file by 
executing a GET #x, LOF (x) / reclen statement. This positions the file pointer at the end of the file in 
preparation for appending. 

Any values entered outside of the ranges given result in "illegal function call" errors. The files 
are not opened. 

If the file is opened as input, attempts to write to the file result in "Bad file mode" errors. 

If the file is opened as output, attempts to read the file result in "Bad file mode" errors. 

Opening a file for OUTPUT or APPEND fails, if the file is already open in any mode. 

Since it is possible to reference the same file in a subdirectory via different paths, it is nearly impossible 
for GW-BASIC to know that it is the same file simply by looking at the path. For this reason, GW- 
BASIC does not let you open the file for output or append if it is on the same disk, even if the path is 
different. For example if mary is your working directory, the following statements all refer to the same 
file: 

OPEN "REPORT" 

OPEN "\SALES\MARY\REPORT" 








OPEN "..\MARY\REPORT" 

OPEN "..\.\MARY\REPORT" 

At any one time, it is possible to have a particular diskette filename open under more than one file 
number. Each file number has a different buffer, so several records from the same file may be kept in 
memory for quick access. This allows different modes to be used for different purposes; or, for program 
clarity, different file numbers to be used for different modes of access. 

If the LEN=reclen option is used, reclen may not exceed the value set by the /s : reclen switch 
option in the command line. 

In a network environment, the use of the OPEN statement is based upon two different sets of 
circumstances: 

• Devices may be shared on a network for specific purposes only, so that OPEN statements may be 
restricted to specific modes among those which might be requested, such as: input, output, 
APPEND, and default (random). 

• Files may be restricted by the implementation of an OPEN statement which allows a process to 
specify an access to the successfully opened file. The access determines a guaranteed exclusivity 
range on that file by the process while the open statement is in effect. 

lock can be one of the following: 

"deny none" mode. No restrictions are placed on the read/write accessibility of the the file 
to another process, except that the default mode is not allowed by any of the modes 
including SHARED. 

"deny read" mode. Once a file is opened with the lock read access, no other process is 
granted read-access to that file. An attempt to open a file with this access will be 
unsuccessful, if the file is currently open in default mode or with a read access. 

"deny write" mode. A file successfully opened with lock write access may not be 
opened for a write access by another process. An attempt to open a file with this access will 
be unsuccessful if the file has been opened in default mode, or with a write access by 
another process. 

"deny all" or "exclusive" mode. If a file is successfully opened with this access, the process 
has exclusive access to the file. A file that is currently open in this mode cannot be opened 
again in any mode by any process. 

"compatibility" mode, in which the compatibility with other BASICs is understood. No 
access is specified. The file may be opened any number of times by a process, provided that 
the file is not currently opened by another process. Other processes are denied access to the 
file while it is open under default access. Therefore, it is functionally exclusive. 

When an attempt is made to open a file that has been previously accessed by another process, the error 
"Permission Denied" will result. An example of a situation generating this error is when a process 
attempts to OPEN SHARED on a file that is already OPEN LOCK READ WRITE by another process. 

If an OPEN statement fails because the mode is incompatible with network-installed sharing access to a 
device, the error generated is "Path/File Access Error." An example of this is when a process is 
attempting to OPEN a file for output on a directory that has been shared for read only. 

For more information about using files in a networking environment, see the LOCK and UNLOCK 
statements. 

Examples: 

10 OPEN "I",2,"INVEN" 


SHARED 


LOCK 

READ 

LOCK 

WRITE 


LOCK 

READ 

WRITE 

default 




Opens file 2, inven, for sequential input. 



OPEN "COM(n) Statement 

Purpose: 

To allocate a buffer to support RS-232 asynchronous communications with other computers and peripheral devices in the same manner as OPEN for disk 
files. 


Syntax: 

OPEN "C0M[/3] : [speed] [/parity] [/ data] [ , s top] [ / NS] [ / CS [r] ] [ / OS [r ] ] [ / CD[ a] ] [ / LF] [/PS]" AS [#]Tiieru;7i [LEN^rumber] 


Comments: 

COM [ a ] is a valid communications device: com 1: or com2:. 


speed is a literal integer specifying the transmit/receive baud rate. Valid speeds are as follows: 75, 110, 150, 300, 600, 1200, 1800,2400,4800, and 9600. 
The default is 300 bps. 

parity is a one-character literal specifying the parity for transmitting and receiving. Valid characters specifying parity are as follows: 


S SPACE Parity bit always transmitted and received as space (0 bit). 

M MARK Parity bit always transmitted and received as mark (1 bit). 

0 ODD Odd transmit parity; odd receive parity checking. Default is even. 

E EVEN Even transmit parity; even receive parity checking. Even is default. 

N NONE No transmit parity. No receive parity checking. 


da ta is a literal integer indicating number of transmit/receive data bits. Valid values for the number of data bits are 4, 5, 6, 7, or 8, the default is 7 bits. 


Note 

Four data bits with no parity is illegal; eight data bits with any parity is illegal. 


stop is a literal integer expression returning a valid file number. Valid values for number of stop bits are 1 and 2. If omitted, 75 and 110 bps transmit two 
stop bits. All others transmit one stop bit. 

filenum is a number between 1 and the maximum number of files allowed. A communications device may be opened to only one file number at a time. 
The filenum is associated with the file for as long as the file is open, and is used to refer other COM I/O statements to the file. Any coding errors within 
the filename string result in "Bad file name" errors. An indication as to which parameters are in error is not given. 

number is the maximum number of bytes which can be read from the communications buffer when using GET or PUT default of 128 bytes. 

A "Device timeout" error occurs if "data set ready" (DSR) is not detected. 

The RS, CS, DS, DC, LF, and PE options affect the line signals as follows: 


Option 

Function 

RS 

suppresses RTS (request to send) 

CS [u] 

controls CTS (clear to send) 

DS [u] 

controls DSR (data set ready) 

CD[u] 

controls CD (carrier detect) 

LF 

sends a line feed at each return 

PE 

enables parity checking 


n is the number of milliseconds to wait (0-65535) for that signal before a device timeout error occurs. Defaults are: CSIOOO, DSIOOO, and CDO. If RS was 
specified then CSO is the default. If n is omitted, then timeout is set to 0. 

See Appendix F in the GW-BASIC User's Guide for more information about communications. 

Examples: 

In the following, File 1 is opened for communications with all defaults: speed at 300 bps, even parity, seven data bits, and one stop bit. 

10 OPEN "COMl:" AS 1 

In the following, File 2 is opened for communications at 2400 bps. Parity and number of data bits are defaulted. 

20 OPEN "COMl:2400" AS #2 

In the following, File 1 is opened for asynchronous I/O at 1200 bits/second. No parity is to be produced or checked. 

10 OPEN "COMl:1200/N/8" AS #1 





OPTION BASE Statement 


Purpose: 

To declare the minimum value for array subscripts. 

Syntax: 

OPTION BASE n 
Comments: 

u is 1 or 0. The default base is 0. 

If the statement OPTION BASE 1 is executed, the lowest value an array subscript can have is 1. 

An array subscript may never have a negative value. 

OPTION BASE gives an error only if you change the base value. This allows chained programs to 
have OPTION BASE statements as long as the value is not changed from the initial setting. 

Note 

You must code the option base statement before you can define or use any arrays. If an attempt is 
made to change the option base value after any arrays are in use, an error results. 




OUT Statement 


Purpose: 

To send a byte to a maehine output port. 

Syntax: 

OUT h,j 

Comments: 

h and j are integer expressions, h may be within the range of 0 to 65535. j may be within the range 
of 0 to 255. h is a maehine port number, and j is the data to be transmitted. 

OUT is the eomplementary statement to the INP funetion. 

Examples: 

100 OUT 12345,225 

Outputs the decimal value 225 to port number 12345. In assembly language, this is equivalent to the 
following: 

MOV DX,12345 
MOV AL,255 
OUT DX,AL 


PAINT Statement 


Purpose: 

To fill in a graphics figure with the selected attribute. 

Syntax: 

PAINT (x start, y start) [,paint attribute), border attribute] [,bckgrnd attribute]] 


Comments: 

The PAINT statement fills in an arbitrary graphics figure of the specified border attribute with the specified paint attribute. If 
paint attribute is not given, it will default to the foreground attribute (3 or 1). border attribute defaults to paint 
attribute. See the COLOR and PALETTE statements for more information. 

PAINT must start on a non-border point, otherwise, PAINT will have no effect. 

PAINT can fill any figure, but painting jagged edges or very complex figures may result in an "Out of memory" error. The 
CLEAR statement may be used to increase the amount of stack space available. 

Points that are specified outside the limits of the screen will not be plotted and no error will occur. 

See the SCREEN statement for a description of the different screen modes. 


Paint Tiling 

PAINT tiling is similar to LINE styling. Like LINE, PAINT looks at a tiling mask each time a point is put down on the screen. 
If paint attribute is omitted, the standard foreground attribute is used. 

If paint attribute is a numeric formula, then the number must be a valid color, and it is used to paint the area as before. 
If paint attribute is a string formula, then tiling is performed as follows: 

The tile mask is always eight bits wide and may be from 1 to 64 bytes long. 

Each byte in the tile string masks eight bits along the x axis when putting down points. Each byte of the tile string is rotated as 
required to align along the y axis, such that: 

tile_byte_mask=y MOD tile_length 


where y is the position of the graphics cursor on the y-axis. 


tile_length is the length in bytes of the tile string defined by the user (1 to 64 bytes). 

This is done so that the tile pattern is replicated uniformly over the entire screen (as if a PAINT (0,0).. were used). 


X Increases --&gt; 
x,y 87654321 
0,0 |x|x|x|x|x|x|x|x| 

0,1 |x|x|x|x|x|x|x|x| 

0,2 |x|x|x|x|x|x|x|x| 


Bit of Tile Byte 

Tile byte 1 
Tile byte 2 
Tile byte 3 


0,63 |x|x|x|x|x|x|x|x| Tile byte 64 

(maximum allowed) 


In high-resolution mode (SCREEN 2), the screen can be painted with Xs by the following statement: 

PAINT (320,100),CHR$(&amp;H8I)+CHR$(&amp;H42)+CHR$(&amp;H24)+CHR$(&amp;HI8)+CHR$(&amp;HI8)+CHR$(&amp;H24)+CHR$(&amp;H8I) 


This appears on the screen as follows: 


X increases --&gt; 

0,0 |x| 1 1 1 1 1 |x| 

CHR$ (&amp;H81) 

Tile 

byte 

1 

0,1 

1 1^1 1 1 1 1^1 1 

CHR$(&amp;H42) 

Tile 

byte 

2 

0,2 

1 1 1 X 1 1 1 X 1 1 1 

CHR$(&amp;H24) 

Tile 

byte 

3 

0,3 

1 1 1 1 X 1 X 1 1 1 1 

CHR$(&amp;H18) 

Tile 

byte 

4 

0, 4 

1 1 1 1 X 1 X 1 1 1 1 

CHR$(&amp;H18) 

Tile 

byte 

5 

0, 5 

1 1 1 X 1 1 1 X 1 1 1 

CHR$(&amp;H24) 

Tile 

byte 

6 

0, 6 

1 1 X 1 1 1 1 1 X 1 1 

CHR$(&amp;H42) 

Tile 

byte 

7 

0, 7 

1 X 1 1 1 1 1 1 1 X 1 

CHR$(&amp;H81) 

Tile 

byte 

8 














Since there are two bits per pixel in medium-resolution mode (screen 1), each byte of the tile pattern only describes four 
pixels. In this case, every two bits of the tile byte describes one of the four possible colors associated with each of the four 
pixels to be put down. 

bckgrnd attribute specifies the background tile pattern or color byte to skip when checking for boundary termination. 
bckgrnd attribute is a string formula returning one character. When omitted, the default is CHR$ (0) . 

Occasionally, you may want to paint tile over an already painted area that is the same color as two consecutive lines in the tile 
pattern. PAINT quits when it encounters two consecutive lines of the same color as the point being set (the point is surrounded). 
It is not possible to draw alternating blue and red lines on a red background without bckgrnd attribute. PAINT stops as 
soon as the first red pixel is drawn. By specifying red (CHR$ ( &amp;haa) ) as the background attribute, the red line is drawn over the 
red background. 

You cannot specify more than two consecutive bytes in the tile string that match the background attribute. Specifying more than 
two results in an "illegal function call" error. 

Examples: 

10 CLS 
20 SCREEN 1 

30 LINE (0, 0)-(I00, 150), 2, B 
40 PAINT (50, 50), I, 2 
50 LOCATE 20, I 

The PAINT statement in line 40 fills in the box drawn in line 30 with color 1. 


PALETTE, PALETTE USING Statements 

Purpose: 

Changes one or more of the colors in the palette 
Syntax: 

PALETTE [attribute,color] 

PALETTE USING integer-array-name [arrayindex] 

Comments: 

The PALETTE Statement works only for systems equipped with the IBM® Enhanced Graphics 
Adapter (EGA). A GW-BASIC palette contains a set of colors, with each color specified by an 
attribute. Each attribute is paired with an actual display color. This color determines the 
actual visual color on the screen, and is dependent on the setting of your screen mode and your actual 
physical hardware display. 

PALETTE with no arguments sets the palette to a known initial setting. This setting is the same as the 
setting when colors are first initialized. 

If arguments are specified, color will be displayed whenever attribute is specified in any 
statement that specifies a color. Any color changes on the screen occur immediately. Note that when 
graphics statements use color arguments, they are actually referring to attributes and not actual colors. 
PALETTE pairs attributes with actual colors. 

Eor example, assume that the current palette consists of colors 0, 1, 2, and 3. The following 
DRAW statement: 

DRAW "C3L100" 

selects attribute 3, and draws a line of 100 pixels using the color associated with the attribute 3, in this 
case, also 3. If the statement: 

PALETTE 3,2 

is executed, then the color associated with attribute 3 is changed to color 2. All text or graphics 
currently displayed on the screen using attribute 3 are instantaneously changed to color 2. All text or 
graphics subsequently displayed with attribute 3 will also be displayed in color 2. The new palette of 
colors will contain 0, 1,2, and 2. 

With the USING option, all entries in the palette can be modified in one palette statement. The 
integer-array-name argument is the name of an integer array, and the arrayindex specifies the 
index of the first array element in the integer-ar ray-name to use in setting your palette. Each 
attribute in the palette is assigned a corresponding color from this integer array. The array must be 
dimensioned large enough to set all the palette entries after arrayindex. Eor example, if you are 
assigning colors to all 16 attributes, and the index of the first array element given in your palette 
USING statement is 5, then the array must be dimensioned to hold at least 20 elements (since the 
number of elements from 5-20, inclusive, is 16): 

DIM PAL%(20) 


PALETTE USING PAL%(5) 


If the color argument in an array entry is -1, then the mapping for the assoeiated attribute is not 
ehanged. All other negative numbers are illegal values for color. 

You can use the color argument in the COLOR statement to set the default text color. (Remember that 
color arguments in other BASIC statements are actually what are called attributes in this 
discussion.) This color argument specifies the way that text characters appear on the display screen. 
Under a common initial palette setting, points colored with the attribute 0 appear as black on the 
display screen. Using the palette statement, you could, for example, change the mapping of 
attribute 0 from black to white. 

Remember that a palette statement executed without any parameters assigns all attributes their 
default colors. 

The following table lists attribute and color ranges for various monitor types and screen modes: 


Table 1 

SCREEN Color and Attribute Ranges 


SCREEN 



Attribute 

Color 

Mode 

Monitor Attached 

Adapter 

Range 

Range 

0 

Monochrome 

MDPA 

NA 

NA 


Monochrome 

EGA 

0-15 

0-2 


Color 

CGA 

NA 

0-3 U 


Color/Enhanced^^ 

EGA 

0-3 U 

0-15 

1 

Color 

CGA 

NA 

0-3 


Color/Enhanced^^ 

EGA 

0-3 

0-15 

2 

Color 

CGA 

NA 

0-1 


Color/Enhanced^^ 

EGA 

0-1 

0-15 

7 

Color/Enhanced^^ 

EGA 

0-15 

0-15 

8 

Color/Enhanced^^ 

EGA 

0-15 

0-15 

9 

Enhanced^^ 

EGA'’ 

0-3 

0-15 


Enhanced^^ 

EGA‘’ 

0-15 

0-63 

10 

Monochrome 

EGA 

0-3 

0-8 


‘‘Attributes 16-31 refer to blinking versions of colors 0-15 
With 64K of EGA memory 
With greater than 64K of EGA memory 
IBM Enhanced Color Display 
NA = Not Applicable 
CGA = IBM Color Graphics Adapter 
EGA = IBM Enhanced Graphics Adapter 
MDPA = IBM Monochrome Display and Printer Adapter 

See the SCREEN statement reference page for the list of colors available for various SCREEN mode, 
monitor, and graphics adapter combinations. 

Examples: 

PALETTE 0, 2 'Changes all points colored with attribute 0 

'to color 2 


PALETTE 0,-1 


Does not modify the palette 



PALETTE 

USING A%(0) 'Changes each palette entry. Since the 

'array is initialized to zero when it 
'is first declared, all attributes are 
'now mapped to display color zero. The 
'screen will now appear as one single 
'color. However, it will still be 
'possible to execute BASIC statements. 

PALETTE 

'Sets each palette entry to its appropriate 
'initial display color. Actual initial colors 
'depend on your screen hardware configuration 


PCOPY Command 


Purpose: 

To copy one screen page to another in all screen modes. 

Syntax: 

PCOPY sourcepage, destinationpage 

Comments: 

The sourcepage is an integer expression in the range 0 to n, where n is determined by the current 
video-memory size and the size per page for the current screen mode. 

The destinationpage has the same requirements as the sourcepage. 

For more information, see CLEAR and SCREEN . 

Examples: 

This copies the contents of page 1 to page 2: 

PCOPY 1,2 




PEEK Function 


Purpose: 

To read from a specified memory location. 

Syntax: 

PEEK(a) 

Comments: 

Returns the byte (decimal integer within the range of 0 to 255) read from the specified memory 
location a. a must be within the range of 0 to 65535. 

The DEF SEG statement last executed determines the absolute address that will be peeked into. 

PEEK is the complementary function to the POKE statement. 

Examples: 

10 A=PEEK(&amp;H5A00) 

The value of the byte, stored in user-assigned hex offset memory location 5A00 (23040 decimal), will 
be stored in the variable A. 


PEN Statement and Function 


Purpose: 

To read the light pen. 

Syntax: 

As a statement: 

PEN ON 
PEN OFF 
PEN STOP 

As a function: 

X = PEN(ii) 

Comments: 

PEN ON enables the pen read function. 

PEN OFF disables the pen read function. 

PEN STOP disables trapping. It remembers the event so immediate trapping occurs when pen on is 
executed. 

The PEN function is initially off A pen on statement must be executed before any pen read function 
calls can be made, or a pen read function call results in an "illegal function call" error. 

X = PEN(n) reads the light pen coordinates. 

X is the numeric variable receiving the pen value. 

n is an integer within the range of 0 to 9. 

Light pen coordinates: 

n = 0 If PEN was down since last poll, returns-1; if not, returns 0. 

n = 1 Returns the x-pixel coordinate when pen was last activated. The range is within 0 to 

319 for medium resolution; 0 to 639, for high resolution, 
n = 2 Returns the y-pixel coordinate when pen was last activated. The range is within 0 to 
199. 

n = 3 Returns the current pen switch value. Returns -1 if down; 0 if up. 

n = 4 Returns the last known valid x-pixel coordinate. The range is within 0 to 319 for 

medium resolution; or 0 to 639 for high resolution. 
n = 5 Returns the last known valid y-pixel coordinate. The range is within 0 to 199. 

n = 6 Returns the character row position when pen was last activated. The range is within 1 to 

24. 

n = 1 Returns the character column position when pen was last activated. The range is within 
1 to 40, or 1 to 80, depending on the screen width. 
n = 8 Returns the last known valid character row. The range is within 1 to 24. 

n = 9 Returns the last known valid character column position. The range is within 1 to 40, or 1 

to 80, depending on the screen width. 

For execution speed improvements, turn the pen off with a pen off statement for those programs not 
using the light pen. 


When the pen is in the border area of the screen, the values returned will be inaccurate. 

Examples: 

50 PEN ON 

60 FOR 1=1 to 500 

70 X=PEN(0): X1=PEN(3) 

80 PRINT X, XI 
90 NEXT 
100 PEN OFF 

This example prints the pen value since the last poll and the current value. 


PLAY Statement 


Purpose: 

To play music by embedding a musie maero language into the string data type. 

Syntax: 

PLAY string expression 

Comments: 

The single-character commands in play are as follows: 

A-G [#, + ,-] a-G are notes. # or + following a note produces a sharp; - produces a flat. 


L {n) 

Any note followed by #, +, or - must refer to a black key on a piano. 

Sets the length of each note. L4 is a quarter note, LI is a whole note, and so on. n 
may be from 1 to 64. 

Length may also follow the note to ehange the length for that note only. Alb is 
equivalent to L16A. 

MF 

Music foreground, play and sound statements are to run in foreground. That is, 
each subsequent note or sound is not started until the previous note or sound is 
finished. This is the initial default. 

MB 

Music background, play and sound statements are to run in background. That is, 
each note or sound is plaeed in a buffer allowing the BASIC program to continue 
execution while musie plays in the background. As many as 32 notes (or rests) ean 
be played in background at one time. 

MN 

ML 

MS 

N(n) 

Music normal. Each note plays seven-eighths of the time determined by L (length). 
Music legato. Each note plays the full period set by L. 

Music staccato. Each note plays three-quarters of the time determined by L. 

Play note n. n may range from 0 to 84. In the 7 possible octaves, there are 84 notes, 
n set to 0 indicates a rest. 

0(n) 

Octave 0 sets the current oetave. There are 7 oetaves (0 through 6). Default is 4. 
Middle C is at the beginning of octave 3. 

F(n) 

T(n) 

Pause. P may range from 1-64. 

Tempo. T sets the number of L4s in a minute, n may range from 32-255. Default is 
120. 

. (period) 

A period after a note inereases the playing time of the note by 3/2 times the period 
determined by L (length of note) times T (tempo). Multiple periods can appear after 
a note, and the playing time is scaled accordingly. Eor example, A. will cause the 
note A to play one and half times the playing time determined by E (length of the 
note) times T (the tempo); two periods placed after A (A..) will cause the note to be 
played at 9/4 times its ascribed value; an A with three periods (A...) at 27/8, etc. 

Periods may also appear after a P (pause), and increase the pause length as 
described above. 

Xstring; 

Executes a substring, where string is a variable assigned to a string of play 
commands. 


Because of the slow clock interrupt rate, some notes do not play at higher 
tempos; for example, 1.64 at T255. These note/tempo combinations must be 
determined through experimentation. 

A greater-than symbol preceding the note n plays the note in the next higher 
octave. 

A less-than symbol preceding the note n plays the note in the next lower octave. 


Note 

Numeric arguments follow the same syntax described under the draw statement. 


n as an argument can be a constant, or it can be a variable with = in front of it (= variable). A 
semicolon is required after the variable and also after the variable mxstring. 




PLAY(n) Function 

Purpose: 

To return the number of notes currently in the background music queue. 

Syntax: 

PLAY( n ) 

Comments: 

u is a dummy argument, and may be any value. 

PLAY (n) returns 0 when in music foreground mode. 

The maximum returned value of x is 32. 

Examples: 

10 ' when 4 notes are left in 
20 ' queue play another tune 
30 PLAY "MBABCDABCDABCD" 

40 IF PLAY (0) =4 then 200 


200 PLAY "MBCDEFCDEF" 


PMAP Function (Graphics) 

Purpose: 

To map expressions to logieal or physical coordinates. 

Syntax: 

x=PMAP {exp, function) 

Comments: 

This function is valid for graphics modes only. 

X is the physical coordinate of the point that is to be mapped. 
exp is a numeric variable or expression. 

Function Maps 

0 logical expressions to physical x 

1 logical expressions to physical y 

2 physical expressions to logical x 

3 physical expressions to logical y 

PMAP is used with window and view to translate coordinates. 


POINT Function 


Purpose: 

To read the eolor or attribute value of a pixel from the sereen. 

Syntax: 

POINT {x,y) 

POINT ( function) 

Comments: 

In the first syntax, x and y are coordinates of the point to be examined. 

If the point given is out of range, the value -1 is returned. 

See the COLOR and PALETTE statements for valid color and attribute values. 

POINT with one argument allows you to retrieve the current graphics coordinates. 

POINT ( function) returns the value of the current x or y graphics coordinates as follows: 
Function Returns 

0 the current physical x coordinate. 

1 the current physical y coordinate. 

2 the current logical x coordinate if window is active; otherwise, it returns the current 
physical x coordinate as in 0 above. 

3 the current logical y coordinate if window is active; otherwise, it returns the current 
physical y coordinate as in 1 above. 

Example 1: 

10 SCREEN I 
20 FOR C=0 TO 3 
30 PSET (10, 10),C 

40 IF POINT(10, I0)&lt;&gt;C THEN PRINT "BROKEN BASIC=" 

50 NEXT C 

Example 2: 

The following inverts the current state of a point: 

10 SCREEN 2 

20 IF POINT(I, 1)00 THEN PRESET(I, I) ELSE PSET(I, I) 

Example 3: 

The following is another way to invert a point: 

20 PSET (I, I), I-POINT(I, I) 




POKE Statement 


Purpose: 

To write (poke) a byte of data into a memory loeation. 

Syntax: 

POKE a,b 

Comments: 

a and b are integer expressions. 

The integer expression a is the offset address of the memory location to be poked. The def seg 
statement last executed determines the address. GW-BASIC does not check any offsets that are 
specified. 

The integer expression b is the data to be poked. 

b must be within the range of 0 to 255. a must be within the range of 0 to 65535. 

The complementary function to POKE is peek. The argument to peek is an address from which a byte 
is to be read. 

POKE and PEEK are useful for efficient data storage, loading assembly language subroutines, and for 
passing arguments and results to and from assembly language subroutines. 

Examples: 

20 POKE &amp;H5A00, &amp;HFF 

Places the decimal value 255 (&amp;HFF) into the hex offset location (23040 decimal) See PEEK function 
example. 



POS Function 


Purpose: 

To return the eurrent eursor position. 

Syntax: 

POS(c) 

Comments: 

The leftmost position is 1. 
c is a dummy argument. 

Examples: 

10 CLS 
20 WIDTH 80 

30 A$=INKEY$:IF A$=""THEN GOTO 30 ELSE PRINT A$; 

40 IF POS(X)&gt;10 THEN PRINT CHR$(13); 

50 GOTO 30 

Causes a carriage return after the 10th character is printed on each line of the screen. 


PRESET and PSET Statements 


Purpose: 

To display a point at a specified place on the screen during use of the graphics mode. 

Syntax: 

PRESET (x,y) [, color] 

PSET(x,y) [, color] 

Comments: 

(x, y) represents the coordinates of the point. 
color is the color of the point. 

Coordinates can be given in either absolute or relative form. 

Absolute Form 

{absolute x, absolute y) is more common and refers directly to a point without regard to the 
last point referenced. For example; 

( 10 , 10 ) 

Relative Form 

STEP (x offset, y offset) is a point relative to the most recent point referenced. For example: 
STEP (10,10) 

Coordinate values can be beyond the edge of the screen. However, values outside the integer range 
(-32768 to 32767) cause an "Overflow" error. 

(0,0) is always the upper-left comer and (0,199) is the lower-left comer in both high resolution and 
medium resolution. 

See the COLOR and PALETTE statements for more information. 

If the value for color is greater than 3, an "illegal function call" error is returned. 

Example 1: 

The following draws a diagonal line from (0,0) to (100,100). 

10 CLS 
20 SCREEN 1 
30 FOR 1=0 TO 100 
40 PSET (1,1) 

50 NEXT 

60 LOCATE 14,1 

Example 2: 

The following clears out the line by setting each pixel to 0. 

40 FOR 1=100 TO 0 STEP -1 
50 PSET (I,I) , 0 
60 NEXT I 




