p3LastProcessedDay  EQU 0x110  ; last day handled by the scheduler
p3CalcTemp          EQU 0x111  ; temporary value used in comparisons
p3BestMedicine      EQU 0x112  ; best next medicine found
p3BestHour          EQU 0x113  ; hour of the best next medicine
p3BestMinute        EQU 0x114  ; minute of the best next medicine
p3ScheduleFound     EQU 0x115  ; 1 if a next medicine was found
p3ScheduleIndex     EQU 0x116  ; reserved scheduler helper
p3ScheduleMask      EQU 0x117  ; reserved scheduler mask
p3CandidateTomorrow EQU 0x118  ; 1 if current candidate is tomorrow
p3BestTomorrow      EQU 0x119  ; 1 if current best is tomorrow


; TIMER INITIALIZATION


timerInit:
        ; clear scheduler variables
        BANKSEL p3LastProcessedDay

        CLRF    p3LastProcessedDay
        CLRF    p3CalcTemp
        CLRF    p3BestMedicine
        CLRF    p3BestHour
        CLRF    p3BestMinute
        CLRF    p3ScheduleFound
        CLRF    p3ScheduleIndex
        CLRF    p3ScheduleMask
        CLRF    p3CandidateTomorrow
        CLRF    p3BestTomorrow


        BANKSEL TickL

        CLRF    TickL
        CLRF    TickH

        CLRF    ClockHour
        CLRF    ClockMinute
        CLRF    ClockSecond
        CLRF    ClockSub

        CLRF    DaySerial


        ; set Timer2 period
        BANKSEL PR2
        MOVLW   TIMER_PERIOD
        MOVWF   PR2


        BANKSEL TMR2
        CLRF    TMR2


        ; clear old Timer2 flag
        BANKSEL PIR1
        BCF     PIR1,TMR2IF


        ; enable Timer2 interrupt
        BANKSEL PIE1
        BSF     PIE1,TMR2IE


        ; enable peripheral and global interrupts
        BANKSEL INTCON
        BSF     INTCON,PEIE
        BSF     INTCON,GIE


        ; start Timer2
        BANKSEL T2CON
        MOVLW   TIMER_CONTROL
        MOVWF   T2CON


        ; return to bank 0 before leaving
        BANKSEL PORTA
        BCF     STATUS,IRP

        RETURN


; TIMER ISR
; called every 10 ms from the main interrupt wrapper

timerIsr:

        ; increase the 16-bit tick counter

        BANKSEL TickL
        INCF    TickL,F

        BTFSC   STATUS,Z
        INCF    TickH,F


        ; check if the clock is paused

        BANKSEL ClockRun
        BTFSS   ClockRun,0
        RETURN


        ; add the new tick to the software clock

        BANKSEL ClockSub

        MOVLW   SIM_STEP
        ADDWF   ClockSub,F


        MOVLW   TICKS_SECOND
        SUBWF   ClockSub,W

        BTFSS   STATUS,C
        RETURN


        ; remove one full second from ClockSub
        MOVWF   ClockSub


        ; increase seconds

        INCF    ClockSecond,F


        MOVLW   D'60'
        SUBWF   ClockSecond,W

        BTFSS   STATUS,C
        RETURN


        ; if seconds reach 60, reset them and increase minutes

        CLRF    ClockSecond
        INCF    ClockMinute,F


        MOVLW   D'60'
        SUBWF   ClockMinute,W

        BTFSS   STATUS,C
        RETURN


        ; if minutes reach 60, reset them and increase hours

        CLRF    ClockMinute
        INCF    ClockHour,F


        MOVLW   D'24'
        SUBWF   ClockHour,W

        BTFSS   STATUS,C
        RETURN


        ; if hour reaches 24, start a new day

        CLRF    ClockHour
        INCF    DaySerial,F

        RETURN


; SCHEDULE RESET
; reset scheduler values after medicine times are set


scheduleReset:
        

        BANKSEL PendingMask

        CLRF    PendingMask
        CLRF    FiredMask

        CLRF    NextMed
        CLRF    NextHour
        CLRF    NextMinute
        CLRF    NextTomorrow


        BANKSEL p3LastProcessedDay

        MOVLW   0xFF
        MOVWF   p3LastProcessedDay

        CLRF    p3CalcTemp
        CLRF    p3BestMedicine
        CLRF    p3BestHour
        CLRF    p3BestMinute
        CLRF    p3ScheduleFound
        CLRF    p3ScheduleIndex
        CLRF    p3ScheduleMask
        CLRF    p3CandidateTomorrow
        CLRF    p3BestTomorrow


        BANKSEL PORTA
        BCF     STATUS,IRP

        RETURN


; SCHEDULE POLL
; check if a new day started
; check if any medicine is due now
; find the next medicine time

schedulePoll:

        ; do not check schedule while clock is paused

        BANKSEL ClockRun
        BTFSS   ClockRun,0
        RETURN


        ; check if ViewDay changed

        BANKSEL ViewDay
        MOVF    ViewDay,W

        BANKSEL p3LastProcessedDay
        XORWF   p3LastProcessedDay,W

        BTFSC   STATUS,Z
        GOTO    sameDay


        ; save the new day
        BANKSEL ViewDay
        MOVF    ViewDay,W

        BANKSEL p3LastProcessedDay
        MOVWF   p3LastProcessedDay


        ; clear FiredMask so medicines can fire again today
        BANKSEL FiredMask
        CLRF    FiredMask


; CHECK MEDICINE 1

sameDay:

        BANKSEL ViewHour

        MOVF    ViewHour,W
        SUBWF   Med1Hour,W

        BTFSS   STATUS,Z
        GOTO    checkMedicine2


        MOVF    ViewMinute,W
        SUBWF   Med1Minute,W

        BTFSS   STATUS,Z
        GOTO    checkMedicine2


        ; check if medicine 1 already fired today
        BTFSC   FiredMask,0
        GOTO    checkMedicine2


        ; mark medicine 1 as fired and pending
        BSF     FiredMask,0
        BSF     PendingMask,0


; CHECK MEDICINE 2

checkMedicine2:

        BANKSEL ViewHour

        MOVF    ViewHour,W
        SUBWF   Med2Hour,W

        BTFSS   STATUS,Z
        GOTO    checkMedicine3


        MOVF    ViewMinute,W
        SUBWF   Med2Minute,W

        BTFSS   STATUS,Z
        GOTO    checkMedicine3


        BTFSC   FiredMask,1
        GOTO    checkMedicine3


        BSF     FiredMask,1
        BSF     PendingMask,1


; CHECK MEDICINE 3

checkMedicine3:

        BANKSEL ViewHour

        MOVF    ViewHour,W
        SUBWF   Med3Hour,W

        BTFSS   STATUS,Z
        GOTO    schedulePollExit


        MOVF    ViewMinute,W
        SUBWF   Med3Minute,W

        BTFSS   STATUS,Z
        GOTO    schedulePollExit


        BTFSC   FiredMask,2
        GOTO    schedulePollExit


        BSF     FiredMask,2
        BSF     PendingMask,2


; FIND THE NEXT EVENT

schedulePollExit:

        BANKSEL PendingMask

        MOVF    PendingMask,F
        BTFSC   STATUS,Z
        GOTO    noPendingNext


        ; if more than one is pending, take the lowest medicine number first
        BTFSC   PendingMask,0
        GOTO    nextPending1

        BTFSC   PendingMask,1
        GOTO    nextPending2

        GOTO    nextPending3


; PENDING MEDICINE 1,2,3

nextPending1:

        MOVLW   D'1'

        BANKSEL p3BestMedicine
        MOVWF   p3BestMedicine


        BANKSEL Med1Hour
        MOVF    Med1Hour,W

        BANKSEL p3BestHour
        MOVWF   p3BestHour


        BANKSEL Med1Minute
        MOVF    Med1Minute,W

        BANKSEL p3BestMinute
        MOVWF   p3BestMinute


        BANKSEL p3BestTomorrow
        CLRF    p3BestTomorrow

        GOTO    finalizeNext




nextPending2:

        MOVLW   D'2'

        BANKSEL p3BestMedicine
        MOVWF   p3BestMedicine


        BANKSEL Med2Hour
        MOVF    Med2Hour,W

        BANKSEL p3BestHour
        MOVWF   p3BestHour


        BANKSEL Med2Minute
        MOVF    Med2Minute,W

        BANKSEL p3BestMinute
        MOVWF   p3BestMinute


        BANKSEL p3BestTomorrow
        CLRF    p3BestTomorrow

        GOTO    finalizeNext




nextPending3:

        MOVLW   D'3'

        BANKSEL p3BestMedicine
        MOVWF   p3BestMedicine


        BANKSEL Med3Hour
        MOVF    Med3Hour,W

        BANKSEL p3BestHour
        MOVWF   p3BestHour


        BANKSEL Med3Minute
        MOVF    Med3Minute,W

        BANKSEL p3BestMinute
        MOVWF   p3BestMinute


        BANKSEL p3BestTomorrow
        CLRF    p3BestTomorrow

        GOTO    finalizeNext


; FIND NEXT MEDICINE IF NOTHING IS PENDING

noPendingNext:

        BANKSEL p3ScheduleFound

        CLRF    p3ScheduleFound
        CLRF    p3BestMedicine
        CLRF    p3BestHour
        CLRF    p3BestMinute
        CLRF    p3BestTomorrow


; CHECK MEDICINE 1 AS NEXT CANDIDATE

        BANKSEL Med1Hour

        MOVF    Med1Hour,W
        XORLW   NO_DIGIT

        BTFSC   STATUS,Z
        GOTO    checkNextMed2


        ; compare medicine 1 hour with current hour

        MOVF    ViewHour,W
        SUBWF   Med1Hour,W


        ; this time already passed, so use tomorrow
        BTFSS   STATUS,C
        GOTO    med1Tomorrow


        ; the medicine is still later today
        BTFSS   STATUS,Z
        GOTO    med1Today


        ; same hour, so compare minutes

        MOVF    ViewMinute,W
        SUBWF   Med1Minute,W


        ; the minute already passed, so use tomorrow
        BTFSS   STATUS,C
        GOTO    med1Tomorrow


        ; same time is already handled, so next one is tomorrow
        BTFSC   STATUS,Z
        GOTO    med1Tomorrow


        ; the medicine is still later this hour
        GOTO    med1Today


med1Today:

        BANKSEL p3BestTomorrow
        CLRF    p3BestTomorrow

        GOTO    saveMedicine1


med1Tomorrow:

        BANKSEL p3BestTomorrow
        MOVLW   D'1'
        MOVWF   p3BestTomorrow


saveMedicine1:

        MOVLW   D'1'

        BANKSEL p3BestMedicine
        MOVWF   p3BestMedicine


        BANKSEL Med1Hour
        MOVF    Med1Hour,W

        BANKSEL p3BestHour
        MOVWF   p3BestHour


        BANKSEL Med1Minute
        MOVF    Med1Minute,W

        BANKSEL p3BestMinute
        MOVWF   p3BestMinute


        BANKSEL p3ScheduleFound
        MOVLW   D'1'
        MOVWF   p3ScheduleFound

        GOTO    checkNextMed2


; CHECK MEDICINE 2 AS NEXT CANDIDATE

checkNextMed2:

        BANKSEL Med2Hour

        MOVF    Med2Hour,W
        XORLW   NO_DIGIT

        BTFSC   STATUS,Z
        GOTO    checkNextMed3


        ; check if this medicine is today or tomorrow

        MOVF    ViewHour,W
        SUBWF   Med2Hour,W


        BTFSS   STATUS,C
        GOTO    med2Tomorrow


        BTFSS   STATUS,Z
        GOTO    med2Today


        ; Same hour
        MOVF    ViewMinute,W
        SUBWF   Med2Minute,W


        BTFSS   STATUS,C
        GOTO    med2Tomorrow


        BTFSC   STATUS,Z
        GOTO    med2Tomorrow


        GOTO    med2Today


med2Today:

        BANKSEL p3CandidateTomorrow
        CLRF    p3CandidateTomorrow

        GOTO    compareMedicine2


med2Tomorrow:

        BANKSEL p3CandidateTomorrow
        MOVLW   D'1'
        MOVWF   p3CandidateTomorrow


; COMPARE MEDICINE 2 WITH CURRENT BEST

compareMedicine2:

        BANKSEL p3ScheduleFound

        MOVF    p3ScheduleFound,F
        BTFSC   STATUS,Z
        GOTO    saveMedicine2


        ; prefer a medicine today over one tomorrow

        BANKSEL p3CandidateTomorrow

        MOVF    p3CandidateTomorrow,W
        XORWF   p3BestTomorrow,W


        ; both are on the same day, so compare their times
        BTFSC   STATUS,Z
        GOTO    compareMedicine2Time


        ; check if this candidate is today
        BTFSS   p3CandidateTomorrow,0
        GOTO    saveMedicine2


        ; keep the current best because it is today
        GOTO    checkNextMed3


; COMPARE MEDICINE 2 TIME

compareMedicine2Time:

        BANKSEL Med2Hour
        MOVF    Med2Hour,W


        BANKSEL p3CalcTemp
        MOVWF   p3CalcTemp


        ; compare candidate hour with the current best hour
        MOVF    p3BestHour,W
        SUBWF   p3CalcTemp,W


        ; candidate is earlier, so save it
        BTFSS   STATUS,C
        GOTO    saveMedicine2


        ; candidate is later, so keep the current best
        BTFSS   STATUS,Z
        GOTO    checkNextMed3


        ; same hour, so compare minutes

        BANKSEL Med2Minute
        MOVF    Med2Minute,W


        BANKSEL p3CalcTemp
        MOVWF   p3CalcTemp


        MOVF    p3BestMinute,W
        SUBWF   p3CalcTemp,W


        ; candidate minute is earlier, so save it
        BTFSS   STATUS,C
        GOTO    saveMedicine2


        ; same or later time
        ; keep the lower medicine number
        GOTO    checkNextMed3


saveMedicine2:

        MOVLW   D'2'

        BANKSEL p3BestMedicine
        MOVWF   p3BestMedicine


        BANKSEL Med2Hour
        MOVF    Med2Hour,W

        BANKSEL p3BestHour
        MOVWF   p3BestHour


        BANKSEL Med2Minute
        MOVF    Med2Minute,W

        BANKSEL p3BestMinute
        MOVWF   p3BestMinute


        BANKSEL p3CandidateTomorrow
        MOVF    p3CandidateTomorrow,W
        MOVWF   p3BestTomorrow


        MOVLW   D'1'
        MOVWF   p3ScheduleFound

        GOTO    checkNextMed3


; CHECK MEDICINE 3 AS NEXT CANDIDATE

checkNextMed3:

        BANKSEL Med3Hour

        MOVF    Med3Hour,W
        XORLW   NO_DIGIT

        BTFSC   STATUS,Z
        GOTO    finalizeNext


        ; check if this medicine is today or tomorrow

        MOVF    ViewHour,W
        SUBWF   Med3Hour,W


        BTFSS   STATUS,C
        GOTO    med3Tomorrow


        BTFSS   STATUS,Z
        GOTO    med3Today


        ; same hour, so compare minutes
        MOVF    ViewMinute,W
        SUBWF   Med3Minute,W


        BTFSS   STATUS,C
        GOTO    med3Tomorrow


        BTFSC   STATUS,Z
        GOTO    med3Tomorrow


        GOTO    med3Today


med3Today:

        BANKSEL p3CandidateTomorrow
        CLRF    p3CandidateTomorrow

        GOTO    compareMedicine3


med3Tomorrow:

        BANKSEL p3CandidateTomorrow
        MOVLW   D'1'
        MOVWF   p3CandidateTomorrow


; COMPARE MEDICINE 3 WITH CURRENT BEST

compareMedicine3:

        BANKSEL p3ScheduleFound

        MOVF    p3ScheduleFound,F
        BTFSC   STATUS,Z
        GOTO    saveMedicine3


        BANKSEL p3CandidateTomorrow

        MOVF    p3CandidateTomorrow,W
        XORWF   p3BestTomorrow,W


        ; both are today or both are tomorrow
        BTFSC   STATUS,Z
        GOTO    compareMedicine3Time


        ; today is better than tomorrow
        BTFSS   p3CandidateTomorrow,0
        GOTO    saveMedicine3


        GOTO    finalizeNext


; COMPARE MEDICINE 3 TIME

compareMedicine3Time:

        BANKSEL Med3Hour
        MOVF    Med3Hour,W


        BANKSEL p3CalcTemp
        MOVWF   p3CalcTemp


        MOVF    p3BestHour,W
        SUBWF   p3CalcTemp,W


        ; candidate is earlier, so save it
        BTFSS   STATUS,C
        GOTO    saveMedicine3


        ; candidate is later, so keep the current best
        BTFSS   STATUS,Z
        GOTO    finalizeNext


        ; same hour, so compare minutes

        BANKSEL Med3Minute
        MOVF    Med3Minute,W


        BANKSEL p3CalcTemp
        MOVWF   p3CalcTemp


        MOVF    p3BestMinute,W
        SUBWF   p3CalcTemp,W


        ; candidate minute is earlier, so save it
        BTFSS   STATUS,C
        GOTO    saveMedicine3


        ; same or later time
        ; keep the lower medicine number
        GOTO    finalizeNext


saveMedicine3:

        MOVLW   D'3'

        BANKSEL p3BestMedicine
        MOVWF   p3BestMedicine


        BANKSEL Med3Hour
        MOVF    Med3Hour,W

        BANKSEL p3BestHour
        MOVWF   p3BestHour


        BANKSEL Med3Minute
        MOVF    Med3Minute,W

        BANKSEL p3BestMinute
        MOVWF   p3BestMinute


        BANKSEL p3CandidateTomorrow
        MOVF    p3CandidateTomorrow,W
        MOVWF   p3BestTomorrow


        MOVLW   D'1'
        MOVWF   p3ScheduleFound

        GOTO    finalizeNext


; UPDATE NEXT MEDICINE VALUES
; only refresh the display if Next values changed

finalizeNext:

        ; compare medicine number

        BANKSEL p3BestMedicine
        MOVF    p3BestMedicine,W


        BANKSEL NextMed
        XORWF   NextMed,W

        BTFSS   STATUS,Z
        GOTO    nextValuesChanged


        ; compare today/tomorrow flag

        BANKSEL p3BestTomorrow
        MOVF    p3BestTomorrow,W


        BANKSEL NextTomorrow
        XORWF   NextTomorrow,W

        BTFSS   STATUS,Z
        GOTO    nextValuesChanged


        ; compare hour

        BANKSEL p3BestHour
        MOVF    p3BestHour,W


        BANKSEL NextHour
        XORWF   NextHour,W

        BTFSS   STATUS,Z
        GOTO    nextValuesChanged


        ; compare minute

        BANKSEL p3BestMinute
        MOVF    p3BestMinute,W


        BANKSEL NextMinute
        XORWF   NextMinute,W

        BTFSS   STATUS,Z
        GOTO    nextValuesChanged


        ; nothing changed, just keep the values
        GOTO    writeNextValues


nextValuesChanged:

        BANKSEL UiDirty
        BSF     UiDirty,0


; SAVE FINAL NEXT VALUES

writeNextValues:

        BANKSEL p3BestMedicine
        MOVF    p3BestMedicine,W

        BANKSEL NextMed
        MOVWF   NextMed


        BANKSEL p3BestHour
        MOVF    p3BestHour,W

        BANKSEL NextHour
        MOVWF   NextHour


        BANKSEL p3BestMinute
        MOVF    p3BestMinute,W

        BANKSEL NextMinute
        MOVWF   NextMinute


        BANKSEL p3BestTomorrow
        MOVF    p3BestTomorrow,W

        BANKSEL NextTomorrow
        MOVWF   NextTomorrow


schedulePollReturn:

        BANKSEL PORTA
        BCF     STATUS,IRP

        RETURN


; SCHEDULE POP
; W = 0 if there is no pending medicine
; W = medicine number 1, 2, or 3
; clear only the pending bit for the medicine we return
; do not clear FiredMask


schedulePop:

        BANKSEL PendingMask

        MOVF    PendingMask,F

        BTFSC   STATUS,Z
        GOTO    noPendingMedicine


        ; check medicine 1 first
        BTFSC   PendingMask,0
        GOTO    popMedicine1


        ; then check medicine 2
        BTFSC   PendingMask,1
        GOTO    popMedicine2


        ; otherwise return medicine 3
        GOTO    popMedicine3


popMedicine1:

        BCF     PendingMask,0
        MOVLW   D'1'

        RETURN


popMedicine2:

        BCF     PendingMask,1
        MOVLW   D'2'

        RETURN


popMedicine3:

        BCF     PendingMask,2
        MOVLW   D'3'

        RETURN


noPendingMedicine:

        CLRW

        RETURN
