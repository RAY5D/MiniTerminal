#include "../inc/SSD1309_Specifics.h"

extern SPI_HandleTypeDef SSD1309_SPI;
extern TIM_HandleTypeDef SSD1309_TIM;

void SSD1309_InitPlatform()
{
	HAL_TIM_Base_Start_IT(&SSD1309_TIM);
}

void SSD1309_WritePin(SSD1309_Pin_t Pin, SSD1309_PinState_t State)
{
	if (State == SSD1309_PinState_0)
	{
		switch (Pin)
		{
			case SSD1309_Pin_PWR:
				SSD1309_PWR_GPIO_Port->BRR = SSD1309_PWR_Pin;
				return;
			case SSD1309_Pin_RES:
				SSD1309_RES_GPIO_Port->BRR = SSD1309_RES_Pin;
				return;
			case SSD1309_Pin_CS:
				SSD1309_CS_GPIO_Port->BRR = SSD1309_CS_Pin;
				return;
			case SSD1309_Pin_DC:
				SSD1309_DC_GPIO_Port->BRR = SSD1309_DC_Pin;
				return;
			default:
				return;
		}
	}
	else
	{
		switch (Pin)
		{
			case SSD1309_Pin_PWR:
				SSD1309_PWR_GPIO_Port->BSRR = SSD1309_PWR_Pin;
				return;
			case SSD1309_Pin_RES:
				SSD1309_RES_GPIO_Port->BSRR = SSD1309_RES_Pin;
				return;
			case SSD1309_Pin_CS:
				SSD1309_CS_GPIO_Port->BSRR = SSD1309_CS_Pin;
				return;
			case SSD1309_Pin_DC:
				SSD1309_DC_GPIO_Port->BSRR = SSD1309_DC_Pin;
				return;
			default:
				return;
		}
	}
}

void SSD1309_PeriInt_Stop()
{
	(SSD1309_TIM.Instance)->DIER &= ~TIM_DIER_UIE;
}

void SSD1309_PeriInt_Start()
{
	(SSD1309_TIM.Instance)->DIER |= TIM_DIER_UIE;
}

SSD1309_LLReturn_t SSD1309_WriteCMD(uint8_t CMD)
{
	static uint8_t Buffer = 0;
	if (HAL_SPI_GetState(&SSD1309_SPI) != HAL_SPI_STATE_READY)
	{
		return SSD1309_LLReturn_BUSY;
	}
	else
	{
		Buffer = CMD;
		SSD1309_WritePin(SSD1309_Pin_CS, SSD1309_PinState_0);
		SSD1309_WritePin(SSD1309_Pin_DC, SSD1309_PinState_0);
		if (HAL_SPI_Transmit_IT(&SSD1309_SPI, &Buffer, 1) != HAL_OK)
		{
			Error_Handler();
			return SSD1309_LLReturn_BUSY;
		}
		else
		{
			return SSD1309_LLReturn_OK;
		}
	}
}

SSD1309_LLReturn_t SSD1309_WriteDataDMA(uint8_t* Buffer, uint32_t Count)
{
	if (HAL_SPI_GetState(&SSD1309_SPI) != HAL_SPI_STATE_READY)
	{
		return SSD1309_LLReturn_BUSY;
	}
	else
	{
		SSD1309_WritePin(SSD1309_Pin_CS, SSD1309_PinState_0);
		SSD1309_WritePin(SSD1309_Pin_DC, SSD1309_PinState_1);
		if (HAL_SPI_Transmit_DMA(&SSD1309_SPI, Buffer, Count) != HAL_OK)
		{
			Error_Handler();
			return SSD1309_LLReturn_BUSY;
		}
		else
		{
			return SSD1309_LLReturn_OK;
		}
	}
}
