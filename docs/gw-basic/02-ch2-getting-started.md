<!-- Chunked from GWBASIC.txt, lines 1208-1559. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Guide - Chapter 2: Getting Started

Chapter 2 

Getting Started with GW-BASIC 

This chapter describes how to load GW-BASIC into your system. It also explains the two different types 
of operation modes, line formats, and the various elements of GW-BASIC. 

2.1 Loading GW-BASIC 

To use the GW-BASIC language, you must load it into the memory of your computer from your 
working copy of the MS-DOS diskette. Use the following procedure: 

1. Turn on your computer. 

2. Insert your working copy of the MS-DOS diskette into Drive A of your computer, and press 

RETURN. 

3. Type the following command after the A&gt; prompt, and press return: 

gwbasic 

Once you enter GW-BASIC, the GW-BASIC prompt. Ok, will replace the MS-DOS prompt, A&gt;. 

On the screen, the line xxxxx Bytes Free indicates how many bytes are available for use in memory 
while using GW-BASIC. 

The function key (fi-fio) assignments appear on the bottom line of the screen. These function keys can 
be used to eliminate key strokes and save you time. Chapter 4, "The GW-BASIC Screen Editor," 
contains detailed information on function keys. 

2.2 Modes of Operation 

Once GW-BASIC is initialized (loaded), it displays the Ok prompt. Ok means GW-BASIC is at 
command level; that is, it is ready to accept commands. At this point, GW-BASIC may be used in either 
of two modes: direct mode or indirect mode. 

2.2.1 Direct Mode 

In the direct mode, GW-BASIC statements and commands are executed as they are entered. Results of 
arithmetic and logical operations can be displayed immediately and/or stored for later use, but the 
instructions themselves are lost after execution. This mode is useful for debugging and for using GW- 
BASIC as a calculator for quick computations that do not require a complete program. 

2.2.2 Indirect Mode 

The indirect mode is used to enter programs. Program lines are always preceded by line numbers, and 
are stored in memory. The program stored in memory is executed by entering the RUN command. 

2.3 The GW-BASIC Command Line Format 

The GW-BASIC command line lets you change the environment or the conditions that apply while 
using GW-BASIC. 


Note 





When you specify modifications to the operating environment of GW-BASIC, be sure to maintain the 
parameter sequence shown in the syntax statement. To skip a parameter, insert a comma. This will let 
the computer know that you have no changes to that particular parameter. 


GW-BASIC uses a command line of the following form: 

gwbasic [ filename] [&lt;stdin] [[&gt;] &gt;stdout] [/f:n] [/i] [/s:n] [/c:n] [/m: [n] [,n]] [/d] 

filename is the name of a GW-BASIC program file. If this parameter is present, GW-BASIC proceeds 
as if a RUN command had been given. If no extension is provided for the filename, a default file 
extension of .BAS is assumed. The .BAS extension indicates that the file is a GW-BASIC file. The 
maximum number of characters a filename may contain is eight with a decimal and three extension 
characters. 

&lt;stdin redirects GW-BASIC's standard input to be read from the specified file. When used, it must 
appear before any switches. 

This might be used when you have multiple files that might be used by your program and you wish to 
specify a particular input file. 

&gt;stdout redirects GW-BASIC's standard output to the specified file or device. When used, it must 
appear before any switches. Using &gt;&gt; before stdout causes output to be appended. 

GW-BASIC can be redirected to read from standard input (keyboard) and write to standard output 
(screen) by providing the input and output filenames on the command line as follows: 

gwbasic program name &lt;input file[&gt;]&gt;output file 

An explanation of file redirection follows this discussion of the GW-BASIC command line. 

Switches appear frequently in command lines; they designate a specified course of action for the 
command, as opposed to using the default for that setting. A switch parameter is preceded by a slash (/). 

/f:n sets the maximum number of files that may be opened simultaneously during the execution of a 
GW-BASIC program. Each file requires 194 bytes for the File Control Block (FCB) plus 128 bytes for 
the data buffer. The data buffer size may be altered with the /s: switch. If the /f: switch is omitted, the 
maximum number of open files defaults to 3. This switch is ignored unless the /i switch is also 
specified on the command line. 

/i makes GW-BASIC statically allocate space required for file operations, based on the /s and /f 
switches. 

/s : n sets the maximum record length allowed for use with files. The record length option in the open 
statement cannot exceed this value. If the /s: switch is omitted, the record length defaults to 128 bytes. 
The maximum record size is 32767. 

/c: n controls RS-232 communications. If RS-232 cards are present, /c: 0 disables RS-232 support, 
and any subsequent I/O attempts for each RS-232 card present. If the /c: switch is omitted, 256 bytes 
are allocated for the receive buffer and 128 bytes for the transmit buffer for each card present. 

The /c: switch has no affect when RS-232 cards are not present. The /c : n switch allocates n bytes for 
the receive buffer and 128 bytes for the transmit buffer for each RS-232 card present. 

/m:n[,n] sets the highest memory location (first n) and maximum block size (second n) used by GW¬ 
BASIC. GW-BASIC attempts to allocate 64K bytes of memory for the data and stack segments. If 
machine language subroutines are to be used with GW-BASIC programs, use the /m: switch to set the 
highest location that GW-BASIC can use. The maximum block size is in multiples of 16. It is used to 
reserve space for user programs (assembly language subroutines) beyond GW-BASIC's workspace. 



The default for maximum bloek size is the highest memory loeation. The default for the highest memory 
loeation is 64K bytes unless maximum bloek size is speeified, in whieh ease the default is the maximum 
bloek size (in multiples of 16). 

/d allows eertain funetions to return double-preeision results. When the /d switeh is speeified, 
approximately 3000 bytes of additional eode spaee are used. The funetions affeeted are atn, cos, exp, 
LOG, SIN, SQR, and TAN. 

Note 

All switeh numbers may be speeified as deeimal, oetal (preeeded by &amp;o), or hexadeeimal (preeeded by 
&amp;h). 

Sample GW-BASIC eommand lines are as follows: 

The following uses 64K bytes of memory and three files; loads and exeeutes the program file 
payroll.has: 

A&gt;gwbasic PAYROLL 

The following uses 64K bytes of memory and six files; loads and exeeutes the program file invent, has: 

A&gt;gwbasic INVENT /F:6 

The following disables RS-232 support and uses only the first 32K bytes of memory. 32K bytes above 
that are reserved for user programs: 

A&gt;gwbasic /C:0 /M:32768,4096 

The following uses four files and allows a maximum reeord length of 512 bytes: 

A&gt;gwbasic /F:4 /S:5I2 

The following uses 64K bytes of memory and three files. Alloeates 512 b34es to RS-232 reeeive buffers 
and 128 bytes to transmit buffers, and loads and exeeutes the program file tty.has: 

A&gt;gwbasic TTY /C:512 

For more information about RS-232 Communieations, see Appendix F . 

Redirection of Standard Inpnt and Outpnt 

When redireeted, all input, line input, input$, and inkey$ statements are read from the speeified 
input file instead of the keyboard. 

All PRINT statements write to the speeified output file instead of the sereen. 

Error messages go to standard output and to the sereen. 

File input from kybd : is still read from the keyboard. 

File output to SCRN : still outputs to the sereen. 

GW-BASIC eontinues to trap keys when the ON key (n) statement is used. 

Typing ctrl-break when output is redireeted eauses GW-BASIC to elose any open files, issue the 
message "Break in line nnnn" to Standard output, exit GW-BASIC, and return to MS-DOS. 

When input is redireeted, GW-BASIC eontinues to read from this souree until a ctrl-z is deteeted. This 
eondition ean be tested with the end-of-file (EOF) fimetion. If the file is not terminated by a ctrl-z, or if 
a GW-BASIC file input statement tries to read past the end of file, then any open files are elosed, and 
GW-BASIC returns to MS-DOS. 





For further information about these statements and other statements, funetions, eommands, and 
variables mentioned in this text, refer to the GW-BASIC User's Reference. 

Some examples of redireetion follow. 

GWBASIC MYPROG &gt;DATA.OUT 

Data read by the input and line input statements eontinues to eome from the keyboard. Data 
output by the print statement goes into the data.out file. 

gwbasic MYPROG &lt;DATA.IN 

Data read by the input and line input statements eomes from data.in. Data output by print 
eontinues to go to the sereen. 

gwbasic MYPROG &lt;MYINPUT.DAT &gt;MYOUTPUT.DAT 

Data read by the input and line input statements now eome from the file myinput.dat, and data 
output by the print statements goes into myoutput.dat. 

gwbasic MYPROG &lt;\SALES\JOHN\TRANS.DAT »\SALES\SALES.DAT 

Data read by the input and line input statements now eomes from the file \sales\john\trans.dat. 
Data output by the print statement is appended to the file \sales\sales.dat. 


2.4 GW-BASIC Statements, Functions, Commands, and Variables 

A GW-BASIC program is made up of several elements: ke 3 ^ords, eommands, statements, funetions, 
and variables. 

2.4.1 Keywords 

GW-BASIC keywords, sueh as print, goto, and return have speeial signifieanee for the GW¬ 
BASIC Interpreter. GW-BASIC interprets keywords as part of statements or eommands. 

Keywords are also ealled reserved words. They eannot be used as variable names, or the system will 
interpret them as eommands. However, ke 5 Avords may be embedded within variable names. 

Keywords are stored in the system as tokens (1- or 2-byte eharaeters) for the most effieient use of 
memory spaee. 

2.4.2 Commands 

Commands and statements are both exeeutable instruetions. The differenee between eommands and 
statements is that eommands are generally exeeuted in the direet mode, or eommand level of the 
interpreter. They usually perform some type of program maintenanee sueh as editing, loading, or saving 
programs. When GW-BASIC is invoked and the GW-BASIC prompt. Ok, appears, the system assumes 
eommand level. 

2.4.3 Statements 

A statement, sueh as on error . . . goto, is a group of GW-BASIC ke 3 ^ords generally used in GW¬ 
BASIC program lines as part of a program. When the program is run, statements are exeeuted when, 
and as, they appear. 


2.4.4 Functions 

The GW-BASIC Interpreter performs both numerie and string funetions. 

2.4.4.1 Numeric Functions 

The GW-BASIC Interpreter ean perform certain mathematical (arithmetical or algebraic) calculations. 
For example, it calculates the sine (sin), cosine (cos), or tangent (tan) of angle x. 

Unless otherwise indicated, only integer and single-precision results are returned by numeric functions. 

2.4.4.2 String Functions 

String functions operate on strings. For example, TIME$ and date$ return the time and date known by 
the system. If the current time and date are entered during system start up, the correct time and date are 
given (the internal clock in the computer keeps track). 

2.4.4.3 Uscr-Dcfincd Functions 

Functions can be user-defined by means of the dee fn statement. These functions can be either string 
or numeric. 

2.4.5 Variables 

Certain groups of alphanumeric characters are assigned values and are called variables. When variables 
are built into the GW-BASIC program they provide information as they are executed. 

For example, err defines the latest error which occurred in the program; ERL gives the location of that 
error. Variables can also be defined and/or redefined by the user or by program content. 

All GW-BASIC commands, statements, functions, and variables are individually described in the GW- 
BASIC User's Reference. 

2.5 Line Format 

Each of the elements of GW-BASIC can make up sections of a program that are called statements. 
These statements are very similar to sentences in English. Statements are then put together in a logical 
manner to create programs. The GW-BASIC User's Reference describes all of the statements available 
for use in GW-BASIC. 

In a GW-BASIC program, lines have the following format: 

nnnnn statement[statements] 
nnnnn is a line number 
statement is a GW-BASIC statement. 

A GW-BASIC program line always begins with a line number and must contain at least one character, 
but no more than 255 characters. Eine numbers indicate the order in which the program lines are stored 
in memory, and are also used as references when branching and editing. The program line ends when 
you press the return key. 

Depending on the logic of your program, there may be more than one statement on a line. If so, each 
must be separated by a colon ( : ). Each of the lines in a program should be preceded by a line number. 
This number may be any whole integer from 0 to 65529. It is customary to use line numbers such as 10, 
20, 30, and 40, in order to leave room for any additional lines that you may wish to include later. Since 
the computer will run the statements in numerical order, additional lines needn't appear in consecutive 
order on the screen: for example, if you entered line 35 after line 60, the computer would still run line 


35 after line 30 and before line 40. This teehnique may save your reentering an entire program in order 
to inelude one line that you have forgotten. 

The width of your sereen is 80 eharaeters. If your statement exeeeds this width, the eursor will wrap to 
the next sereen line automatieally. Only when you press the return key will the eomputer aeknowledge 
the end of the line. Resist the temptation to press return as you approaeh the edge of the sereen (or 
beyond). The eomputer will automatieally wrap the line for you. You ean also press ctrl-return, whieh 
eauses the eursor to move to the beginning of the next sereen line without actually entering the line. 
When you press return, the entire logical line is passed to GW-BASIC for storage in the program. 

In GW-BASIC, any line of text that begins with a numeric character is considered a program line and is 
processed in one of three ways after the return key is pressed: 

• A new line is added to the program. This occurs if the line number is legal (within the range of 0 
through 65529), and if at least one alpha or special character follows the line number in the line. 

• An existing line is modified. This occurs if the line number matches the line number of an 
existing line in the program. The existing line is replaced with the text of the newly-entered line. 
This process is called editing. 


Note 

Reuse of an existing line number causes all of the information contained in the original line to be 
lost. Be careful when entering numbers in the indirect mode. You may erase some program lines 
by accident. 


• An existing line is deleted. This occurs if the line number matches the line number of an existing 
line, and the entered line contains only a line number. If an attempt is made to delete a nonexistent 
line, an "Undefined line number" error message is displayed. 

2.6 Returning to MS-DOS 

Before you return to MS-DOS, you must save the work you have entered under GW-BASIC, or the 
work will be lost. 

To return to MS-DOS, type the following after the Ok prompt, and press return: 

system 

The system returns to MS-DOS, and the A&gt; prompt appears on your screen. 




