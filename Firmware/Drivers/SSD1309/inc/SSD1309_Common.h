/*
 * SSD1309_Common.h
 *
 *  Created on: Sep 28, 2024
 *      Author: RAY5D
 */

#ifndef SSD1309_INC_SSD1309_COMMON_H_
#define SSD1309_INC_SSD1309_COMMON_H_

#include <stdint.h>

typedef enum
{
	SSD1309_Return_OK = 0,
	SSD1309_Return_BUSY = 1
} SSD1309_Return_t;

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

#endif /* SSD1309_INC_SSD1309_COMMON_H_ */
