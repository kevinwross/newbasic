<!-- Chunked from GWBASIC.txt, lines 1790-2110. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Guide - Chapter 4: The Screen Editor

Chapter 4 

The GW-BASIC Screen Editor 

You can edit GW-BASIC program lines as you enter them, or after they have been saved in a program 
fde. 

4.1 Editing Lines in New Files 

If an ineorreet eharaeter is entered as a line is being typed, it ean be deleted with the backspace or 
DEL keys, or with ctrl-h. After the eharaeter is deleted, you ean eontinue to type on the line. 

The ESC key lets you delete a line from the sereen that is in the proeess of being typed. In other words, 
if you have not pressed the return key, and you wish to delete the eurrent line of entry, press the esc 
key. 

To delete the entire program eurrently residing in memory, enter the NEW eommand. NEW is usually 
used to clear memory prior to entering a new program. 

4.2 Editing Lines in Saved Files 

After you have entered your GW-BASIC program and saved it, you may discover that you need to 
make some ehanges. To make these modifications, use the list statement to display the program 
lines that are aflfeeted; 

1. Reload the program. 

2. Type the LIST eommand, or press the Fi key. 

3. Type the line number, or range of numbers, to be edited. 

The lines will appear on your sereen. 

4,2,1 Editing the Information in a Program Line 

You can make ehanges to the information in a line by positioning the eursor where the ehange is to be 
made, and by doing one of the following; 

• Typing over the eharacters that are already there. 

• Deleting eharaeters to the left of the eursor, using the backspace key. 

• Deleting characters at the cursor position using the del key on the number pad. 

• Inserting characters at the eursor position by pressing the ins key on the number pad. This 
moves the characters following the cursor to the right making room for the new information. 

• Adding to or truncating characters at the end of the program line. 

If you have ehanged more than one line, be sure to press return on each modified line. The modified 
lines will be stored in the proper numerieal sequenee, even if the lines are not updated in numerieal 
order. 


Note 



A program line will not actually have changes recorded within the GW-BASIC program until the 
RETURN key is pressed with the cursor positioned somewhere on the edited line. 


You do not have to move the cursor to the end of the line before pressing the return key. The GW- 
BASIC Interpreter remembers where each line ends, and transfers the whole line, even if return is 
pressed while the cursor is located in the middle or at the beginning of the line. 

To truncate, or cut off, a line at the current cursor position, type ctrl-end or ctrl-e, followed by 
pressing the return key. 

If you have originally saved your program to a program file, make sure that you save the edited 
version of your program. If you do not do this, your modifications will not be recorded. 


4.3 Special Keys 

The GW-BASIC Interpreter recognizes nine of the numeric keys on the right side of your keyboard. It 
also recognizes the backspace key, esc key, and the ctrl key. The following keys and key sequences 
have special functions in GW-BASIC: 


Key Function 


BACKSPACE 
or CTRL-H 


Deletes the last character typed, or deletes the character to the left of the cursor. 
All characters to the right of the cursor are moved left one position. Subsequent 
characters and lines within the current logical line are moved up as with the del 
key. 


CTRL-BREAK 
or CTRL-C 


Returns to the direct mode, without saving changes made to the current line. It 
will also exit auto line-numbering mode. 


CTRL-CURSOR Movcs the cursor to the beginning of the previous word. The previous word is 
LEFT or CTRL- defined as the next character to the left of the cursor in the set A to Z or in the set 
® 0 to 9. 


Moves the cursor to the beginning of the next word. The next word is defined as 
the next character to the right of the cursor in the set A to Z or in the set 0 to 9. In 
other words, the cursor moves to the next number or letter after a blank or other 
special character. 

CURSOR 

DOWN or Moves the cursor down one line on the screen. 

CTRL- 


CTRL-CURSOR 
RIGHT or 
CTRL-F 


CURSOR LEFT Moves the cursor one position left. When the cursor is advanced beyond the left 
or CTRL-] gdge of the screen, it will wrap to the right side of the screen on the preceding 
line. 


CURSOR 
RIGHT or 
CTRL-\ 


Moves the cursor one position right. When the cursor is advanced beyond the 
right edge of the screen, it will wrap to the left side of the screen on the following 
line. 


CURSOR UP or 
CTRL-6 


Moves the cursor up one line on the screen. 




CTRL- 

BACKSPACE 
or DEL 


CTRL-END or 
CTRL-E 

CTRL-N or 
END 

CTRL-RETURN 
or CTRL-J 


CTRL-M or 
RETURN 

CTRL-[ or ESC 
CTRL-G 

CTRL-K or 
HOME 

CTRL-HOME 
or CTRL-L 

CTRL-R or INS 


Deletes the character positioned over the cursor. All characters to the right of 
the one deleted are then moved one position left to fill in where the deletion 
was made.If a logical line extends beyond one physical line, characters on 
subsequent lines are moved left one position to fill in the previous space, and 
the character in the first column of each subsequent line is moved up to the end 
of the preceding line. del (delete) is the opposite of ins (insert). Deleting text 
reduces logical line length. 

Erases from the cursor position to the end of the logical line. All physical 
screen lines are erased until the terminating return is found. 

Moves the cursor to the end of the logical line. Characters typed from this 
position are added to the line. 

Moves the cursor to the beginning of the next screen line. This lets you create 
logical program lines which are longer than the physical screen width. Logical 
lines may be up to 255 characters long. This function may also be used as a line 
feed. 

Enters a line into the GW-BASIC program. It also moves the cursor to the next 
logical line. 

Erases the entire logical line on which the cursor is located. 

Causes a beep to emit from your computer's speaker. 

Moves the cursor to the upper left corner of the screen. The screen contents are 
unchanged. 

Clears the screen and positions the cursor in the upper left corner of the screen. 

Turns the Insert Mode on and off. Insert Mode is indicated by the cursor 
blotting the lower half of the character position. In Graphics Mode, the normal 
cursor covers the whole character position. When Insert Mode is active, only 
the lower half of the character position is blanked by the cursor. 

When Insert Mode is off, characters typed replace existing characters on the 
line. The spacebar erases the character at the current cursor position and moves 
the cursor one character to the right. The cursor-right key moves the cursor 
one character to the right, but does not delete the character. 

When Insert Mode is off, pressing the tab key moves the cursor over characters 
until the next tab stop is reached. Tab stops occur every eight character 
positions. 

When Insert Mode is on, characters following the cursor are moved to the right 
as typed characters are inserted before them at the current cursor position. After 
each keystroke, the cursor moves one position to the right. Line wrapping is 
observed. That is, as characters move off the right side of the screen, they are 
inserted from the left on subsequent lines. Insertions increase logical line 
length. 

When Insert Mode is on, pressing the tab key causes blanks to be inserted from 
current cursor position to the next tab stop. Line wrapping is observed as above. 



Places the computer in a pause state. To resume operation, press any other 
key. 

Causes characters printed on the screen to echo to the line printer (Iptl:). In 
other words, you will be printing what you type on the screen. Pressing ctrl- 
PRTSC a seeond time turns off the echoing of characters to Iptl:. 

Sends the current screen contents to the printer, effectively creating a 
snapshot of the screen. 

Moves the cursor to the next tab stop. Tab stops oecur every eight columns. 

4.4 Function Keys 

Certain keys or combinations of keys let you perform frequently-used commands or functions with a 
minimum number of keystrokes. These keys are ealled function keys. 

The special function keys that appear on the left side of your keyboard can be temporarily redefined 
to meet the programming requirements and specific functions that your program may require. 

Function keys allow rapid entry of as many as 15 characters into a program with one keystroke. These 
keys are located on the left side of your keyboard and are labeled Fi through Fio. GW-BASIC has 
already assigned special functions to each of these keys. You will notice that after you load GW- 
BASIC, these special key functions appear on the bottom line of your screen. These key assignments 
have been selected for you as some of the most frequently used commands. 

Initially, the function keys are assigned the following special functions; 

Table 4,1 

GW-BASIC Function Key Assignments 


KeyFunction 

KeyFunction 

FI LIST 

F6 ,"LPTl:"r 

F2 RUNf 

F7 TRONf 

F3 LOAD" 

F8 TROEFf 

F4 SAVE" 

F9 KEY 

F5 CONTf 

FIO SCREEN 0,0, of 

Note 


The ( following a function indicates that you needn't press the return key after the funetion key. The 
selected command will be immediately executed. 


CTRL-NUM 
LOCK or 
CTRL-S 

CTRL-PRTSC 


SHIFT + PRTSC 


CTRL-I or TAB 


If you choose, you may change the assignments of these keys. Any one or all of the 10 function keys 
may be redefined. For more information, see the KEY and ON KEY statements in the GW-BASIC 
User's Reference. 






