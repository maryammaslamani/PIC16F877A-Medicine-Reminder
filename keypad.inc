; variables
RawKey          EQU     0xC0
CandidateKey    EQU     0xC1
StableKey       EQU     0xC2
KeyArmed        EQU     0xC3
KeyEvent        EQU     0xC4
Columns         EQU     0xC5
KeyStartL       EQU     0xC6
KeyStartH       EQU     0xC7
KeyElapsedL     EQU     0xC8
KeyElapsedH     EQU     0xC9
KeyCount        EQU     0xCA
ColumnCode      EQU     0xCB
DigitValue      EQU     0xCC
TenResult       EQU     0xCD
TenCounter      EQU     0xCE
KEY_AMBIGUOUS   EQU     0xFE

; keypad entry
keypadBegin
        BANKSEL EntryCount
        CLRF    EntryCount
        CLRF    EntryHour
        CLRF    EntryMinute
        CLRF    EntryReady
        CLRF    EntryError
        MOVLW   NO_DIGIT
        MOVWF   EntryD0
        MOVWF   EntryD1
        MOVWF   EntryD2
        MOVWF   EntryD3
        MOVLW   D'1'
        MOVWF   UiDirty
        CALL    DebounceInit
        BANKSEL PORTC
        RETURN

; called once per main-loop iteration indirectly
keypadPoll
        BANKSEL EntryReady
        MOVF    EntryReady,F
        BTFSS   STATUS,Z
        RETURN                  ; Accepted pair is saved until used
        CALL    ReadKey
        BANKSEL KeyEvent
        MOVF    KeyEvent,F
        BTFSC   STATUS,Z
        GOTO    PollDone
        MOVF    KeyEvent,W
        XORLW   KEY_CLEAR
        BTFSC   STATUS,Z
        GOTO    ClearEntry
        MOVF    KeyEvent,W
        XORLW   KEY_CONFIRM
        BTFSC   STATUS,Z
        GOTO    ConfirmEntry

; only enter '0'-'9' binary digit
        MOVLW   '0'
        SUBWF   KeyEvent,W
        BTFSS   STATUS,C
        GOTO    PollDone
        MOVWF   DigitValue
        MOVLW   D'10'
        SUBWF   DigitValue,W
        BTFSC   STATUS,C
        GOTO    PollDone
        BANKSEL EntryCount
        MOVLW   D'4'
        SUBWF   EntryCount,W
        BTFSC   STATUS,C
        GOTO    PollDone       ; Ignore fifth and later digits
        MOVF    EntryCount,W
        BTFSC   STATUS,Z
        GOTO    PutDigit0
        MOVF    EntryCount,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        GOTO    PutDigit1
        MOVF    EntryCount,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        GOTO    PutDigit2
        BANKSEL DigitValue
        MOVF    DigitValue,W
        BANKSEL EntryD3
        MOVWF   EntryD3
        GOTO    DigitStored
PutDigit0
        BANKSEL DigitValue
        MOVF    DigitValue,W
        BANKSEL EntryD0
        MOVWF   EntryD0
        GOTO    DigitStored
PutDigit1
        BANKSEL DigitValue
        MOVF    DigitValue,W
        BANKSEL EntryD1
        MOVWF   EntryD1
        GOTO    DigitStored
PutDigit2
        BANKSEL DigitValue
        MOVF    DigitValue,W
        BANKSEL EntryD2
        MOVWF   EntryD2
DigitStored
        INCF    EntryCount,F
        CLRF    EntryError
        MOVLW   D'1'
        MOVWF   UiDirty
        GOTO    PollDone


ClearEntry
        BANKSEL EntryCount
        CLRF    EntryCount
        CLRF    EntryHour
        CLRF    EntryMinute
        CLRF    EntryError
        MOVLW   NO_DIGIT
        MOVWF   EntryD0
        MOVWF   EntryD1
        MOVWF   EntryD2
        MOVWF   EntryD3
        MOVLW   D'1'
        MOVWF   UiDirty
        GOTO    PollDone

; # for confirmation
ConfirmEntry
        BANKSEL EntryCount
        MOVF    EntryCount,W
        XORLW   D'4'
        BTFSS   STATUS,Z
        GOTO    ShortEntry

; EntryHour = 10*EntryD0 + EntryD1
        MOVF    EntryD0,W
        CALL    MultiplyTen
        BANKSEL EntryD1
        ADDWF   EntryD1,W
        MOVWF   EntryHour
        MOVLW   D'24'
        SUBWF   EntryHour,W
        BTFSC   STATUS,C
        GOTO    RangeEntry

; EntryMinute = 10*EntryD2 + EntryD3
        MOVF    EntryD2,W
        CALL    MultiplyTen
        BANKSEL EntryD3
        ADDWF   EntryD3,W
        MOVWF   EntryMinute
        MOVLW   D'60'
        SUBWF   EntryMinute,W
        BTFSC   STATUS,C
        GOTO    RangeEntry
        CLRF    EntryError
        MOVLW   D'1'
        MOVWF   EntryReady
        MOVWF   UiDirty
        GOTO    PollDone
ShortEntry
        MOVLW   ERROR_SHORT
        GOTO    SetEntryError
RangeEntry
        MOVLW   ERROR_RANGE
SetEntryError
        MOVWF   EntryError
        MOVLW   D'1'
        MOVWF   UiDirty
PollDone
        BANKSEL PORTC
        RETURN

; returns W = 10*input
MultiplyTen
        BANKSEL TenCounter
        MOVWF   TenCounter
        CLRF    TenResult
MultiplyLoop
        MOVF    TenCounter,F
        BTFSC   STATUS,Z
        GOTO    MultiplyDone
        MOVLW   D'10'
        ADDWF   TenResult,F
        DECF    TenCounter,F
        GOTO    MultiplyLoop
MultiplyDone
        MOVF    TenResult,W
        BANKSEL PORTC
        RETURN

; initialise debounce disarmed
DebounceInit
        BANKSEL RawKey
        CLRF    RawKey
        CLRF    CandidateKey
        CLRF    KeyArmed
        CLRF    KeyEvent
        MOVLW   0xFF
        MOVWF   StableKey
        BANKSEL NowL
        MOVF    NowL,W
        BANKSEL KeyStartL
        MOVWF   KeyStartL
        BANKSEL NowH
        MOVF    NowH,W
        BANKSEL KeyStartH
        MOVWF   KeyStartH
        BANKSEL PORTC
        MOVLW   KEY_ROW_IDLE
        MOVWF   KEY_PORT
        RETURN

; scan every call, debounce by elapsed real ticks
ReadKey
        BANKSEL KeyEvent
        CLRF    KeyEvent
        CALL    ScanKeypad
        BANKSEL RawKey
        MOVF    RawKey,W
        XORLW   KEY_AMBIGUOUS
        BTFSS   STATUS,Z
        GOTO    CompareCandidate
; Ambiguous input locks out events until a stable full release
        CLRF    KeyArmed
CompareCandidate
        MOVF    CandidateKey,W
        XORWF   RawKey,W
        BTFSC   STATUS,Z
        GOTO    CandidateUnchanged
        MOVF    RawKey,W
        MOVWF   CandidateKey
        BANKSEL NowL
        MOVF    NowL,W
        BANKSEL KeyStartL
        MOVWF   KeyStartL
        BANKSEL NowH
        MOVF    NowH,W
        BANKSEL KeyStartH
        MOVWF   KeyStartH
        GOTO    ReadDone
CandidateUnchanged
        MOVF    StableKey,W
        XORWF   CandidateKey,W
        BTFSC   STATUS,Z
        GOTO    ReadDone

        MOVF    KeyStartL,W
        BANKSEL NowL
        SUBWF   NowL,W
        BANKSEL KeyElapsedL
        MOVWF   KeyElapsedL
        MOVF    KeyStartH,W    
        BTFSS   STATUS,C
        ADDLW   D'1'           ; include borrow in high-byte subtrahend
        BANKSEL NowH
        SUBWF   NowH,W
        BANKSEL KeyElapsedH
        MOVWF   KeyElapsedH
        MOVF    KeyElapsedH,F
        BTFSS   STATUS,Z
        GOTO    CandidateStable
        MOVLW   DEBOUNCE_TICKS
        SUBWF   KeyElapsedL,W
        BTFSS   STATUS,C
        GOTO    ReadDone
CandidateStable
        MOVF    CandidateKey,W
        MOVWF   StableKey
        MOVF    StableKey,F
        BTFSC   STATUS,Z
        GOTO    StableRelease
        MOVF    StableKey,W
        XORLW   KEY_AMBIGUOUS
        BTFSC   STATUS,Z
        GOTO    ReadDone
        MOVF    KeyArmed,F
        BTFSC   STATUS,Z
        GOTO    ReadDone
        MOVF    StableKey,W
        MOVWF   KeyEvent
        CLRF    KeyArmed
        GOTO    ReadDone
StableRelease
        MOVLW   D'1'
        MOVWF   KeyArmed
ReadDone
        BANKSEL PORTC
        RETURN

; only exactly one detected key is accepted
; multi-column or multi-row patterns return KEY_AMBIGUOUS
ScanKeypad
        BANKSEL RawKey
        CLRF    RawKey
        CLRF    KeyCount

; Row A: 1,2,3
        BANKSEL KEY_PORT
        MOVLW   B'00001110'
        MOVWF   KEY_PORT
        NOP
        CALL    ReadColumns
        BANKSEL ColumnCode
        MOVWF   ColumnCode
        MOVF    ColumnCode,F
        BTFSC   STATUS,Z
        GOTO    ScanRowB
        MOVF    ColumnCode,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        GOTO    ScanAmbiguous
        MOVLW   '0'
        ADDWF   ColumnCode,W
        CALL    StoreKey

; Row B: 4,5,6
ScanRowB
        BANKSEL KEY_PORT
        MOVLW   B'00001101'
        MOVWF   KEY_PORT
        NOP
        CALL    ReadColumns
        BANKSEL ColumnCode
        MOVWF   ColumnCode
        MOVF    ColumnCode,F
        BTFSC   STATUS,Z
        GOTO    ScanRowC
        MOVF    ColumnCode,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        GOTO    ScanAmbiguous
        MOVLW   '3'
        ADDWF   ColumnCode,W
        CALL    StoreKey

; Row C: 7,8,9
ScanRowC
        BANKSEL KEY_PORT
        MOVLW   B'00001011'
        MOVWF   KEY_PORT
        NOP
        CALL    ReadColumns
        BANKSEL ColumnCode
        MOVWF   ColumnCode
        MOVF    ColumnCode,F
        BTFSC   STATUS,Z
        GOTO    ScanRowD
        MOVF    ColumnCode,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        GOTO    ScanAmbiguous
        MOVLW   '6'
        ADDWF   ColumnCode,W
        CALL    StoreKey

; Row D: *,0,#
ScanRowD
        BANKSEL KEY_PORT
        MOVLW   B'00000111'
        MOVWF   KEY_PORT
        NOP
        CALL    ReadColumns
        BANKSEL ColumnCode
        MOVWF   ColumnCode
        MOVF    ColumnCode,F
        BTFSC   STATUS,Z
        GOTO    ScanFinish
        MOVF    ColumnCode,W
        XORLW   D'4'
        BTFSC   STATUS,Z
        GOTO    ScanAmbiguous
        MOVF    ColumnCode,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        GOTO    ScanStar
        MOVF    ColumnCode,W
        XORLW   D'2'
        BTFSC   STATUS,Z
        GOTO    ScanZero
        MOVLW   KEY_CONFIRM
        CALL    StoreKey
        GOTO    ScanFinish
ScanStar
        MOVLW   KEY_CLEAR
        CALL    StoreKey
        GOTO    ScanFinish
ScanZero
        MOVLW   '0'
        CALL    StoreKey
ScanFinish
        BANKSEL KeyCount
        MOVF    KeyCount,W
        XORLW   D'1'
        BTFSC   STATUS,Z
        GOTO    ScanDone
        MOVF    KeyCount,F
        BTFSC   STATUS,Z
        GOTO    ScanNone
ScanAmbiguous
        MOVLW   KEY_AMBIGUOUS
        MOVWF   RawKey
        GOTO    ScanDone
ScanNone
        CLRF    RawKey
ScanDone
        BANKSEL KEY_PORT
        MOVLW   KEY_ROW_IDLE
        MOVWF   KEY_PORT
        RETURN

; one PORTC sample per row: 0=no key, 1/2/3=column, 4=ambiguous
ReadColumns
        BANKSEL KEY_PORT
        MOVF    KEY_PORT,W
        ANDLW   KEY_COL_MASK
        BANKSEL Columns
        MOVWF   Columns
        MOVF    Columns,W
        XORLW   B'01110000'
        BTFSC   STATUS,Z
        GOTO    ColumnNone
        MOVF    Columns,W
        XORLW   B'01100000'
        BTFSC   STATUS,Z
        GOTO    ColumnOne
        MOVF    Columns,W
        XORLW   B'01010000'
        BTFSC   STATUS,Z
        GOTO    ColumnTwo
        MOVF    Columns,W
        XORLW   B'00110000'
        BTFSC   STATUS,Z
        GOTO    ColumnThree
        BANKSEL KEY_PORT
        RETLW   D'4'
ColumnNone
        BANKSEL KEY_PORT
        RETLW   D'0'
ColumnOne
        BANKSEL KEY_PORT
        RETLW   D'1'
ColumnTwo
        BANKSEL KEY_PORT
        RETLW   D'2'
ColumnThree
        BANKSEL KEY_PORT
        RETLW   D'3'

; W = detected ASCII key
StoreKey
        BANKSEL RawKey
        MOVWF   RawKey
        INCF    KeyCount,F
        RETURN
