10 OPEN "O", #1, "result.txt"
20 SCREEN 720
30 CALL blInit(640, 400)
40 PRINT #1, "INIT"; blStatus%
50 B% = blCreate%(1, 3, 1, 10, 10, 100, 24, "OK", 100, 0, 0)
60 PRINT #1, "BUTTON"; blStatus%; B%
70 CALL blPaintAll
80 PRINT #1, "PAINT"; blStatus%
90 F% = blFindAt%(20, 20)
100 PRINT #1, "FIND"; F%
110 H% = blTestClickAt%(20, 20, 1)
120 T% = blNextEvent%()
130 PRINT #1, "EVENT"; T%; blEventCtl%(); blEventAction%()
140 CLOSE #1
150 END
