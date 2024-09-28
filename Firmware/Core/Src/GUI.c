/*
 * GUI.c
 *
 *  Created on: Sep 27, 2024
 *      Author: RAY5D
 */

#include "GUI.h"

void GUI_Draw(GBW_Instance_t* Instance)
{
	GBW_Fill(Instance, GBW_Color_0);

	uint32_t temp = GBW_GetTick() % 1024;
	Instance->FrameBuffer[temp] = 0xff;////////

}
