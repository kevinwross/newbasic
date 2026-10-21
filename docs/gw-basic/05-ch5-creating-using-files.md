<!-- Chunked from GWBASIC.txt, lines 2111-2784. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Guide - Chapter 5: Creating and Using Files

Chapter 5 

Creating and Using Files 

There are two types of files in MS-DOS systems; 

• Program files, which contain the program or instructions for the computer 

• Data files, which contain information used or created by program files 

5.1 Program File Commands 

The following are the commands and statements most frequently used with program files. The GW-BASIC User's Reference 
contains more information on each of them. 

SAVE filename[,a.] [,p] 

Writes to diskette the program currently residing in memory. 

LOAD filename[, r] 

Loads the program from a diskette into memory. LOAD deletes the current contents of memory and closes all files before loading the 
program. 

RUN filename[rr] 

Loads the program from a diskette into memory and runs it immediately. RUN deletes the current contents of memory and closes all 
files before loading the program. 

MERGE filename 

Loads the program from a diskette into memory, but does not delete the current program already in memory. 

KILL filename 

Deletes the file from a diskette. This command can also be used with data files. 

NAME old filename AS new filename 

Changes the name of a diskette file. Only the name of the file is changed. The file is not modified, and it remains in the same space 
and position on the disk. This command can also be used with data files. 


5.2 Data Files 

GW-BASIC programs can work with two types of data files; 

• Sequential files 

• Random access files 

Sequential files are easier to create than random access files, but are limited in flexibility and speed when accessing data. Data 
written to a sequential file is a series of ASCII characters. Data is stored, one item after another (sequentially), in the order sent. 
Data is read back in the same way. 

Creating and accessing random access files requires more program steps than sequential files, but random files require less room on 
the disk, because GW-BASIC stores them in a compressed format in the form of a string. 

The following sections discuss how to create and use these two types of data files. 


5.2.1 Creating a Sequential File 

The following statements and functions are used with sequential files; 

CLOSE LOF 

EOF OPEN 

INPUT# PRINT# 

LINE INPUT#PRINT# USING 
LOG UNLOCK 

LOCK WRITE# 

The following program steps are required to create a sequential file and access the data in the file; 

1. Open the file in output (o) mode. The current program will use this file first for output; 

OPEN "0", #1filename" 


2. Write data to the file using the PRINT# or WRITE# statement; 

PRINT#I,A$ 

PRINT#I,B$ 

PRINT#I,C$ 

3. To access the data in the file, you must close the file and reopen it in input (i) mode: 

CLOSE #I 

OPEN "I ", #I, " ifi Jena/ne 

4. Use the input# or line input# statement to read data from the sequential file into the program; 

INPUT#I,X$, Y$, Z$ 

Example 1 is a short program that creates a sequential file, data, from information input at the terminal. 


Example 1 


10 

OPEN 

"0",#1,"DATA" 


20 

INPUT 

"NAME";N$ 


30 

IF N$^ 

="DONE" THEN 

END 

40 

INPUT 

"DEPARTMENT" 

;D$ 

50 

INPUT 

"DATE HIRED" 

;H$ 

60 

PRINT#I,N$;","D$", 

";H$ 

70 

PRINT 

:GOTO 20 


RUN 




NAME? MICKEY MOUSE 
DEPARTMENT? AUDIO/VISUAL AIDS 
DATE HIRED? 01/12/72 
NAME? SHERLOCK HOLMES 
DEPARTMENT? RESEARCH 
DATE HIRED? 12/03/65 
NAME? EBENEEZER SCROOGE 
DEPARTMENT? ACCOUNTING 
DATE HIRED? 04/27/78 
NAME? SUPER MANN 
DEPARTMENT? MAINTENANCE 
DATE HIRED? 08/16/78 
NAME? DONE 
OK 

5.2.2 Accessing a Sequential File 

The program in Example 2 accesses the file data, created in the program in Example 1, and displays the name of everyone hired in 
1978. 

Example 2 

10 OPEN "I",#I,"DATA" 

20 INPUT#I,N$,D$,H$ 

30 IF RIGHT$(H$,2)="78" THEN PRINT N$ 

40 GOTO 20 
50 CLOSE #I 
RUN 

EBENEEZER SCROOGE 
SUPER MANN 
Input past end in 20 
Ok 

The program in Example 2 reads, sequentially, every item in the file. When all the data has been read, line 20 causes an "Input 
past end" error. To avoid this error, insert line 15, which uses the EOF function to test for end of file: 

15 IF EOF(I) THEN END 

and change line 40 to GOTO 15. 

Aprogram that creates a sequential file can also write formatted data to the diskette with the PRINT# USING statement. For 
example, the following statement could be used to write numeric data to diskette without explicit delimiters; 

PRINT#I, USING"####.##,"; A, B, C, D 

The comma at the end of the format string serves to separate the items in the disk file. 

The LOG function, when used with a sequential file, returns the number of 128-byte records that have been written to or read from 
the file since it was opened. 


5.2.3 Adding Data to a Sequential File 

When a sequential file is opened in 0 mode, the current contents are destroyed. To add data to an existing file without destroying its 
contents, open the file in append (a) mode. 

The program in Example 3 can be used to create, or to add onto a file called names. This program illustrates the use of LINE 
INPUT. LINE INPUT will read in characters until it sees a carriage return indicator, or until it has read 255 characters. It does not 
stop at quotation marks or commas. 

Example 3 

10 ON ERROR GOTO 2000 
20 OPEN "A", #1, "NAMES" 
no REM ADD NEW ENTRIES TO FILE 
120 INPUT "NAME"; N$ 

130 IF N$="" THEN 200 'CARRIAGE RETURN EXITS INPUT LOOP 
140 LINE INPUT "ADDRESS? A$ 

150 LINE INPUT "BIRTHDAY? "; B$ 

160 PRINT#I, N$ 

170 PRINT#I, A$ 

180 PRINT#I, B$ 

190 PRINT: GOTO 120 

200 CLOSE #I 

2000 ON ERROR GOTO 0 

In lines 10 and 2000 the ON ERROR GOTO statement is being used. This statement enables error trapping and specifies the first line 
(2000) of the error handling subroutine. Line 10 enables the error handling routine. Line 2000 disables the error handling routine 
and is the point where GW-BASIC branches to print the error messages. 

5.3 Random Access Files 

Information in random access files is stored and accessed in distinct, numbered units called records. Since the information is called 
by number, the data can be called from any disk location; the program needn't read the entire disk, as when seeking sequential files, 
to locate data. GW-BASIC supports large random files. The maximum logical record number is 2^^ -1. 

The following statements and functions are used with random files; 

CLOSEFIELD MKI$ 

CVD LOG MKS$ 

CVI LOCK OPEN 

CVS LOF PUT 

EOF LSET/RSETUNLOCK 
ET MKD$ 

5.3.1 Creating a Random Access File 

The following program steps are required to create a random data file; 

1. Open the file for random access (r) mode. The following example specifies a record length of 32 bytes. If the record length is 
omitted, the default is 128 bytes. 

OPEN "R", #1, "filename" , 32 

2. Use the field statement to allocate space in the random buffer for the variables that will be written to the random file; 

FIELD#I, 20 AS N$, 4 AS A$, 8 AS P$ 

In this example, the first 20 positions (bytes) in the random file buffer are allocated to the string variable NS. The next 4 
positions are allocated to AS; the next 8 to PS. 

3. Use LSET or RSET to move the data into the random buffer fields in left- or right-justified format (L=left SET;R=right SET). 
Numeric values must be made into strings when placed in the buffer. MKI$ converts an integer value into a string; MKS$ 
converts a single-precision value, and MKD$ converts a double-precision value. 

LSET N$=X$ 

LSET A$=MKS$(AMT) 

LSET P$=TEL$ 

4. Write the data from the buffer to the diskette using the put statement; 


PUT #1, CODE% 


The program in Example 4 takes information keyed as input at the terminal and writes it to a random access data file. Each time the 
PUT statement is executed, a record is written to the file. In the example, the 2-digit CODE% input in line 30 becomes the record 
number. 


Note 

Do not use a fielded string variable in an INPUT or LET statement. This causes the pointer for that variable to point into string space 
instead of the random file buffer. 


Example 4 


10 

OPEN "R", #1, 

"INFOFILE", 32 

20 

FIELD#1, 20 AS 

N$, 4 

AS A$, 8 

30 

INPUT "2-DIGIT 

CODE", 

: CODE% 

40 

INPUT "NAME"; 

X$ 


50 

INPUT "AMOUNT" 

; AMT 


60 

INPUT "PHONE"; 

TEL$ : 

PRINT 

70 

LSET N$=X$ 



80 

LSET A$=MKS$(AMT) 


90 

LSET P$=TEL$ 




100 PUT #1, CODE% 

110 GOTO 30 

5.3.2 Accessing a Random Access File 

The following program steps are required to access a random file: 

1. Open the file in R mode: 

OPEN "R", #1, "filename", 32 

2. Use the field statement to allocate space in the random buffer for the variables that will be read from the file; 

FIELD, #1, 20 AS N$, 4 AS A$, 8 AS P$ 

In this example, the first 20 positions (bytes) in the random file buffer are allocated to the string variable NS. The next 4 
positions are allocated to AS; the next 8 to PS. 


Note 

In a program that performs both INPUT and OUTPUT on the same random file, you can often use just one OPEN statement and 
one FIELD statement. 


3. Use the get statement to move the desired record into the random buffer. 

GET #1, CODE% 

The data in the buffer can now be accessed by the program. 

4. Convert numeric values back to numbers using the convert functions: CVI for integers, CVS for single-precision values, and 
CVD for double-precision values. 

PRINT N$ 

PRINT CVS(A$) 


The program in Example 5 accesses the random file, infofile, that was created in Example 4. By inputting the 3-digit code, the 
information associated with that code is read from the file and displayed. 


Example 5 


10 

OPEN 

"R",#1,"INFOFILE 

", 32 

20 

FIELD 

#1, 20 AS N$, 4 

AS A$, 8 

30 

INPUT 

"2-DIGIT CODE"; 

CODE% 

40 

GET #1, CODE% 


50 

PRINT 

N$ 


60 

PRINT 

USING "$$###.## 

";CVS (A$) 

70 

PRINT 

P$:PRINT 


80 

GOTO 

30 







With random files, the LOC function returns the current record number. The current record number is the last record number used in 
a GET or PUT statement. For example, the following line ends program execution if the current record number in file#l is higher 
than 99: 

IF LOC(1)&gt;99 THEN END 

Example 6 is an inventory program that illustrates random file access. In this program, the record number is used as the part 
number, and it is assumed that the inventory will contain no more than 100 different part numbers. 

Lines 900-960 initialize the data file by writing CHR$ (2 5 5) as the first character of each record. This is used later (line 270 and 
line 500) to determine whether an entry already exists for that part number. 

Lines 130-220 display the different inventory functions that the program performs. When you type in the desired function number, 
line 230 branches to the appropriate subroutine. 

Example 6 

120 OPEN"R",#1,"INVEN.DAT",39 

125 FIELD#1,1 AS F$,30 AS D$, 2 AS Q$,2 AS R$,4 AS P$ 

130 PRINT:PRINT "FUNCTIONSPRINT 
135 PRINT 1,"INITIALIZE FILE" 

140 PRINT 2,"CREATE A NEW ENTRY" 

150 PRINT 3,"DISPLAY INVENTORY FOR ONE PART" 

160 PRINT 4,"ADD TO STOCK" 

170 PRINT 5,"SUBTRACT FROM STOCK" 

180 PRINT 6,"DISPLAY ALL ITEMS BELOW REORDER LEVEL" 

220 PRINT:PRINT:INPUT"FUNCTION";FUNCTION 

225 IF (FUNCTION&lt;l)OR(FUNCTION&gt;6) THEN PRINT "BAD FUNCTION NUMBER":GOTO 130 

230 ON FUNCTION GOSUB 900,250,390,480,560,680 

240 GOTO 220 

250 REM BUILD NEW ENTRY 

260 GOSUB 840 

270 IF ASC(F$) &lt; &gt; 255 THEN INPUT"OVERWRITE";A$: IF A$ &lt; &gt; "Y" THEN RETURN 
280 LSET F$=CHR$(0) 

290 INPUT "DESCRIPTION";DESC$ 

300 LSET D$=DESC$ 

310 INPUT "QUANTITY IN STOCK";Q% 

320 LSET Q$=MKI$(Q%) 

330 INPUT "REORDER LEVEL";R% 

340 LSET R$=MKI$(R%) 

350 INPUT "UNIT PRICE";P 
360 LSET P$=MKS$(P) 

370 PUT#1,PART% 

380 RETURN 

390 REM DISPLAY ENTRY 
400 GOSUB 840 

410 IF ASC(F$)=255 THEN PRINT "NULL ENTRY":RETURN 
420 PRINT USING "PART NUMBER ###";PART% 

430 PRINT D$ 

440 PRINT USING "QUANTITY ON HAND #####";CVI(Q$) 

450 PRINT USING "REORDER LEVEL #####";CVI(R$) 

460 PRINT USING "UNIT PRICE $$##.##";CVS(P$) 

470 RETURN 

480 REM ADD TO STOCK 
490 GOSUB 840 

500 IF ASC(F$)=255 THEN PRINT "NULL ENTRY":RETURN 
510 PRINT D$:INPUT "QUANTITY TO ADD";A% 

520 Q%=CVI(Q$)+A% 

530 LSET Q$=MKI$(Q%) 

540 PUT#1,PART% 

550 RETURN 

560 REM REMOVE FROM STOCK 
570 GOSUB 840 

580 IF ASC(F$)=255 THEN PRINT "NULL ENTRY":RETURN 
590 PRINT D$ 

600 INPUT "QUANTITY TO SUBTRACT";S% 

610 Q%=CVI(Q$) 

620 IF (Q%-S%)&lt;0 THEN PRINT "ONLY";Q%;"IN STOCK" :GOTO 600 
630 Q%=Q%-S% 

640 IF Q%= &lt; CVI(R$) THEN PRINT "QUANTITY NOW";Q%;"REORDER LEVEL";CVI(R$) 

650 LSET Q$=MKI$(Q%) 


660 

670 

680 

690 

710 

720 

730 

740 

840 

850 

890 

900 

910 

920 

930 

940 

950 

960 


PUT#1,PART% 

RETURN 

REM DISPLAY ITEMS BELOW REORDER LEVEL4 
FOR 1=1 TO 100 
GET#1,I 

IF CVI(Q$)&lt;CVI(R$) THEN PRINT D$;"QUANTITY";CVI(Q$) TAB(50) "REORDER LEVEL";CVI(R$) 

NEXT I 
RETURN 

INPUT "PART NUMBER";PART% 

IF(PART% &lt; l)OR(PART% &gt; 100) THEN PRINT "BAD PART NUMBER":GOTO 840 ELSE GET#1,PART%:RETURN 
END 

REM INITIALIZE FILE 

INPUT "ARE YOU SURE";B$:IF B$ &lt; &gt; "Y" THEN RETURN 
LSET F$=CHR$(255) 

FOR 1=1 TO 100 
PUT#1,I 
NEXT I 
RETURN 


