<!-- Chunked from GWBASIC.txt, lines 1-1077. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# Front matter, table of contents, and manual introduction

GW-BASIC User's Reference 

Microsoft Corporation 

Information in this document is subject to change without notice and does not represent a 
commitment on the part of Microsoft Corporation. The software described in this document is 
furnished under a license agreement or nondisclosure agreement. The software may be used or copied 
only in accordance with the terms of the agreement. It is against the law to copy this software on 
magnetic tape, disk, or any other medium for any purpose other than the purchaser's personal use. 

© Copyright Microsoft Corporation, 1986, 1987. All rights reserved. 

Portions copyright COMPAQ Computer Corporation, 1985 

Simultaneously published in the United States and Canada. 

Microsoft®, MS-DOS®, GW-BASIC®, and the Microsoft logo are registered trademarks of 
Microsoft Corporation. 

EGA® and IBM® are registered trademarks of International Business Machines Corporation. 
Document Number 410130013-330-R02-0787 


Introduction 

This manual is an alphabetical reference to GW-BASIC instructions: statements, functions, 
commands, and variables. 

The name and type of each instruction is listed at the top of the page, and is followed by: 




Purpose 

Syntax 

Comments 

Examples 

Notes 


The purpose of the instruction 

The complete notation of the instruction 

Pertinent information about the instruction, and what happens when it 
encountered by GW-BASIC 

An illustration of the instruction as it might appear in a program 
Any special information about the instruction 



User’s Guide 


Chapters 

1. Welcome-&gt; PDF page 8 


2. Getting Started-&gt; PDF page 10 


3. Reviewing and Practicing- 

-&gt; pDp cage 16 

4. The Screen Editor- 

&gt; PDF nage 20 

5. Creating and Using Files- 

-&gt; pDp page 24 


6. Constants. Variables. Expressions and Operators-&gt; PDF page 30 

Appendicies 


A. Error Codes and Messages-&gt; PDF page 40 


B. Mathematical Functions — 

-&gt; pp)p page 45 

C. ASCII Character Codes — 

-&gt; pp)p page 46 

D. Assembly Language- 

-&gt; pdf nage 48 

E. Converting Programs- 

-&gt; pdf nage 54 

F. Communications- 

-&gt; PDF nage 56 

G. Hexadecimal Eauivalents - 

-&gt; pdf nage 61 

H. Key Scan Codes- 

-&gt; PDF nape 62 

I. Characters Recognized- 

-&gt; pdf nage 63 


Glossary-&gt; PDF page 64 


User’s Reference 

ABS Function- 

ASC Function- 

ATN Function- 

AUTO Command — 


BEEP Statement — 

BLOAD Command 

BSAVE Command - 


CALL Statement- 

CDBL Function- 

CHAIN Statement — 

CHDIR Command — 

CHR$ Function- 

CINT Function- 

CIRCLE Statement — 

CLEAR Command — 

CLOSE Statement — 

CLS Statement- 

COLOR Statement — 

COM(n) Statement — 

COMMON Statement 

CONT Command- 

COS Function- 

CSNG Function- 

CSRLIN Variable —- 

CVD Function- 

CVI Function- 

CVS Function- 


-&gt; PDF page 79 

-&gt; PDF page 80 

-&gt; PDF page 81 
-&gt; PDF page 82 

—-&gt; PDF page 83 
-&gt; PDF page 84 

-&gt; PDF page 85 

—&gt; PDF page 86 

—&gt; PDF page 89 
-&gt; PDF page 90 

-&gt; PDF page 92 

—&gt; PDF page 93 

—&gt; PDF page 94 
-&gt; PDF page 95 

-&gt; PDF page 97 

-&gt; PDF page 98 

-&gt; PDF page 99 

-&gt; PDF page 100 

-&gt; PDF page 102 

-&gt; PDF page 103 

-&gt; PDF page 104 

-&gt; PDF page 105 

—&gt; PDF page 106 

-&gt; PDF page 107 

-&gt; PDF page 108 

■&gt; PDF page 108 

-&gt; PDF page 108 

























































































DATA Statement-&gt; PDF page 109 

DATES Statement-&gt; PDF page 111 

DEF FN Statement-&gt; PDF page 112 

DEFINT Statement-&gt; PDF page 113 

DEFDBL Statement-&gt; PDF page 113 

DEFSNG Statement-&gt; PDF page 113 

DEFSTR Statement-&gt; PDF page 113 

DEF SEG Statement-&gt; PDF page 114 

DEF USR Statement-&gt; PDF page 115 

DELETE Command-&gt; PDF page 116 

DIM Statement-&gt; PDF page 117 

DRAW Statement-&gt; PDF page 118 


EDIT Command-&gt; PDF page 121 

END Statement-&gt; PDF page 122 

ENVIRON Statement-&gt; PDF page 123 

ENVIRONS Function-&gt; PDF page 125 

EOF Function-&gt; PDF page 127 

ERASE Statement-&gt; PDF page 128 

ERDEVrSl Variables-&gt; PDF page 129 

ERL Variable-&gt; PDF page 130 

ERR Variable-&gt; PDF page 130 

ERROR Statement-&gt; PDF page 131 

EXP Function-&gt; PDF page 133 

EXTERR Function-&gt; PDF page 134 


FIELD Statement-&gt; PDF page 135 

FILES Command-&gt; PDF page 136 

FIX Function-&gt; PDF page 137 

FOR Statement-&gt; PDF page 138 

FRE Function-&gt; PDF page 140 

GET Statement (Filesl-&gt; PDF page 141 

GET Statement (Graphicsl-&gt; PDF page 142 

GOSUB-RETURN Statement-&gt; PDF page 144 

GOTO Statement-&gt; PDF page 145 

HEXS Function-&gt; PDF page 146 


IF Statement-&gt; PDF page 147 


INKEYS Variable —- 

-&gt; PDF nase 149 

INP Function- 

-—&gt; PDF naue 150 

INPUT Statement- 

-&gt; PDF nage 151 

INPUT# Statement — 

-&gt; pdf nage 153 

INPUTS Function- 

-&gt; pdf nage 154 

INSTR Function- 

-&gt; pdf nage 155 

INT Function- 

—&gt; PDF nage 156 

lOCTL Statement- 

-&gt; pdf nage 157 

lOCTLS Function- 

-&gt; pdf nage 158 


KEY Statement-&gt; PDF page 159 


KEYlnl Statement — 

-&gt; PDF nage 161 

KILL Command- 

-&gt; pdf nage 162 

LEFTS Function- 

-&gt; pdf nage 163 

LEN Function- 

-&gt; pdf nage 164 

LET Statement- 

-&gt; pdf nage 165 

LINE Statement- 

-&gt; pdf nage 166 








































































































LINE INPUT Statement-&gt; PDF page 168 

LINE INPUT# Statement-&gt; PDF page 169 

LIST Command-&gt; PDF page 170 

LLIST Command-&gt; PDF page 171 

LOAD Command-&gt; PDF page 172 

LOG Function-&gt; PDF page 173 

LOCATE Statement-&gt; PDF page 174 

LOCK Statement-&gt; PDF page 175 

LOF Function-&gt; PDF page 177 

LOG Function-&gt; PDF page 178 

LPOS Function-&gt; PDF page 179 

LPRINT Statement-&gt; PDF page 180 

LPRINT USING Statement-&gt; PDF page 180 

LSET Statement-&gt; PDF page 181 


MERGE Command-&gt; PDF page 182 

MIPS Function-&gt; PDF page 183 

MIPS Statement-&gt; PDF page 184 

MKDIR Command-&gt; PDF page 185 

MKD$ Function-&gt; PDF page 186 

MKI$ Function-&gt; PDF page 186 

MKS$ Function-&gt; PDF page 186 

NAME Command-&gt; PDF page 187 

NEXT Statement-&gt; PDF page 138 

NEW Command-&gt; PDF page 188 


OCT$ Function-&gt; PDF page 189 

ON COMlnl Statement-&gt; PDF page 190 

ON KEYlnl Statement-&gt; PDF page 190 

ON PEN Statement-&gt; PDF page 190 

ON PLAYlnl Statement-&gt; PDF page 190 

ON STRIGlnl Statement-&gt; PDF page 190 

ON TIMERlnl Statement-&gt; PDF page 190 

ON ERROR GOTO Statement-&gt; PDF page 194 

ON-GOSUB Statement-&gt; PDF page 195 

ON-GOTO Statement-&gt; PDF page 195 

OPEN Statement-&gt; PDF page 196 

OPEN "COMlnl Statement-&gt; PDF page 200 

OPTION BASE Statement-&gt; PDF page 201 

OUT Statement-&gt; PDF page 202 


PAINT Statement- 

-&gt; pdf page 203 

PALETTE Statement- 

-&gt; PDF page 205 

PALETTE USING Statement-&gt; PDF nase 205 

PCOPY Command- 

-&gt; pdf nage 208 

PEEK Function- 

—&gt; PDF nage 209 

PEN Statement and Function-&gt; PDF nage 210 

PLAY Statement- 

-—&gt; PDF nage 212 

PLAYfnl Function- 

-&gt; pdf nage 214 

PMAP Function- 

—&gt; PDF page 215 

POINT Function- 

—&gt; PDF page 216 

POKE Statement- 

—&gt; PDF page 217 

POS Function- 

&gt; PDF nage 218 

PRESET Statement- 

-&gt; pdf nage 219 

PSET Statement- 

—&gt; PDF nage 219 

PRINT Statement- 

-&gt; PDF nage 220 

PRINT USING Statement 

-&gt; pdf nage 221 

PRINT# Statement- 

-&gt; pdf nage 224 
















































































































PRINT# USING Statement-&gt; PDF page 224 


PUT Statement tFilesI- 

-&gt; pdf naae 226 

PUT Statement tGranhicsI — 

-&gt; pdf naae 227 

RANDOMIZE Statement- 

-&gt; PDF naae 229 


READ Statement-&gt; PDF page 231 

REM Statement-&gt; PDF page 233 

RENUM Command-&gt; PDF page 234 

RESET Command-&gt; PDF page 235 

RESTORE Statement-&gt; PDF page 236 

RESUME Statement-&gt; PDF page 237 

RETURN Statement-&gt; PDF page 238 

RIGHTS Function-&gt; PDF page 239 

RMDIR Command-&gt; PDF page 240 

RND Function-&gt; PDF page 241 

RSET Statement-&gt; PDF page 181 

RUN Command-&gt; PDF page 242 


SAVE Command — 

SCREEN Function — 

SCREEN Statement - 

SGN Function- 

SHELL Statement — 

SIN Function- 

SOUND Statement - 

SPACES Function — 

SPC Function- 

SOR Function- 

STICK Function —- 

STOP Statement- 

STRS Function- 

STRIG Statement —■ 

STRIGfnI Statement 

STRINGS Function - 

SWAP Statement — 

SYSTEM Command 


-&gt; PDF page 243 

-&gt; pDp page 244 

-&gt; PDF page 245 

--&gt; PDF page 250 
-&gt; PDF page 251 

■&gt; PDF page 525 
-&gt; PDF page 253 

-&gt; PDF page 255 

-&gt; PDF page 256 

-&gt; PDF page 257 
—&gt; PDF page 258 

—&gt; PDF page 259 

--&gt; PDF page 260 

-&gt; PDF page 261 

-&gt; PDF page 262 

-&gt; PDF page 263 

-&gt; PDF page 264 

-&gt; PDF page 265 


TAB Function-&gt; PDF page 266 

TAN Function-&gt; PDF page 267 

TIMES Statement-&gt; PDF page 268 

TIMER Function-&gt; PDF page 270 

TROFF Command-&gt; PDF page 271 

TRON Command-&gt; PDF page 271 


UNLOCK Statement-&gt; PDF page 272 

USR Function-&gt; PDF page 274 


VAL Function-&gt; PDF page 275 

VARPTR Function-&gt; PDF page 276 

VARPTRS Function-&gt; PDF page 278 

VIEW Statement-&gt; PDF page 279 

VIEW PRINT Statement-&gt; PDF page 280 


WAIT Statement-&gt; PDF page 281 

WHILE-WEND Statement-&gt; PDF page 282 

WIDTH Statement-&gt; PDF page 283 

WINDOW Statement-&gt; PDF page 285 

WRITE Statement-&gt; PDF page 287 

WRITE# Statement-&gt; PDF page 288 













































































































