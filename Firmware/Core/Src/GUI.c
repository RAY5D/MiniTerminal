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
	// This is still jsut a demo for display functionality
	static GBW_Coord_t Corner1_Old = {0, 0};
	static GBW_Coord_t Corner2_Old = {0, 0};
	static GBW_Coord_t Corner1_New = {0, 0};
	static GBW_Coord_t Corner2_New = {0, 0};
	static int32_t R_Old = 0;
	static int32_t R_New = 0;

	uint32_t BeatPeriod = 625;
	static uint32_t LastBeatTick = 0;
	uint32_t Tick = GBW_GetTick();
//	uint32_t BeatPhase1000th = (1000 * (GBW_GetTick() % BeatPeriod)) / BeatPeriod;

	if (Tick - LastBeatTick >= BeatPeriod)
	{
		LastBeatTick = Tick - Tick % BeatPeriod;

		Corner1_Old = Corner1_New;
		Corner2_Old = Corner2_New;

		Corner1_New = (GBW_Coord_t){rand() % (Instance->ResX * 2) - (Instance->ResX / 2), rand() % (Instance->ResY * 2) - (Instance->ResY / 2)};
		Corner2_New = (GBW_Coord_t){rand() % (Instance->ResX * 2) - (Instance->ResX / 2), rand() % (Instance->ResY * 2) - (Instance->ResY / 2)};

//		Corner1_New = (GBW_Coord_t){rand() % (Instance->ResX), rand() % (Instance->ResY)};
//		Corner2_New = (GBW_Coord_t){rand() % (Instance->ResX), rand() % (Instance->ResY)};

		R_Old = R_New;
		R_New = rand() % 16;
	}
	int32_t BeatPhase1000th = (1000 * (Tick - LastBeatTick)) / BeatPeriod;

	GBW_Coord_t Corner1_Now, Corner2_Now;
	Corner1_Now.X = (Corner1_Old.X) + ((BeatPhase1000th * (Corner1_New.X - Corner1_Old.X)) / 1000);
	Corner1_Now.Y = (Corner1_Old.Y) + ((BeatPhase1000th * (Corner1_New.Y - Corner1_Old.Y)) / 1000);
	Corner2_Now.X = (Corner2_Old.X) + ((BeatPhase1000th * (Corner2_New.X - Corner2_Old.X)) / 1000);
	Corner2_Now.Y = (Corner2_Old.Y) + ((BeatPhase1000th * (Corner2_New.Y - Corner2_Old.Y)) / 1000);
	int32_t R_Now = R_Old + (BeatPhase1000th * (R_New - R_Old) / 1000);

	GBW_Draw_Fill(Instance, GBW_Color_0);

//	for (uint32_t x = 0; x < 128; x++)
//	{
//		for (uint32_t y = 0; y < 64; y++)
//		{
////			GBW_Draw_Pixel_Fast(Instance, GBW_Color_0, x, y);
//			GBW_Draw_Pixel_Safe(Instance, GBW_Color_0, x, y);
//
//		}
//	}

//	for (int i = 0; i < 8; i++)
//	{
//		GBW_Draw_Circle_Safe(Instance, GBW_Color_1, (GBW_Coord_t){(i * 16 + BeatPhase1000th * Instance->ResX / 1000) % Instance->ResX, 16}, i);
//		GBW_Draw_Disk_Safe(Instance, GBW_Color_1, (GBW_Coord_t){(i * 16 + BeatPhase1000th * Instance->ResX / 1000) % Instance->ResX, 32}, i);
//	}

	GBW_Draw_Pixel_Safe(Instance, GBW_Color_1, Corner1_New.X, Corner1_New.Y);
	GBW_Draw_Pixel_Safe(Instance, GBW_Color_1, Corner2_New.X, Corner2_New.Y);


	GPIOB->BSRR = 1 << 8;///////
//	GBW_Draw_SlidRect_Safe(Instance, GBW_Color_0, (GBW_Coord_t){1, 1}, (GBW_Coord_t){126, 62});

//	GBW_Draw_RoundSlidRect_Safe(Instance, GBW_Color_1, Corner1_Now, Corner2_Now, R_Now);
	GBW_Draw_RoundHlowRect_Safe(Instance, GBW_Color_1, Corner1_Now, Corner2_Now, R_Now);

	GPIOB->BRR = 1 << 8;///////

}
