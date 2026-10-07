; BUILD AND TIMING CONSTANTS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

IFNDEF FAST_DEMO
FAST_DEMO       EQU     D'0'        ; 0 = normal; 1 = 60x clock
ENDIF

CLOCK_HZ        EQU     D'4000000'
TICK_MS         EQU     D'10'
TICKS_SECOND    EQU     D'100'
TIMER_PERIOD    EQU     D'249'      ; PR2
TIMER_CONTROL   EQU     0x4D        ; on, prescale 4, postscale 10
DEBOUNCE_TICKS  EQU     D'2'        ; 20 ms stable input
MESSAGE_TICKS   EQU     D'200'      ; 2 REAL seconds in both modes

IF FAST_DEMO == D'0'
SIM_STEP        EQU     D'1'        ; 1 simulated 10 ms per tick
ALARM_TICKS     EQU     D'2000'     ; 20 simulated seconds
BLINK_TICKS     EQU     D'25'       ; 250 ms per change
ELSE
SIM_STEP        EQU     D'60'
ALARM_TICKS     EQU     D'500'       ; ceil(2000 / 60): 340 real ms
BLINK_TICKS     EQU     D'5'        ; 50 ms per change in fast demo
ENDIF

; PIN LABELS ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

LCD_PORT        EQU     PORTD
LCD_RS          EQU     D'1'        ; RD1, physical pin 20
LCD_E           EQU     D'2'        ; RD2, physical pin 21
; LCD D4..D7 = RD4..RD7. LCD R/W = GND
KEY_PORT        EQU     PORTC
; 3x4 phone keypad:
; rows RC0..RC3; columns RC4..RC6 with external 10k pull-ups
; RC7 stays configured as an unused input and is ignored by keypad code
KEY_ROW_MASK    EQU     B'00001111' ; RC0..RC3: four row outputs
KEY_COL_MASK    EQU     B'01110000' ; RC4..RC6: three column inputs
KEY_ROW_IDLE    EQU     B'00001111' ; all rows HIGH when not scanning
KEY_COL_SHIFT   EQU     D'4'        ; first column starts at RC4
ALARM_PORT      EQU     PORTB
LED1_BIT        EQU     D'0'        ; RB0, pin 33
LED2_BIT        EQU     D'1'        ; RB1, pin 34
LED3_BIT        EQU     D'2'        ; RB2, pin 35
BUZZER_BIT      EQU     D'3'        ; RB3, pin 36 -> NPN driver
ACK_BIT         EQU     D'4'        ; RB4, pin 37; LOW = pressed

; VALUE CODES ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

APP_WELCOME     EQU     D'0'
APP_SETUP       EQU     D'1'
APP_RUN         EQU     D'2'
SCREEN_WELCOME  EQU     D'0'
SCREEN_ENTRY    EQU     D'1'
SCREEN_NORMAL   EQU     D'2'
SCREEN_ALARM    EQU     D'3'
SCREEN_RESULT   EQU     D'4'
RESULT_NONE     EQU     D'0'
RESULT_TAKEN    EQU     D'1'
RESULT_MISSED   EQU     D'2'
TARGET_CLOCK    EQU     D'0'
TARGET_MED1     EQU     D'1'
TARGET_MED2     EQU     D'2'
TARGET_MED3     EQU     D'3'
ERROR_NONE      EQU     D'0'
ERROR_SHORT     EQU     D'1'        ; fewer than four digits
ERROR_RANGE     EQU     D'2'        ; invalid HH or MM
NO_DIGIT        EQU     0xFF
KEY_NONE        EQU     0x00        ; raw keypad ASCII return code
KEY_CLEAR       EQU     '*'
KEY_CONFIRM     EQU     '#'
MED1_MASK       EQU     B'00000001'
MED2_MASK       EQU     B'00000010'
MED3_MASK       EQU     B'00000100'

; RAM: BANK 0, 0x20..0x4F ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

UiScreen        EQU     0x20
UiDirty         EQU     0x21
SetupTarget     EQU     0x22
EntryCount      EQU     0x23
EntryD0         EQU     0x24
EntryD1         EQU     0x25
EntryD2         EQU     0x26
EntryD3         EQU     0x27
EntryHour       EQU     0x28
EntryMinute     EQU     0x29
EntryReady      EQU     0x2A
EntryError      EQU     0x2B
ClockHour       EQU     0x2C
ClockMinute     EQU     0x2D
ClockSecond     EQU     0x2E
ClockSub        EQU     0x2F
ClockRun        EQU     0x30
TickL           EQU     0x31
TickH           EQU     0x32
DaySerial       EQU     0x33
Med1Hour        EQU     0x34
Med1Minute      EQU     0x35
Med2Hour        EQU     0x36
Med2Minute      EQU     0x37
Med3Hour        EQU     0x38
Med3Minute      EQU     0x39
PendingMask     EQU     0x3A
FiredMask       EQU     0x3B
NextMed         EQU     0x3C
NextHour        EQU     0x3D
NextMinute      EQU     0x3E
NextTomorrow    EQU     0x3F
ViewHour        EQU     0x40
ViewMinute      EQU     0x41
ViewSecond      EQU     0x42
ViewDay         EQU     0x43
ActiveMed       EQU     0x44
LastMed         EQU     0x45
ResultCode      EQU     0x46
AppState        EQU     0x50
NowL            EQU     0x51        
NowH            EQU     0x52        
MessageStartL   EQU     0x53
MessageStartH   EQU     0x54
AlarmStartL     EQU     0x55
AlarmStartH     EQU     0x56
BlinkStartL     EQU     0x57
BlinkStartH     EQU     0x58
AckCandidate    EQU     0x59
AckStable       EQU     0x5A
AckStartL       EQU     0x5B
AckStartH       EQU     0x5C
AckEvent        EQU     0x5D
AckArmed        EQU     0x5E
OutShadow       EQU     0x5F
ElapsedL        EQU     0x60
ElapsedH        EQU     0x61
StartL          EQU     0x62       
StartH          EQU     0x63
PreviousMinute  EQU     0x64
PreviousDay     EQU     0x65
RawAck          EQU     0x66
NewScreen       EQU     0x67

SaveW           EQU     0x70
SaveStatus      EQU     0x71
SavePclath      EQU     0x72
SaveFsr         EQU     0x73