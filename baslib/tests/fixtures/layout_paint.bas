10 OPEN "O", #1, "result.txt"
20 SCREEN 720
30 CALL blInit(640, 400)
40 P% = blCreate%(1, 2, 0, 10, 10, 300, 200, "", 0, 0, 0)
50 CALL blSetLayout(P%, 1, 4, 4)
60 B% = blCreate%(P%, 3, 1, 0, 0, 100, 24, "", 0, 0, 0)
70 L% = blCreate%(P%, 3, 5, 0, 0, 100, 20, "Label", 0, 0, 0)
80 CALL blLayoutAll
90 PRINT #1, "POS"; blCtl%(B%,11); blCtl%(B%,12); blCtl%(B%,9); blCtl%(B%,10)
100 PRINT #1, "LAYOUT"; blCtl%(L%,11); blCtl%(L%,12)
110 CALL blPaintAll
120 PRINT #1, "PIX"; POINT(15,15)
130 CLOSE #1
140 END
