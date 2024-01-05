.include "m328pdef.inc"
.include "delayMacro.inc"
.include "UART.inc"

.cseg

.def A = r18
.def AH = r17
.def B = r24
.def CHAR = r21

.org 0x00
	; I/O Pins Configuration
	SBI DDRB, PB0			; Set PB0 pin for Output to BUZZER
	CBI PORTB, PB0			; Clear PORTB register

	LDI B, ~(1 << PD7)  ; Set PD7 for PIR Input
	out DDRD, B
	
	; ADC Configuration for LDR
	LDI A,0b11000111	; [ADEN ADSC ADATE ADIF ADIE ADIE ADPS2 ADPS1 ADPS0]
	STS ADCSRA,A
	
	LDI A, 0b01100000	; [REFS1 REFS0 ADLAR – MUX3 MUX2 MUX1 MUX0]
	STS ADMUX, A			; Select ADC0 (PC0) pin
	SBI PORTC, PC0		; Enable Pull-up Resistor

	; UART Configuration
	SBI DDRD, PD1	; Set PD1 (TX) as Output
	CBI PORTD, PD1	; TX Low (initial state)
	CBI DDRD, PD0	; Set PD0 (RX) as Input
	SBI PORTD, PD0	; Enable Pull-up Resistor on RX
	Serial_begin	; Initialize UART Protocol
	
	LDI A,0x00
	LDI AH,0x00
	LDI B,0x00

MAIN_LOOP:
	Serial_read CHAR

	CPI CHAR ,'1'
	BREQ BUZZER_ON
	CPI CHAR,'0'
	BREQ BUZZER_OFF


	LDS A, ADCSRA		; Start Analog to Digital Conversion
	ORI A, (1 << ADSC)
	STS ADCSRA, A

wait:
	LDS A, ADCSRA		; wait for conversion to complete
	SBRC A, ADSC
rjmp wait
	LDS A,ADCL			; Must Read ADCL before ADCH
	LDS AH,ADCH
	CPI AH,250			; compare LDR reading with our desired threshold
	brsh NIGHT			; jump if same or higher (AH >= 200)
	BRLT DAY

rjmp MAIN_LOOP
NIGHT:
	LDI B,0x00
	IN B, PIND			; Read value from PIR pin (PD7)
    ANDI B, (1 << PD7)	; Mask other bits (PD7 is 7th bit)
	CPI B,0
	BRSH BUZZER_ON
rjmp BUZZER_OFF
DAY:
	CBI PORTB,0			; BUZZER OFF  As it is DAYTIME
	Serial_writeReg r25 ;lDR
	delay 2500
rjmp MAIN_LOOP
BUZZER_ON:
		SBI PORTB,0
rjmp MAIN_LOOP
BUZZER_OFF:
		CBI PORTB,0
rjmp MAIN_LOOP

