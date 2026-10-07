; INITIAL OUTPUTS AND BUTTON STATE ;;;;;;;;;;;;;;;;;;;;;;;;;

alarmInit
        CLRF    OutShadow
        MOVF    OutShadow,W
        MOVWF   ALARM_PORT      ; All four outputs OFF
        CLRF    ActiveMed
        CLRF    LastMed
        CLRF    ResultCode
        CLRF    AckEvent
        CLRF    AckArmed
        MOVLW   D'1'
        MOVWF   AckCandidate    ; Must first observe a stable release
        MOVWF   AckStable
        RETURN

; COHERENT SNAPSHOT ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

timeSnapshot
        BCF     INTCON,GIE
        MOVF    TickL,W
        MOVWF   NowL
        MOVF    TickH,W
        MOVWF   NowH
        MOVF    ClockHour,W
        MOVWF   ViewHour
        MOVF    ClockMinute,W
        MOVWF   ViewMinute
        MOVF    ClockSecond,W
        MOVWF   ViewSecond
        MOVF    DaySerial,W
        MOVWF   ViewDay
        BSF     INTCON,GIE

        MOVF    ViewMinute,W
        SUBWF   PreviousMinute,W
        BTFSS   STATUS,Z
        GOTO    p4ClockChanged
        MOVF    ViewDay,W
        SUBWF   PreviousDay,W
        BTFSC   STATUS,Z
        RETURN
p4ClockChanged
        MOVF    ViewMinute,W
        MOVWF   PreviousMinute
        MOVF    ViewDay,W
        MOVWF   PreviousDay
        MOVLW   D'1'
        MOVWF   UiDirty
        RETURN

; 16-BIT ELAPSED TIME ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Input: StartH:StartL. Output: ElapsedH:ElapsedL
; Subtraction wraps correctly when the 16-bit tick wraps

elapsedTime
        MOVF    StartL,W
        SUBWF   NowL,W          
        MOVWF   ElapsedL
        MOVF    StartH,W        
        BTFSS   STATUS,C        
        ADDLW   D'1'            
        SUBWF   NowH,W
        MOVWF   ElapsedH
        RETURN

; ACKNOWLEDGMENT BUTTON DEBOUNCE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ackPoll
        CLRF    AckEvent
        CLRF    RawAck
        BTFSS   ALARM_PORT,ACK_BIT
        INCF    RawAck,F        ; LOW pin becomes logical pressed=1

        MOVF    AckCandidate,W
        SUBWF   RawAck,W
        BTFSC   STATUS,Z
        GOTO    p4AckSame

        MOVF    RawAck,W
        MOVWF   AckCandidate
        MOVF    NowL,W
        MOVWF   AckStartL
        MOVF    NowH,W
        MOVWF   AckStartH
        RETURN                  ; Start timing the new candidate

p4AckSame
        MOVF    AckStable,W
        SUBWF   AckCandidate,W
        BTFSC   STATUS,Z
        RETURN                  ; No possible state change
        MOVF    AckStartL,W
        MOVWF   StartL
        MOVF    AckStartH,W
        MOVWF   StartH
        CALL    elapsedTime
        MOVF    ElapsedH,F
        BTFSS   STATUS,Z
        GOTO    p4AckChanged
        MOVLW   DEBOUNCE_TICKS
        SUBWF   ElapsedL,W
        BTFSS   STATUS,C
        RETURN                  ; Not stable long enough

p4AckChanged
        MOVF    AckCandidate,W
        MOVWF   AckStable
        MOVF    AckStable,F
        BTFSC   STATUS,Z
        GOTO    p4AckReleased

        MOVF    ActiveMed,F
        BTFSC   STATUS,Z
        GOTO    p4AckDiscard     ; Press outside an alarm is ignored
        MOVF    AckArmed,F
        BTFSC   STATUS,Z
        RETURN                  ; Held press cannot clear next alarm
        MOVLW   D'1'
        MOVWF   AckEvent
p4AckDiscard
        CLRF    AckArmed
        RETURN

p4AckReleased
        MOVLW   D'1'
        MOVWF   AckArmed
        RETURN

; MAIN APPLICATION STATE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

appPoll
        MOVF    AppState,W
        XORLW   APP_WELCOME
        BTFSC   STATUS,Z
        GOTO    p4Welcome
        MOVF    AppState,W
        XORLW   APP_SETUP
        BTFSC   STATUS,Z
        GOTO    p4Setup
        GOTO    p4Run

p4Welcome
        MOVF    MessageStartL,W
        MOVWF   StartL
        MOVF    MessageStartH,W
        MOVWF   StartH
        CALL    elapsedTime
        MOVF    ElapsedH,F
        BTFSS   STATUS,Z
        GOTO    p4BeginSetup
        MOVLW   MESSAGE_TICKS
        SUBWF   ElapsedL,W
        BTFSS   STATUS,C
        RETURN

p4BeginSetup
        MOVLW   APP_SETUP
        MOVWF   AppState
        CLRF    SetupTarget
        PAGESEL keypadBegin
        CALL    keypadBegin
        PAGESEL $
        RETURN

p4Setup
        PAGESEL keypadPoll
        CALL    keypadPoll
        PAGESEL $
        MOVF    EntryReady,F
        BTFSC   STATUS,Z
        RETURN
        CLRF    EntryReady      ; Consume the accepted time once

        MOVF    SetupTarget,F
        BTFSC   STATUS,Z
        GOTO    p4SaveClock
        MOVF    SetupTarget,W
        XORLW   TARGET_MED1
        BTFSC   STATUS,Z
        GOTO    p4SaveMed1
        MOVF    SetupTarget,W
        XORLW   TARGET_MED2
        BTFSC   STATUS,Z
        GOTO    p4SaveMed2
        GOTO    p4SaveMed3

p4SaveClock
        MOVF    EntryHour,W
        MOVWF   ClockHour       ; ClockRun is still zero
        MOVF    EntryMinute,W
        MOVWF   ClockMinute
        CLRF    ClockSecond
        CLRF    ClockSub
        GOTO    p4NextEntry

p4SaveMed1
        MOVF    EntryHour,W
        MOVWF   Med1Hour
        MOVF    EntryMinute,W
        MOVWF   Med1Minute
        GOTO    p4NextEntry

p4SaveMed2
        MOVF    EntryHour,W
        MOVWF   Med2Hour
        MOVF    EntryMinute,W
        MOVWF   Med2Minute
        GOTO    p4NextEntry

p4SaveMed3
        MOVF    EntryHour,W
        MOVWF   Med3Hour
        MOVF    EntryMinute,W
        MOVWF   Med3Minute
        PAGESEL scheduleReset
        CALL    scheduleReset
        PAGESEL $
        MOVLW   APP_RUN
        MOVWF   AppState
        MOVLW   D'1'
        MOVWF   ClockRun        ; Clock starts after ALL setup
        MOVWF   UiDirty
        RETURN

p4NextEntry
        INCF    SetupTarget,F
        PAGESEL keypadBegin
        CALL    keypadBegin
        PAGESEL $
        RETURN

p4Run
        CALL    resultPoll      ; Expire an old two-second message
        MOVF    ActiveMed,F
        BTFSC   STATUS,Z
        GOTO    startNextAlarm
        GOTO    alarmPoll

; ACTIVATE ONE WAITING MEDICINE ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

startNextAlarm
        PAGESEL schedulePop
        CALL    schedulePop     ; W = 0, 1, 2 or 3; removes pending bit
        PAGESEL $
        MOVWF   ActiveMed
        MOVF    ActiveMed,F
        BTFSC   STATUS,Z
        RETURN

        MOVF    NowL,W
        MOVWF   AlarmStartL
        MOVWF   BlinkStartL
        MOVWF   AckStartL
        MOVF    NowH,W
        MOVWF   AlarmStartH
        MOVWF   BlinkStartH
        MOVWF   AckStartH
        CLRF    AckEvent
        CLRF    AckArmed
        MOVLW   D'1'
        MOVWF   AckCandidate    ; New alarm requires a fresh release
        MOVWF   AckStable
        MOVWF   UiDirty

        CALL    medicineMask    ; W = selected LED bit
        MOVWF   OutShadow
        BSF     OutShadow,BUZZER_BIT
        MOVF    OutShadow,W
        MOVWF   ALARM_PORT
        RETURN

medicineMask
        MOVF    ActiveMed,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   MED1_MASK
        MOVF    ActiveMed,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   MED2_MASK
        RETLW   MED3_MASK       ; Caller guarantees 1..3

; ACTIVE ALARM ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

alarmPoll
        MOVF    AlarmStartL,W
        MOVWF   StartL
        MOVF    AlarmStartH,W
        MOVWF   StartH
        CALL    elapsedTime

; Compare elapsed time with a 16-bit limit...................
; At the deadline, timeout wins over an event seen in that loop

        MOVLW   HIGH ALARM_TICKS
        SUBWF   ElapsedH,W
        BTFSS   STATUS,C
        GOTO    p4BeforeDeadline
        BTFSS   STATUS,Z
        GOTO    p4Missed
        MOVLW   LOW ALARM_TICKS
        SUBWF   ElapsedL,W
        BTFSC   STATUS,C
        GOTO    p4Missed

p4BeforeDeadline
        MOVF    AckEvent,F
        BTFSS   STATUS,Z
        GOTO    p4Taken

        MOVF    BlinkStartL,W
        MOVWF   StartL
        MOVF    BlinkStartH,W
        MOVWF   StartH
        CALL    elapsedTime
        MOVF    ElapsedH,F
        BTFSS   STATUS,Z
        GOTO    p4ToggleLed
        MOVLW   BLINK_TICKS
        SUBWF   ElapsedL,W
        BTFSS   STATUS,C
        RETURN

p4ToggleLed
        MOVF    NowL,W
        MOVWF   BlinkStartL
        MOVF    NowH,W
        MOVWF   BlinkStartH
        CALL    medicineMask
        XORWF   OutShadow,F     ; Toggle LED, keep buzzer ON
        MOVF    OutShadow,W
        MOVWF   ALARM_PORT
        RETURN

p4Taken
        MOVLW   RESULT_TAKEN
        GOTO    finishAlarm
p4Missed
        MOVLW   RESULT_MISSED

finishAlarm
        MOVWF   ResultCode
        MOVF    ActiveMed,W
        MOVWF   LastMed
        CLRF    ActiveMed
        CLRF    OutShadow
        MOVF    OutShadow,W
        MOVWF   ALARM_PORT      ; Stop the completed medicine
        CLRF    AckEvent
        CLRF    AckArmed
        MOVF    NowL,W
        MOVWF   MessageStartL
        MOVF    NowH,W
        MOVWF   MessageStartH
        MOVLW   D'1'
        MOVWF   UiDirty
        GOTO    startNextAlarm  ; Immediate next alarm, no 2 s wait

; RESULT MESSAGE IS AN OVERLAY, NOT A BLOCKING DELAY ;;;;;;;;

resultPoll
        MOVF    ResultCode,F
        BTFSC   STATUS,Z
        RETURN
        MOVF    MessageStartL,W
        MOVWF   StartL
        MOVF    MessageStartH,W
        MOVWF   StartH
        CALL    elapsedTime
        MOVF    ElapsedH,F
        BTFSS   STATUS,Z
        GOTO    p4ClearResult
        MOVLW   MESSAGE_TICKS
        SUBWF   ElapsedL,W
        BTFSS   STATUS,C
        RETURN
p4ClearResult
        CLRF    ResultCode
        MOVLW   D'1'
        MOVWF   UiDirty
        RETURN

; SELECT THE LCD SCREEN ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

uiSelect
        MOVLW   SCREEN_WELCOME
        MOVWF   NewScreen
        MOVF    AppState,W
        XORLW   APP_WELCOME
        BTFSC   STATUS,Z
        GOTO    p4SetScreen
        MOVLW   SCREEN_ENTRY
        MOVWF   NewScreen
        MOVF    AppState,W
        XORLW   APP_SETUP
        BTFSC   STATUS,Z
        GOTO    p4SetScreen
        MOVLW   SCREEN_RESULT
        MOVWF   NewScreen
        MOVF    ResultCode,F
        BTFSS   STATUS,Z
        GOTO    p4SetScreen
        MOVLW   SCREEN_ALARM
        MOVWF   NewScreen
        MOVF    ActiveMed,F
        BTFSS   STATUS,Z
        GOTO    p4SetScreen
        MOVLW   SCREEN_NORMAL
        MOVWF   NewScreen
p4SetScreen
        MOVF    NewScreen,W
        SUBWF   UiScreen,W
        BTFSC   STATUS,Z
        RETURN
        MOVF    NewScreen,W
        MOVWF   UiScreen
        MOVLW   D'1'
        MOVWF   UiDirty
        RETURN
