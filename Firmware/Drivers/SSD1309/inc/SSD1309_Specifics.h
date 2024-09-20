#ifndef SSD1309_SPECIFICS_H_
#define SSD1309_SPECIFICS_H_

#include <stdint.h>
#include "main.h" // For HAL inclusion and HW peripheral definition

#define SSD1309_SPI hspi1 // SPI used for transfer, Project specific
#define SSD1309_TIM htim4 // Timer used for periodic handler, Project specific

#define SSD1309_ResX (128) // Project specific
#define SSD1309_ResY (64)  // Project specific

typedef enum
{
	SSD1309_LLReturn_OK = 0,
	SSD1309_LLReturn_BUSY = 1
} SSD1309_LLReturn_t;

typedef enum
{
	SSD1309_Pin_PWR = 0,
	SSD1309_Pin_RES = 1,
	SSD1309_Pin_CS  = 2,
	SSD1309_Pin_DC  = 3
} SSD1309_Pin_t;

typedef enum
{
	SSD1309_PinState_0 = 0,
	SSD1309_PinState_1 = 1
} SSD1309_PinState_t;

void SSD1309_InitPlatform();
void SSD1309_WritePin(SSD1309_Pin_t Pin, SSD1309_PinState_t State);
void SSD1309_PeriInt_Stop();
void SSD1309_PeriInt_Start();
SSD1309_LLReturn_t SSD1309_WriteCMD(uint8_t CMD);
SSD1309_LLReturn_t SSD1309_WriteDataDMA(uint8_t* Buffer, uint32_t Count);

#endif /* SSD1309_SPECIFICS_H_ */
