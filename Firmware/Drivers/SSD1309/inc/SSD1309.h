#ifndef SSD1309_H_
#define SSD1309_H_

// Include
#include <stdint.h>
#include "SSD1309_Specifics.h"

// Parameters
#ifndef SSD1309_Res_X
	#define SSD1309_Res_X (128)
#endif

#ifndef SSD1309_Res_Y
	#define SSD1309_Res_Y (64)
#endif

#define SSD1309_CMDQueueLen (32)

// Typedefs
typedef enum
{
	SSD1309_State_POR_0,
	SSD1309_State_POR_1,
	SSD1309_State_CMD_Send,
	SSD1309_State_Flush_Send,
	SSD1309_State_Idle
} SSD1309_State_t;

typedef enum
{
	SSD1309_Return_OK,
	SSD1309_Return_BUSY
} SSD1309_Return_t;

typedef struct
{
	uint8_t Data[SSD1309_CMDQueueLen];
	uint8_t Start;
	uint8_t End;
	uint8_t Used;
} SSD1309_CMDQueue_t;

typedef struct
{
	uint8_t* Data;
	uint32_t Size;
} SSD1309_DataBuffer_t;

// Exported Functions
void SSD1309_PeriodicHandler(); // Call in timer interrupt, Typ 10kHz @ 72MHz, can be faster
void SSD1309_TsfrCpltHandler(); // Call in HAL_SPI_TxCpltCallback()

void SSD1309_Init();
SSD1309_Return_t SSD1309_CheckReady(); // Check if the IC is completely idle
SSD1309_Return_t SSD1309_FlushData(uint8_t* Data, uint32_t Size); // mark frame buffer ready to flush
SSD1309_Return_t SSD1309_QueueCMD(uint8_t* Data, uint32_t Size); // Add config to queue

#endif /* SSD1309_H */
