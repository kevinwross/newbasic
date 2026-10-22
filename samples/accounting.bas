10 REM ================================================================
20 REM NEWBASIC ACCOUNTING - DOUBLE ENTRY BOOKKEEPING.
30 REM SCREENS: JOURNAL ENTRY, BILLING, PAYMENTS. FORMS USE LOCATE.
40 REM REPORTS USE PRINT USING. RUN FROM THE REPO ROOT.
45 REM DATE FIELDS OPEN ON TODAY: TYPE TO REPLACE IT, BACKSPACE TO EDIT.
47 REM DATES ARE MM/DD/YYYY AND MUST BE REAL DATES (1900-2099).
50 REM DATA: samples\accounting.dat, ONE "|"-SEPARATED RECORD PER ENTRY:
55 REM TYPE|REF|DATE|DESCRIPTION|DEBIT|CREDIT|AMOUNT  (1=J 2=BILL 3=PAY)
60 REM ================================================================
70 DIM AccountNumber(20), AccountName$(20), AccountType(20), AccountBalance(20)
80 DIM JournalReference$(300), JournalDate$(300), JournalDescription$(300), JournalDebitAccount(300), JournalCreditAccount(300), JournalAmount(300), JournalEntryType(300), DataField$(10)
90 DataFilePath$ = "samples\accounting.dat"
92 TodayDate$ = MID$(DATE$, 1, 2) + "/" + MID$(DATE$, 4, 2) + "/" + MID$(DATE$, 7, 4)
95 BalanceReportFormat$ = "  \" + SPACE$(34) + "\  ####,###.##  ####,###.##"
96 AmountReportFormat$ = "  \" + SPACE$(34) + "\  ####,###.##"
97 JournalReportFormat$ = "  !  \" + SPACE$(8) + "\  \" + SPACE$(8) + "\  \" + SPACE$(20) + "\  ####/####  ####,###.##"
98 InvoiceReportFormat$ = "  \" + SPACE$(8) + "\  \" + SPACE$(18) + "\  ####,###.##  ####,###.##  ####,###.##"
99 GOSUB 5000
100 GOSUB 5100
110 GOSUB 200
120 END
200 REM ---- MAIN MENU ------------------------------------------------
210 CLS
220 LOCATE 2,31: PRINT "NEWBASIC ACCOUNTING"
230 LOCATE 4,29: PRINT "1  JOURNAL ENTRY"
240 LOCATE 5,29: PRINT "2  BILL A CUSTOMER"
250 LOCATE 6,29: PRINT "3  ACCEPT A PAYMENT"
260 LOCATE 7,29: PRINT "4  JOURNAL"
270 LOCATE 8,29: PRINT "5  TRIAL BALANCE"
280 LOCATE 9,29: PRINT "6  INCOME STATEMENT"
290 LOCATE 10,29: PRINT "7  BALANCE SHEET"
300 LOCATE 11,29: PRINT "8  QUIT"
310 LOCATE 13,29: PRINT "CHOICE: ";
320 LOCATE 13,37: INPUT "", MenuChoice$
330 IF MenuChoice$ = "1" THEN GOSUB 1000 : GOTO 210
340 IF MenuChoice$ = "2" THEN GOSUB 1500 : GOTO 210
350 IF MenuChoice$ = "3" THEN GOSUB 2000 : GOTO 210
360 IF MenuChoice$ = "4" THEN GOSUB 3750 : GOSUB 8000 : GOTO 210
370 IF MenuChoice$ = "5" THEN GOSUB 2600 : GOSUB 8000 : GOTO 210
380 IF MenuChoice$ = "6" THEN GOSUB 2900 : GOSUB 8000 : GOTO 210
390 IF MenuChoice$ = "7" THEN GOSUB 3250 : GOSUB 8000 : GOTO 210
400 IF MenuChoice$ = "8" THEN RETURN
410 GOTO 210
1000 REM ---- JOURNAL ENTRY (ANY TWO ACCOUNTS) -------------------------
1010 CLS
1020 LOCATE 2,31: PRINT "JOURNAL ENTRY"
1030 LOCATE 4,5: PRINT "REFERENCE  :"
1040 LOCATE 5,5: PRINT "DATE       :"
1050 LOCATE 6,5: PRINT "DESCRIPTION:"
1060 LOCATE 8,5: PRINT "DEBIT ACCT :"
1070 LOCATE 9,5: PRINT "CREDIT ACCT:"
1080 LOCATE 10,5: PRINT "AMOUNT     :"
1090 LOCATE 12,5: PRINT "ACCOUNT NUMBERS"
1100 LOCATE 13,5: PRINT "1000 CASH              1100 ACCOUNTS RECEIVABLE"
1110 LOCATE 14,5: PRINT "1200 SUPPLIES          1500 EQUIPMENT"
1120 LOCATE 15,5: PRINT "2000 ACCOUNTS PAYABLE  2100 SALES TAX PAYABLE"
1130 LOCATE 16,5: PRINT "3000 OWNERS EQUITY     3100 RETAINED EARNINGS"
1140 LOCATE 17,5: PRINT "4000 SERVICE REVENUE"
1150 LOCATE 18,5: PRINT "5000 RENT EXPENSE      5100 SALARIES EXPENSE"
1160 LOCATE 19,5: PRINT "5200 UTILITIES EXP     5300 SUPPLIES EXPENSE"
1170 LOCATE 21,5: PRINT "BLANK REFERENCE RETURNS TO THE MENU."
1200 GOSUB 7200 : LOCATE 4,22: PRINT SPACE$(45); : LOCATE 4,22: INPUT "", EntryReference$
1210 IF EntryReference$ = "" THEN RETURN
1220 IF INSTR(EntryReference$, "|") > 0 THEN StatusMessage$ = "NO | IN THE REFERENCE" : GOSUB 7000 : GOTO 1200
1225 GOSUB 7200 : EditValue$ = TodayDate$
1230 EditRow = 5 : EditColumn = 22 : GOSUB 7600 : EntryDate$ = EditValue$
1240 GOSUB 7800 : IF DateIsValid = 1 THEN 1250
1242 IF DateErrorCode = 1 THEN StatusMessage$ = "USE MM/DD/YYYY (E.G. 10/20/2026)" ELSE StatusMessage$ = "THAT DATE DOES NOT EXIST"
1244 GOSUB 7000 : EditValue$ = EntryDate$ : GOTO 1230
1250 IF INSTR(EntryDate$, "|") > 0 THEN StatusMessage$ = "NO | IN THE DATE" : GOSUB 7000 : GOTO 1225
1260 GOSUB 7200 : LOCATE 6,22: PRINT SPACE$(45); : LOCATE 6,22: LINE INPUT "", EntryDescription$
1270 IF EntryDescription$ = "" THEN StatusMessage$ = "A DESCRIPTION IS REQUIRED" : GOSUB 7000 : GOTO 1260
1280 IF INSTR(EntryDescription$, "|") > 0 THEN StatusMessage$ = "NO | IN THE DESCRIPTION" : GOSUB 7000 : GOTO 1260
1290 GOSUB 7200 : LOCATE 8,22: PRINT SPACE$(45); : LOCATE 8,22: DebitAccountNumber = 0 : INPUT "", DebitAccountNumber
1300 AccountNumberInput = DebitAccountNumber : GOSUB 4000
1310 IF FoundAccountIndex = 0 THEN StatusMessage$ = "ACCOUNT " + MID$(STR$(DebitAccountNumber), 2) + " IS NOT IN THE CHART" : GOSUB 7000 : GOTO 1290
1320 DebitAccountIndex = FoundAccountIndex : LOCATE 8,34: PRINT SPACE$(38); : LOCATE 8,34: PRINT AccountName$(DebitAccountIndex)
1330 GOSUB 7200 : LOCATE 9,22: PRINT SPACE$(45); : LOCATE 9,22: CreditAccountNumber = 0 : INPUT "", CreditAccountNumber
1340 IF CreditAccountNumber = DebitAccountNumber THEN StatusMessage$ = "DEBIT AND CREDIT MUST BE DIFFERENT ACCOUNTS" : GOSUB 7000 : GOTO 1330
1350 AccountNumberInput = CreditAccountNumber : GOSUB 4000
1360 IF FoundAccountIndex = 0 THEN StatusMessage$ = "ACCOUNT " + MID$(STR$(CreditAccountNumber), 2) + " IS NOT IN THE CHART" : GOSUB 7000 : GOTO 1330
1370 CreditAccountIndex = FoundAccountIndex : LOCATE 9,34: PRINT SPACE$(38); : LOCATE 9,34: PRINT AccountName$(CreditAccountIndex)
1380 GOSUB 7200 : LOCATE 10,22: PRINT SPACE$(45); : LOCATE 10,22: EntryAmount = 0 : INPUT "", EntryAmount
1390 IF EntryAmount <= 0 THEN StatusMessage$ = "THE AMOUNT MUST BE POSITIVE" : GOSUB 7000 : GOTO 1380
1400 EntryAmount = INT(EntryAmount * 100 + 0.5) / 100
1410 LOCATE 10,22: PRINT SPACE$(45); : LOCATE 10,22: PRINT USING "####,###.##"; EntryAmount
1420 IF JournalEntryCount >= 300 THEN StatusMessage$ = "THE JOURNAL IS FULL" : GOSUB 7000 : RETURN
1430 LOCATE 23,5: PRINT SPACE$(70);
1440 LOCATE 23,5: PRINT USING "DR #####   CR #####   AMOUNT ####,###.##"; DebitAccountNumber; CreditAccountNumber; EntryAmount
1450 LOCATE 24,5: PRINT SPACE$(70); : PRINT "PRESS ENTER TO POST, OR N TO CANCEL: ";
1460 INPUT "", Confirmation$
1470 IF Confirmation$ = "N" OR Confirmation$ = "n" THEN RETURN
1480 JournalEntryCount = JournalEntryCount + 1
1485 JournalEntryType(JournalEntryCount) = 1 : JournalReference$(JournalEntryCount) = EntryReference$ : JournalDate$(JournalEntryCount) = EntryDate$ : JournalDescription$(JournalEntryCount) = EntryDescription$ : JournalDebitAccount(JournalEntryCount) = DebitAccountNumber : JournalCreditAccount(JournalEntryCount) = CreditAccountNumber : JournalAmount(JournalEntryCount) = EntryAmount
1490 GOSUB 5300
1495 RETURN
1500 REM ---- BILL A CUSTOMER: DR A/R, CR THE INCOME ACCOUNT -------------
1510 CLS
1520 LOCATE 2,29: PRINT "BILL A CUSTOMER"
1530 LOCATE 4,5: PRINT "INVOICE #  :"
1540 LOCATE 5,5: PRINT "DATE       :"
1550 LOCATE 6,5: PRINT "CUSTOMER   :"
1560 LOCATE 8,5: PRINT "INCOME ACCT:"
1570 LOCATE 9,5: PRINT "AMOUNT     :"
1580 LOCATE 11,5: PRINT "INCOME ACCOUNTS (BLANK USES 4000)"
1590 LOCATE 12,5: PRINT "4000 SERVICE REVENUE"
1600 LOCATE 14,5: PRINT "POSTS DR 1100 A/R AND CR THE INCOME ACCOUNT."
1610 GOSUB 7200 : LOCATE 4,22: PRINT SPACE$(45); : LOCATE 4,22: INPUT "", EntryReference$
1620 IF EntryReference$ = "" THEN RETURN
1630 IF INSTR(EntryReference$, "|") > 0 THEN StatusMessage$ = "NO | IN THE INVOICE NUMBER" : GOSUB 7000 : GOTO 1610
1640 GOSUB 4100
1650 IF InvoiceIndex > 0 THEN StatusMessage$ = "INVOICE " + EntryReference$ + " HAS ALREADY BEEN BILLED" : GOSUB 7000 : GOTO 1610
1655 GOSUB 7200 : EditValue$ = TodayDate$
1660 EditRow = 5 : EditColumn = 22 : GOSUB 7600 : EntryDate$ = EditValue$
1670 GOSUB 7800 : IF DateIsValid = 1 THEN 1680
1672 IF DateErrorCode = 1 THEN StatusMessage$ = "USE MM/DD/YYYY (E.G. 10/20/2026)" ELSE StatusMessage$ = "THAT DATE DOES NOT EXIST"
1674 GOSUB 7000 : EditValue$ = EntryDate$ : GOTO 1660
1680 IF INSTR(EntryDate$, "|") > 0 THEN StatusMessage$ = "NO | IN THE DATE" : GOSUB 7000 : GOTO 1655
1690 GOSUB 7200 : LOCATE 6,22: PRINT SPACE$(45); : LOCATE 6,22: LINE INPUT "", CustomerName$
1700 IF CustomerName$ = "" THEN StatusMessage$ = "A CUSTOMER NAME IS REQUIRED" : GOSUB 7000 : GOTO 1690
1710 IF INSTR(CustomerName$, "|") > 0 THEN StatusMessage$ = "NO | IN THE CUSTOMER NAME" : GOSUB 7000 : GOTO 1690
1720 GOSUB 7200 : LOCATE 8,22: PRINT SPACE$(45); : LOCATE 8,22: IncomeAccountNumber = 4000 : INPUT "", IncomeAccountNumber
1730 AccountNumberInput = IncomeAccountNumber : GOSUB 4000
1740 IF FoundAccountIndex = 0 THEN StatusMessage$ = "ACCOUNT " + MID$(STR$(IncomeAccountNumber), 2) + " IS NOT IN THE CHART" : GOSUB 7000 : GOTO 1720
1750 IF AccountType(FoundAccountIndex) <> 4 THEN StatusMessage$ = "THAT ACCOUNT IS NOT AN INCOME ACCOUNT" : GOSUB 7000 : GOTO 1720
1760 LOCATE 8,34: PRINT SPACE$(38); : LOCATE 8,34: PRINT AccountName$(FoundAccountIndex)
1770 GOSUB 7200 : LOCATE 9,22: PRINT SPACE$(45); : LOCATE 9,22: EntryAmount = 0 : INPUT "", EntryAmount
1780 IF EntryAmount <= 0 THEN StatusMessage$ = "THE AMOUNT MUST BE POSITIVE" : GOSUB 7000 : GOTO 1770
1790 EntryAmount = INT(EntryAmount * 100 + 0.5) / 100
1800 LOCATE 9,22: PRINT SPACE$(45); : LOCATE 9,22: PRINT USING "####,###.##"; EntryAmount
1810 IF JournalEntryCount >= 300 THEN StatusMessage$ = "THE JOURNAL IS FULL" : GOSUB 7000 : RETURN
1820 LOCATE 20,5: PRINT SPACE$(70);
1830 LOCATE 20,5: PRINT USING "DR 1100 A/R   CR #####   AMOUNT ####,###.##"; IncomeAccountNumber; EntryAmount
1840 LOCATE 22,5: PRINT SPACE$(70); : PRINT "PRESS ENTER TO POST, OR N TO CANCEL: ";
1850 INPUT "", Confirmation$
1860 IF Confirmation$ = "N" OR Confirmation$ = "n" THEN RETURN
1870 JournalEntryCount = JournalEntryCount + 1
1880 JournalEntryType(JournalEntryCount) = 2 : JournalReference$(JournalEntryCount) = EntryReference$ : JournalDate$(JournalEntryCount) = EntryDate$ : JournalDescription$(JournalEntryCount) = CustomerName$ : JournalDebitAccount(JournalEntryCount) = 1100 : JournalCreditAccount(JournalEntryCount) = IncomeAccountNumber : JournalAmount(JournalEntryCount) = EntryAmount
1890 GOSUB 5300
1895 RETURN
2000 REM ---- ACCEPT A PAYMENT: DR CASH, CR A/R --------------------------
2010 CLS
2020 LOCATE 2,28: PRINT "ACCEPT A PAYMENT"
2030 LOCATE 3,5: PRINT "OPEN INVOICES"
2040 LOCATE 4,1: PRINT "  INVOICE"; TAB(15); "CUSTOMER"; TAB(42); "BILLED"; TAB(57); "PAID"; TAB(70); "OPEN"
2050 LOCATE 5,1: PRINT "  " + STRING$(10, "-") + "  " + STRING$(20, "-") + "  " + STRING$(11, "-") + "  " + STRING$(11, "-") + "  " + STRING$(11, "-")
2060 ScreenRow = 6
2070 FOR EntryCursor = 1 TO JournalEntryCount
2080 IF JournalEntryType(EntryCursor) <> 2 THEN 2140
2090 EntryReference$ = JournalReference$(EntryCursor) : GOSUB 4200
2100 OpenBalance = JournalAmount(EntryCursor) - PaidToDate
2110 IF OpenBalance <= 0.005 THEN 2140
2120 IF ScreenRow > 13 THEN 2135
2125 LOCATE ScreenRow,1: PRINT USING InvoiceReportFormat$; JournalReference$(EntryCursor); JournalDescription$(EntryCursor); JournalAmount(EntryCursor); PaidToDate; OpenBalance : ScreenRow = ScreenRow + 1
2130 GOTO 2140
2135 LOCATE 14,1: PRINT "  ... MORE OPEN INVOICES"
2140 NEXT EntryCursor
2150 IF ScreenRow > 6 THEN 2180
2160 LOCATE 16,5: PRINT "(NO OPEN INVOICES)"
2170 INPUT "", Confirmation$
2175 RETURN
2180 LOCATE 16,5: PRINT "INVOICE #  :";
2190 LOCATE 16,18: PRINT SPACE$(20); : LOCATE 16,18: INPUT "", EntryReference$
2200 IF EntryReference$ = "" THEN RETURN
2210 GOSUB 4100
2220 IF InvoiceIndex = 0 THEN StatusMessage$ = "INVOICE " + EntryReference$ + " IS NOT IN THE LEDGER" : GOSUB 7000 : GOTO 2190
2230 GOSUB 4200
2240 OpenBalance = JournalAmount(InvoiceIndex) - PaidToDate
2250 IF OpenBalance <= 0.005 THEN StatusMessage$ = "INVOICE " + EntryReference$ + " IS ALREADY PAID" : GOSUB 7000 : GOTO 2190
2260 GOSUB 7200 : LOCATE 18,5: PRINT USING "OPEN ####,###.##   AMOUNT: "; OpenBalance;
2270 LOCATE 18,33: PRINT SPACE$(20); : LOCATE 18,33: EntryAmount = OpenBalance : INPUT "", EntryAmount
2280 IF EntryAmount <= 0 THEN StatusMessage$ = "THE AMOUNT MUST BE POSITIVE" : GOSUB 7000 : GOTO 2270
2290 IF EntryAmount > OpenBalance + 0.005 THEN StatusMessage$ = "THE PAYMENT EXCEEDS THE OPEN BALANCE" : GOSUB 7000 : GOTO 2270
2300 EntryAmount = INT(EntryAmount * 100 + 0.5) / 100
2310 LOCATE 18,33: PRINT SPACE$(20); : LOCATE 18,33: PRINT USING "####,###.##"; EntryAmount
2320 GOSUB 7200 : LOCATE 19,5: PRINT "DATE       :"; : EditValue$ = TodayDate$
2330 EditRow = 19 : EditColumn = 18 : GOSUB 7600 : EntryDate$ = EditValue$
2340 GOSUB 7800 : IF DateIsValid = 1 THEN 2350
2342 IF DateErrorCode = 1 THEN StatusMessage$ = "USE MM/DD/YYYY (E.G. 10/20/2026)" ELSE StatusMessage$ = "THAT DATE DOES NOT EXIST"
2344 GOSUB 7000 : EditValue$ = EntryDate$ : GOTO 2330
2350 IF INSTR(EntryDate$, "|") > 0 THEN StatusMessage$ = "NO | IN THE DATE" : GOSUB 7000 : GOTO 2320
2360 IF JournalEntryCount >= 300 THEN StatusMessage$ = "THE JOURNAL IS FULL" : GOSUB 7000 : RETURN
2370 LOCATE 21,5: PRINT SPACE$(70);
2380 LOCATE 21,5: PRINT USING "DR 1000 CASH   CR 1100 A/R   AMOUNT ####,###.##"; EntryAmount
2390 LOCATE 22,5: PRINT SPACE$(70); : PRINT "PRESS ENTER TO POST, OR N TO CANCEL: ";
2400 INPUT "", Confirmation$
2410 IF Confirmation$ = "N" OR Confirmation$ = "n" THEN RETURN
2420 JournalEntryCount = JournalEntryCount + 1
2430 JournalEntryType(JournalEntryCount) = 3 : JournalReference$(JournalEntryCount) = EntryReference$ : JournalDate$(JournalEntryCount) = EntryDate$ : JournalDescription$(JournalEntryCount) = "PAYMENT " + JournalDescription$(InvoiceIndex) : JournalDebitAccount(JournalEntryCount) = 1000 : JournalCreditAccount(JournalEntryCount) = 1100 : JournalAmount(JournalEntryCount) = EntryAmount
2440 GOSUB 5300
2450 RETURN
2600 REM ---- TRIAL BALANCE ---------------------------------------------
2610 GOSUB 6000
2620 CLS
2630 PRINT "TRIAL BALANCE"
2640 PRINT "AS OF "; DATE$
2650 PRINT
2660 PRINT "  ACCOUNT"; TAB(47); "DEBIT"; TAB(59); "CREDIT"
2670 PRINT "  " + STRING$(36, "-") + "  " + STRING$(11, "-") + "  " + STRING$(11, "-")
2680 TotalDebits = 0 : TotalCredits = 0
2690 FOR AccountCursor = 1 TO AccountCount
2700 DebitBalance = 0 : CreditBalance = 0
2710 IF AccountBalance(AccountCursor) > 0 THEN DebitBalance = AccountBalance(AccountCursor)
2720 IF AccountBalance(AccountCursor) < 0 THEN CreditBalance = -AccountBalance(AccountCursor)
2730 TotalDebits = TotalDebits + DebitBalance : TotalCredits = TotalCredits + CreditBalance
2740 PRINT USING BalanceReportFormat$; AccountName$(AccountCursor); DebitBalance; CreditBalance
2750 NEXT AccountCursor
2760 PRINT "  " + STRING$(36, "-") + "  " + STRING$(11, "-") + "  " + STRING$(11, "-")
2770 PRINT USING BalanceReportFormat$; "TOTAL"; TotalDebits; TotalCredits
2780 IF TotalDebits <> TotalCredits THEN 2800
2790 PRINT "DEBITS EQUAL CREDITS." : RETURN
2800 PRINT "OUT OF BALANCE - CHECK THE JOURNAL." : RETURN
2900 REM ---- INCOME STATEMENT ------------------------------------------
2910 GOSUB 6000
2920 CLS
2930 PRINT "INCOME STATEMENT"
2940 PRINT "PERIOD ENDING "; DATE$
2950 PRINT
2960 PRINT "REVENUE"
2970 RevenueTotal = 0
2980 FOR AccountCursor = 1 TO AccountCount
2990 IF AccountType(AccountCursor) <> 4 THEN 3030
3000 ReportValue = -AccountBalance(AccountCursor) : RevenueTotal = RevenueTotal + ReportValue
3010 IF ReportValue <> 0 THEN PRINT USING AmountReportFormat$; AccountName$(AccountCursor); ReportValue
3030 NEXT AccountCursor
3040 PRINT USING AmountReportFormat$; "TOTAL REVENUE"; RevenueTotal
3050 PRINT
3060 PRINT "EXPENSES"
3070 ExpenseTotal = 0
3080 FOR AccountCursor = 1 TO AccountCount
3090 IF AccountType(AccountCursor) <> 5 THEN 3130
3100 ReportValue = AccountBalance(AccountCursor) : ExpenseTotal = ExpenseTotal + ReportValue
3110 IF ReportValue <> 0 THEN PRINT USING AmountReportFormat$; AccountName$(AccountCursor); ReportValue
3130 NEXT AccountCursor
3140 PRINT USING AmountReportFormat$; "TOTAL EXPENSES"; ExpenseTotal
3150 PRINT
3160 IF RevenueTotal - ExpenseTotal < 0 THEN 3190
3170 PRINT USING AmountReportFormat$; "NET INCOME"; RevenueTotal - ExpenseTotal
3180 RETURN
3190 PRINT USING AmountReportFormat$; "NET LOSS"; RevenueTotal - ExpenseTotal
3200 RETURN
3250 REM ---- BALANCE SHEET ---------------------------------------------
3260 GOSUB 6000
3270 CLS
3280 PRINT "BALANCE SHEET"
3290 PRINT "AS OF "; DATE$
3300 PRINT
3310 PRINT "ASSETS"
3320 TotalAssets = 0
3330 FOR AccountCursor = 1 TO AccountCount
3340 IF AccountType(AccountCursor) <> 1 THEN 3380
3350 ReportValue = AccountBalance(AccountCursor) : TotalAssets = TotalAssets + ReportValue
3360 IF ReportValue <> 0 THEN PRINT USING AmountReportFormat$; AccountName$(AccountCursor); ReportValue
3380 NEXT AccountCursor
3390 PRINT USING AmountReportFormat$; "TOTAL ASSETS"; TotalAssets
3400 PRINT
3410 PRINT "LIABILITIES"
3420 TotalLiabilities = 0
3430 FOR AccountCursor = 1 TO AccountCount
3440 IF AccountType(AccountCursor) <> 2 THEN 3480
3450 ReportValue = -AccountBalance(AccountCursor) : TotalLiabilities = TotalLiabilities + ReportValue
3460 IF ReportValue <> 0 THEN PRINT USING AmountReportFormat$; AccountName$(AccountCursor); ReportValue
3480 NEXT AccountCursor
3490 PRINT USING AmountReportFormat$; "TOTAL LIABILITIES"; TotalLiabilities
3500 PRINT
3510 PRINT "EQUITY"
3520 TotalEquity = 0
3530 FOR AccountCursor = 1 TO AccountCount
3540 IF AccountType(AccountCursor) <> 3 THEN 3580
3550 ReportValue = -AccountBalance(AccountCursor) : TotalEquity = TotalEquity + ReportValue
3560 IF ReportValue <> 0 THEN PRINT USING AmountReportFormat$; AccountName$(AccountCursor); ReportValue
3580 NEXT AccountCursor
3590 NetIncome = 0
3600 FOR AccountCursor = 1 TO AccountCount
3610 IF AccountType(AccountCursor) = 4 THEN NetIncome = NetIncome - AccountBalance(AccountCursor)
3620 IF AccountType(AccountCursor) = 5 THEN NetIncome = NetIncome - AccountBalance(AccountCursor)
3630 NEXT AccountCursor
3640 PRINT USING AmountReportFormat$; "CURRENT EARNINGS"; NetIncome
3650 TotalEquity = TotalEquity + NetIncome
3660 PRINT USING AmountReportFormat$; "TOTAL EQUITY"; TotalEquity
3670 PRINT
3680 PRINT USING AmountReportFormat$; "LIABILITIES + EQUITY"; TotalLiabilities + TotalEquity
3690 IF TotalAssets = TotalLiabilities + TotalEquity THEN 3720
3700 PRINT "CHECK: ASSETS DO NOT EQUAL LIABILITIES PLUS EQUITY."
3710 RETURN
3720 PRINT "BALANCED."
3730 RETURN
3750 REM ---- JOURNAL: JournalCode$ J = JOURNAL, B = BILLING, P = PAYMENT --
3760 CLS
3770 PRINT "JOURNAL"
3780 PRINT "AS OF "; DATE$
3790 PRINT
3800 PRINT "  T"; TAB(6); "DATE"; TAB(18); "REF"; TAB(30); "DESCRIPTION"; TAB(54); "DR/CR"; TAB(65); "AMOUNT"
3810 PRINT "  " + "-" + "  " + STRING$(10, "-") + "  " + STRING$(10, "-") + "  " + STRING$(22, "-") + "  " + STRING$(4, "-") + "/" + STRING$(4, "-") + "  " + STRING$(11, "-")
3820 IF JournalEntryCount > 0 THEN 3850
3830 PRINT "  (NO ENTRIES YET)"
3840 RETURN
3850 FOR EntryCursor = 1 TO JournalEntryCount
3860 JournalCode$ = "J"
3870 IF JournalEntryType(EntryCursor) = 2 THEN JournalCode$ = "B"
3880 IF JournalEntryType(EntryCursor) = 3 THEN JournalCode$ = "P"
3890 PRINT USING JournalReportFormat$; JournalCode$; JournalDate$(EntryCursor); JournalReference$(EntryCursor); JournalDescription$(EntryCursor); JournalDebitAccount(EntryCursor); JournalCreditAccount(EntryCursor); JournalAmount(EntryCursor)
3900 NEXT EntryCursor
3910 PRINT "  " + "-" + "  " + STRING$(10, "-") + "  " + STRING$(10, "-") + "  " + STRING$(22, "-") + "  " + STRING$(4, "-") + "/" + STRING$(4, "-") + "  " + STRING$(11, "-")
3920 PostedTotal = 0
3930 FOR EntryCursor = 1 TO JournalEntryCount : PostedTotal = PostedTotal + JournalAmount(EntryCursor) : NEXT EntryCursor
3940 PRINT USING "  ENTRIES: ###   TOTAL POSTED: ####,###.##"; JournalEntryCount; PostedTotal
3950 RETURN
4000 REM ---- FIND AN ACCOUNT: AccountNumberInput IN, FoundAccountIndex OUT --
4010 FoundAccountIndex = 0
4020 FOR AccountCursor = 1 TO AccountCount
4030 IF AccountNumber(AccountCursor) = AccountNumberInput THEN FoundAccountIndex = AccountCursor : AccountCursor = AccountCount
4040 NEXT AccountCursor
4050 RETURN
4100 REM ---- FIND A BILLING: EntryReference$ IN, InvoiceIndex OUT ----------
4110 InvoiceIndex = 0
4120 FOR LedgerCursor = 1 TO JournalEntryCount
4130 IF JournalEntryType(LedgerCursor) = 2 AND JournalReference$(LedgerCursor) = EntryReference$ THEN InvoiceIndex = LedgerCursor : LedgerCursor = JournalEntryCount
4140 NEXT LedgerCursor
4150 RETURN
4200 REM ---- TOTAL PAYMENTS AGAINST EntryReference$: PaidToDate OUT -------
4210 PaidToDate = 0
4220 FOR LedgerCursor = 1 TO JournalEntryCount
4230 IF JournalEntryType(LedgerCursor) = 3 AND JournalReference$(LedgerCursor) = EntryReference$ THEN PaidToDate = PaidToDate + JournalAmount(LedgerCursor)
4240 NEXT LedgerCursor
4250 RETURN
4400 REM ---- SPLIT DataLine$ ON "|" INTO DataField$(1..FieldCount) ---------
4410 FieldCount = 0 : FieldStart = 1
4420 FieldEnd = INSTR(FieldStart, DataLine$, "|")
4430 IF FieldEnd = 0 THEN FieldCount = FieldCount + 1 : DataField$(FieldCount) = MID$(DataLine$, FieldStart) : RETURN
4440 FieldCount = FieldCount + 1 : DataField$(FieldCount) = MID$(DataLine$, FieldStart, FieldEnd - FieldStart)
4450 FieldStart = FieldEnd + 1 : GOTO 4420
5000 REM ---- READ THE CHART OF ACCOUNTS FROM DATA ----------------------
5010 AccountCount = 0
5020 READ AccountNumberInput, DataAccountName$, DataAccountType
5030 IF AccountNumberInput = 0 THEN 5060
5040 AccountCount = AccountCount + 1 : AccountNumber(AccountCount) = AccountNumberInput : AccountName$(AccountCount) = DataAccountName$ : AccountType(AccountCount) = DataAccountType
5050 GOTO 5020
5060 RETURN
5100 REM ---- CREATE IF NEEDED AND LOAD THE JOURNAL FILE ---------------
5110 OPEN DataFilePath$ FOR APPEND AS #1
5120 CLOSE #1
5130 OPEN DataFilePath$ FOR INPUT AS #1
5140 JournalEntryCount = 0
5150 IF EOF(1) THEN 5250
5160 IF JournalEntryCount >= 300 THEN 5250
5170 LINE INPUT #1, DataLine$
5180 GOSUB 4400
5190 IF FieldCount <> 7 THEN 5230
5200 JournalEntryCount = JournalEntryCount + 1
5210 JournalEntryType(JournalEntryCount) = VAL(DataField$(1)) : JournalReference$(JournalEntryCount) = DataField$(2) : JournalDate$(JournalEntryCount) = DataField$(3) : JournalDescription$(JournalEntryCount) = DataField$(4) : JournalDebitAccount(JournalEntryCount) = VAL(DataField$(5)) : JournalCreditAccount(JournalEntryCount) = VAL(DataField$(6)) : JournalAmount(JournalEntryCount) = VAL(DataField$(7))
5220 GOTO 5150
5230 CLS : PRINT "THE DATA FILE IS NOT IN THIS PROGRAM'S FORMAT."
5240 PRINT "RENAME OR DELETE samples\accounting.dat, THEN RUN AGAIN."
5245 END
5250 CLOSE #1
5260 RETURN
5300 REM ---- APPEND THE CURRENT ENTRY TO THE JOURNAL FILE -------------
5310 DataLine$ = MID$(STR$(JournalEntryType(JournalEntryCount)), 2) + "|" + JournalReference$(JournalEntryCount) + "|" + JournalDate$(JournalEntryCount) + "|" + JournalDescription$(JournalEntryCount) + "|" + MID$(STR$(JournalDebitAccount(JournalEntryCount)), 2) + "|" + MID$(STR$(JournalCreditAccount(JournalEntryCount)), 2) + "|" + MID$(STR$(JournalAmount(JournalEntryCount)), 2)
5320 OPEN DataFilePath$ FOR APPEND AS #1
5330 PRINT #1, DataLine$
5340 CLOSE #1
5350 RETURN
6000 REM ---- BALANCES: DEBIT POSITIVE, CREDIT NEGATIVE -----------------
6010 FOR AccountCursor = 1 TO AccountCount : AccountBalance(AccountCursor) = 0 : NEXT AccountCursor
6020 FOR EntryCursor = 1 TO JournalEntryCount
6030 FOR AccountCursor = 1 TO AccountCount
6040 IF AccountNumber(AccountCursor) = JournalDebitAccount(EntryCursor) THEN AccountBalance(AccountCursor) = AccountBalance(AccountCursor) + JournalAmount(EntryCursor)
6050 IF AccountNumber(AccountCursor) = JournalCreditAccount(EntryCursor) THEN AccountBalance(AccountCursor) = AccountBalance(AccountCursor) - JournalAmount(EntryCursor)
6060 NEXT AccountCursor
6070 NEXT EntryCursor
6080 RETURN
7000 REM ---- SHOW THE CURRENT MESSAGE ON THE ERROR LINE ---------------
7010 LOCATE 22,5: PRINT SPACE$(70);
7020 LOCATE 22,5: PRINT StatusMessage$;
7030 RETURN
7200 REM ---- CLEAR THE ERROR LINE BEFORE THE NEXT FIELD ----------------
7210 LOCATE 22,5: PRINT SPACE$(70);
7220 RETURN
7600 REM ---- EDIT A FIELD AT EditRow,EditColumn. EditValue$ IS THE RESULT --
7605 EditingField = 0
7610 LOCATE EditRow, EditColumn: PRINT EditValue$; SPACE$(25); : LOCATE EditRow, EditColumn + LEN(EditValue$)
7620 KeyPress$ = INKEY$
7625 IF KeyPress$ <> "" THEN 7640
7630 FOR WaitCounter = 1 TO 300 : NEXT WaitCounter
7635 GOTO 7620
7640 IF KeyPress$ = CHR$(13) THEN 7740
7650 IF KeyPress$ = CHR$(8) THEN 7700
7660 IF LEN(KeyPress$) <> 1 THEN 7620
7670 CharacterCode = ASC(KeyPress$)
7675 IF CharacterCode < 32 OR CharacterCode > 126 THEN 7620
7680 IF EditingField = 0 THEN EditValue$ = "" : EditingField = 1
7690 EditValue$ = EditValue$ + KeyPress$
7695 GOTO 7720
7700 IF LEN(EditValue$) > 0 THEN EditValue$ = LEFT$(EditValue$, LEN(EditValue$) - 1) : EditingField = 1
7720 LOCATE EditRow, EditColumn: PRINT EditValue$; SPACE$(25); : LOCATE EditRow, EditColumn + LEN(EditValue$)
7730 GOTO 7620
7740 LOCATE EditRow, EditColumn: PRINT EditValue$; SPACE$(25); : LOCATE EditRow, EditColumn
7750 RETURN
7800 REM ---- VALIDATE EntryDate$ AS MM/DD/YYYY. DateIsValid, DateErrorCode --
7810 DateIsValid = 0 : DateErrorCode = 1
7820 IF LEN(EntryDate$) <> 10 THEN RETURN
7830 IF MID$(EntryDate$, 3, 1) <> "/" OR MID$(EntryDate$, 6, 1) <> "/" THEN RETURN
7840 FOR CharacterPosition = 1 TO 10
7850 IF CharacterPosition = 3 OR CharacterPosition = 6 THEN 7870
7860 CharacterCode = ASC(MID$(EntryDate$, CharacterPosition, 1)) : IF CharacterCode < 48 OR CharacterCode > 57 THEN RETURN
7870 NEXT CharacterPosition
7880 MonthNumber = VAL(MID$(EntryDate$, 1, 2)) : DayNumber = VAL(MID$(EntryDate$, 4, 2)) : YearNumber = VAL(MID$(EntryDate$, 7, 4))
7890 IF MonthNumber < 1 OR MonthNumber > 12 THEN DateErrorCode = 2 : RETURN
7900 IF YearNumber < 1900 OR YearNumber > 2099 THEN DateErrorCode = 2 : RETURN
7910 DaysInMonth = 31
7920 IF MonthNumber = 4 OR MonthNumber = 6 OR MonthNumber = 9 OR MonthNumber = 11 THEN DaysInMonth = 30
7930 IF MonthNumber <> 2 THEN 7960
7940 DaysInMonth = 28
7950 IF YearNumber MOD 4 = 0 THEN DaysInMonth = 29
7952 IF YearNumber MOD 100 = 0 AND YearNumber MOD 400 <> 0 THEN DaysInMonth = 28
7960 IF DayNumber < 1 OR DayNumber > DaysInMonth THEN DateErrorCode = 2 : RETURN
7970 DateIsValid = 1
7980 RETURN
8000 REM ---- WAIT FOR THE READER --------------------------------------
8010 LOCATE 24,5: PRINT SPACE$(70);
8020 LOCATE 24,5: PRINT "PRESS ENTER TO CONTINUE: ";
8030 INPUT "", Confirmation$
8040 RETURN
9000 REM ---- THE CHART: NUMBER, NAME, TYPE ----------------------------
9010 DATA 1000,CASH,1
9020 DATA 1100,ACCOUNTS RECEIVABLE,1
9030 DATA 1200,SUPPLIES,1
9040 DATA 1500,EQUIPMENT,1
9050 DATA 2000,ACCOUNTS PAYABLE,2
9060 DATA 2100,SALES TAX PAYABLE,2
9070 DATA 3000,OWNERS EQUITY,3
9080 DATA 3100,RETAINED EARNINGS,3
9090 DATA 4000,SERVICE REVENUE,4
9100 DATA 5000,RENT EXPENSE,5
9110 DATA 5100,SALARIES EXPENSE,5
9120 DATA 5200,UTILITIES EXPENSE,5
9130 DATA 5300,SUPPLIES EXPENSE,5
9140 DATA 0,END,0
