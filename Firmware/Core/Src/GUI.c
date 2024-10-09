/*
 * GUI.c
 *
 *  Created on: Sep 27, 2024
 *      Author: RAY5D
 */

#include "GUI.h"
#include "main.h"//////
// Main draw function of display, handles GUI
void GUI_Draw(GBW_Instance_t* Instance)
{
	GPIOB->BSRR = 1 << 8;///////
	static uint32_t BeatPeriod = 625;
	uint32_t BeatPhase1000th = (1000 * (GBW_GetTick() % BeatPeriod)) / BeatPeriod;

	GBW_Fill(Instance, GBW_Color_0);

//	for (uint32_t x = 0; x < 128; x++)
//	{
//		for (uint32_t y = 0; y < 64; y++)
//		{
////			GBW_Draw_Pixel_Fast(Instance, GBW_Color_0, x, y);
//			GBW_Draw_Pixel_Safe(Instance, GBW_Color_0, x, y);
//
//		}
//	}
//	GBW_Draw_Pixel_Fast(Instance, GBW_Color_0, x, y);
	// This is still jsut a demo for display functionality

	for (int i = 0; i < 8; i++)
	{
		GBW_Draw_Circle_Safe(Instance, GBW_Color_1, i * 16 + 8, 16, i);
	}

	GBW_Draw_Circle_Safe(Instance, GBW_Color_1, 64, 32, ((1000 - BeatPhase1000th) * 64 / 1000));
//	for (int i = 0; i < 64; i += 2)
//	{
//		GBW_Draw_Circle_Safe(Instance, GBW_Color_1, 0, 0, i);
//		GBW_Draw_Circle_Safe(Instance, GBW_Color_1, 127, 63, i + 1);
//
//	}
//	GBW_Draw_Pixel_Safe(Instance, GBW_Color_1, BeatPhase1000th * 128 / 1000, 63);

//	uint32_t Index = GBW_GetTick() % (Instance->FrameBuffer_Len);
//	Instance->FrameBuffer[Index] = 0x01;

	static int32_t X1 = 0;
	static int32_t Y1 = 0;
	static int32_t X2 = 128;
	static int32_t Y2 = 64;

	X1 += (rand() % 5) - 2;
	X2 += (rand() % 5) - 2;
	Y1 += (rand() % 5) - 2;
	Y2 += (rand() % 5) - 2;

	GBW_Draw_SlidRect_Safe(Instance, GBW_Color_1, X1, X2, Y1, Y2);

	GPIOB->BRR = 1 << 8;///////
}
