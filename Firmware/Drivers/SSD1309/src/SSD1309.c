#include "SSD1309.h"

static SSD1309_State_t SSD1309_State = SSD1309_State_POR_0;
static SSD1309_CMDQueue_t SSD1309_CMDQueue =
{
	{
		0xAE, // Turn off OLED panel
		0xD5, // Set display clock divide ratio/oscillator frequency
		0xF0, //     Mfr: 0x80
		0xA8, // Set multiplex ratio
		0x3F, //     Mfr: 1/64 0x3f
		0xD3, // Set display offset
		0x00, //     Mfr: No offset reset)
		0x40, // Set display start line
		0xA6, // Set normal display
		0xA4, // Disable entire display on
		0xA1, // Set segment re-map, Mfr: 127 to 0
		0xC8, // Set COM output scan direction, Mfr: Remapped
		0xDA, // Set COM pins hardware configuration
		0x12, //     Mfr: Alt COM pin config reset), Disable COM L/R remap reset)
		0x81, // Set contrast control
		0xBF, //     Mfr: 0xBF
		0xD9, // Set pre-charge period
		0x25, //     Mfr: 0x25, Reset: 0x22
		0xDB, // Set V_COMH deselect level
		0x34, //     Mfr: 0x34 reset)
		0x20, // Set memory addressing mode
		0x00, //     Mfr: X, RAY5D: Horizontal
		0x21, // Set column address
		0x00, //     Mfr: X, RAY5D: 0
		0x7F, //     Mfr: X, RAY5D: 127
		0x22, // Set page address
		0x00, //     Mfr: X, RAY5D: 0
		0x07, //     Mfr: X, RAY5D: 7
		0xAF  // Turn on OLED panel
	},
	0,
	29,
	29
};

static SSD1309_DataBuffer_t SSD1309_DataBuffer =
{
	NULL,
	0
};

// Exported Function Definition
void SSD1309_PeriodicHandler()
{
	if (SSD1309_State == SSD1309_State_POR_0)
	{
		SSD1309_WritePin(SSD1309_Pin_RES, SSD1309_PinState_0);
		SSD1309_WritePin(SSD1309_Pin_PWR, SSD1309_PinState_0);
		SSD1309_State = SSD1309_State_POR_1;
	}
	else if (SSD1309_State == SSD1309_State_POR_1)
	{
		SSD1309_WritePin(SSD1309_Pin_RES, SSD1309_PinState_1);
		SSD1309_WritePin(SSD1309_Pin_PWR, SSD1309_PinState_1);
		SSD1309_State = SSD1309_State_Idle;
	}
	else if (SSD1309_State == SSD1309_State_Idle)
	{
		if (SSD1309_CMDQueue.Used > 0)
		{
			uint8_t CMD = SSD1309_CMDQueue.Data[SSD1309_CMDQueue.Start];
			if (SSD1309_WriteCMD(CMD) == SSD1309_LLReturn_OK)
			{
				SSD1309_CMDQueue.Used --;
				SSD1309_CMDQueue.Start = (SSD1309_CMDQueue.Start + 1) % SSD1309_CMDQueueLen;
				SSD1309_State = SSD1309_State_CMD_Send;
			}
		}
		else if (SSD1309_DataBuffer.Size > 0)
		{
			if (SSD1309_WriteDataDMA(SSD1309_DataBuffer.Data, SSD1309_DataBuffer.Size) == SSD1309_LLReturn_OK)
			{
				SSD1309_DataBuffer.Data = NULL;
				SSD1309_DataBuffer.Size = 0;
				SSD1309_State = SSD1309_State_Flush_Send;
			}
		}
	}
}

void SSD1309_TsfrCpltHandler()
{
	if (SSD1309_State == SSD1309_State_CMD_Send)
	{
		SSD1309_WritePin(SSD1309_Pin_CS, SSD1309_PinState_1);
		SSD1309_State = SSD1309_State_Idle;
	}
	else if (SSD1309_State == SSD1309_State_Flush_Send)
	{
		SSD1309_WritePin(SSD1309_Pin_CS, SSD1309_PinState_1);
		SSD1309_State = SSD1309_State_Idle;
	}
	else // Other application sharing hardware
	{
		return;
	}
}

void SSD1309_Init()
{
	SSD1309_InitPlatform();
}

uint32_t SSD1309_CheckReady() /// change to Return_t and see how to implement in GBW where it does not include SSD1309.h
{
	if (SSD1309_DataBuffer.Size == 0 && SSD1309_CMDQueue.Used == 0 && SSD1309_State == SSD1309_State_Idle)
	{
		return SSD1309_Return_OK;
	}
	else
	{
		return SSD1309_Return_BUSY;
	}
}

uint32_t SSD1309_SendFrame(uint8_t* Data, uint32_t Size) /// change to Return_t and see how to implement in GBW where it does not include SSD1309.h
{
	if (SSD1309_CheckReady() != SSD1309_Return_OK)
	{
		return SSD1309_Return_BUSY;
	}
	else
	{
		SSD1309_QueueCMD((uint8_t[]){ 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07 }, 6);
		SSD1309_DataBuffer.Data = Data;
		SSD1309_DataBuffer.Size = Size;
		return SSD1309_Return_OK;
	}
}

SSD1309_Return_t SSD1309_QueueCMD(uint8_t* Data, uint32_t Size)
{
	if (Size <= SSD1309_CMDQueueLen - SSD1309_CMDQueue.Used) // CMD queued
	{
		SSD1309_PeriInt_Stop();

		for (uint32_t i = 0; i < Size; i ++)
		{
			SSD1309_CMDQueue.Data[SSD1309_CMDQueue.End] = *(Data + i);
			SSD1309_CMDQueue.End = (SSD1309_CMDQueue.End + 1) % SSD1309_CMDQueueLen;
		}
		SSD1309_CMDQueue.Used += Size;

		SSD1309_PeriInt_Start();
		return SSD1309_Return_OK;
	}
	else // Not enough space in queue
	{
		return SSD1309_Return_BUSY;
	}
}
