/*******************************************************
This program was created by the CodeWizardAVR V3.37 UL 
Automatic Program Generator
� Copyright 1998-2019 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : Keypad Driver and LCD
Version : 01
Date    : 10/18/2019
Author  : Kaitlyn Gabardi and Alston Correya
Company : University of Wisconsin-Madison
Description:
The following project allows for reading and
decoding a 4x4 keypad and writing to a 4x40 text LCD. 
Using the WRLATCH Output Latch and the RDSTATUS Input
Latch on the STK500/600 daughter card, data is written 
to and read from the keypad. The address bus is also
utilized in order to enable/disable the decoding chip
that allows for the transmission of across the chip.
Data is then written to an LCD to display the keypad
output.

Chip type               : ATmega2560
Program type            : Application
AVR Core Clock frequency: 14.745600 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 2048
*******************************************************/

#include <io.h>
#include <stringconversion.h>
#include <delay.h>

/* the LCD module is connected to PORTC */
#asm
	.equ __lcd_port=0x08;
#endasm

//Header file that is HD44780 Compatible
#include <lcd4x40.h>

//Define macros to improve readability
#define WRDISABLE	0b00110110	//WRLATCH disable address
#define WRENABLE	0b00010110	//WRLATCH enable address
#define RDDISABLE	0b00111100	//RDSTATUS disable address
#define RDENABLE	0b00011100	//RDSTATE enable address
#define ADDRESSBUS	PORTK		//PORTK connected to address bus
#define DATABUSIN	PIND		//Reading from data bus
#define DATABUSOUT	PORTD		//Writing to data bus
#define DATASTATUS	DDRD		//I/O status of PORTD
#define COLUMN1		0b11101111	//Column 1 driving bit pattern

//Declare your global variables here
//char message[] = "Lab 07";	//Message to display on LCD
//unsigned char testchar = 0;	//Test variable for WROUTPUT function
//unsigned char ASCII1[9];		//8-bit array of length 9 to hold 8-bit value
//unsigned char ASCII2[17];		//16-bit array of length 17 to hold 16-bit value
char S0, S1, S2;				//store current state of State Machine
char pressed;					//store status of any switch pressed in keypad
int value;						//store current switch status from all switches
char lcd_out;					//store current state of LCD display
char released;					//store status of whether a key is released
int display;					//variable to display to LCD


/**********************************************************
PreLab Question 4
Takes the state of a 16-bit switch status from a
4x4 keypad and returns the corresponding ASCII value
**********************************************************/
char switch2ASCII(int switch){

unsigned char bitlocation;			//store the current bitlocation being checked
unsigned int a,b,c;					//watch variables to help debug function

bitlocation = 0;					//Initialize bitlocation
b = 0x0001;							//Initialize mask

   while(bitlocation < 16){
	   a = switch;
	   c = a & b;					//Mask switch status with bitlocation based mask
	   
	   if (c != 0){					//If switch detected as pressed, break out of loop
		   break;
	   }
	   bitlocation++;				//increment bitlocation
	   b = b << 1;					//change mask to check next bitlocation
   }
   
   //Return the corresponding ASCII character based on which switch is pressed
   if (bitlocation == 0x00){
	   return 0x31;					//switch 0 -> 1
   }
   
   else if (bitlocation == 0x01){	//switch 1 -> 4
	   return 0x34;
   }
   
   else if (bitlocation == 0x02){	//switch 2 -> 7
	   return 0x37;
   }
   
   else if (bitlocation == 0x03){	//switch 3 -> *
	   return 0x2A;
   }
   
   else if (bitlocation == 0x04){	//switch 4 -> 2
	   return  0x32;
   }
   
   else if (bitlocation == 0x05){	//switch 5 -> 5
	   return 0x35;
   }
   
   else if (bitlocation == 0x06){	//switch 6 -> 8
	   return 0x38;
   }
   
   else if (bitlocation == 0x07){	//switch 7 -> 0
	   return 0x30;
   }
   
   else if (bitlocation == 0x08){	//switch 8 -> 3
	   return 0x33;
   }
   
   else if (bitlocation == 0x09){	//switch 9 -> 6
	   return 0x36;
   }
   
   else if (bitlocation == 0x0A){	//switch 10 -> 9
	   return 0x39;
   }
   
   else if (bitlocation == 0x0B){	//switch 11 -> #
	   return 0x23;
   }
   
   else if (bitlocation == 0x0C){	//switch 12 -> A
	   return 0x41;
   }
   
   else if (bitlocation == 0x0D){	//switch 13 -> B
	   return 0x42;
   }
   
   else if (bitlocation == 0x0E){	//switch 14 -> C
	   return 0x43;
   }
   
   else if (bitlocation == 0x0F){	//switch 15 -> D
	   return 0x44;
   }
   
   else{
	   return 0x00;					//if no switch detected, return NULL
   }
}


/**********************************************************
Function to drive the WRLATCH output with an 8-bit value.
**********************************************************/
void WriteLatch(char columndrive){
	ADDRESSBUS = WRDISABLE;		//Address to Disable WRLATCH
	DATASTATUS = 0xFF;			//Change PORTD to output
	
	DATABUSOUT = columndrive;	//Send data on data bus to drive column
	delay_us(1);			//Wait for data bus to stabilize
	
	ADDRESSBUS = WRENABLE;		//Address to Enable WRLATCH
	delay_us(1);			//Wait for decoder output to propagate across chip
	
	ADDRESSBUS = WRDISABLE;		//Disable decoder chip0
}


/**********************************************************
Function to read the 8-bit RDSTATUS input.
**********************************************************/
char ReadStatus(){
	unsigned char rowread;		//Variable to store state of RDSTATUS header
	
	DATASTATUS = 0x00;			//Make all PORTB pins input
	ADDRESSBUS = RDENABLE;		//Address to Enable RDSTATUS
	
	delay_us(1);				//Wait for decoder output to propagate across chip
	rowread = DATABUSIN;		//Store header status in variable
	
	ADDRESSBUS = RDDISABLE;		//Disable decoder chip
	
	return rowread;				//Return status of RDSTATUS header
}


/**********************************************************
Function to drive and read individual rows and columns
on the keypad and return the 16-bit int state of all
sixteen switches by bit.
**********************************************************/
unsigned int KeypadSwitch(){
	int i;							//For loop counter variable
	unsigned char charvalue = 0;	//Store current row status
	unsigned int intvalue = 0;		//Store current nibble status
	char COLUMN;					//Current column status
	unsigned int status;			//Current keypad (switches) status
	
	status = 0x0000;				//initialize status as zero;
	
	for(i=0; i<4; i++){
		
		//Drive Keypad columns depending on the current column
		COLUMN = ((COLUMN1 << (i*1)) | 0b00001111);
		//Write to WRLATCH (drive column [i])
		WriteLatch(COLUMN);
		
		//Read from RDSTATUS (read rows)
		charvalue = ReadStatus();
		
		//Convert 8-bit row data to int and place in lowest nibble
		intvalue = (unsigned int)((~charvalue) >> 4);
		
		//Shift the nibble depending on column i
		intvalue = (intvalue << (i*4));
		
		//Overlay nibble value from row onto the current status
		status = (status | intvalue);
		
	}
		return status;	//return 4x4 switch status 
}


/**********************************************************
Function to drive a State Machine that consists of three
states and outputs the pressed switch to the LCD display
when the first key is released.
**********************************************************/
void StateMachine(){
	
	//Check if any switch is pressed
	if (KeypadSwitch() != 0x0000){
		pressed = 1;	
	}	
	else{
		pressed = 0;		
	}
	
	//Check if any key has been released
	if (KeypadSwitch() < value){
		released = 1;
		display = value ^ KeypadSwitch();	//find switch status of released key
	}
	else{
		released = 0;
	}
	
	//Store current value of switch status
	value = KeypadSwitch();
			
	//STATE EQUATIONS (Entrance and Exit Criteria)
	S0 = (S0 || S2 && lcd_out) &&! S1;		
	S1 = (S1 || (S0 && pressed)) &&! S2;
	S2 = (S2 || (S1 && released)) &&! S0;
		
	//OUTPUT EQUATIONS
	if (S2 &&! S1 &&! S0){ 
		lcd_clear();
		//display keypad character equivalent of switch
		lcd_putchar(switch2ASCII(display));	
		lcd_out = 1;			//turn on LCD display
	}	
}

// Timer1 output compare A interrupt service routine
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
	//Display message "Lab 07" (Q6)
	//Clear existing message on LCD
	//lcd_clear();
	//Display message to LCD
	//lcd_puts(message);
	
	//Testing the ReadStatus() function and display to LCD (Q13)
	//CtoBString(ReadStatus(), ASCII1)		//convert 8-bit value into binary and
											//stored in char array
	//lcd_clear();							//clear LCD display
	//lcd_puts(ASCII1);						//display 8-bit binary value
	
	//Testing code for KeypadSwitch() (Q14)
	//ItoBString(KeypadSwitch(), ASCII2);	//convert 16-bit value into binary and
											//stored in char array
	//lcd_clear();							//clear LCD display
	//lcd_puts(ASCII2);						//display 16-bit binary value
	
	StateMachine();							//State Machine function to display released
											//key on the LCD
}

void main(void)
{
	//Initialize states at boot up only
	S0 = 1;
	S1 = 0;
	S2 = 0;
	
	//Initialize LCD and clear the display
	lcd_init();	

// PORT & TIMER/INTERRUPT CONFIGURATION
{

// Crystal Oscillator division factor: 1
#pragma optsize-
CLKPR=(1<<CLKPCE);
CLKPR=(0<<CLKPCE) | (0<<CLKPS3) | (0<<CLKPS2) | (0<<CLKPS1) | (0<<CLKPS0);
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

// Input/Output Ports initialization
// Port A initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRA=(0<<DDA7) | (0<<DDA6) | (0<<DDA5) | (0<<DDA4) | (0<<DDA3) | (0<<DDA2) | (0<<DDA1) | (0<<DDA0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTA=(0<<PORTA7) | (0<<PORTA6) | (0<<PORTA5) | (0<<PORTA4) | (0<<PORTA3) | (0<<PORTA2) | (0<<PORTA1) | (0<<PORTA0);

// Port B initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRB=(0<<DDB7) | (0<<DDB6) | (0<<DDB5) | (0<<DDB4) | (0<<DDB3) | (0<<DDB2) | (0<<DDB1) | (0<<DDB0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

// Port C initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRC=(0<<DDC7) | (0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (0<<DDC3) | (0<<DDC2) | (0<<DDC1) | (0<<DDC0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTC=(0<<PORTC7) | (0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (0<<PORTC1) | (0<<PORTC0);

// Port D initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRD=(0<<DDD7) | (0<<DDD6) | (0<<DDD5) | (0<<DDD4) | (0<<DDD3) | (0<<DDD2) | (0<<DDD1) | (0<<DDD0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTD=(0<<PORTD7) | (0<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (0<<PORTD3) | (0<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);

// Port E initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRE=(0<<DDE7) | (0<<DDE6) | (0<<DDE5) | (0<<DDE4) | (0<<DDE3) | (0<<DDE2) | (0<<DDE1) | (0<<DDE0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTE=(0<<PORTE7) | (0<<PORTE6) | (0<<PORTE5) | (0<<PORTE4) | (0<<PORTE3) | (0<<PORTE2) | (0<<PORTE1) | (0<<PORTE0);

// Port F initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRF=(0<<DDF7) | (0<<DDF6) | (0<<DDF5) | (0<<DDF4) | (0<<DDF3) | (0<<DDF2) | (0<<DDF1) | (0<<DDF0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTF=(0<<PORTF7) | (0<<PORTF6) | (0<<PORTF5) | (0<<PORTF4) | (0<<PORTF3) | (0<<PORTF2) | (0<<PORTF1) | (0<<PORTF0);

// Port G initialization
// Function: Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRG=(0<<DDG5) | (0<<DDG4) | (0<<DDG3) | (0<<DDG2) | (0<<DDG1) | (0<<DDG0);
// State: Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTG=(0<<PORTG5) | (0<<PORTG4) | (0<<PORTG3) | (0<<PORTG2) | (0<<PORTG1) | (0<<PORTG0);

// Port H initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRH=(0<<DDH7) | (0<<DDH6) | (0<<DDH5) | (0<<DDH4) | (0<<DDH3) | (0<<DDH2) | (0<<DDH1) | (0<<DDH0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTH=(0<<PORTH7) | (0<<PORTH6) | (0<<PORTH5) | (0<<PORTH4) | (0<<PORTH3) | (0<<PORTH2) | (0<<PORTH1) | (0<<PORTH0);

// Port J initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRJ=(0<<DDJ7) | (0<<DDJ6) | (0<<DDJ5) | (0<<DDJ4) | (0<<DDJ3) | (0<<DDJ2) | (0<<DDJ1) | (0<<DDJ0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTJ=(0<<PORTJ7) | (0<<PORTJ6) | (0<<PORTJ5) | (0<<PORTJ4) | (0<<PORTJ3) | (0<<PORTJ2) | (0<<PORTJ1) | (0<<PORTJ0);

// Port K initialization
// Function: Bit7=In Bit6=In Bit5=Out Bit4=Out Bit3=Out Bit2=Out Bit1=Out Bit0=Out 
DDRK=(0<<DDK7) | (0<<DDK6) | (1<<DDK5) | (1<<DDK4) | (1<<DDK3) | (1<<DDK2) | (1<<DDK1) | (1<<DDK0);
// State: Bit7=0 Bit6=0 Bit5=0 Bit4=0 Bit3=0 Bit2=0 Bit1=0 Bit0=0 
PORTK=(0<<PORTK7) | (0<<PORTK6) | (0<<PORTK5) | (0<<PORTK4) | (0<<PORTK3) | (0<<PORTK2) | (0<<PORTK1) | (0<<PORTK0);

// Port L initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRL=(0<<DDL7) | (0<<DDL6) | (0<<DDL5) | (0<<DDL4) | (0<<DDL3) | (0<<DDL2) | (0<<DDL1) | (0<<DDL0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTL=(0<<PORTL7) | (0<<PORTL6) | (0<<PORTL5) | (0<<PORTL4) | (0<<PORTL3) | (0<<PORTL2) | (0<<PORTL1) | (0<<PORTL0);

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: Timer 0 Stopped
// Mode: Normal top=0xFF
// OC0A output: Disconnected
// OC0B output: Disconnected
TCCR0A=(0<<COM0A1) | (0<<COM0A0) | (0<<COM0B1) | (0<<COM0B0) | (0<<WGM01) | (0<<WGM00);
TCCR0B=(0<<WGM02) | (0<<CS02) | (0<<CS01) | (0<<CS00);
TCNT0=0x00;
OCR0A=0x00;
OCR0B=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 1843.200 kHz
// Mode: CTC top=OCR1A
// OC1A output: Disconnected
// OC1B output: Disconnected
// OC1C output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer Period: 16.667 ms
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: On
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR1A=(0<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<COM1C1) | (0<<COM1C0) | (0<<WGM11) | (0<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (1<<CS11) | (0<<CS10);
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x78;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;
OCR1CH=0x00;
OCR1CL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer2 Stopped
// Mode: Normal top=0xFF
// OC2A output: Disconnected
// OC2B output: Disconnected
ASSR=(0<<EXCLK) | (0<<AS2);
TCCR2A=(0<<COM2A1) | (0<<COM2A0) | (0<<COM2B1) | (0<<COM2B0) | (0<<WGM21) | (0<<WGM20);
TCCR2B=(0<<WGM22) | (0<<CS22) | (0<<CS21) | (0<<CS20);
TCNT2=0x00;
OCR2A=0x00;
OCR2B=0x00;

// Timer/Counter 3 initialization
// Clock source: System Clock
// Clock value: Timer3 Stopped
// Mode: Normal top=0xFFFF
// OC3A output: Disconnected
// OC3B output: Disconnected
// OC3C output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer3 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR3A=(0<<COM3A1) | (0<<COM3A0) | (0<<COM3B1) | (0<<COM3B0) | (0<<COM3C1) | (0<<COM3C0) | (0<<WGM31) | (0<<WGM30);
TCCR3B=(0<<ICNC3) | (0<<ICES3) | (0<<WGM33) | (0<<WGM32) | (0<<CS32) | (0<<CS31) | (0<<CS30);
TCNT3H=0x00;
TCNT3L=0x00;
ICR3H=0x00;
ICR3L=0x00;
OCR3AH=0x00;
OCR3AL=0x00;
OCR3BH=0x00;
OCR3BL=0x00;
OCR3CH=0x00;
OCR3CL=0x00;

// Timer/Counter 4 initialization
// Clock source: System Clock
// Clock value: Timer4 Stopped
// Mode: Normal top=0xFFFF
// OC4A output: Disconnected
// OC4B output: Disconnected
// OC4C output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer4 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR4A=(0<<COM4A1) | (0<<COM4A0) | (0<<COM4B1) | (0<<COM4B0) | (0<<COM4C1) | (0<<COM4C0) | (0<<WGM41) | (0<<WGM40);
TCCR4B=(0<<ICNC4) | (0<<ICES4) | (0<<WGM43) | (0<<WGM42) | (0<<CS42) | (0<<CS41) | (0<<CS40);
TCNT4H=0x00;
TCNT4L=0x00;
ICR4H=0x00;
ICR4L=0x00;
OCR4AH=0x00;
OCR4AL=0x00;
OCR4BH=0x00;
OCR4BL=0x00;
OCR4CH=0x00;
OCR4CL=0x00;

// Timer/Counter 5 initialization
// Clock source: System Clock
// Clock value: Timer5 Stopped
// Mode: Normal top=0xFFFF
// OC5A output: Disconnected
// OC5B output: Disconnected
// OC5C output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer5 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR5A=(0<<COM5A1) | (0<<COM5A0) | (0<<COM5B1) | (0<<COM5B0) | (0<<COM5C1) | (0<<COM5C0) | (0<<WGM51) | (0<<WGM50);
TCCR5B=(0<<ICNC5) | (0<<ICES5) | (0<<WGM53) | (0<<WGM52) | (0<<CS52) | (0<<CS51) | (0<<CS50);
TCNT5H=0x00;
TCNT5L=0x00;
ICR5H=0x00;
ICR5L=0x00;
OCR5AH=0x00;
OCR5AL=0x00;
OCR5BH=0x00;
OCR5BL=0x00;
OCR5CH=0x00;
OCR5CL=0x00;

// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (0<<TOIE0);

// Timer/Counter 1 Interrupt(s) initialization
TIMSK1=(0<<ICIE1) | (0<<OCIE1C) | (0<<OCIE1B) | (1<<OCIE1A) | (0<<TOIE1);

// Timer/Counter 2 Interrupt(s) initialization
TIMSK2=(0<<OCIE2B) | (0<<OCIE2A) | (0<<TOIE2);

// Timer/Counter 3 Interrupt(s) initialization
TIMSK3=(0<<ICIE3) | (0<<OCIE3C) | (0<<OCIE3B) | (0<<OCIE3A) | (0<<TOIE3);

// Timer/Counter 4 Interrupt(s) initialization
TIMSK4=(0<<ICIE4) | (0<<OCIE4C) | (0<<OCIE4B) | (0<<OCIE4A) | (0<<TOIE4);

// Timer/Counter 5 Interrupt(s) initialization
TIMSK5=(0<<ICIE5) | (0<<OCIE5C) | (0<<OCIE5B) | (0<<OCIE5A) | (0<<TOIE5);

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: Off
// INT3: Off
// INT4: Off
// INT5: Off
// INT6: Off
// INT7: Off
EICRA=(0<<ISC31) | (0<<ISC30) | (0<<ISC21) | (0<<ISC20) | (0<<ISC11) | (0<<ISC10) | (0<<ISC01) | (0<<ISC00);
EICRB=(0<<ISC71) | (0<<ISC70) | (0<<ISC61) | (0<<ISC60) | (0<<ISC51) | (0<<ISC50) | (0<<ISC41) | (0<<ISC40);
EIMSK=(0<<INT7) | (0<<INT6) | (0<<INT5) | (0<<INT4) | (0<<INT3) | (0<<INT2) | (0<<INT1) | (0<<INT0);
// PCINT0 interrupt: Off
// PCINT1 interrupt: Off
// PCINT2 interrupt: Off
// PCINT3 interrupt: Off
// PCINT4 interrupt: Off
// PCINT5 interrupt: Off
// PCINT6 interrupt: Off
// PCINT7 interrupt: Off
// PCINT8 interrupt: Off
// PCINT9 interrupt: Off
// PCINT10 interrupt: Off
// PCINT11 interrupt: Off
// PCINT12 interrupt: Off
// PCINT13 interrupt: Off
// PCINT14 interrupt: Off
// PCINT15 interrupt: Off
// PCINT16 interrupt: Off
// PCINT17 interrupt: Off
// PCINT18 interrupt: Off
// PCINT19 interrupt: Off
// PCINT20 interrupt: Off
// PCINT21 interrupt: Off
// PCINT22 interrupt: Off
// PCINT23 interrupt: Off
PCMSK0=(0<<PCINT7) | (0<<PCINT6) | (0<<PCINT5) | (0<<PCINT4) | (0<<PCINT3) | (0<<PCINT2) | (0<<PCINT1) | (0<<PCINT0);
PCMSK1=(0<<PCINT15) | (0<<PCINT14) | (0<<PCINT13) | (0<<PCINT12) | (0<<PCINT11) | (0<<PCINT10) | (0<<PCINT9) | (0<<PCINT8);
PCMSK2=(0<<PCINT23) | (0<<PCINT22) | (0<<PCINT21) | (0<<PCINT20) | (0<<PCINT19) | (0<<PCINT18) | (0<<PCINT17) | (0<<PCINT16);
PCICR=(0<<PCIE2) | (0<<PCIE1) | (0<<PCIE0);

// USART0 initialization
// USART0 disabled
UCSR0B=(0<<RXCIE0) | (0<<TXCIE0) | (0<<UDRIE0) | (0<<RXEN0) | (0<<TXEN0) | (0<<UCSZ02) | (0<<RXB80) | (0<<TXB80);

// USART1 initialization
// USART1 disabled
UCSR1B=(0<<RXCIE1) | (0<<TXCIE1) | (0<<UDRIE1) | (0<<RXEN1) | (0<<TXEN1) | (0<<UCSZ12) | (0<<RXB81) | (0<<TXB81);

// USART2 initialization
// USART2 disabled
UCSR2B=(0<<RXCIE2) | (0<<TXCIE2) | (0<<UDRIE2) | (0<<RXEN2) | (0<<TXEN2) | (0<<UCSZ22) | (0<<RXB82) | (0<<TXB82);

// USART3 initialization
// USART3 disabled
UCSR3B=(0<<RXCIE3) | (0<<TXCIE3) | (0<<UDRIE3) | (0<<RXEN3) | (0<<TXEN3) | (0<<UCSZ32) | (0<<RXB83) | (0<<TXB83);

// Analog Comparator initialization
// Analog Comparator: Off
// The Analog Comparator's positive input is
// connected to the AIN0 pin
// The Analog Comparator's negative input is
// connected to the AIN1 pin
ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIC) | (0<<ACIS1) | (0<<ACIS0);
ADCSRB=(0<<ACME);
// Digital input buffer on AIN0: On
// Digital input buffer on AIN1: On
DIDR1=(0<<AIN0D) | (0<<AIN1D);

// ADC initialization
// ADC disabled
ADCSRA=(0<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0);

// SPI initialization
// SPI disabled
SPCR=(0<<SPIE) | (0<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (0<<SPR1) | (0<<SPR0);

// TWI initialization
// TWI disabled
TWCR=(0<<TWEA) | (0<<TWSTA) | (0<<TWSTO) | (0<<TWEN) | (0<<TWIE);

// Alphanumeric LCD initialization
// Connections are specified in the
// Project|Configure|C Compiler|Libraries|Alphanumeric LCD menu:
// RS - PORTC Bit 0
// RD - PORTC Bit 1
// EN - PORTC Bit 2
// D4 - PORTC Bit 4
// D5 - PORTC Bit 5
// D6 - PORTC Bit 6
// D7 - PORTC Bit 7
// Characters/line: 8
//lcd_init(8);

// Globally enable interrupts
#asm("sei")

}

// While Loop for testing
/*while (1)
{		
	//Testing WRLATCH Output Function (Q13)
	//WriteLatch(testchar);
	//testchar++;				//Increment testchar to show bit update
	//delay_ms(4);				//Set the summation rate			
		
}*/
}
