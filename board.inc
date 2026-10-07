; agreed initial hardware configuration

boardInit
        BANKSEL PORTA
        CLRF    INTCON          ; All interrupts disabled
        CLRF    PORTA
        CLRF    PORTB           ; LEDs and buzzer OFF
        MOVLW   0x0F
        MOVWF   PORTC           ; All keypad rows HIGH
        CLRF    PORTD
        CLRF    PORTE
        CLRF    ADCON0          ; ADC OFF
        BANKSEL CMCON
        MOVLW   0x07
        MOVWF   CMCON           ; Comparators OFF
        CLRF    CVRCON
        BANKSEL CCP1CON
        CLRF    CCP1CON         ; PWM/capture/compare OFF
        CLRF    CCP2CON
        CLRF    SSPCON          ; SPI/I2C OFF
        CLRF    RCSTA           ; Serial port OFF
        CLRF    T1CON
        CLRF    T2CON

        BANKSEL TRISA
        MOVLW   B'00111111'
        MOVWF   TRISA           ; Unused PORTA = inputs
        MOVLW   B'11110000'
        MOVWF   TRISB           ; RB0..3 outputs, RB4..7 inputs
        MOVWF   TRISC           ; RC0..3 rows, RC4..6 columns; RC7 unused
        CLRF    TRISD           ; LCD port outputs
        MOVLW   B'00000111'
        MOVWF   TRISE           ; PORTE inputs; parallel mode OFF
        MOVLW   0x06
        MOVWF   ADCON1          ; All analogue-capable pins digital
        MOVLW   0x87
        MOVWF   OPTION_REG      ; PORTB weak pull-ups OFF
        CLRF    PIE1
        CLRF    PIE2
        BANKSEL PORTA
        CLRF    PIR1
        CLRF    PIR2
        RETURN

; Clear only allocated BANK 0 RAM at reset

ramInit
        BCF     STATUS,IRP
        MOVLW   0x20
        MOVWF   FSR
p4ClearRam
        CLRF    INDF
        INCF    FSR,F
        MOVLW   0x70
        SUBWF   FSR,W
        BTFSS   STATUS,Z
        GOTO    p4ClearRam
        RETURN