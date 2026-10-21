<!-- Chunked from GWBASIC.txt, lines 3982-4050. GW-BASIC User's Guide and User's Reference, Microsoft 1986/1987 (doc 410130013-330-R02-0787). Text is verbatim, including OCR artifacts. Copyrighted material - local reference only, do not redistribute. -->
# Appendix B: Mathematical Functions

Appendix B 

Mathematical Functions 

Mathematical functions not intrinsic to GW-BASIC can be calculated as follows: 


Function 

Secant 
Cosecant 
Cotangent 
Inverse Sine 
Inverse Cosine 
Inverse Secant 
Inverse Cosecant 
Inverse Cotangent 
Hyperbolic Sine 
Hyperbolic Cosine 
Hyperbolic Tangent 
Hyperbolic Secant 
Hyperbolic Cosecant 
Hyperbolic Cotangent 
Inverse Hyperbolic Sine 
Inverse Hyperbolic Cosine 
Inverse Hyperbolic Tangent 
Inverse Hyperbolic Cosecant 
Inverse Hyperbolic Secant 
Inverse Hyperbolic Cotangent 


GW-BASIC Equivalent 

SEC (X)=l/COS(X) 

CSC(X)=1/SIN(X) 

COT(X)=1/TAN(X) 

ARCSIN(X)=ATN(X/SQR(-X*X+1) ) 

ARCCOS(X)=ATN (X/SQR(-X*X+1))+ PI/2 

ARCSEC(X)=ATN(X/SQR(X*X-1))+SGN(SGN(X)-l)* PI/2 

ARCCSC(X)=ATN(X/SQR(X*X-1))+SGN(X)-l)* PI/2 

ARCCOT(X)=ATN(X)+ PI/2 

SINK(X)=(EXP(X)-EXP(-X))/2 

COSH(X)=(EXP(X)+EXP(-X))/2 

TANH(X)=EXP(X)-EXP(-X))/+(EXP(X)+EXP(-X)) 

SECH(X)=2/(EXP(X)+EXP(-X)) 

CSCH(X)=2/(EXP(X)-EXP(-X) ) 

COTH(X)=EXP(-X) / (EXP(X)-EXP(-X) )*2 + 1 
ARCSINH(X)=LOG(X/SQR(X*X+l) ) 
ARCCOSH(X)=LOG(X+SQR(X*X-l) ) 

ARCTANH(X)=LOG( (1+X)/ (1-X) ) /2 
ARCCSCH(X)=LOG(SGN(X)*SQR(X*X+1)+1)/X 
ARCSECH(X)=LOG(SQR(-X*X+l)+l) /X 
ARCCOTH(X)=LOG((X+1)/(X-1))/2 


