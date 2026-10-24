10 OPEN "O", #1, "result.txt"
20 SCREEN 720
30 blReqW% = 640 : blReqH% = 400
40 GOSUB blApiInit
50 PRINT #1, "INIT"; blStatus%
60 blReqClass% = 3 : blReqParent% = 1 : blReqKind% = 1
70 blReqX% = 10 : blReqY% = 10 : blReqW% = 100 : blReqH% = 24
80 blReqText$ = "OK" : blReqAction% = 100 : blReqPad% = 2
90 GOSUB blApiCreate
100 PRINT #1, "BUTTON"; blStatus%; blRetHandle%
110 GOSUB blApiPaintAll
120 PRINT #1, "PAINT"; blStatus%
130 blReqX% = 20 : blReqY% = 20 : GOSUB blApiFindAt
140 PRINT #1, "FIND"; blRetHandle%
150 blReqX% = 20 : blReqY% = 20 : blReqButton% = 1 : GOSUB blApiTestClickAt
160 GOSUB blApiNextEvent
170 PRINT #1, "EVENT"; blEventType%; blEventCtl%; blEventAction%
180 CLOSE #1
190 END
