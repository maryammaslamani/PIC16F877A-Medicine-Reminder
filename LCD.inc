; RS=RD1, E=RD2, D4-D7=RD4-RD7

; LCD variables
p1Timer1       EQU     0xA0          ; delay counter
p1TimerX       EQU     0xA1          ; delay repeat counter
p1OutCod       EQU     0xA2          ; byte being sent to LCD
p1Select       EQU     0xA3          ; 0 command, 1 data
p1Busy         EQU     0xA4          ; 1 while screen is being drawn
p1Phase        EQU     0xA5          ; current render phase
p1Pos          EQU     0xA6          ; character position 0..15
p1Screen       EQU     0xA7          ; local copy of UiScreen
p1Target       EQU     0xA8          ; local copy of SetupTarget
p1EntryD0      EQU     0xA9          ; entry digit 0
p1EntryD1      EQU     0xAA          ; entry digit 1
p1EntryD2      EQU     0xAB          ; entry digit 2
p1EntryD3      EQU     0xAC          ; entry digit 3
p1EntryError   EQU     0xAD          ; local entry error
p1ViewHour     EQU     0xAE          ; hour shown on LCD
p1ViewMinute   EQU     0xAF          ; minute shown on LCD
p1NextMed      EQU     0xB0          ; next medicine number
p1NextHour     EQU     0xB1          ; next reminder hour
p1NextMinute   EQU     0xB2          ; next reminder minute
p1ActiveMed    EQU     0xB3          ; medicine currently alarming
p1LastMed      EQU     0xB4          ; last medicine handled
p1ResultCode   EQU     0xB5          ; taken or missed result
p1Temp         EQU     0xB6          ; temporary value
p1Tens         EQU     0xB7          ; tens counter


; LCD mode values
P1_COMMAND     EQU     D'0'          ; RS=0
P1_DATA        EQU     D'1'          ; RS=1


; initialize the LCD at startup
; clear local state, configure LCD, then print welcome screen
lcdInit
        BCF     STATUS,IRP
        BANKSEL p1Busy
        CLRF    p1Busy               ; not drawing yet
        CLRF    p1Phase              ; start from phase 0
        CLRF    p1Pos                ; start from first character
        CLRF    p1Select             ; command mode

        BANKSEL PORTD
        CLRF    PORTD                ; clear LCD pins

        CALL    p1HwInit             ; configure LCD
        CALL    p1StartRender        ; take a screen snapshot

; lcdInit is allowed to finish the whole welcome screen here
p1InitDrawLoop
        CALL    p1RenderStep
        BANKSEL p1Busy
        MOVF    p1Busy,F
        BTFSS   STATUS,Z
        GOTO    p1InitDrawLoop

        BANKSEL UiDirty
        CLRF    UiDirty              ; update request handled
        BCF     STATUS,IRP
        RETURN


; main LCD service
; if an update is needed, it sends only one LCD byte each call
lcdService
        BCF     STATUS,IRP
        BANKSEL p1Busy
        MOVF    p1Busy,F
        BTFSS   STATUS,Z
        GOTO    p1ServiceStep

        BANKSEL UiDirty
        MOVF    UiDirty,F
        BTFSC   STATUS,Z
        GOTO    p1ServiceDone

        CLRF    UiDirty              ; update request handled
        CALL    p1StartRender        ; take a screen snapshot

; continue the current screen update
p1ServiceStep
        CALL    p1RenderStep

; return with bank 0 selected
p1ServiceDone
        BANKSEL UiScreen
        BCF     STATUS,IRP
        RETURN


; start a new screen update
; copy shared variables to local copies first
; this prevents values changing in the middle of one screen
p1StartRender
        BANKSEL UiScreen
        MOVF    UiScreen,W
        BANKSEL p1Screen
        MOVWF   p1Screen

        BANKSEL SetupTarget
        MOVF    SetupTarget,W
        BANKSEL p1Target
        MOVWF   p1Target

        BANKSEL EntryD0
        MOVF    EntryD0,W
        BANKSEL p1EntryD0
        MOVWF   p1EntryD0
        BANKSEL EntryD1
        MOVF    EntryD1,W
        BANKSEL p1EntryD1
        MOVWF   p1EntryD1
        BANKSEL EntryD2
        MOVF    EntryD2,W
        BANKSEL p1EntryD2
        MOVWF   p1EntryD2
        BANKSEL EntryD3
        MOVF    EntryD3,W
        BANKSEL p1EntryD3
        MOVWF   p1EntryD3
        BANKSEL EntryError
        MOVF    EntryError,W
        BANKSEL p1EntryError
        MOVWF   p1EntryError

        BANKSEL ViewHour
        MOVF    ViewHour,W
        BANKSEL p1ViewHour
        MOVWF   p1ViewHour
        BANKSEL ViewMinute
        MOVF    ViewMinute,W
        BANKSEL p1ViewMinute
        MOVWF   p1ViewMinute

        BANKSEL NextMed
        MOVF    NextMed,W
        BANKSEL p1NextMed
        MOVWF   p1NextMed
        BANKSEL NextHour
        MOVF    NextHour,W
        BANKSEL p1NextHour
        MOVWF   p1NextHour
        BANKSEL NextMinute
        MOVF    NextMinute,W
        BANKSEL p1NextMinute
        MOVWF   p1NextMinute

        BANKSEL ActiveMed
        MOVF    ActiveMed,W
        BANKSEL p1ActiveMed
        MOVWF   p1ActiveMed
        BANKSEL LastMed
        MOVF    LastMed,W
        BANKSEL p1LastMed
        MOVWF   p1LastMed
        BANKSEL ResultCode
        MOVF    ResultCode,W
        BANKSEL p1ResultCode
        MOVWF   p1ResultCode

        BANKSEL p1Busy
        MOVLW   D'1'
        MOVWF   p1Busy
        CLRF    p1Phase              ; start from phase 0
        CLRF    p1Pos                ; start from first character

        BANKSEL UiScreen
        RETURN


; draw one step of the screen
; phase 0 = set line 1 address
; phase 1 = print line 1
; phase 2 = set line 2 address
; phase 3 = print line 2
p1RenderStep
        BANKSEL p1Phase
        MOVF    p1Phase,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        GOTO    p1RenderL1Addr

        MOVF    p1Phase,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        GOTO    p1RenderL1Char

        MOVF    p1Phase,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        GOTO    p1RenderL2Addr

        GOTO    p1RenderL2Char

; set LCD cursor to line 1
p1RenderL1Addr
        MOVLW   0x80                 ; line 1 address
        CALL    p1SendCommand
        BANKSEL p1Phase
        MOVLW   D'1'
        MOVWF   p1Phase
        CLRF    p1Pos                ; start from first character
        BANKSEL UiScreen
        RETURN

; get one line 1 character and send it
p1RenderL1Char
        CALL    p1GetLine1Char
        CALL    p1WriteData
        BANKSEL p1Pos
        INCF    p1Pos,F
        MOVF    p1Pos,W
        XORLW   D'16'
        BTFSS   STATUS,Z
        GOTO    p1RenderReturn
        MOVLW   D'2'
        MOVWF   p1Phase
        CLRF    p1Pos                ; start from first character
        GOTO    p1RenderReturn

; set LCD cursor to line 2
p1RenderL2Addr
        MOVLW   0xC0                 ; line 2 address
        CALL    p1SendCommand
        BANKSEL p1Phase
        MOVLW   D'3'
        MOVWF   p1Phase
        CLRF    p1Pos                ; start from first character
        GOTO    p1RenderReturn

; get one line 2 character and send it
p1RenderL2Char
        CALL    p1GetLine2Char
        CALL    p1WriteData
        BANKSEL p1Pos
        INCF    p1Pos,F
        MOVF    p1Pos,W
        XORLW   D'16'
        BTFSS   STATUS,Z
        GOTO    p1RenderReturn
        CLRF    p1Busy               ; not drawing yet
        CLRF    p1Phase              ; start from phase 0
        CLRF    p1Pos                ; start from first character

; one render step is finished
p1RenderReturn
        BANKSEL UiScreen
        RETURN


; delay used during LCD startup
p1OneMs
        BANKSEL p1Timer1
        MOVLW   D'249'               ; delay count
        MOVWF   p1Timer1
p1OneMsLoop
        NOP
        DECFSZ  p1Timer1,F
        GOTO    p1OneMsLoop
        BANKSEL PORTD
        RETURN

; repeat the 1 ms delay, count comes in W
p1Xms
        BANKSEL p1TimerX
        MOVWF   p1TimerX
p1XmsLoop
        CALL    p1OneMs
        BANKSEL p1TimerX
        DECFSZ  p1TimerX,F
        GOTO    p1XmsLoop
        BANKSEL PORTD
        RETURN

; short wait after a normal LCD byte
p1ByteDelay
        BANKSEL p1Timer1
        MOVLW   D'20'
        MOVWF   p1Timer1
p1ByteDelayLoop
        NOP
        DECFSZ  p1Timer1,F
        GOTO    p1ByteDelayLoop
        BANKSEL PORTD
        RETURN

; pulse E so LCD accepts the nibble
p1PulseE
        BANKSEL PORTD
        BSF     PORTD,LCD_E          ; E high
        NOP
        BCF     PORTD,LCD_E          ; E low
        RETURN


; send one byte to LCD in 4-bit mode
; send high nibble first then low nibble
p1SendByte
        BANKSEL p1OutCod
        MOVWF   p1OutCod

        MOVF    p1OutCod,W
        ANDLW   0xF0                 ; keep upper nibble
        BANKSEL PORTD
        MOVWF   PORTD

        BANKSEL p1Select
        BTFSS   p1Select,0
        GOTO    p1SendHighPulse
        BANKSEL PORTD
        BSF     PORTD,LCD_RS

; pulse E for the high nibble
p1SendHighPulse
        BANKSEL PORTD
        CALL    p1PulseE

        BANKSEL p1OutCod
        SWAPF   p1OutCod,W           ; move low nibble to upper bits
        ANDLW   0xF0                 ; keep upper nibble
        BANKSEL PORTD
        MOVWF   PORTD

        BANKSEL p1Select
        BTFSS   p1Select,0
        GOTO    p1SendLowPulse
        BANKSEL PORTD
        BSF     PORTD,LCD_RS

; pulse E for the low nibble
p1SendLowPulse
        BANKSEL PORTD
        CALL    p1PulseE
        CALL    p1ByteDelay
        BANKSEL PORTD
        RETURN

; RS=0, so W is sent as an LCD command
p1SendCommand
        BANKSEL p1Select
        CLRF    p1Select             ; command mode
        CALL    p1SendByte
        BANKSEL PORTD
        RETURN

; RS=1, so W is sent as a character
p1WriteData
        BANKSEL p1Select
        BSF     p1Select,0
        CALL    p1SendByte
        BANKSEL PORTD
        RETURN


; LCD hardware initialization
; switch LCD to 4-bit, 2-line mode and clear the display
p1HwInit
        MOVLW   D'100'
        CALL    p1Xms

        BANKSEL p1Select
        CLRF    p1Select             ; command mode

        BANKSEL PORTD
        MOVLW   0x30
        MOVWF   PORTD
        CALL    p1PulseE

        MOVLW   D'5'
        CALL    p1Xms

        CALL    p1PulseE
        CALL    p1OneMs

        CALL    p1PulseE
        CALL    p1OneMs

        BANKSEL PORTD
        BCF     PORTD,4
        CALL    p1PulseE
        CALL    p1OneMs

        MOVLW   0x28                 ; 4-bit mode, 2 lines
        CALL    p1SendCommand
        MOVLW   0x08                 ; display off
        CALL    p1SendCommand
        MOVLW   0x01                 ; clear LCD
        CALL    p1SendCommand

        MOVLW   D'2'
        CALL    p1Xms

        MOVLW   0x06                 ; cursor moves right
        CALL    p1SendCommand
        MOVLW   0x80                 ; line 1 address
        CALL    p1SendCommand
        MOVLW   0x0C                 ; display on, cursor off
        CALL    p1SendCommand

        BANKSEL PORTD
        RETURN


; get the tens digit of a binary value and return ASCII in W
p1AsciiTens
        BANKSEL p1Temp
        MOVWF   p1Temp
        CLRF    p1Tens
; subtract 10 until the tens count is found
p1TensLoop
        MOVLW   D'10'
        SUBWF   p1Temp,W
        BTFSS   STATUS,C
        GOTO    p1TensDone
        MOVWF   p1Temp
        INCF    p1Tens,F
        GOTO    p1TensLoop
; convert tens value to ASCII
p1TensDone
        MOVF    p1Tens,W
        ADDLW   '0'
        BANKSEL PORTD
        RETURN


; get the ones digit of a binary value and return ASCII in W
p1AsciiOnes
        BANKSEL p1Temp
        MOVWF   p1Temp
; keep subtracting 10 until value is below 10
p1OnesLoop
        MOVLW   D'10'
        SUBWF   p1Temp,W
        BTFSS   STATUS,C
        GOTO    p1OnesDone
        MOVWF   p1Temp
        GOTO    p1OnesLoop
; convert the remaining value to ASCII
p1OnesDone
        MOVF    p1Temp,W
        ADDLW   '0'
        BANKSEL PORTD
        RETURN


; convert one entered digit to ASCII
; NO_DIGIT is displayed as '_'
p1EntryAscii
        BANKSEL p1Temp
        MOVWF   p1Temp
        MOVF    p1Temp,W
        XORLW   NO_DIGIT
        BTFSC   STATUS,Z
        GOTO    p1EntryUnderscore
        MOVF    p1Temp,W
        ADDLW   '0'
        BANKSEL PORTD
        RETURN
; no digit entered yet
p1EntryUnderscore
        MOVLW   '_'
        BANKSEL PORTD
        RETURN


; choose which line 1 text belongs to the current screen
p1GetLine1Char
        BANKSEL p1Screen
        MOVF    p1Screen,W
        XORLW   SCREEN_WELCOME
        BTFSC   STATUS,Z
        GOTO    p1L1Welcome
        MOVF    p1Screen,W
        XORLW   SCREEN_ENTRY
        BTFSC   STATUS,Z
        GOTO    p1L1Entry
        MOVF    p1Screen,W
        XORLW   SCREEN_NORMAL
        BTFSC   STATUS,Z
        GOTO    p1L1Normal
        MOVF    p1Screen,W
        XORLW   SCREEN_ALARM
        BTFSC   STATUS,Z
        GOTO    p1L1Alarm
        MOVF    p1Screen,W
        XORLW   SCREEN_RESULT
        BTFSC   STATUS,Z
        GOTO    p1L1Result
        RETLW   ' '

; Welcome screen: "Welcome to"
p1L1Welcome
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'W'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'l'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   'c'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   'o'
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'm'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   't'
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        RETLW   'o'
        RETLW   ' '

; Entry screen: "Set Time" or "Set Med n"
p1L1Entry
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'S'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   't'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   ' '

        MOVF    p1Target,F
        BTFSC   STATUS,Z
        GOTO    p1L1EntryClock

        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   'M'
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'd'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSS   STATUS,Z
        RETLW   ' '
        MOVF    p1Target,W
        ADDLW   '0'
        RETURN

; finish "Set Time" text
p1L1EntryClock
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   'T'
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'm'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   'e'
        RETLW   ' '

; Normal screen: "Time HH:MM"
p1L1Normal
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'T'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'm'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        GOTO    p1L1HourTens
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        GOTO    p1L1HourOnes
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   ':'
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        GOTO    p1L1MinuteTens
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        GOTO    p1L1MinuteOnes
        RETLW   ' '
; current hour tens
p1L1HourTens
        MOVF    p1ViewHour,W
        CALL    p1AsciiTens
        RETURN
; current hour ones
p1L1HourOnes
        MOVF    p1ViewHour,W
        CALL    p1AsciiOnes
        RETURN
; current minute tens
p1L1MinuteTens
        MOVF    p1ViewMinute,W
        CALL    p1AsciiTens
        RETURN
; current minute ones
p1L1MinuteOnes
        MOVF    p1ViewMinute,W
        CALL    p1AsciiOnes
        RETURN

; Alarm screen: "Take Medicine n"
p1L1Alarm
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'T'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'a'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'k'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'M'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   'd'
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        RETLW   'c'
        MOVF    p1Pos,W
        XORLW   D'10'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'11'
        BTFSC   STATUS,Z
        RETLW   'n'
        MOVF    p1Pos,W
        XORLW   D'12'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'13'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'14'
        BTFSS   STATUS,Z
        RETLW   ' '
        MOVF    p1ActiveMed,W
        ADDLW   '0'
        RETURN

; Result screen: choose Taken or Missed
p1L1Result
        MOVF    p1ResultCode,W
        XORLW   RESULT_TAKEN
        BTFSC   STATUS,Z
        GOTO    p1L1Taken
        MOVF    p1ResultCode,W
        XORLW   RESULT_MISSED
        BTFSC   STATUS,Z
        GOTO    p1L1Missed
        RETLW   ' '

; print "Dose Taken"
p1L1Taken
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'D'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'o'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   's'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'T'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'a'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   'k'
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        RETLW   'n'
        RETLW   ' '

; print "Dose Missed"
p1L1Missed
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'D'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'o'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   's'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'M'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   's'
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   's'
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'10'
        BTFSC   STATUS,Z
        RETLW   'd'
        RETLW   ' '


; choose which line 2 text belongs to the current screen
p1GetLine2Char
        BANKSEL p1Screen
        MOVF    p1Screen,W
        XORLW   SCREEN_WELCOME
        BTFSC   STATUS,Z
        GOTO    p1L2Welcome
        MOVF    p1Screen,W
        XORLW   SCREEN_ENTRY
        BTFSC   STATUS,Z
        GOTO    p1L2Entry
        MOVF    p1Screen,W
        XORLW   SCREEN_NORMAL
        BTFSC   STATUS,Z
        GOTO    p1L2Normal
        MOVF    p1Screen,W
        XORLW   SCREEN_ALARM
        BTFSC   STATUS,Z
        GOTO    p1L2Alarm
        MOVF    p1Screen,W
        XORLW   SCREEN_RESULT
        BTFSC   STATUS,Z
        GOTO    p1L2Result
        RETLW   ' '

; Welcome screen: "Med Reminder"
p1L2Welcome
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'M'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'd'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   'R'
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'm'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   'n'
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        RETLW   'd'
        MOVF    p1Pos,W
        XORLW   D'10'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'11'
        BTFSC   STATUS,Z
        RETLW   'r'
        RETLW   ' '

; Entry screen: show HH:MM or underscores
p1L2Entry
        MOVF    p1EntryError,F
        BTFSS   STATUS,Z
        GOTO    p1L2EntryError

        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        GOTO    p1L2EntryD0
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        GOTO    p1L2EntryD1
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   ':'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        GOTO    p1L2EntryD2
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        GOTO    p1L2EntryD3
        RETLW   ' '
; first entered digit
p1L2EntryD0
        MOVF    p1EntryD0,W
        CALL    p1EntryAscii
        RETURN
; second entered digit
p1L2EntryD1
        MOVF    p1EntryD1,W
        CALL    p1EntryAscii
        RETURN
; third entered digit
p1L2EntryD2
        MOVF    p1EntryD2,W
        CALL    p1EntryAscii
        RETURN
; fourth entered digit
p1L2EntryD3
        MOVF    p1EntryD3,W
        CALL    p1EntryAscii
        RETURN

; invalid entry: "Invalid HH:MM"
p1L2EntryError
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'I'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'n'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'v'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   'a'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   'l'
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'd'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   'H'
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        RETLW   'H'
        MOVF    p1Pos,W
        XORLW   D'10'
        BTFSC   STATUS,Z
        RETLW   ':'
        MOVF    p1Pos,W
        XORLW   D'11'
        BTFSC   STATUS,Z
        RETLW   'M'
        MOVF    p1Pos,W
        XORLW   D'12'
        BTFSC   STATUS,Z
        RETLW   'M'
        RETLW   ' '

; Normal screen: show next medicine and time
p1L2Normal
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'N'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'x'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   't'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   ' '

        MOVF    p1NextMed,F
        BTFSC   STATUS,Z
        GOTO    p1L2NoNext

        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'M'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSS   STATUS,Z
        GOTO    p1L2NextAfterMed
        MOVF    p1NextMed,W
        ADDLW   '0'
        RETURN
; continue after medicine number
p1L2NextAfterMed
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        GOTO    p1L2NextHourTens
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        GOTO    p1L2NextHourOnes
        MOVF    p1Pos,W
        XORLW   D'10'
        BTFSC   STATUS,Z
        RETLW   ':'
        MOVF    p1Pos,W
        XORLW   D'11'
        BTFSC   STATUS,Z
        GOTO    p1L2NextMinuteTens
        MOVF    p1Pos,W
        XORLW   D'12'
        BTFSC   STATUS,Z
        GOTO    p1L2NextMinuteOnes
        RETLW   ' '
; next reminder hour tens
p1L2NextHourTens
        MOVF    p1NextHour,W
        CALL    p1AsciiTens
        RETURN
; next reminder hour ones
p1L2NextHourOnes
        MOVF    p1NextHour,W
        CALL    p1AsciiOnes
        RETURN
; next reminder minute tens
p1L2NextMinuteTens
        MOVF    p1NextMinute,W
        CALL    p1AsciiTens
        RETURN
; next reminder minute ones
p1L2NextMinuteOnes
        MOVF    p1NextMinute,W
        CALL    p1AsciiOnes
        RETURN

; no next medicine: show -- --:--
p1L2NoNext
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   '-'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   '-'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   '-'
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSC   STATUS,Z
        RETLW   '-'
        MOVF    p1Pos,W
        XORLW   D'10'
        BTFSC   STATUS,Z
        RETLW   ':'
        MOVF    p1Pos,W
        XORLW   D'11'
        BTFSC   STATUS,Z
        RETLW   '-'
        MOVF    p1Pos,W
        XORLW   D'12'
        BTFSC   STATUS,Z
        RETLW   '-'
        RETLW   ' '

; Alarm screen line 2: "Press ACK"
p1L2Alarm
        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'P'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'r'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   's'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   's'
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'A'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   'C'
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   'K'
        RETLW   ' '

; Result screen line 2
; ActiveMed=0 -> show last medicine
; ActiveMed=1..3 -> show the next active medicine
p1L2Result
        MOVF    p1ActiveMed,F
        BTFSS   STATUS,Z
        GOTO    p1L1Alarm

        MOVF    p1Pos,W
        XORLW   D'0'
        BTFSC   STATUS,Z
        RETLW   'M'
        MOVF    p1Pos,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        RETLW   'd'
        MOVF    p1Pos,W
        XORLW   D'3'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        RETLW   'c'
        MOVF    p1Pos,W
        XORLW   D'5'
        BTFSC   STATUS,Z
        RETLW   'i'
        MOVF    p1Pos,W
        XORLW   D'6'
        BTFSC   STATUS,Z
        RETLW   'n'
        MOVF    p1Pos,W
        XORLW   D'7'
        BTFSC   STATUS,Z
        RETLW   'e'
        MOVF    p1Pos,W
        XORLW   D'8'
        BTFSC   STATUS,Z
        RETLW   ' '
        MOVF    p1Pos,W
        XORLW   D'9'
        BTFSS   STATUS,Z
        RETLW   ' '
        MOVF    p1LastMed,W
        ADDLW   '0'
        RETURN
