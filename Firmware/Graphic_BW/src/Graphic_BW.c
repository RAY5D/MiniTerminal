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
void GBW_Draw_Fill(GBW_Instance_t* Instance, GBW_Color_t Color)
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
void GBW_Draw_SlidRect_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2)
{
	uint8_t TopByte = 0xff << (Corner1.Y % 8);		// Top page
	uint8_t BtmByte = 0xff >> (7 - Corner2.Y % 8); // Bottom page
	uint8_t AllByte = TopByte & BtmByte;    // For rectangles in one page
	int32_t TopY = Corner1.Y / 8;
	int32_t BtmY = Corner2.Y / 8;

	if (Color)
	{
		if (BtmY - TopY >= 2) // Across >= 3 pages
		{
			int32_t IndexBase_Top = TopY * Instance->ResX;
			int32_t IndexBase_Btm = BtmY * Instance->ResX;
			for (int32_t X = Corner1.X; X <= Corner2.X; X++)
			{
				Instance->FrameBuffer[IndexBase_Top + X] |= TopByte;
				Instance->FrameBuffer[IndexBase_Btm + X] |= BtmByte;
			}

			for (int32_t Y = TopY + 1; Y < BtmY; Y++)
			{
				int32_t I_Max = Y * Instance->ResX + Corner2.X;
				for (int32_t Index = Y * Instance->ResX + Corner1.X; Index <= I_Max; Index++)
				{
					Instance->FrameBuffer[Index] = 0xff;
				}
			}
		}
		else if (BtmY - TopY >= 1) // Across >= 2 pages
		{
			for (int32_t X = Corner1.X; X <= Corner2.X; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] |= TopByte;
				Instance->FrameBuffer[BtmY * Instance->ResX + X] |= BtmByte;
			}
		}
		else // In 1 page
		{
			for (int32_t X = Corner1.X; X <= Corner2.X; X++)
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
			int32_t IndexBase_Top = TopY * Instance->ResX;
			int32_t IndexBase_Btm = BtmY * Instance->ResX;
			for (int32_t X = Corner1.X; X <= Corner2.X; X++)
			{
				Instance->FrameBuffer[IndexBase_Top + X] &= TopByte;
				Instance->FrameBuffer[IndexBase_Btm + X] &= BtmByte;
			}

			for (int32_t Y = TopY + 1; Y < BtmY; Y++)
			{
				int32_t I_Max = Y * Instance->ResX + Corner2.X;
				for (int32_t Index = Y * Instance->ResX + Corner1.X; Index <= I_Max; Index++)
				{
					Instance->FrameBuffer[Index] = 0x00;
				}
			}
		}
		else if (BtmY - TopY >= 1) // Across >= 2 pages
		{
			for (int32_t X = Corner1.X; X <= Corner2.X; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] &= TopByte;
				Instance->FrameBuffer[BtmY * Instance->ResX + X] &= BtmByte;
			}
		}
		else // In 1 page
		{
			for (int32_t X = Corner1.X; X <= Corner2.X; X++)
			{
				Instance->FrameBuffer[TopY * Instance->ResX + X] &= AllByte;
			}
		}
	}
}

// Graphic function, draws a solid rectangle
void GBW_Draw_SlidRect_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2)
{
	GBW_Coord_t Safe1, Safe2;

	if (Corner1.X <= Corner2.X) {Safe1.X = Corner1.X; Safe2.X = Corner2.X;} else {Safe1.X = Corner2.X; Safe2.X = Corner1.X;}
	if (Corner1.Y <= Corner2.Y) {Safe1.Y = Corner1.Y; Safe2.Y = Corner2.Y;} else {Safe1.Y = Corner2.Y; Safe2.Y = Corner1.Y;}

	if (Safe2.X < 0 || Safe1.X >= Instance->ResX || Safe2.Y < 0 || Safe1.Y >= Instance->ResY) {return;} // Entirely off-screen

	if (Safe1.X < 0) {Safe1.X = 0;}
	if (Safe1.Y < 0) {Safe1.Y = 0;}
	if (Safe2.X >= Instance->ResX) {Safe2.X = Instance->ResX - 1;}
	if (Safe2.Y >= Instance->ResY) {Safe2.Y = Instance->ResY - 1;}

	GBW_Draw_SlidRect_Fast(Instance, Color, Safe1, Safe2);
}

// Graphic function, draws a hollow rectangle
void GBW_Draw_HlowRect_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2)
{
	GBW_Draw_HLine_Fast(Instance, Color, Corner1.X, Corner2.X, Corner1.Y);
	GBW_Draw_HLine_Fast(Instance, Color, Corner1.X, Corner2.X, Corner2.Y);
	GBW_Draw_VLine_Fast(Instance, Color, Corner1.X, Corner1.Y, Corner2.Y);
	GBW_Draw_VLine_Fast(Instance, Color, Corner2.X, Corner1.Y, Corner2.Y);
}

// Graphic function, draws a hollow rectangle
void GBW_Draw_HlowRect_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2)
{
	GBW_Coord_t Safe1 = {(Corner1.X <= Corner2.X ? Corner1.X : Corner2.X), (Corner1.Y <= Corner2.Y ? Corner1.Y : Corner2.Y)};
	GBW_Coord_t Safe2 = {(Corner1.X <= Corner2.X ? Corner2.X : Corner1.X), (Corner1.Y <= Corner2.Y ? Corner2.Y : Corner1.Y)};

	if (Safe2.X < 0 || Safe1.X >= Instance->ResX || Safe2.Y < 0 || Safe1.Y >= Instance->ResY) {return;} // Entirely off-screen

	GBW_Draw_HLine_Safe(Instance, Color, Safe1.X, Safe2.X, Safe1.Y);
	GBW_Draw_HLine_Safe(Instance, Color, Safe1.X, Safe2.X, Safe2.Y);
	GBW_Draw_VLine_Safe(Instance, Color, Safe1.X, Safe1.Y, Safe2.Y);
	GBW_Draw_VLine_Safe(Instance, Color, Safe2.X, Safe1.Y, Safe2.Y);
}

// Graphic function, draws horizontal line
void GBW_Draw_HLine_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X1, GBW_CoordElmt_t X2, GBW_CoordElmt_t Y)
{
	uint8_t MaskByte = 0x01 << (Y % GBW_PixelPerData);
	int32_t IndexBase = (Y / GBW_PixelPerData) * Instance->ResX;

	if (Color)
	{
		for (GBW_CoordElmt_t X = X1; X <= X2; X++)
		{
			Instance->FrameBuffer[IndexBase + X] |= MaskByte;
		}
	}
	else
	{
		MaskByte = ~MaskByte;
		for (GBW_CoordElmt_t X = X1; X <= X2; X++)
		{
			Instance->FrameBuffer[IndexBase + X] &= MaskByte;
		}
	}
}

// Graphic function, draws horizontal line
void GBW_Draw_HLine_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X1, GBW_CoordElmt_t X2, GBW_CoordElmt_t Y)
{
	GBW_CoordElmt_t SX1, SX2;

	if (X1 <= X2) {SX1 = X1; SX2 = X2;} else {SX1 = X2; SX2 = X1;}

	if (SX2 < 0 || SX1 >= Instance->ResX || Y < 0 || Y >= Instance->ResY) {return;} // Entirely off-screen

	if (SX1 < 0) {SX1 = 0;}
	if (SX2 >= Instance->ResX) {SX2 = Instance->ResX - 1;}

	GBW_Draw_HLine_Fast(Instance, Color, SX1, SX2, Y);
}

// Graphic function, draws vertical line
void GBW_Draw_VLine_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X,  GBW_CoordElmt_t Y1, GBW_CoordElmt_t Y2)
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
			Instance->FrameBuffer[TopY * Instance->ResX + X] |= TopByte;
			for (int32_t Y = TopY + 1; Y < BtmY; Y++)
			{
				Instance->FrameBuffer[Y * Instance->ResX + X] = 0xff;
			}
			Instance->FrameBuffer[BtmY * Instance->ResX + X] |= BtmByte;
		}
		else if (BtmY - TopY >= 1) // Across >= 2 pages
		{
			Instance->FrameBuffer[TopY * Instance->ResX + X] |= TopByte;
			Instance->FrameBuffer[BtmY * Instance->ResX + X] |= BtmByte;
		}
		else // In 1 page
		{
			Instance->FrameBuffer[TopY * Instance->ResX + X] |= AllByte;
		}
	}
	else
	{
		TopByte = ~TopByte;
		BtmByte = ~BtmByte;
		AllByte = ~AllByte;

		if (BtmY - TopY >= 2) // Across >= 3 pages
		{
			Instance->FrameBuffer[TopY * Instance->ResX + X] &= TopByte;
			for (int32_t Y = TopY + 1; Y < BtmY; Y++)
			{
				Instance->FrameBuffer[Y * Instance->ResX + X] = 0x00;
			}
			Instance->FrameBuffer[BtmY * Instance->ResX + X] &= BtmByte;
		}
		else if (BtmY - TopY >= 1) // Across >= 2 pages
		{
			Instance->FrameBuffer[TopY * Instance->ResX + X] &= TopByte;
			Instance->FrameBuffer[BtmY * Instance->ResX + X] &= BtmByte;
		}
		else // In 1 page
		{
			Instance->FrameBuffer[TopY * Instance->ResX + X] &= AllByte;
		}
	}
}

// Graphic function, draws vertical line
void GBW_Draw_VLine_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X, GBW_CoordElmt_t Y1, GBW_CoordElmt_t Y2)
{
	GBW_CoordElmt_t SY1, SY2;

	if (Y1 <= Y2) {SY1 = Y1; SY2 = Y2;} else {SY1 = Y2; SY2 = Y1;}

	if (SY2 < 0 || SY1 >= Instance->ResY || X < 0 || X >= Instance->ResX) {return;} // Entirely off-screen

	if (SY1 < 0) {SY1 = 0;}
	if (SY2 >= Instance->ResY) {SY2 = Instance->ResY - 1;}

	GBW_Draw_VLine_Fast(Instance, Color, X, SY1, SY2);
}

// Graphic function, draws circle based on Bresenham’s algorithm
void GBW_Draw_Circle_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R)
{
	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

	// Draw pixels for X_0
	GBW_Draw_Pixel_Fast(Instance, Color, Center.X, Center.Y + LoopY);
	GBW_Draw_Pixel_Fast(Instance, Color, Center.X, Center.Y - LoopY);
	GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LoopY, Center.Y);
	GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LoopY, Center.Y);

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
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LoopX, Center.Y + LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LoopX, Center.Y + LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LoopX, Center.Y - LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LoopX, Center.Y - LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LoopY, Center.Y + LoopX);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LoopY, Center.Y + LoopX);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LoopY, Center.Y - LoopX);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LoopY, Center.Y - LoopX);
	}
}

// Graphic function, draws circle based on Bresenham’s algorithm
void GBW_Draw_Circle_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R)
{
	if (R < 0) {R = -R;} // Negative R

	if (Center.X + R < 0) {return;} // Off-screen
	if (Center.X - R >= Instance->ResX) {return;} // Off-screen
	if (Center.Y + R < 0) {return;} // Off-screen
	if (Center.Y - R >= Instance->ResY) {return;} // Off-screen

	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

	// Draw pixels for X_0
	GBW_Draw_Pixel_Safe(Instance, Color, Center.X, Center.Y + LoopY);
	GBW_Draw_Pixel_Safe(Instance, Color, Center.X, Center.Y - LoopY);
	GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LoopY, Center.Y);
	GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LoopY, Center.Y);

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
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LoopX, Center.Y + LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LoopX, Center.Y + LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LoopX, Center.Y - LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LoopX, Center.Y - LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LoopY, Center.Y + LoopX);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LoopY, Center.Y + LoopX);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LoopY, Center.Y - LoopX);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LoopY, Center.Y - LoopX);
	}
}

// Graphic function, draws disk based on Bresenham’s algorithm
void GBW_Draw_Disk_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R)
{
	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

	// Draw pixels for X_0
	GBW_Draw_Pixel_Fast(Instance, Color, Center.X, Center.Y);
	for (int32_t LineY = 1; LineY <= LoopY; LineY++)
	{
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X, Center.Y + LineY);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X, Center.Y - LineY);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LineY, Center.Y);
		GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LineY, Center.Y);
	}

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
		for (int32_t LineY = LoopX; LineY <= LoopY; LineY++)
		{
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LoopX, Center.Y + LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LoopX, Center.Y + LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LoopX, Center.Y - LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LoopX, Center.Y - LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LineY, Center.Y + LoopX);
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LineY, Center.Y + LoopX);
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X + LineY, Center.Y - LoopX);
			GBW_Draw_Pixel_Fast(Instance, Color, Center.X - LineY, Center.Y - LoopX);
		}
	}
}

// Graphic function, draws disk based on Bresenham’s algorithm
void GBW_Draw_Disk_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R)
{
	if (R < 0) {R = -R;} // Negative R

	if (Center.X + R < 0) {return;} // Off-screen
	if (Center.X - R >= Instance->ResX) {return;} // Off-screen
	if (Center.Y + R < 0) {return;} // Off-screen
	if (Center.Y - R >= Instance->ResY) {return;} // Off-screen

	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

	// Draw pixels for X_0
	GBW_Draw_Pixel_Safe(Instance, Color, Center.X, Center.Y);
	for (int32_t LineY = 1; LineY <= LoopY; LineY++)
	{
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X, Center.Y + LineY);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X, Center.Y - LineY);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LineY, Center.Y);
		GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LineY, Center.Y);
	}

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
		for (int32_t LineY = LoopX; LineY <= LoopY; LineY++)
		{
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LoopX, Center.Y + LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LoopX, Center.Y + LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LoopX, Center.Y - LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LoopX, Center.Y - LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LineY, Center.Y + LoopX);
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LineY, Center.Y + LoopX);
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X + LineY, Center.Y - LoopX);
			GBW_Draw_Pixel_Safe(Instance, Color, Center.X - LineY, Center.Y - LoopX);
		}
	}
}

// Graphic function, draws hollow rectangle with rounded corners
void GBW_Draw_RoundHlowRect_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R)
{
	// C1
	//   I0        I1
	//
	//   I2        I3
	//               C2

	GBW_Coord_t Inner0 = {Corner1.X + R, Corner1.Y + R};
	GBW_Coord_t Inner1 = {Corner2.X - R, Corner1.Y + R};
	GBW_Coord_t Inner2 = {Corner1.X + R, Corner2.Y - R};
	GBW_Coord_t Inner3 = {Corner2.X - R, Corner2.Y - R};

	GBW_Draw_HLine_Fast(Instance, Color, Inner0.X, Inner1.X, Corner1.Y);
	GBW_Draw_HLine_Fast(Instance, Color, Inner2.X, Inner3.X, Corner2.Y);
	GBW_Draw_VLine_Fast(Instance, Color, Corner1.X, Inner0.Y, Inner2.Y);
	GBW_Draw_VLine_Fast(Instance, Color, Corner2.X, Inner1.Y, Inner3.Y);

	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

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
		GBW_Draw_Pixel_Fast(Instance, Color, Inner0.X - LoopX, Inner0.Y - LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Inner0.X - LoopY, Inner0.Y - LoopX);

		GBW_Draw_Pixel_Fast(Instance, Color, Inner1.X + LoopX, Inner1.Y - LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Inner1.X + LoopY, Inner1.Y - LoopX);

		GBW_Draw_Pixel_Fast(Instance, Color, Inner2.X - LoopX, Inner2.Y + LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Inner2.X - LoopY, Inner2.Y + LoopX);

		GBW_Draw_Pixel_Fast(Instance, Color, Inner3.X + LoopX, Inner3.Y + LoopY);
		GBW_Draw_Pixel_Fast(Instance, Color, Inner3.X + LoopY, Inner3.Y + LoopX);
	}
}

// Graphic function, draws hollow rectangle with rounded corners
void GBW_Draw_RoundHlowRect_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R)
{
	// S1
	//   I0        I1
	//
	//   I2        I3
	//               S2

	GBW_Coord_t Safe1 = {(Corner1.X <= Corner2.X ? Corner1.X : Corner2.X), (Corner1.Y <= Corner2.Y ? Corner1.Y : Corner2.Y)};
	GBW_Coord_t Safe2 = {(Corner1.X <= Corner2.X ? Corner2.X : Corner1.X), (Corner1.Y <= Corner2.Y ? Corner2.Y : Corner1.Y)};

	if (Safe2.X < 0 || Safe1.X >= Instance->ResX || Safe2.Y < 0 || Safe1.Y >= Instance->ResY) {return;} // Entirely off-screen

	if (Safe2.X - Safe1.X < (R * 2)) {R = (Safe2.X - Safe1.X) / 2;}
	if (Safe2.Y - Safe1.Y < (R * 2)) {R = (Safe2.Y - Safe1.Y) / 2;}

	GBW_Coord_t Inner0 = {Safe1.X + R, Safe1.Y + R};
	GBW_Coord_t Inner1 = {Safe2.X - R, Safe1.Y + R};
	GBW_Coord_t Inner2 = {Safe1.X + R, Safe2.Y - R};
	GBW_Coord_t Inner3 = {Safe2.X - R, Safe2.Y - R};

	GBW_Draw_HLine_Safe(Instance, Color, Inner0.X, Inner1.X, Safe1.Y);
	GBW_Draw_HLine_Safe(Instance, Color, Inner2.X, Inner3.X, Safe2.Y);
	GBW_Draw_VLine_Safe(Instance, Color, Safe1.X, Inner0.Y, Inner2.Y);
	GBW_Draw_VLine_Safe(Instance, Color, Safe2.X, Inner1.Y, Inner3.Y);

	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

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
		GBW_Draw_Pixel_Safe(Instance, Color, Inner0.X - LoopX, Inner0.Y - LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Inner0.X - LoopY, Inner0.Y - LoopX);

		GBW_Draw_Pixel_Safe(Instance, Color, Inner1.X + LoopX, Inner1.Y - LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Inner1.X + LoopY, Inner1.Y - LoopX);

		GBW_Draw_Pixel_Safe(Instance, Color, Inner2.X - LoopX, Inner2.Y + LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Inner2.X - LoopY, Inner2.Y + LoopX);

		GBW_Draw_Pixel_Safe(Instance, Color, Inner3.X + LoopX, Inner3.Y + LoopY);
		GBW_Draw_Pixel_Safe(Instance, Color, Inner3.X + LoopY, Inner3.Y + LoopX);
	}
}

// Graphic function, draws solid rectangle with rounded corners
void GBW_Draw_RoundSlidRect_Fast(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R)
{
	// C1  M1         |
	//     |          |
	// L1--I0        I1R1--
	//     |          |
	//     |          |
	// --L2I2        I3--R2
	//     |          |
	//     |         M2  C2

	GBW_Coord_t Inner0 = {Corner1.X + R, Corner1.Y + R};
	GBW_Coord_t Inner1 = {Corner2.X - R, Corner1.Y + R};
	GBW_Coord_t Inner2 = {Corner1.X + R, Corner2.Y - R};
	GBW_Coord_t Inner3 = {Corner2.X - R, Corner2.Y - R};

	GBW_Coord_t L1 = {Corner1.X, Inner0.Y};
	GBW_Coord_t L2 = {Inner2.X - 1, Inner2.Y};
	GBW_Coord_t R1 = {Inner1.X + 1, Inner1.Y};
	GBW_Coord_t R2 = {Corner2.X, Inner3.Y};
	GBW_Coord_t M1 = {Inner0.X, Corner1.Y};
	GBW_Coord_t M2 = {Inner3.X, Corner2.Y};

	GBW_Draw_SlidRect_Fast(Instance, Color, L1, L2);
	GBW_Draw_SlidRect_Fast(Instance, Color, R1, R2);
	GBW_Draw_SlidRect_Fast(Instance, Color, M1, M2);

	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

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
		for (int32_t LineY = LoopX; LineY <= LoopY; LineY++)
		{
			GBW_Draw_Pixel_Fast(Instance, Color, Inner0.X - LoopX, Inner0.Y - LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Inner0.X - LineY, Inner0.Y - LoopX);

			GBW_Draw_Pixel_Fast(Instance, Color, Inner1.X + LoopX, Inner1.Y - LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Inner1.X + LineY, Inner1.Y - LoopX);

			GBW_Draw_Pixel_Fast(Instance, Color, Inner2.X - LoopX, Inner2.Y + LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Inner2.X - LineY, Inner2.Y + LoopX);

			GBW_Draw_Pixel_Fast(Instance, Color, Inner3.X + LoopX, Inner3.Y + LineY);
			GBW_Draw_Pixel_Fast(Instance, Color, Inner3.X + LineY, Inner3.Y + LoopX);
		}
	}
}

// Graphic function, draws solid rectangle with rounded corners
void GBW_Draw_RoundSlidRect_Safe(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R)
{
	// S1  M1         |
	//     |          |
	// L1--I0        I1R1--
	//     |          |
	//     |          |
	// --L2I2        I3--R2
	//     |          |
	//     |         M2  S2

	GBW_Coord_t Safe1 = {(Corner1.X <= Corner2.X ? Corner1.X : Corner2.X), (Corner1.Y <= Corner2.Y ? Corner1.Y : Corner2.Y)};
	GBW_Coord_t Safe2 = {(Corner1.X <= Corner2.X ? Corner2.X : Corner1.X), (Corner1.Y <= Corner2.Y ? Corner2.Y : Corner1.Y)};

	if (Safe2.X < 0 || Safe1.X >= Instance->ResX || Safe2.Y < 0 || Safe1.Y >= Instance->ResY) {return;} // Entirely off-screen

	if (Safe2.X - Safe1.X < (R * 2)) {R = (Safe2.X - Safe1.X) / 2;}
	if (Safe2.Y - Safe1.Y < (R * 2)) {R = (Safe2.Y - Safe1.Y) / 2;}

	GBW_Coord_t Inner0 = {Safe1.X + R, Safe1.Y + R};
	GBW_Coord_t Inner1 = {Safe2.X - R, Safe1.Y + R};
	GBW_Coord_t Inner2 = {Safe1.X + R, Safe2.Y - R};
	GBW_Coord_t Inner3 = {Safe2.X - R, Safe2.Y - R};

	GBW_Coord_t L1 = {Safe1.X, Inner0.Y};
	GBW_Coord_t L2 = {Inner2.X - 1, Inner2.Y};
	GBW_Coord_t R1 = {Inner1.X + 1, Inner1.Y};
	GBW_Coord_t R2 = {Safe2.X, Inner3.Y};
	GBW_Coord_t M1 = {Inner0.X, Safe1.Y};
	GBW_Coord_t M2 = {Inner3.X, Safe2.Y};

	GBW_Draw_SlidRect_Safe(Instance, Color, L1, L2);
	GBW_Draw_SlidRect_Safe(Instance, Color, R1, R2);
	GBW_Draw_SlidRect_Safe(Instance, Color, M1, M2);

	int32_t LoopX = 0;
	int32_t LoopY = R;
	int32_t D = 3 - 2 * R; // Decision parameter determines whether to decrement Y, this is already the D for X_1, first D calculation in loop is for X_2

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
		for (int32_t LineY = LoopX; LineY <= LoopY; LineY++)
		{
			GBW_Draw_Pixel_Safe(Instance, Color, Inner0.X - LoopX, Inner0.Y - LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Inner0.X - LineY, Inner0.Y - LoopX);

			GBW_Draw_Pixel_Safe(Instance, Color, Inner1.X + LoopX, Inner1.Y - LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Inner1.X + LineY, Inner1.Y - LoopX);

			GBW_Draw_Pixel_Safe(Instance, Color, Inner2.X - LoopX, Inner2.Y + LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Inner2.X - LineY, Inner2.Y + LoopX);

			GBW_Draw_Pixel_Safe(Instance, Color, Inner3.X + LoopX, Inner3.Y + LineY);
			GBW_Draw_Pixel_Safe(Instance, Color, Inner3.X + LineY, Inner3.Y + LoopX);
		}
	}
}



