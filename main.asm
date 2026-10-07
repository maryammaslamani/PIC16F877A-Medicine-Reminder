        PROCESSOR 16F877A
        RADIX   HEX
        #INCLUDE "p16f877a.inc"
        __CONFIG _XT_OSC & _WDT_OFF & _PWRTE_ON & _BODEN_ON & _LVP_OFF & _CPD_OFF & _WRT_OFF & _CP_OFF & _DEBUG_OFF

        #INCLUDE "common.inc"

; RESET AND INTERRUPT VECTORS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ORG     0x0000
        NOP                     
        GOTO    init

        ORG     0x0004
        MOVWF   SaveW           ; Save W before changing it
        SWAPF   STATUS,W
        CLRF    STATUS          ; Bank 0 and IRP = 0
        MOVWF   SaveStatus
        MOVF    PCLATH,W
        MOVWF   SavePclath
        MOVF    FSR,W
        MOVWF   SaveFsr
        CLRF    PCLATH          ; Local branches are in page 0

        BTFSS   PIR1,TMR2IF     ; Timer2 is the only enabled source
        GOTO    p4Restore
        BCF     PIR1,TMR2IF
        PAGESEL timerIsr
        CALL    timerIsr        
        PAGESEL $
        BANKSEL PORTA

p4Restore
        MOVF    SaveFsr,W
        MOVWF   FSR
        MOVF    SavePclath,W
        MOVWF   PCLATH
        SWAPF   SaveStatus,W
        MOVWF   STATUS
        SWAPF   SaveW,F
        SWAPF   SaveW,W
        RETFIE                  

; INITIALISATION ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

init
        CLRF    PCLATH
        CALL    boardInit
        CALL    ramInit
        CALL    alarmInit
        MOVLW   0xFF
        MOVWF   PreviousMinute
        MOVWF   PreviousDay
        MOVLW   D'1'
        MOVWF   UiDirty

        PAGESEL lcdInit
        CALL    lcdInit         ; Initialise and DRAW welcome
        PAGESEL $
        PAGESEL timerInit
        CALL    timerInit       ; Start 10 ms ticks, clock paused
        PAGESEL $

; MAIN LOOP ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

mainLoop
        CALL    timeSnapshot
        CALL    ackPoll         ; Debounce even while no alarm exists

        PAGESEL schedulePoll
        CALL    schedulePoll    ; Keeps collecting due reminders
        PAGESEL $

        CALL    appPoll         ; Welcome / input / run state
        CALL    uiSelect

        PAGESEL lcdService
        CALL    lcdService      ; Small, non-blocking display step
        PAGESEL $
        GOTO    mainLoop

; INCLUDED BOARD AND APPLICATION ROUTINES ;;;;;;;;;;;;;;;;;;

        #INCLUDE "board.inc"
        #INCLUDE "alarm_controller.inc"

; INCLUDED FUNCTION MODULES ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ORG     0x0800
        #INCLUDE "LCD.inc"

        ORG     0x1000
        #INCLUDE "keypad.inc"

        ORG     0x1800
        #INCLUDE "clock.inc"
	
        END