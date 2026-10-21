<!-- Chunked from GWBASIC.txt, lines 1560-1789. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Guide - Chapter 3: Reviewing and Practicing

Chapter 3 

Reviewing and Practicing GW-BASIC 

The practice sessions in this chapter will help you review what you have learned. If you have not done 
so, this is a good time to turn on your computer and load the GW-BASIC Interpreter. 

3.1 Example for the Direct Mode 

You can use your computer in the direct mode to perform fundamental arithmetic operations. GW- 
BASIC recognizes the following symbols as arithmetic operators; 

Operation GW-BASIC Operator 

Addition + 

Subtraction 

Multiplication* 

Division / 

To enter a problem, respond to the Ok prompt with a question mark (?), followed by the statement of 
the problem you want to solve, and press the return key. In GW-BASIC, the question mark can be 
used interchangeably with the keyword print. The answer is then displayed. 

Type the following and press the return key: 

?2 + 2 

GW-BASIC will display the answer on your screen; 

?2 + 2 
4 
Ok 

To practice other arithmetic operations, replace the + sign with the desired operator. 

The GW-BASIC language is not restricted to arithmetic functions. You can also enter complex 
algebraic and trigonometric functions. The formats for these functions are provided in Chapter 6, 
"Constants, Variables, Expressions and Operators." 

3.2 Examples for the Indirect Mode 

The GW-BASIC language can be used for functions other than simple algebraic calculations. You can 
create a program that performs a series of operations and then displays the answer. To begin 
programming, you create lines of instructions called statements. Remember that there can be more 
than one statement on a line, and that each line is preceded by a number. 

For example, to create the command print 2+3 as a statement, type the following: 

10 print 2+3 

When you press the return key, the cursor shifts to the next line, but nothing else happens. To make 
the computer perform the calculation, type the following and press the return key: 

run 

Your screen should look like this: 




Ok 

10 print 2+3 
run 
5 
Ok 

You have just written a program in GW-BASIC. 

The computer reserves its calculation until specifically commanded to continue (with the RUN 
command). This allows you to enter more lines of instruction. When you type the RUN command, the 
computer does the addition and displays the answer. 

The following program has two lines of instructions. Type it in; 

10 x=3 

20 print 2+x 

Now use the run command to have the computer calculate the answer. 

Your screen should look like this; 

Ok 

10 x=3 

20 print 2+x 
run 
5 
Ok 

The two features that distinguish a program from a calculation are 

1. The numbered lines 

2. The use of the run command 

These features let the computer know that all the statements have been typed and the computation can 
be carried out from beginning to end. It is the numbering of the lines that first signals the computer 
that this is a program, not a calculation, and that it must not do the actual computation until the RUN 
command is entered. 

In other words, calculations are done under the direct mode. Programs are written under the indirect 
mode. 

To display the entire program again, type the LIST command and press the return key; 

list 

Your screen should look like this; 

Ok 

10 x=3 

20 print 2+x 
run 
Ok 
5 
Ok 
list 
10 X=3 

20 PRINT 2+X 
Ok 


You'll notice a slight change in the program. The lowerease letters you entered have been eonverted 
into upperease letters. The list command makes this ehange automatically. 

3.3 Function Keys 

Function keys are keys that have been assigned to frequently-used commands. The ten function keys 
are loeated on the left side of your keyboard. A guide to these keys and their assigned commands 
appears on the bottom of the GW-BASIC screen. To save time and keystrokes, you can press a 
funetion key instead of typing a command name. 

For example, to list your program again, you needn't type the LIST command; you can use the 
funetion key assign to it, instead; 

• Press the fi key. 

• Press RETURN. 

Your program should appear on the screen. 

To run the program, simply press the F2 key, whieh is assigned to the RUN eommand. 

As you learn more commands, you'll learn how to use keys F3 through Fio. Chapter 4, "The GW- 
BASIC Screen Editor," eontains more information about keys used in GW-BASIC. 

3.4 Editing Lines 

There are two basie ways to change lines. You can 

• Delete and replaee them 

• Alter them with the edit eommand 

To delete a line, simply type the line number and press the return key. For example, if you type 12 
and press the return key, line number 12 is deleted from your program. 

To use the edit command, type the eommand edit, followed by the number of the line you want to 
change. For example, type the following, and press the return key: 

edit 10 

You can then use the following keys to perform editing; 

Key Function 

CURSOR UP, Moves the eursor within the statement 

CURSOR DOWN, 

CURSOR left, 

CURSOR right 

BACKSPACE Deletes the character to the left of the eursor 

delete (del) Deletes the current character 

insert (ins) Lets you insert characters to the left of the cursor. 

For example, to modify statement (line) 10 to read x=4, use the cursor-right control key to move the 
eursor under the 3, and then type a 4. The number 4 replaees the number 3 in the statement. 

Now press the return key, and then the F 2 key. 

Your screen displays the following: 




Ok 

10 X=4 
RUN 
6 
Ok 

3.5 Saving Your Program File 

Creating a program is like ereating a data file. The program is a file that eontains speeifie instructions, 
or statements, for the computer. In order to use the program again, you must save it, just as you would 
a data file. 

To save a fide in GW-BASIC, use the following procedure; 

1. Press the F4 key. The command word SAVE" appears on your screen. 

2. Type a name for the program, and press the return key. The file is saved under the name you 
specified. 

To recall a saved file, use the following procedure; 

1. Press the F3 key. The command load LOAD" appears on your screen. 

2. Type the name of the file. 

3. Press RETURN. 

The file is loaded into memory, and ready for you to list, edit, or run. 


