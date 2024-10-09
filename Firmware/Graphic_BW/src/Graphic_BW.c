#include "Graphic_BW.h"

static GBW_Tick_t GBW_Tick = 0;

// Initializes an instance, call after declaring the struct
void GBW_Init
(
	GBW_Instance_t* Instance,
	uint32_t ResX,
	uint32_t ResY,
	uint8_t* FrameBuffer,
	uint32_t FrameBuffer_Len,
	uint32_t MinFramePeriod,
	void (*DrawFunc)(GBW_Instance_t*),
	uint32_t (*SendFunc)(uint8_t*, uint32_t),
	uint32_t (*CheckFunc)(void)
)
{
	int CheckOK = 1;

	Instance->ResX = ResX;
	Instance->ResY = ResY;

	if (FrameBuffer_Len != ResX * ResY / GBW_PixelPerData)
	{
		CheckOK = 0;
	}

	Instance->FrameBuffer = FrameBuffer;
	Instance->FrameBuffer_Len = FrameBuffer_Len;
	Instance->MinFramePeriod = MinFramePeriod;
	Instance->LastFrameTick = 0;
	Instance->GUIDrawCB = DrawFunc;
	Instance->DriverFlushCB = SendFunc;
	Instance->DriverCheckIdleCB = CheckFunc;

	if (CheckOK)
	{
		Instance->State = GBW_State_Ready;
	}
	else
	{
		Instance->State = GBW_State_Reset;
	}
}

// Handles redraw and send, call in main loop
void GBW_LoopHandler(GBW_Instance_t* Instance)
{
	switch(Instance->State)
	{
		case GBW_State_Reset:
			break;
		case GBW_State_Ready:
			GBW_Tick_t CrntTick = GBW_GetTick();
			if (CrntTick - Instance->MinFramePeriod >= Instance->LastFrameTick)
			{
				// New frame needed
				Instance->LastFrameTick = CrntTick;
				Instance->State = GBW_State_Drawing;
				Instance->GUIDrawCB(Instance);
			}
			break;
		case GBW_State_Drawing:
			if (Instance->DriverFlushCB(Instance->FrameBuffer, Instance->FrameBuffer_Len) == 0)
			{
				Instance->DriverFlushCB(Instance->FrameBuffer, Instance->FrameBuffer_Len);
				Instance->State = GBW_State_Sending;
			}
			break;
		case GBW_State_Sending:
			if (Instance->DriverCheckIdleCB() == 0)
			{
				Instance->State = GBW_State_Ready;
			}
			break;
		default:
			break;
	}
}

// Increments GBW tick, call in systick ISR
void GBW_TickInc()
{
	GBW_Tick ++;
}

// Get GBW tick, used for frame rate, GUI can use this function as well
GBW_Tick_t GBW_GetTick()
{
	return GBW_Tick;
}

// Graphic function, fills the entire frame buffer to a color
void GBW_Fill(GBW_Instance_t* Instance, GBW_Color_t Color)
{
	int32_t i_Max = (Instance->ResX * Instance->ResY / 8);

	if (Color)
	{
		for (int32_t i = 0; i < i_Max; i++)
		{
			Instance->FrameBuffer[i] = 0xff;
		}
	}
	else
	{
		for (int32_t i = 0; i < i_Max; i++)
		{
			Instance->FrameBuffer[i] = 0x00;
		}
	}
}

// Graphic function, draws a solid rectangle
void GBW_Draw_SlidRect_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y1, int32_t Y2)
{
	uint8_t TopByte = 0xff << (Y1 % 8);		// Top page
	uint8_t BtmByte = 0xff >> (7 - Y2 % 8); // Bottom page
	uint8_t AllByte = TopByte & BtmByte;    // For rectangles in one page
	int32_t TopY = Y1 / 8;
	int32_t BtmY = Y2 / 8;

	if (Color)
	{
		if (BtmY - TopY >= 2) // Across >= 3 pages
		{
			for (int32_t X = X1; X <= X2; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] |= TopByte;
				for (int32_t Y = TopY + 1; Y < BtmY; Y++)
				{
					Instance->FrameBuffer[Y * Instance->ResX + X] = 0xff;
				}
				Instance->FrameBuffer[BtmY * Instance->ResX + X] |= BtmByte;
			}
		}
		else if (BtmY - TopY >= 1) // Across >= 2 pages
		{
			for (int32_t X = X1; X <= X2; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] |= TopByte;
				Instance->FrameBuffer[BtmY * Instance->ResX + X] |= BtmByte;
			}
		}
		else // In 1 page
		{
			for (int32_t X = X1; X <= X2; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] |= AllByte;
			}
		}
	}
	else
	{
		TopByte = ~TopByte;
		BtmByte = ~BtmByte;
		AllByte = ~AllByte;

		if (BtmY - TopY >= 2) // Across >= 3 pages
		{
			for (int32_t X = X1; X <= X2; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] &= TopByte;
				for (int32_t Y = TopY + 1; Y < BtmY; Y++)
				{
					Instance->FrameBuffer[Y * Instance->ResX + X] = 0x00;
				}
				Instance->FrameBuffer[BtmY * Instance->ResX + X] &= BtmByte;
			}
		}
		else if (BtmY - TopY >= 1) // Across >= 2 pages
		{
			for (int32_t X = X1; X <= X2; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] &= TopByte;
				Instance->FrameBuffer[BtmY * Instance->ResX + X] &= BtmByte;
			}
		}
		else // In 1 page
		{
			for (int32_t X = X1; X <= X2; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] &= AllByte;
			}
		}
	}
}

// Graphic function, draws a solid rectangle
void GBW_Draw_SlidRect_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y1, int32_t Y2)
{
	int32_t SX1, SX2, SY1, SY2;

	if (X1 <= X2) {SX1 = X1; SX2 = X2;} else {SX1 = X2; SX2 = X1;}
	if (Y1 <= Y2) {SY1 = Y1; SY2 = Y2;} else {SY1 = Y2; SY2 = Y1;}

	if (SX2 < 0 || SX1 >= (int32_t)Instance->ResX || SY2 < 0 || SY1 >= (int32_t)Instance->ResY) {return;} // Entirely off-screen

	if (SX1 < 0) {SX1 = 0;}
	if (SY1 < 0) {SY1 = 0;}
	if (SX2 >= (int32_t)Instance->ResX) {SX2 = (int32_t)Instance->ResX - 1;}
	if (SY2 >= (int32_t)Instance->ResY) {SY2 = (int32_t)Instance->ResY - 1;}

	GBW_Draw_SlidRect_Fast(Instance, Color, SX1, SX2, SY1, SY2);
}

//void GBW_Draw_HLine_Fast(uint8_t* Buffer, uint32_t Color, int32_t X1, int32_t X2, int32_t Y)
//{
//	int32_t Buffer_ResX = *(((int32_t*) Buffer)    );
//	uint8_t* Buffer_Data = (uint8_t*)(Buffer + GBW_BufferMetaBytes);
//
//	uint8_t MaskByte = 0x01 << (Y % 8);
//	Y /= 8;
//
//	if (Color)
//	{
//		for (int32_t X = X1; X <= X2; X++)
//		{
//			Buffer_Data[Y * Buffer_ResX + X] |= MaskByte;
//		}
//	}
//	else
//	{
//		MaskByte = ~MaskByte;
//		for (int32_t X = X1; X <= X2; X++)
//		{
//			Buffer_Data[Y * Buffer_ResX + X] &= MaskByte;
//		}
//	}
//}
//
//void GBW_Draw_HLine_Safe(uint8_t* Buffer, uint32_t Color, int32_t X1, int32_t X2, int32_t Y)
//{
//	int32_t Buffer_ResX = *(((int32_t*) Buffer)    );
//	int32_t Buffer_ResY = *(((int32_t*) Buffer) + 1);
//
//	int32_t SX1, SX2;
//
//	if (X1 <= X2) {SX1 = X1; SX2 = X2;} else {SX1 = X2; SX2 = X1;}
//
//	if (SX2 < 0 || SX1 >= (int32_t)Buffer_ResX || Y < 0 || Y >= (int32_t)Buffer_ResY) {return;} // Entirely off-screen
//
//	if (SX1 < 0) {SX1 = 0;}
//	if (SX2 >= Buffer_ResX) {SX2 = Buffer_ResX - 1;}
//
//	GBW_Draw_HLine_Fast(Buffer, Color, SX1, SX2, Y);
//}
//
//void GBW_Draw_VLine_Fast(uint8_t* Buffer, uint32_t Color, int32_t X, int32_t Y1, int32_t Y2)
//{
//	int32_t Buffer_ResX = *(((int32_t*) Buffer)    );
//	uint8_t* Buffer_Data = (uint8_t*)(Buffer + GBW_BufferMetaBytes);
//
//	uint8_t TopByte = 0xff << (Y1 % 8);		// Top page
//	uint8_t BtmByte = 0xff >> (7 - Y2 % 8); // Bottom page
//	uint8_t AllByte = TopByte & BtmByte;    // For rectangles in one page
//	int32_t TopY = Y1 / 8;
//	int32_t BtmY = Y2 / 8;
//
//	if (Color)
//	{
//		if (BtmY - TopY >= 2) // Across >= 3 pages
//		{
//			Buffer_Data[TopY * Buffer_ResX + X] |= TopByte;
//			for (int32_t Y = TopY + 1; Y < BtmY; Y++)
//			{
//				Buffer_Data[Y * Buffer_ResX + X] = 0xff;
//			}
//			Buffer_Data[BtmY * Buffer_ResX + X] |= BtmByte;
//		}
//		else if (BtmY - TopY >= 1) // Across >= 2 pages
//		{
//			Buffer_Data[TopY * Buffer_ResX + X] |= TopByte;
//			Buffer_Data[BtmY * Buffer_ResX + X] |= BtmByte;
//		}
//		else // In 1 page
//		{
//			Buffer_Data[TopY * Buffer_ResX + X] |= AllByte;
//		}
//	}
//	else
//	{
//		TopByte = ~TopByte;
//		BtmByte = ~BtmByte;
//		AllByte = ~AllByte;
//
//		if (BtmY - TopY >= 2) // Across >= 3 pages
//		{
//			Buffer_Data[TopY * Buffer_ResX + X] &= TopByte;
//			for (int32_t Y = TopY + 1; Y < BtmY; Y++)
//			{
//				Buffer_Data[Y * Buffer_ResX + X] = 0x00;
//			}
//			Buffer_Data[BtmY * Buffer_ResX + X] &= BtmByte;
//		}
//		else if (BtmY - TopY >= 1) // Across >= 2 pages
//		{
//			Buffer_Data[TopY * Buffer_ResX + X] &= TopByte;
//			Buffer_Data[BtmY * Buffer_ResX + X] &= BtmByte;
//		}
//		else // In 1 page
//		{
//			Buffer_Data[TopY * Buffer_ResX + X] &= AllByte;
//		}
//	}
//}
//
//void GBW_Draw_VLine_Safe(uint8_t* Buffer, uint32_t Color, int32_t X, int32_t Y1, int32_t Y2)
//{
//	int32_t Buffer_ResX = *(((int32_t*) Buffer)    );
//	int32_t Buffer_ResY = *(((int32_t*) Buffer) + 1);
//
//	int32_t SY1, SY2;
//
//	if (Y1 <= Y2) {SY1 = Y1; SY2 = Y2;} else {SY1 = Y2; SY2 = Y1;}
//
//	if (SY2 < 0 || SY1 >= (int32_t)Buffer_ResY || X < 0 || X >= (int32_t)Buffer_ResX) {return;} // Entirely off-screen
//
//	if (SY1 < 0) {SY1 = 0;}
//	if (SY2 >= Buffer_ResY) {SY2 = Buffer_ResY - 1;}
//
//	GBW_Draw_VLine_Fast(Buffer, Color, X, SY1, SY2);
//}

//void GBW_Draw_Circle_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, int32_t X, int32_t Y, int32_t R)
//{
//	int32_t O_X = X;
//	int32_t O_Y = Y;
//	if (R < 0) {R = -R;} // Negative R
//	if (R == 0) {return;} // R = 0
//	if (R == 1) // Special case R = 1
//	{
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X, O_Y);
//		return;
//	}
//
//	if (O_X + (R - 1) < 0) {return;} // Off-screen
//	if (O_X - (R - 1) >= (int32_t)Instance->ResX) {return;} // Off-screen
//	if (O_Y + (R - 1) < 0) {return;} // Off-screen
//	if (O_Y - (R - 1) >= (int32_t)Instance->ResY) {return;} // Off-screen
//
//	int32_t LoopY = R - 1;
//	int32_t RR = R * R;
//
//	for (int LoopX = 0; LoopX < LoopY; LoopX++)
//	{
//		if (LoopX * LoopX + LoopY * LoopY >= RR)
//		{
//			LoopY -= 1;
//		}
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopX, O_Y + LoopY);
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopX, O_Y + LoopY);
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopX, O_Y - LoopY);
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopX, O_Y - LoopY);
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopY, O_Y + LoopX);
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopY, O_Y + LoopX);
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopY, O_Y - LoopX);
//		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopY, O_Y - LoopX);
//	}
//}

// Graphic function, draws circle based on Bresenham’s algorithm
void GBW_Draw_Circle_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, int32_t X, int32_t Y, int32_t R)
{
	int32_t O_X = X;
	int32_t O_Y = Y;
	if (R < 0) {R = -R;} // Negative R

	if (O_X + R < 0) {return;} // Off-screen
	if (O_X - R >= (int32_t)Instance->ResX) {return;} // Off-screen
	if (O_Y + R < 0) {return;} // Off-screen
	if (O_Y - R >= (int32_t)Instance->ResY) {return;} // Off-screen

	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

	// Draw pixels for X_0
	GBW_Draw_Pixel_Safe(Instance, Color, O_X, O_Y + LoopY);
	GBW_Draw_Pixel_Safe(Instance, Color, O_X, O_Y - LoopY);
	GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopY, O_Y);
	GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopY, O_Y);

	while (LoopX < LoopY) // No need for <=, LoopY lags 1 iteration, and is always decremented in the last iteration, which needs <= as smaller Y is stricter
	{
		// Check D for X_1, X_2, X_3... , the calculate D for X_2, X_3, X_4..., then draw X_1, X_2, X_3...
		// The order is a bit confusing, but D calculation always leads comparison and drawing by 1 iteration.
		// This is because the calculation can be simplified to just multiplication.
		if (D > 0) // D for X_1 and onward
		{
			LoopY--; // Y decision for X_1 and onward
			D = D + 4 * (LoopX - LoopY) + 10; // Calculate D for X_2 and onward based on D of X_1 and onward
		}
		else
		{
			D = D + 4 * LoopX + 6; // Calculate D for X_2 and onward based on D of X_1 and onward
		}

		LoopX ++; // For X_1 and onward

		// Draw the pixels using the new coordinates
		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopX, O_Y + LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopX, O_Y + LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopX, O_Y - LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopX, O_Y - LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopY, O_Y + LoopX);
		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopY, O_Y + LoopX);
		GBW_Draw_Pixel_Safe(Instance, Color, O_X + LoopY, O_Y - LoopX);
		GBW_Draw_Pixel_Safe(Instance, Color, O_X - LoopY, O_Y - LoopX);
	}
}
