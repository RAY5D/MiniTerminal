#ifndef GRAPHIC_BW_H_
#define GRAPHIC_BW_H_

// Include
#include <stdint.h>
#include "Graphic_BW_Specifics.h"

// Defines
#define GBW_PixelPerData (8)

// Typedef

// Only one format is supported
//        X0 X1 X2 ...
//     Y0 b0 b0 b0 ...
//     Y1 b1 b1 b1 ...
//     Y2 b2 b2 b2 ...
//     Y3 b3 b3 b3 ...
//     Y4 b4 b4 b4 ...
//     ...
typedef uint32_t GBW_Tick_t;

typedef enum
{
	GBW_Color_0 = 0,
	GBW_Color_1 = 1
} GBW_Color_t;

typedef enum
{
	GBW_State_Reset,	// Not initialized
	GBW_State_Ready,	// Waiting for frame rate limit
	GBW_State_Drawing,	// Rendering
	GBW_State_Sending	// Sending to screen
} GBW_State_t;

typedef struct _GBW_Instance_t
{
	uint32_t ResX;
	uint32_t ResY;
	uint8_t* FrameBuffer;
	uint32_t FrameBuffer_Len;
	GBW_State_t State;
	uint32_t FrameRate;
	uint32_t LastFrame;
	void (*Draw)(struct _GBW_Instance_t*);
	uint32_t (*Send)(uint8_t*, uint32_t);
	uint32_t (*Check)();
} GBW_Instance_t;

// Functions
void GBW_Init
(
	GBW_Instance_t* Instance,
	uint32_t ResX,
	uint32_t ResY,
	uint8_t* FrameBuffer,
	uint32_t FrameBuffer_Len,
	uint32_t FrameRate,
	void (*DrawFunc)(GBW_Instance_t*),
	uint32_t (*SendFunc)(uint8_t*, uint32_t),
	uint32_t (*CheckFunc)()
);
void GBW_LoopHandler(GBW_Instance_t* Instance);
void GBW_TickInc();
GBW_Tick_t GBW_GetTick();

///change all below to instance instead of buffer
void GBW_Fill(GBW_Instance_t* Instance, GBW_Color_t Color);
//void GBW_Draw_Pixel_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X, int32_t Y);
//void GBW_Draw_Pixel_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X, int32_t Y);
//void GBW_Draw_SlidRect_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y1, int32_t Y2);
//void GBW_Draw_SlidRect_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y1, int32_t Y2);
//void GBW_Draw_HLine_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y);
//void GBW_Draw_HLine_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y);
//void GBW_Draw_VLine_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X,  int32_t Y1, int32_t Y2);
//void GBW_Draw_VLine_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X,  int32_t Y1, int32_t Y2);
//void GBW_Draw_Circle_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X, int32_t Y, int32_t R);

#endif /* GRAPHIC_BW_H */
