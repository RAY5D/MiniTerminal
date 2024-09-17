/*
 * SSD1309 OLED Demo.cpp
 *
 * Created: 2022-10-08 Sat 06:31:43 PM
 * Author : RAY5D
 */ 

// Description:
//     Prepare for Micro Console firmware development
// HW Summary:
//     ATMEGA16U2 Dev Board, Rev 1, 3V3, 8 MHZ
//     4 Keys Module
//     OLED Module, SSD1309, 128 * 64

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL
#include <util/delay.h>

#include "FontW5H7.h"

#define Key_PIN PIND
#define Key_DDR DDRD
#define Key_0_P PORTD4
#define Key_1_P PORTD5
#define Key_2_P PORTD6
#define Key_3_P PORTD7

#define OLED_PORT PORTB
#define OLED_DDR DDRB
#define OLED_SS_P PORTB0 // Not really HW
#define OLED_SCLK_P PORTB1 // HW
#define OLED_MOSI_P PORTB2 // HW
//#define OLED_MISO_P PORTB3 // HW
#define OLED_RES_P PORTB4 // Reset
#define OLED_DC_P PORTB5 // Data / Command

#define DBG_PORT PORTB
#define DBG_DDR DDRB
#define DBG_1_P PORTB6
#define DBG_2_P PORTB7

//#define BELL_P PORTC6 // HW 

#define Console_W 21
#define Console_H 7

#define ScreenSaver_Timeout 60
#define Bell_Timeout 20

const uint8_t SSD1309_Startup_PROGMEM[] PROGMEM =
{
	0xFD, 0x12, // Set Command Lock
	0xAE, // Set Display Off
	0xD5, 0xA0, // Set Display Clock Divide Ratio/Oscillator Frequency
	0xA8, 0x3F, // Set Multiplex Ratio
	0xD3, 0x00, // Set Display Offset
	0x40, // Set Display Start Line
	0xA1, // Set Segment Re-Map
	0xC8, // Set COM Output Scan Direction
	0xDA, 0x12, // Set COM Pins Hardware Configuration
	0x81, 0xBF, // Set Contrast Control
	0xD9, 0x25, // Set Pre-Charge Period
	0xDB, 0x34, // Set VCOMH Deselect Level
	0xA4, // Set Entire Display On
	0xA6, // Set Normal Display
	0x20, 0x02, // Set Memory Addressing Mode (Added for Page Addressing Mode)
	0xAF //Set Display On
};

const uint8_t TermBuff_Len = Console_W * Console_H;
char TermBuff_Arr[TermBuff_Len] = {0};
	
volatile uint8_t* SSD1309_CrntByte = 0;
volatile uint8_t SSD1309_ByteLeft = 0;
volatile uint8_t SSD1309_ByteIncr = 0;

volatile uint8_t Cursor_Pos = 0;
volatile uint8_t TermBuff_Start = 0;
volatile uint8_t TermBuff_Used = 0;

volatile uint8_t Status = 0; // 0: OK

volatile uint_fast16_t SysTick_MS = 0; // MS tick updated by timer
volatile uint8_t ScreenSaver_Counter = ScreenSaver_Timeout; // Time till screensaver starts
volatile uint8_t Bell_Counter = 0; // Time till bell stops

void Init();

void SplashScreen();
void RenderStatusBar();
void RenderConsole();
void ScreenSaver_Start();
void ScreenSaver_Stop();

void AddCharToTerminal(char CharAdding);

void SSD1309_SendBegin(uint8_t Type, uint8_t* DataAddr, uint8_t Len, uint8_t Incr);
void SSD1309_SetColumnStartAddressForPageAddressingMode(uint8_t Column);
void SSD1309_SetPageStartAddressForPageAddressingMode(uint8_t Page);
void SSD1309_FillByte(uint8_t Byte, uint8_t Count);

ISR(SPI_STC_vect)
{
	if (SSD1309_ByteLeft)
	{
		SPDR = *SSD1309_CrntByte;
		SSD1309_CrntByte += SSD1309_ByteIncr;
		SSD1309_ByteLeft--;
	}
	else
	{
		SSD1309_CrntByte = 0;
	}
}

ISR(USART1_RX_vect)
{
	ScreenSaver_Counter = ScreenSaver_Timeout;
	if (UCSR1A & 0x10) //FE1
	{
		Status |= 0x01; // Frame Error
	}
	if (UCSR1A & 0x08) //DOR1
	{
		Status |= 0x02; // Overrun
	}
	if (UCSR1A & 0x04) //UPE1
	{
		Status |= 0x04; // Parity Error
	}
	AddCharToTerminal(UDR1);
}

ISR(TIMER0_OVF_vect)
{
	SysTick_MS++;
	if (SysTick_MS == 1000)
	{
		SysTick_MS = 0;
		if (ScreenSaver_Counter > 0)
		{
			ScreenSaver_Counter--;
		}
	}
	if (Bell_Counter > 0)
	{
		Bell_Counter--;
		if (Bell_Counter == 0)
		{
			TCCR1A &= ~(1 << COM1A1); // Disable OC1A
		}
	}
}

int main(void)
{
	uint8_t Loop = 0;
	Init();
	SplashScreen();
	
	while (1)
	{	
		if (ScreenSaver_Counter > 0)
		{
			RenderStatusBar();
			RenderConsole();
		}
		else
		{
			ScreenSaver_Start();
			while (ScreenSaver_Counter == 0);
			ScreenSaver_Stop();
		}
		
		if (Key_PIN & 1 << Key_0_P)
		{
			//Ctrl = (Ctrl + 1) % 0x20;//AddCharToTerminal('\t');
		}
		if (Key_PIN & 1 << Key_1_P)
		{
			//Ctrl = (Ctrl + 0x1F) % 0x20;
		}
		if (Key_PIN & 1 << Key_2_P)
		{
			//AddCharToTerminal(Ctrl);
		}
		if (Key_PIN & 1 << Key_3_P)
		{
			AddCharToTerminal(Loop % 26 + 'a');
		}
		Loop++;
	}
}

void Init() // Initialization
{
	uint8_t OLED_Startup_Len = sizeof(SSD1309_Startup_PROGMEM);
	uint8_t OLED_Startup_CMD[OLED_Startup_Len];
	
	for (uint8_t i = 0; i < OLED_Startup_Len; i++)
	{
		*(OLED_Startup_CMD + i) = pgm_read_byte_near(SSD1309_Startup_PROGMEM + i);
	}
	
	// IO
	// Keys
	Key_DDR &= ~((1 << Key_0_P) | (1 << Key_1_P) | (1 << Key_2_P) | (1 << Key_3_P));
	// DBG
	DBG_DDR |=  (1 << DBG_1_P);
	DBG_DDR |=  (1 << DBG_2_P);
	
	// OLED SPI
	OLED_DDR |=  (1 << OLED_SS_P);
	// OLED_DDR &= ~(1 << OLED_MISO_P);
	OLED_DDR |=  (1 << OLED_SCLK_P);
	OLED_DDR |=  (1 << OLED_MOSI_P);
	OLED_DDR |=  (1 << OLED_RES_P);
	OLED_DDR |=  (1 << OLED_DC_P);
	
	OLED_PORT &= ~(1 << OLED_RES_P); // Reset OLED
	_delay_us(5);
	OLED_PORT |=  (1 << OLED_RES_P);
	
	OLED_PORT &= ~(1 << OLED_SS_P); // Select OLED
	
	// SPI
	SPCR |=  (1 << MSTR); // Master mode
	SPCR &= ~((1 << SPR1) | (1 << SPR0)); // Set speed
	SPSR |=  (1 << SPI2X); // Set speed
	SPCR |=  (1 << SPE); // Enable SPI
	
	// UART
	//UBRR1H = 0; // 250K baud
	//UBRR1L = 1; // 250K baud
	UBRR1H = 0; // 38400 baud
	UBRR1L = 12; // 38400 baud
	UCSR1B |= (1 << RXEN1); // Enable receiver
	
	// Timer 0 (System Tick)
	TCCR0A |= (1 << WGM01 | 1 << WGM00); // Fast PWM Mode, WGM = 111
	TCCR0B |= (1 << WGM02);
	OCR0A = 124; // Trigger when counting up to this
	TCCR0B |= (1 << CS01 | 1 << CS00); // Set 1/64 divider and run
	
	// Timer 1 (Bell)
	DDRC |= 1 << DDC6;
	OCR1AH = 0x00; // Duty
	OCR1AL = 0x7f;
	ICR1H = 0x07; // Freq 2.1K
	ICR1L = 0x71;
	TCCR1B = (1 << WGM13); // Freq and phase correct PWM
	//TCCR1A = (1 << COM1A1); // Enable OC1A
	TCCR1B |= 1 << CS10; // Set clock source and run


	// Interrupt
	UCSR1B |= (1 << RXCIE1); // Enable RX Interrupt
	SPCR |=  (1 << SPIE); // Enable SPI interrupt
	TIMSK0 |= (1 << TOIE0); // Enable Timer 0 Interrupt
	sei(); // Enable interrupt
	
	// OLED startup sequence
	SSD1309_SendBegin(0, OLED_Startup_CMD, OLED_Startup_Len, 1);
}

void SplashScreen()
{
	const uint8_t* Font = fontW5H7;
	const uint8_t Font_HeaderLen = pgm_read_byte_near(Font);
	const uint8_t Font_W = pgm_read_byte_near(Font + 1);
	uint8_t Char_BMP[Font_W + 1];
	
	uint8_t CMD[2];
	CMD[0] = 0x81;
	CMD[1] = 0x01;
	SSD1309_SendBegin(0, CMD, 2, 1);

	for (uint8_t Page = 0; Page <= 1; Page++)
	{
		SSD1309_SetPageStartAddressForPageAddressingMode(Page);
		SSD1309_SetColumnStartAddressForPageAddressingMode(0);
		SSD1309_FillByte(0x00, 128);
	}

	SSD1309_SetPageStartAddressForPageAddressingMode(2);
	SSD1309_SetColumnStartAddressForPageAddressingMode(0);
	SSD1309_FillByte(0x00, (Font_W + 1) * 4 + 1); // Pad 1 col for GUI design
	
	char StrRendering[14] = "Mini Terminal";
	char* CharRendering = StrRendering;
	while (*CharRendering)
	{
		for (uint8_t Line = 0; Line < Font_W; Line++)
		{
			Char_BMP[Line] = pgm_read_byte_near(Font + Font_HeaderLen + (*CharRendering) * Font_W + Line);
		}
		Char_BMP[Font_W] = 0x00;
		SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
		CharRendering++;
	}
	SSD1309_FillByte(0x00, (Font_W + 1) * 4 + 1);

	SSD1309_SetPageStartAddressForPageAddressingMode(3);
	SSD1309_SetColumnStartAddressForPageAddressingMode(0);
	SSD1309_FillByte(0x00, 128);

	SSD1309_SetPageStartAddressForPageAddressingMode(4);
	SSD1309_SetColumnStartAddressForPageAddressingMode(0);
	SSD1309_FillByte(0x00, (Font_W + 1) * 9 + 1 + 3);

	StrRendering[0] = 'V';
	StrRendering[1] = '0';
	StrRendering[2] = '\0';
	CharRendering = StrRendering;
	while (*CharRendering)
	{
		for (uint8_t Line = 0; Line < Font_W; Line++)
		{
			Char_BMP[Line] = pgm_read_byte_near(Font + Font_HeaderLen + (*CharRendering) * Font_W + Line);
		}
		Char_BMP[Font_W] = 0x00;
		SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
		CharRendering++;
	}
	SSD1309_FillByte(0x00, (Font_W + 1) * 10 + 1 - 3);
	
	for (uint8_t Page = 5; Page <= 7; Page++)
	{
		SSD1309_SetPageStartAddressForPageAddressingMode(Page);
		SSD1309_SetColumnStartAddressForPageAddressingMode(0);
		SSD1309_FillByte(0x00, 128);
	}
	
	for (uint8_t i = 0x01; i <= 0xBF; i++)
	{
		CMD[1] = i;
		SSD1309_SendBegin(0, CMD, 2, 1);
		_delay_ms(3);
	}
	
	_delay_ms(1427);
}

void RenderStatusBar()
{
	const uint8_t* Font = fontW5H7;
	const uint8_t Font_HeaderLen = pgm_read_byte_near(Font);
	const uint8_t Font_W = pgm_read_byte_near(Font + 1);
	uint8_t Char_BMP[Font_W + 1];
		
	SSD1309_SetPageStartAddressForPageAddressingMode(0);
	SSD1309_SetColumnStartAddressForPageAddressingMode(0);
	
	SSD1309_FillByte(0xff, 1); // Pad 1 col for GUI design
	
	// Planned bar format (21 chars)
	// 012345678901234567890
	// 115200 8N1 SSS UUU RN
	//  38400 9N1 SSS UUU OR
	
	char StrRendering[7] = " 38400";
	char* CharRendering = StrRendering;
	while (*CharRendering)
	{
		for (uint8_t Line = 0; Line < Font_W; Line++)
		{
			Char_BMP[Line] = ~pgm_read_byte_near(Font + Font_HeaderLen + (*CharRendering) * Font_W + Line);
		}
		Char_BMP[Font_W] = 0xff;
		SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
		CharRendering++;
	}
	SSD1309_FillByte(0xff, Font_W + 1);

	StrRendering[0] = '8';
	StrRendering[1] = 'N';
	StrRendering[2] = '1';
	StrRendering[3] = '\0'; /// adjust according to UART!
	CharRendering = StrRendering;
	while (*CharRendering)
	{
		for (uint8_t Line = 0; Line < Font_W; Line++)
		{
			Char_BMP[Line] = ~pgm_read_byte_near(Font + Font_HeaderLen + (*CharRendering) * Font_W + Line);
		}
		Char_BMP[Font_W] = 0xff;
		SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
		CharRendering++;
	}
	SSD1309_FillByte(0xff, Font_W + 1);
		
	StrRendering[0] = (TermBuff_Start / 100)      + '0';
	StrRendering[1] = (TermBuff_Start /  10) % 10 + '0';
	StrRendering[2] = (TermBuff_Start      ) % 10 + '0';
	StrRendering[3] = '\0';
	CharRendering = StrRendering;
	while (*CharRendering)
	{
		for (uint8_t Line = 0; Line < Font_W; Line++)
		{
			Char_BMP[Line] = ~pgm_read_byte_near(Font + Font_HeaderLen + (*CharRendering) * Font_W + Line);
		}
		Char_BMP[Font_W] = 0xff;
		SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
		CharRendering++;
	}
	SSD1309_FillByte(0xff, Font_W + 1);

	StrRendering[0] = (TermBuff_Used / 100)      + '0';
	StrRendering[1] = (TermBuff_Used /  10) % 10 + '0';
	StrRendering[2] = (TermBuff_Used      ) % 10 + '0';
	StrRendering[3] = '\0';
	CharRendering = StrRendering;
	while (*CharRendering)
	{
		for (uint8_t Line = 0; Line < Font_W; Line++)
		{
			Char_BMP[Line] = ~pgm_read_byte_near(Font + Font_HeaderLen + (*CharRendering) * Font_W + Line);
		}
		Char_BMP[Font_W] = 0xff;
		SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
		CharRendering++;
	}
	SSD1309_FillByte(0xff, Font_W + 1);
		
	if (Status == 0)
	{
		StrRendering[0] = 'O';
		StrRendering[1] = 'K';
		StrRendering[2] = '\0';
	}
	else
	{
		StrRendering[0] = 'E';
		if (Status < 10)
		{
			StrRendering[1] = Status + '0';
		}
		else if (Status < 16)
		{
			StrRendering[1] = Status - 10 + 'A';
		}
		else
		{
			StrRendering[1] = '!';
		}
		StrRendering[2] = '\0';
	}
	CharRendering = StrRendering;
	while (*CharRendering)
	{
		for (uint8_t Line = 0; Line < Font_W; Line++)
		{
			Char_BMP[Line] = ~pgm_read_byte_near(Font + Font_HeaderLen + (*CharRendering) * Font_W + Line);
		}
		Char_BMP[Font_W] = 0xff;
		SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
		CharRendering++;
	}
	SSD1309_FillByte(0x00, 1);
}

void RenderConsole()
{
	const uint8_t Font_HeaderLen = pgm_read_byte_near(fontW5H7);
	const uint8_t Font_W = pgm_read_byte_near(fontW5H7 + 1);

	uint8_t RenderCursor = (SysTick_MS < 250 || (SysTick_MS >= 500 && SysTick_MS < 750)) ? 1 : 0;

	uint8_t Row = 1;
	uint8_t Col = 0;
	
	for (Row = 0; Row < Console_H; Row++)
	{
		SSD1309_SetPageStartAddressForPageAddressingMode(Row + 1);
		SSD1309_SetColumnStartAddressForPageAddressingMode(0);
		
		SSD1309_FillByte(0x00, 1); // Pad 1 col for GUI design
		
		for (Col = 0; Col < Console_W; Col++) // Each char
		{
			if (Row * Console_W + Col >= TermBuff_Used) // Unfilled region
			{
				uint8_t Char_BMP[Font_W + 1];
				for (uint8_t Line = 0; Line <= Font_W; Line++)
				{
					Char_BMP[Line] = 0x00;
				}
				if (Row * Console_W + Col == Cursor_Pos && RenderCursor) // Cursor
				{
					for (uint8_t Line = 0; Line < Font_W; Line++)
					{
						Char_BMP[Line] |= 0x80;
					}
				}
				SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
			}
			else // Filled region
			{
				char Char_Char = TermBuff_Arr[(TermBuff_Start + Row * Console_W + Col) % TermBuff_Len];
				
				uint8_t Char_BMP[Font_W + 1];
				for (uint8_t Line = 0; Line < Font_W; Line++)
				{
					Char_BMP[Line] = pgm_read_byte_near(fontW5H7 + Font_HeaderLen + Char_Char * Font_W + Line);
				}
				Char_BMP[Font_W] = 0;
				if (Row * Console_W + Col == Cursor_Pos && RenderCursor) // Cursor
				{
					for (uint8_t Line = 0; Line < Font_W; Line++)
					{
						Char_BMP[Line] |= 0x80;
					}
				}
				SSD1309_SendBegin(1, Char_BMP, Font_W + 1, 1);
			}
		}
		SSD1309_FillByte(0x00, 1);
	}
}

void ScreenSaver_Start()
{
	SSD1309_SetPageStartAddressForPageAddressingMode(0);
	SSD1309_SetColumnStartAddressForPageAddressingMode(0);
	SSD1309_FillByte(0x01, 128);
	
	for (uint8_t Page = 1; Page < 8; Page++)
	{
		SSD1309_SetPageStartAddressForPageAddressingMode(Page);
		SSD1309_SetColumnStartAddressForPageAddressingMode(0);
		SSD1309_FillByte(0x00, 128);
	}
	
	uint8_t CMD[8];
	
	CMD[0] = 0x81;
	CMD[1] = 0x01;
	SSD1309_SendBegin(0, CMD, 2, 1);
	
	CMD[0] = 0x2A;
	CMD[1] = 0x00;
	CMD[2] = 0x00;
	CMD[3] = 0x04;
	CMD[4] = 0x07;
	CMD[5] = 0x3F;
	CMD[6] = 0x00;
	CMD[7] = 0x7F;
	SSD1309_SendBegin(0, CMD, 8, 1);
	
	CMD[0] = 0x2F;
	SSD1309_SendBegin(0, CMD, 8, 1);
}

void ScreenSaver_Stop()
{
	uint8_t CMD[2];
	
	CMD[0] = 0x2E;
	SSD1309_SendBegin(0, CMD, 1, 1);
	
	CMD[0] = 0x81;
	CMD[1] = 0xBF;
	SSD1309_SendBegin(0, CMD, 2, 1);
	
	CMD[0] = 0xD3;
	CMD[1] = 0x00;
	SSD1309_SendBegin(0, CMD, 2, 1);
	
	CMD[0] = 0x40;
	SSD1309_SendBegin(0, CMD, 1, 1);
}

void AddCharToTerminal(char CharAdding)
{
	DBG_PORT |= (1 << DBG_1_P);//////
	UCSR1B &= ~(1 << RXCIE1); // Disable RX Interrupt

	// This function was almost too complex for its use, 
	// but a lot of consideration was put into optimizing 
	// for speed. Mod operations was kept to a minimum. 
	// Supported Control Characters
	//	Hex		Name	Key			Description
	//	0x00	NUL		Ctrl + @	Do nothing
	//	0x01			Ctrl + A	
	//	0x02			Ctrl + B	
	//	0x03			Ctrl + C	
	//	0x04			Ctrl + D	
	//	0x05			Ctrl + E	
	//	0x06			Ctrl + F	
	//	0x07	BEL		Ctrl + G	Bell
	//	0x08	BS		Ctrl + H	Backspace, move 1 left
	//	0x09	TAB		Ctrl + I	Horizontal Tab
	//	0x0A	LF		Ctrl + J	Line Feed, new line
	//	0x0B	VT		Ctrl + K	Vertical Tab, move 1 down
	//	0x0C	FF		Ctrl + L	Form Feed, clears page
	//	0x0D	CR		Ctrl + M	Carriage Return, move to start of line
	//	0x0E			Ctrl + N	
	//	0x0F			Ctrl + O	
	//	0x10			Ctrl + P	
	//	0x11			Ctrl + Q	
	//	0x12			Ctrl + R	
	//	0x13			Ctrl + S	
	//	0x14			Ctrl + T	
	//	0x15			Ctrl + U	
	//	0x16			Ctrl + V	
	//	0x17			Ctrl + W	
	//	0x18			Ctrl + X	
	//	0x19			Ctrl + Y	
	//	0x1A			Ctrl + Z	
	//	0x1B			Ctrl +  	
	//	0x1C			Ctrl +  	
	//	0x1D			Ctrl +  	
	//	0x1E			Ctrl +  	
	//	0x1F			Ctrl +  	
	
	if (CharAdding == '\0') // 0x00
	{
		
	}
	else if (CharAdding == 0x07) // 0x07
	{
		TCCR1A |= 1 << COM1A1; // Enable OC1A
		Bell_Counter = Bell_Timeout;
	}
	else if (CharAdding == 0x08) // 0x08
	{
		if (Cursor_Pos > 0)
		{
			Cursor_Pos--;
		}
	}
	else if (CharAdding == '\t') // 0x09
	{
		uint8_t TabSize = 4;
		uint8_t Cursor_Col = Cursor_Pos % Console_W;
		uint8_t Increment = TabSize - ((Cursor_Col + TabSize) % TabSize);
		if (Cursor_Col + Increment >= Console_W)
		{
			AddCharToTerminal('\n'); // Tab to next line, same as line feed
		}
		else
		{
			uint8_t Fill_Addr = ((uint_fast16_t)TermBuff_Start + (uint_fast16_t)Cursor_Pos) % (uint_fast16_t)TermBuff_Len; // Might overflow so 16 bits
			for (uint8_t i = Fill_Addr; i < Fill_Addr + Increment; i++)
			{
				if (Cursor_Pos >= TermBuff_Used) // Fill
				{
					TermBuff_Arr[i] = ' ';
					TermBuff_Used++;
				}
				Cursor_Pos++;
			}
		}
	}
	else if (CharAdding == '\n') // 0x0A
	{
		uint8_t Cursor_Col = Cursor_Pos % Console_W;
		uint8_t Increment = Console_W - Cursor_Col;
			
		if (Cursor_Pos + Increment > TermBuff_Used) // Fill needed
		{
			if (Cursor_Pos + Increment >= TermBuff_Len) // Scroll needed
			{
				Cursor_Pos -= Console_W;
				TermBuff_Start = (TermBuff_Start + Console_W) % TermBuff_Len;
				TermBuff_Used -= Console_W;
			}
			// Scroll not needed
			uint8_t Fill_Amnt = Cursor_Pos + Increment - TermBuff_Used;
			uint8_t Fill_Addr = (TermBuff_Start + TermBuff_Used) % TermBuff_Len;
			for (uint8_t i = 0; i < Fill_Amnt; i++)
			{
				TermBuff_Arr[Fill_Addr] = ' ';
				Fill_Addr++;
				if (Fill_Addr >= TermBuff_Len)
				{
					Fill_Addr -= TermBuff_Len;
				}
			}
			TermBuff_Used += Fill_Amnt;
			Cursor_Pos += Increment;
		}
		else // Fill not needed
		{
			Cursor_Pos += Increment;
		}
	}
	else if (CharAdding == 0x0B) // 0x0B
	{
		uint8_t Increment = Console_W;
		
		if (Cursor_Pos + Increment > TermBuff_Used) // Fill needed
		{
			if (Cursor_Pos + Increment >= TermBuff_Len) // Scroll needed
			{
				Cursor_Pos -= Console_W;
				TermBuff_Start = (TermBuff_Start + Console_W) % TermBuff_Len;
				TermBuff_Used -= Console_W;
			}
			// Scroll not needed
			uint8_t Fill_Amnt = Cursor_Pos + Increment - TermBuff_Used;
			uint8_t Fill_Addr = (TermBuff_Start + TermBuff_Used) % TermBuff_Len;
			for (uint8_t i = 0; i < Fill_Amnt; i++)
			{
				TermBuff_Arr[Fill_Addr] = ' ';
				Fill_Addr++;
				if (Fill_Addr >= TermBuff_Len)
				{
					Fill_Addr -= TermBuff_Len;
				}
			}
			TermBuff_Used += Fill_Amnt;
			Cursor_Pos += Increment;
		}
		else // Fill not needed
		{
			Cursor_Pos += Increment;
		}
	}
	else if (CharAdding == '\f') // 0x0C
	{
		Cursor_Pos = 0;
		TermBuff_Used = 0;
		TermBuff_Start = 0;
	}
	else if (CharAdding == '\r') // 0x0D
	{
		uint8_t Cursor_Col = Cursor_Pos % Console_W;
		Cursor_Pos -= Cursor_Col;
	}
	else // Other characters
	{
		if (CharAdding >= 128) // Not ASCII
		{
			CharAdding = 128;
		}
		TermBuff_Arr[(TermBuff_Start + Cursor_Pos) % TermBuff_Len] = CharAdding;
		if (Cursor_Pos < TermBuff_Used)
		{
			Cursor_Pos++;
		}
		else
		{
			TermBuff_Used++;
			Cursor_Pos++;
			if (Cursor_Pos >= TermBuff_Len)
			{
				Cursor_Pos -= Console_W;
				TermBuff_Start = (TermBuff_Start + Console_W) % TermBuff_Len;
				TermBuff_Used -= Console_W;
			}
		}
	}
	UCSR1B |= (1 << RXCIE1); // Enable RX Interrupt
	DBG_PORT &= ~(1 << DBG_1_P);///////
}

void SSD1309_SendBegin(uint8_t Type, uint8_t* DataAddr, uint8_t Len, uint8_t Incr)
{
	SSD1309_CrntByte = DataAddr + Incr;
	SSD1309_ByteLeft = Len - 1;
	SSD1309_ByteIncr = Incr;
	if (Type == 1)
	{
		OLED_PORT |=  (1 << OLED_DC_P); // Set data (1) / command (0)
	}
	else
	{
		OLED_PORT &= ~(1 << OLED_DC_P);
	}
	SPDR = *DataAddr;
	
	while (SSD1309_ByteLeft);
}

void SSD1309_SetColumnStartAddressForPageAddressingMode(uint8_t Column)
{
	uint8_t ColCMD[2];
	ColCMD[0] = Column & 0x0F;
	ColCMD[1] = 0x10 | (Column >> 4);
	SSD1309_SendBegin(0, ColCMD, 2, 1);
}

void SSD1309_SetPageStartAddressForPageAddressingMode(uint8_t Page)
{
	uint8_t PageCMD = 0xB0 | Page;
	SSD1309_SendBegin(0, &PageCMD, 1, 1);
}

void SSD1309_FillByte(uint8_t Byte, uint8_t Count)
{
	SSD1309_SendBegin(1, &Byte, Count, 0);
}