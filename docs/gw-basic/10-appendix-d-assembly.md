<!-- Chunked from GWBASIC.txt, lines 5115-5878. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# Appendix D: Assembly Language (Machine Code) Subroutines

Appendix D 

Assembly Language (Machine Code) Subroutines 

This appendix is written primarily for users experieneed in assembly language programming. 

GW-BASIC lets you interface with assembly language subroutines by using the USR function and the 
CALL statement. 

The USR function allows assembly language subroutines to be called in the same way GW-BASIC 
intrinsic functions are called. However, the CALL statement is recommended for interfacing machine 
language programs with GW-BASIC. The CALL statement is compatible with more languages than 
the USR function call, produces more readable source code, and can pass multiple arguments. 

D.l Memory Allocation 

Memory space must be set aside for an assembly language (or machine code) subroutine before it can 
be loaded. There are three recommended ways to set aside space for assembly language routines: 

• Specify an array and use varptr to locate the start of the array before every access. 

• Use the /m switch in the command line. Get GW-BASIC's Data segment (DS), and add the size 
of DS to reference the reserved space above the data segment. 

• Execute a .COM file that stays resident, and store a pointer to it in an unused interrupt vector 
location. 

• There are three recommended ways to load assembly language routines: 

• BLOAD the file. Use DEBUG to load in an .EXE file that is in high memory, run GW-BASIC, 
and BSAVE the .EXE file. 

• Execute a .COM file that contains the routines. Save the pointer to these routines in unused 
interrupt-vector locations, so that your application in GW-BASIC can get the pointer and use 
the routine(s). 

• Place the routine into the specified area. 

If, when an assembly language subroutine is called, more stack space is needed, GW-BASIC stack 
space can be saved, and a new stack set up for use by the assembly language subroutine. The GW- 
BASIC stack space must be restored, however, before returning from the subroutine. 

D.2 CALL Statement 

CALL variablename[(arguments)] 

variablename contains the offset in the current segment of the subroutine being called. 

arguments are the variables or constants, separated by commas, that are to be passed to the routine. 

Eor each parameter in arguments, the 2-byte offset of the parameter's location within the data 
segment (DS) is pushed onto the stack. 

The GW-BASIC return address code segment (CS), and offset (IP) are pushed onto the stack. 

A long call to the segment address given in the last DEE SEG statement and the offset given in 
variablename transfers control to the user's routine. 


The stack segment (SS), data segment (DS), extra segment (ES), and the stack pointer (SP) must be 
preserved. 

Figure D.l shows the state of the stack at the time of the call statement: 

Figure 1 

Figure D.l Stack Layout When the CALL Statement is Activated not shown 

The user's routine now has control. Parameters may be referenced by moving the stack pointer (SP) to 
the base pointer (BP) and adding a positive offset to BP. 

Upon entry, the segment registers DS, ES, and SS all point to the address of the segment that contains 
the GW-BASIC interpreter code. The code segment register CS contains the latest value supplied by 
DEF SEG. If no DEF SEG has been specified, it then points to the same address as DS, ES, and SS 
(the default def seg). 

Figure D.2 shows the condition of the stack during execution of the called subroutine: 

Figure 2 

Figure D.2 Stack Layout During Execution of a CALL Statement not shown 

The following seven rules must be observed when coding a subroutine: 

1. The called routine may destroy the contents of the AX, BX, CX, DX, SI, DI, and BP registers. 
They do not require restoration upon return to GW-BASIC. However, all segment registers and 
the stack pointer must be restored. Good programming practice dictates that interrupts enabled 
or disabled be restored to the state observed upon entry. 

2. The called program must know the number and length of the parame- ters passed. References to 
parameters are positive offsets added to BP, assuming the called routine moved the current 
stack pointer into BP; that is, MOV BP, SP. When 3 parameters are passed, the location of PO is 
at BP+IO, PI is at BP+S, and P2 is at BP-l-6. 

3. The called routine must do a return n (n is two times the number of parameters in the 
argument list) to adjust the stack to the start of the calling sequence. Also, programs must be 
defined by a PROG far statement. 

4. Values are returned to GW-BASIC by including in the argument list the variable name that 
receives the result. 

5. If the argument is a string, the parameter offset points to three bytes called the string descriptor. 
Byte 0 of the string descriptor contains the length of the string (0 to 255). Bytes 1 and 2, 
respectively, are the lower and upper eight bits of the string starting address in string space. 


Note 

The called routine must not change the contents of any of the three bytes of the string 
descriptor. 

6. Strings may be altered by user routines, but their length must not be changed. GW-BASIC 
cannot correctly manipulate strings if their lengths are modified by external routines. 

7. If the argument is a string literal in the program, the string descriptor points to program text. Be 
careful not to alter or destroy your program this way. To avoid unpredictable results, add + " " to 




the string literal in the program. For example, the following line forces the string literal to be 
copied into string space allocated outside of program memory space; 

20 A$="BASIC"+"" 

The string can then be modified without affecting the program. 


Examples: 

100 DEF SEG=&amp;H2000 

110 ACC=&amp;H7FA 

120 CALL ACC(A,B$, C) 


Line 100 sets the segment to 2000 hex. The value of variable ACC is added into the address as the 
low word after the DEF SEC value is left-shifted four bits (this is a function of the microprocessor, not 
of GW-BASIC). Here, ACC is set to &amp;H7FA, so that the call to ACC executes the subroutine at 
location 2000;7FA hex. 

Upon entry, only 16 bytes (eight words) remain available within the allocated stack space. If the 
called program requires additional stack space, then the user program must reset the stack pointer to a 
new allocated space. Be sure to restore the stack pointer adjusted to the start of the calling sequence 
on return to GW-BASIC. 

The following assembly language sequence demonstrates access of the parameters passed and storage 
of a return result in the variable C. 


Note 

The called program must know the variable type for numeric parameters passed. In these examples, 
the following instruction copies only two bytes: 

MOVSW 

This is adequate if variables A and C are integer. It would be necessary to copy four bytes if they were 
single precision, or copy eight bytes if they were double precision. 


MOV BP, S P Gets the current stack position in BP 

MOV BX, 8 [BP] Gets the address of B$ description 

MOV CL, [BX] Gets the length of B$ in CL 

MOV DX, 1 [BX] Gets the address of B$ string descriptor in DX 

MOV s 1 ,10 [ BP ] Gets the address of A in SI 

MOV Dl,6[BP] Gets the pointer to C in DI 

MOVSW Stores variable A in 'C 

RET 6 Restores stack; returns 


D.3 USR Function Calls 

Although the CALL statement is the recommended way of calling assembly language subroutines, the 
USR function call is still available for compatibility with previously-written programs. 




Syntax: 

USR[r!] {argument) 

n is a number from 0 to 9 which specifies the USR routine being called (see DBF USR statement). If n 
is omitted, USRO is assumed. 

argument is any numeric or string expression. 

In GW-BASIC a def seg statement should be executed prior to a USR function call to ensure that the 
code segment points to the subroutine being called. The segment address given in the def seg 
statement determines the starting segment of the subroutine. 

For each USR function call, a corresponding def USR statement must have been executed to define 
the USR function call offset. This offset and the currently active def SEG address determine the 
starting address of the subroutine. 

When the USR function call is made, register AF contains the number type flag (NTF), which specifies 
the type of argument given. The NTF value may be one of the following: 

NTF ValueSpecifies 

2 a two-byte integer (two's complement format) 

3 a string 

4 a single-precision floating point number 

8 a double-precision floating point number 

If the argument of a USR function call is a number (AF&lt;&gt;73), the value of the argument is placed in 
the floating-point accumulator (FAC). The FAC is 8 bytes long and is in the GW-BASIC data 
segment. Register BX will point at the fifth byte of the FAC. Figure D.3 shows the representation of 
all the GW-BASIC number types in the FAC: 

Figure 3 

Figure D.3 Number Types in the Floating Point Accumulator not shown 

If the argument is a single-precision floating-point number: 

• BX+3 is the exponent, minus 128. The binary point is to the left of the most significant bit of 
the mantissa. 

• BX+2 contains the highest seven bits of mantissa with leading 1 suppressed (implied). Bit 7 is 
the sign of the number (0=positive, l=negative). 

• BX+1 contains the middle 8 bits of the mantissa. 

• BX+0 contains the lowest 8 bits of the mantissa. 

If the argument is an integer: 

• BX+I contains the upper eight bits of the argument. 

• BX+0 contains the lower eight bits of the argument. 

If the argument is a double-precision floating-point number: 

• BX+0 through BX+3 are the same as for single precision floating point. 

• BX-1 to BX-4 contain four more bytes of mantissa. BX-4 contains the lowest eight bits of the 
mantissa. 



If the argument is a string (indieated by the value 3 stored in the AL register) the (DX) register pair 
points to three bytes ealled the string descriptor. Byte 0 of the string descriptor contains the length of 
the string (0 to 255). Bytes 1 and 2, respectively, are the lower- and upper-eight bits of the string 
starting address in the GW-BASIC data segment. 

If the argument is a string literal in the program, the string descriptor points to program text. Be 
careful not to alter or destroy programs this way (see the preceding CALL statement). 

Usually, the value returned by a USR function call is the same type (integer, string, single precision, or 
double precision) as the argument that was passed to it. The registers that must be preserved are the 
same as in the call statement. 

A far return is required to exit the USR subroutine. The returned value must be stored in the FAC. 


D.4 Programs That Call Assembly Language Programs 

This section contains two sample GW-BASIC programs that 

• load an assembly language routine to add two numbers together 

• return the sum into memory 

• remain resident in memory 

The code segment and offset to the first routine is stored in interrupt vector at 0:1 OOH. 

Example 1 calls an assembly language subroutine: 

Example 1 

10 DEF SEG=0 

10 0 CS=PEEK(&amp;H102)+PEEK(&amp;H103) *2 5 6 

200 OFFSET=PEEK(&amp;H100)+PEEK(&amp;H101) *256 

250 DEF SEC 

300 C1%=2:C2%=3:C3%=0 

400 TWOSUM=OFFSET 

500 DEF SEG=CS 

600 CALL TWOSUM(Cl%,C2%,C3%) 

700 PRINT C3% 

800 END 

The assembly language subroutine called in the above program must be assembled, linked, and 
converted to a .COM file. The program, when executed prior to the running of the GW-BASIC 
program, will remain in memory until the system power is turned off, or the system is rebooted. 


0100 




org lOOH 




0100 




double segment 








assume cs:double 




0100 

EB 

17 

90 

start: jmp startl 




0103 




usrprg proc far 




0103 

55 



push bp 




0104 

8B 

EC 


mov bp,sp 




0106 

8B 

76 

08 

mov si,[bp]+8 

; get 

address of 

parameter b 

0109 

8B 

04 


mov ax,[si] 

; get 

value of b 


OlOB 

8B 

76 

OA 

mov si,[bp]+10 

; get 

address of 

parameter a 


a 


OlOE 

03 

04 


add ax,[si] 


;add value of a to value of b 

0110 

8B 

7E 

06 

mov di,[bp]+6 


;get address of parameter c 

0113 

89 

05 


mov di,ax 


;store sum in parameter c 

0115 

5D 



pop bp 



0116 

ca 

00 

06 

ret 6 



0119 




usrprg endp 


;Program to put procedure in 
;memory and remain resident. The 
;offset and segment are stored 
;in location 100-103H. 

0119 




startl: 



0119 

B8 

00 

00 

mov ax,0 



one 

8E 

D8 


mov ds,ax 


;data segment to OOOOH 

OllE 

BB 

01 

00 

mov bx,010OH 


;pointer to int vector lOOH 

0121 

83 

7F 

02 0 

emp word ptr 

[bx],0 


0125 

75 

16 


jne quit 


;program already run, exit 

0127 

83 

3F 

00 

emp word ptr2 

[bx],0 


012A 

75 

11 


jne quit 


;program already run exit 

012C 

B8 

01 

03 R 

mov ax,offset 

usrprg 


012F 

89 

07 


mov [bx],ax 


;program offset 

0131 

8C 

c8 


mov ax,cs 



0133 

89 

47 

02 

mov [bx+2],ax 


;data segment 

0136 

OE 



push cs 



0137 

IF 



pop ds 



0138 

BA 

0141 R 

mov dx,offset 

veryend 


013B 

CD 

27 


int 27h 



013D 




quit: 



013D 

CD 

20 


int 20h 



013F 




veryend: 



013F 




double ends 




end start 


Example 2 places the assembly language subroutine in the specified area; 

Example 2 

10 I=0:JC=0 
100 DIM A%(23) 

150 MEM%=VARPTR(A%(1) ) 

200 FOR 1=1 TO 23 
300 READ JC 
400 POKE MEM%,JC 
450 MEM%=MEM%+1 
500 NEXT 

600 C1%=2:C2%=3:C3%=0 
700 TWOSUM=VARPTR(A%(1)) 

800 CALL TWOSUM(Cl%, C2%, C3%) 

900 PRINT C3% 

950 END 

1000 DATA &amp;H55,&amp;H8b,&amp;Hec &amp;H8b,&amp;H76,&amp;H08,&amp;H8b,&amp;H04,&amp;H8b,&amp;H76 
1100 DATA &amp;H0a,&amp;H03,&amp;H04,&amp;H8b,&amp;H7e,&amp;H06,&amp;H89,&amp;H05,&amp;H5d 
1200 DATA &amp;Hca,&amp;H06,&amp;H00 


