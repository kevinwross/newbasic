<!-- Chunked from GWBASIC.txt, lines 2785-3646. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# User's Guide - Chapter 6: Constants, Variables, Expressions, and Operators

Chapter 6 

Constants, Variables, Expressions and Operators 

After you have learned the fundamentals of programming in GW-BASIC, you will find that you will 
want to write more complex programs. The information in this chapter will help you learn more about 
the use of constants, variables, expressions, and operators in GW-BASIC, and how they can be used 
to develop more sophisticated programs. 


6.1 Constants 

Constants are static values the GW-BASIC Interpreter uses during execution of your program. There 
are two types of constants: string and numeric. 

A string constant is a sequence of 0 to 255 alphanumeric characters enclosed in double quotation 
marks. The following are sample string constants: 

HELLO 

$25,000.00 

Number of Employees 

Numeric constants can be positive or negative. When entering a numeric constant in GW-BASIC, you 
should not type the commas. For instance, if the number 10,000 were to be entered as a constant, it 
would be typed as 10000. There are five types of numeric constants: integer, fixed-point, floating¬ 
point, hexadecimal, and octal. 


Constant Description 

Integer Whole numbers between -32768 and +32161. They do not contain decimal 

points. 

Fixed-Point Positive or negative real numbers that contain decimal points. 


Floating-Point Positive or negative numbers represented in exponential form (similar to 
Constants scientific notation). A floating-point constant consists of an optionally-signed 
integer or fixed-point number (the mantissa), followed by the letter E and an 
optionally-signed integer (the exponent). The allowable range for floating-point 
constants is 3.0xl0‘^^ to 1.7x10^^. For example: 

235.988E-7=.0000235988 
2359E6=2359000000 


Hexadecimal Hexadecimal numbers with prefix &amp;H. For example: 

&amp;H7 6 
&amp;H32F 


Octal 


Octal numbers with the prefix &amp;0 or &amp;. For example: 

&amp;0347 


&amp;1234 


6,1,1 Single- and Double-Precision Form for Numeric Constants 

Numeric constants can be integers, single-precision, or double-precision numbers. Integer constants 
are stored as whole numbers only. Single-precision numeric constants are stored with 7 digits 
(although only 6 may be accurate). Double-precision numeric constants are stored with 17 digits of 
precision, and printed with as many as 16 digits. 

A single-precision constant is any numeric constant with either 

• Seven or fewer digits 

• Exponential form using E 

• A trailing exclamation point ( ! ) 

A double-precision constant is any numeric constant with either 

• Eight or more digits 

• Exponential form using D 

• A trailing number sign (#) 


The following are examples of single- and double-precision numeric constants: 


Single-Precision ConstantsDouble-Precision Constants 


46.8 

-1.09E-06 

3489.0 

22.5! 


345692811 
-1.09432D-06 
3490.0# 
7654321.1234 


6.2 Variables 

Variables are the names that you have chosen to represent values used in a GW-BASIC program. The 
value of a variable may be assigned specifically, or may be the result of calculations in your program. 
If a variable is assigned no value, GW-BASIC assumes the variable's value to be zero. 


6,2,1 Variable Names and Declarations 

GW-BASIC variable names may be any length; up to 40 characters are significant. The characters 
allowed in a variable name are letters, numbers, and the decimal point. The first character in the 
variable name must be a letter. Special type declaration characters are also allowed. 

Reserved words (all the words used as GW-BASIC commands, statements, functions, and operators) 
can't be used as variable names. However, if the reserved word is embedded within the variable name, 
it will be allowed. 

Variables may represent either numeric values or strings. 


6,2,2 Type Declaration Characters 

Type declaration characters indicate what a variable represents. The following type declaration 
characters are recognized: 

CharacterType of Variable 

$ String variable 

% Integer variable 


! Single-precision 

variable 

^ Double-precision 

variable 

The following are sample variable names for each type; 

Variable Type Sample Name 

String variable N $ 

Integer variable L l M l T % 

Single-precision variable minimum! 

Double-precision variablePl# 

The default type for a numeric variable name is single-precision. Double-precision, while very 
accurate, uses more memory space and more calculation time. Single-precision is sufficiently accurate 
for most applications. However, the seventh significant digit (if printed) will not always be accurate. 
You should be very careful when making conversions between integer, single-precision, and double¬ 
precision variables. 

The following variable is a single-precision value by default: 

ABC 

Variables beginning with FN are assumed to be calls to a user-defined function. 

The GW-BASIC statements defint, defstr, defsng, and defdbl may be included in a program 
to declare the types of values for certain variable names. 

6,2,3 Array Variables 

An array is a group or table of values referenced by the same variable name. Each element in an array 
is referenced by an array variable that is a subscripted integer or an integer expression. The subscript 
is enclosed within parentheses. An array variable name has as many subscripts as there are 
dimensions in the array. 

For example, 

V(I0) 

references a value in a one-dimensional array, while 

T(I,4) 

references a value in a two-dimensional array. 

The maximum number of dimensions for an array in GW-BASIC is 255. The maximum number of 
elements per dimension is 32767. 


Note 

If you are using an array with a subscript value greater than 10, you should use the dim statement. 
Refer to the GW-BASIC User's Reference for more information. If a subscript greater than the 
maximum specified is used, you will receive the error message "Subscript out of range." 


Multidimensional arrays (more than one subscript separated by commas) are useful for storing tabular 
data. For example, A(l,4) could be used to represent a two-row, five-column array such as the 
following; 





Column 01234 

Row 0 10 20 30 40 50 

Row 1 60 70 80 90 100 

In this example, element A(l,2)=80 and A(0,3)=40. 

Rows and eolumns begin with 0, not 1, unless otherwise declared. For more information, see the 
OPTION BASE Statement in the GW-BASIC User's Reference . 

6,2,4 Memory Space Requirements for Variable Storage 

The different types of variables require different amounts of storage. Depending on the storage and 
memory capacity of your computer and the size of the program that you are developing, these can be 
important considerations. 

Variable Required Bytes of Storage 

Integer 2 

Single-precision 4 
Double-precisionS 

Arrays Required Bytes of Storage 

Integer 2 per element 

Single-precision 4 per element 
Double-precisionS per element 

Strings: 

Three bytes overhead, plus the present contents of the string as one byte for each character in the 
string. Quotation marks marking the beginning and end of each string are not counted. 

6.3 Type Conversion 

When necessary, GW-BASIC converts a numeric constant from one type of variable to another, 
according to the following rules: 

• If a numeric constant of one type is set equal to a numeric variable of a different type, the 
number is stored as the type declared in the variable name. 

For example: 

10 A% = 23.42 
20 PRINT A% 

RUN 

23 

If a string variable is set equal to a numeric value or vice versa, a "Type Mismatch" error 
occurs. 

• During an expression evaluation, all of the operands in an arithmetic or relational operation are 
converted to the same degree of precision; that is, that of the most precise operand. Also, the 
result of an arithmetic operation is returned to this degree of precision. For example: 

10 D# = 6#/7 
20 PRINT D# 

RUN 

. 8571428571428571 



The arithmetic is performed in double-precision, and the result is returned in D# as a double¬ 
precision value. 

10 D = 6#/7 
20 PRINT D 
RUN 

. 8571429 

The arithmetic is performed in double-precision, and the result is returned to D (single-precision 
variable) rounded and printed as a single-precision value. 

• Logical operators convert their operands to integers and return an integer result. Operands must 
be within the range of -32768 to 32767 or an "Overflow" error occurs. 

• When a floating-point value is converted to an integer, the fractional portion is rounded. For 
example: 

10 C% = 55.88 
20 PRINT C% 

RUN 

56 

• If a double-precision variable is assigned a single-precision value, only the first seven digits 
(rounded), of the converted number are valid. This is because only seven digits of accuracy 
were supplied with the single-precision value. The absolute value of the difference between the 
printed double-precision number, and the original single-precision value, is less than 6.3E-8 
times the original single-precision value. For example: 

10 A = 2.04 
20 B# = A 
30 PRINT A;B# 

RUN 

2.04 2.039999961853027 


6.4 Expressions and Operators 

An expression may be simply a string or numeric constant, a variable, or it may combine constants 
and variables with operators to produce a single value. 

Operators perform mathematical or logical operations on values. The operators provided by GW- 
BASIC are divided into four categories: 

• Arithmetic 

• Relational 

• Logical 

• Functional 

6,4,1 Arithmetic Operators 

The following are the arithmetic operators recognized by GW-BASIC. They appear in order of 
precedence. 

OperatorOperation 


^ Exponentiation 

Negation 

* Multiplication 

I Floating-point 

Division 
+ Addition 

Subtraction 

Operations within parentheses are performed first. Inside the parentheses, the usual order of 
precedence is maintained. 

The following are sample algebraic expressions and their GW-BASIC counterparts: 


Algebraic ExpressionBASIC Expression 

X-Z/Y 

(X-Y)/Z 

XY/Z 

X*Y/Z 

X+Y/Z 

(X+Y)/Z 

(X2)Y 

(X"2) ^'Y 

xyZ 

X^ (Y^'Z) 

X(-Y) 

X*(-Y) 


Two consecutive operators must be separated by parentheses. 

6.4.1.1 Integer Division and Modulus Arithmetic 

Two additional arithmetic operators are available: integer division and modulus arithmetic. 

Integer division is denoted by the backslash (\). The operands are rounded to integers (must be within 
the range of -32768 to 32767) before the division is performed, and the quotient is truncated to an 
integer. 

The following are examples of integer division: 

10\4 = 2 
25.68\6.99 = 3 

In the order of occurrence within GW-BASIC, the integer division will be performed just after 
floating-point division. 

Modulus arithmetic is denoted by the operator MOD. It gives the integer value that is the remainder of 
an integer division. 

The following are examples of modulus arithmetic: 


10.4 MOD 4 = 

2 



(10/4=2 with 

a 

remainder 

2) 

25.68 MOD 6. 

99 

= 5 


(26/7=3 with 

a 

remainder 

5) 


In the order of occurrence within GW-BASIC, modulus arithmetic follows integer division. The INT 
and FIX functions, described in the GW-BASIC User's Reference, are also useful in modulus 
arithmetic. 



6.4.1.2 Overflow and Division by Zero 

If, during the evaluation of an expression, a division by zero is eneountered, the "Division by 
zero" error message appears, machine infinity with the sign of the numerator is supplied as the result 
of the division, and execution continues. 

If the evaluation of an exponentiation results in zero being raised to a negative power, the "Division 
by zero" error message appears, positive machine infinity is supplied as the result of the 
exponentiation, and execution continues. 

If overflow occurs, the "Overflow" error message appears, machine infinity with the algebraically 
correct sign is supplied as the result, and execution continues. The errors that occur in overflow and 
division by zero will not be trapped by the error trapping function. 

6.4.2 Relational Operators 

Relational operators let you compare two values. The result of the comparison is either true (-1) or 
false (0). This result can then be used to make a decision regarding program flow. 

Table 6.1 displays the relational operators. 


Table 6,1 

Relational Operators 


OperatorRelation Tested 

Expression 

= 

Equality 

X=Y 

&lt;&gt; 

Inequality 

XOY 

&lt; 

Less than 

X&lt;Y 

&gt; 

Greater than 

X&gt;Y 

&lt;= 

Less than or equal to 

X&lt;=Y 

&gt;= 

Greater than or equal toX&gt;=Y 


The equal sign is also used to assign a value to a variable. See the let statement in the GW-BASIC 
User's Reference . 

When arithmetic and relational operators are combined in one expression, the arithmetic is always 
performed first: 

X+Y &lt; (T-1)/Z 

This expression is true if the value of X plus Y is lessthan the value of T-1 divided by Z. 

6,4,3 Logical Operators 

Logical operators perform tests on multiple relations, bit manipulation, or Boolean operations. The 
logical operator returns a bit-wise result which is either true (not zero) or false (zero). In an 
expression, logical operations are performed after arithmetic and relational operations. The outcome 
of a logical operation is determined as shown in the following table. The operators are listed in order 
of precedence. 

Table 6,2 

Results Returned by Logical Operations 


Operation Value 

NOT X 


Value 


Result 

NOTX 





T 


F 


F 


T 

AND 

X 

Y 

X 

AND Y 


T 

T 

T 


T 

F 

F 


F 

T 

F 


F 

F 

F 

OR 

X 

Y 

X OR 

Y 


T 

T 

T 


T 

F 

T 


F 

T 

T 


F 

F 

F 

XOR 

X 

Y 

X 

XOR Y 


T 

T 

F 


T 

F 

T 


F 

T 

T 


F 

F 

F 

EQV 

X 

Y 

X 

EQV Y 


T 

T 

T 


T 

F 

E 


F 

T 

E 


F 

F 

T 

IMP 

X 

Y 

X IMP 

Y 


T 

T 

T 


T 

F 

E 


F 

T 

T 


F 

F 

T 


Just as the relational operators can be used to make decisions regarding program flow, logical 
operators can connect two or more relations and return a true or false value to be used in a decision. 
For example; 

IF D&lt;200 AND F&lt;4 THEN 80 
IF I&gt;10 OR K&lt;0 THEN 50 
IF NOT P THEN 100 

Logical operators convert their operands to 16-bit, signed, two's complement integers within the range 
of -32768 to +32767 (if the operands are not within this range, an error results). If both operands are 
supplied as 0 or -1, logical operators return 0 or -1. The given operation is performed on these integers 
in bits; that is, each bit of the result is determined by the corresponding bits in the two operands. 

Thus, it is possible to use logical operators to test bytes for a particular bit pattern. For instance, the 
AND operator may be used to mask all but one of the bits of a status byte at a machine I/O port. The 
OR operator may be used to merge two bytes to create a particular binary value. The following 
examples demonstrate how the logical operators work: 

Example Explanation 


63 AND 16=16 63 = binary mill and 16 = binary 10000, SO 63 AND 16 = 16 
15 AND 14=14 15 = binary nil and 14 = binary 1110, so 15 AND 14 = 14 (binary 1110) 

-1 AND 8=8 -1 = binary 1111111111111111 and 8 = binary 1000, so-1 AND 8 = 8 

4 OR 2 = 6 4 = binary 100 and 2 = binary 10, so 4 OR 2 = 6 (binary 110) 

10 OR 10=10 10 =binary 1010, so 1010 OR 1010 =1010 (10) 

-1 =binary 1111111111111111 and -2 = binary 1111111111111110, so-1 OR-2 
-1 OR -2=-l = -1. The bit complement of 16 zeros is 16 ones, which is the two's complement 
representation of -1. 

NOT x=- (X+l) The two's complement of any integer is the bit complement plus one. 

6.4.4 Functional Operators 

A function is used in an expression to call a predetermined operation that is to be performed on an 
operand. GW-BASIC has intrinsic functions that reside in the system, such as SQR (square root) or 
SIN (sine). 

GW-BASIC also allows user-defined functions written by the programmer. See the def fn statement 
in the GW-BASIC User's Reference . 

6.4.5 String Operators 

To compare strings, use the same relational operators used with numbers; 

OperatorMeaning 

= Equal to 

&lt;&gt; Unequal 

&lt; Less than 

&gt; Greater than 

&lt;= Less than or equal to 

&gt;= Greater than or equal to 

The GW-BASIC Interpreter compares strings by taking one character at a time from each string and 
comparing their ASCII codes. If the ASCII codes in each string are the same, the strings are equal. If 
the ASCII codes differ, the lower code number will precede the higher code. If the interpreter reaches 
the end of one string during string comparison, the shorter string is said to be smaller, providing that 
both strings are the same up to that point. Leading and trailing blanks are significant. 

Lor example; 

"AA" &lt; "AB" 

"FILENAME" = "FILENAME" 

"X&amp;" &gt; "X#" 

"CL " &gt; "CL" 

"kg" &gt; "KG" 

"SMYTH" &lt; "SMYTHE" 

B$ &lt; "9/12/78" where B$ = "8/12/78" 

String comparisons can also be used to test string values or to alphabetize strings. All string constants 
used in comparison expressions must be enclosed in quotation marks. 

Strings can be concatenated by using the plus (+) sign. Lor example; 



10 A$="FILE":B$="NAME" 
20 PRINT A$+B$ 

30 PRINT "NEW " + A$+B$ 
RUN 

FILENAME 
NEW FILENAME 


