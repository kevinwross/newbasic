10 OPEN "O", #1, "result.txt"
20 SCREEN 720
30 blReqW% = 640 : blReqH% = 400
40 GOSUB blApiInit
50 blReqClass% = 2 : blReqParent% = 1
60 blReqX% = 10 : blReqY% = 10 : blReqW% = 300 : blReqH% = 200
70 blReqMode% = 1 : blReqPad% = 4 : blReqGap% = 4
80 GOSUB blApiCreate : blPanel% = blRetHandle%
90 blReqClass% = 3 : blReqParent% = blPanel% : blReqKind% = 1
100 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 24
110 GOSUB blApiCreate : blButton% = blRetHandle%
120 blReqClass% = 3 : blReqParent% = blPanel% : blReqKind% = 5
130 blReqX% = 0 : blReqY% = 0 : blReqW% = 100 : blReqH% = 20
140 blReqText$ = "Label" : GOSUB blApiCreate
150 GOSUB blApiLayoutAll
160 PRINT #1, "POS"; blCtl%(blButton%,11); blCtl%(blButton%,12); blCtl%(blButton%,9); blCtl%(blButton%,10)
170 PRINT #1, "LAYOUT"; blCtl%(blRetHandle%,11); blCtl%(blRetHandle%,12)
180 GOSUB blApiPaintAll
190 PRINT #1, "PIX"; POINT(15,15)
200 CLOSE #1
210 END
