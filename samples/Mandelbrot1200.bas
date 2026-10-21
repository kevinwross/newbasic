10 REM Mandelbrot1200 - an interactive Mandelbrot in SCREEN 720.
11 REM
12 REM Left-click centres the view on the point clicked and zooms in. Right-click centres
13 REM and zooms out. Escape ends the program.
14 REM
15 REM Escape is handled by this program, not by the interpreter: it polls INKEY$ and runs
16 REM END when it sees CHR$(27), in the click loop and between render rows so a long
17 REM render can be left early.
18 REM
19 REM Each view is drawn twice: a coarse pass of 8-pixel blocks so the shape appears at
20 REM once, then the full-resolution pass.
30 SCREEN 720
40 CLS
50 MX = 48
60 REM World window. The canvas is 16:9 with square pixels, so both spans scale together.
70 X0 = -2.9: X1 = 1.4
80 Y0 = -1.21: Y1 = 1.21
100 GOSUB 1000
110 REM Wait for a click, but leave if Escape was pressed.
120 K$ = INKEY$
130 IF K$ = CHR$(27) THEN END
140 B = MOUSEB
150 IF B = 0 THEN GOTO 120
160 CX = X0 + MOUSEX * (X1 - X0) / 1279
170 CY = Y1 - MOUSEY * (Y1 - Y0) / 719
180 W = (X1 - X0) / 2: H = (Y1 - Y0) / 2
190 IF B = 2 THEN W = (X1 - X0) * 2: H = (Y1 - Y0) * 2
200 REM Stop zooming out once the whole set is comfortably inside the view.
210 IF W > 8 THEN W = 8: H = W * 720 / 1280
220 X0 = CX - W / 2: X1 = CX + W / 2
230 Y0 = CY - H / 2: Y1 = CY + H / 2
240 GOTO 100
1000 REM Draw the current view: pass 1 uses 8-pixel blocks, pass 2 fills every pixel.
1010 FOR PA = 1 TO 2
1020 ST = 8: IF PA = 2 THEN ST = 2
1030 FOR PY = 0 TO 716 STEP ST
1040 IF INKEY$ = CHR$(27) THEN END
1050 CY = Y1 - PY * (Y1 - Y0) / 719
1060 FOR PX = 0 TO 1276 STEP ST
1070 CX = X0 + PX * (X1 - X0) / 1279
1080 ZX = 0: ZY = 0: IT = 0
1090 WHILE IT < MX AND ZX * ZX + ZY * ZY <= 4
1100   T = ZX * ZX - ZY * ZY + CX
1110   ZY = 2 * ZX * ZY + CY
1120   ZX = T
1130   IT = IT + 1
1140 WEND
1150 C = 0
1160 IF IT < MX THEN MU = IT + 1 - LOG(LOG(SQR(ZX * ZX + ZY * ZY))) / LOG(2)
1170 IF IT < MX THEN C = 16 + (INT(MU * 5) MOD 240)
1180 LINE (PX, PY)-(PX + ST - 1, PY + ST - 1), C, BF
1190 NEXT PX
1200 NEXT PY
1210 NEXT PA
1220 RETURN
