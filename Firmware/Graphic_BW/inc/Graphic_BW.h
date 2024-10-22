#ifndef GRAPHIC_BW_H_
#define GRAPHIC_BW_H_

// Include
#include <stdint.h>

// Defines
#define GBW_PixelPerData (8)

// Typedef
typedef uint32_t GBW_Tick_t;
typedef int32_t GBW_CoordElmt_t;

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

typedef struct
{
	GBW_CoordElmt_t X;
	GBW_CoordElmt_t Y;
} GBW_Coord_t;

typedef struct _GBW_Instance_t
{
	GBW_CoordElmt_t ResX;
	GBW_CoordElmt_t ResY;
	uint8_t* FrameBuffer;
	uint32_t FrameBuffer_Len;
	GBW_State_t State;
	uint32_t MinFramePeriod;
	uint32_t LastFrameTick;
	void (*GUIDrawCB)(struct _GBW_Instance_t*);
	uint32_t (*DriverFlushCB)(uint8_t*, uint32_t);
	uint32_t (*DriverCheckIdleCB)(void);
} GBW_Instance_t;

// Only one format is supported
//        X0 X1 X2 ...
//     Y0 b0 b0 b0 ...
//     Y1 b1 b1 b1 ...
//     Y2 b2 b2 b2 ...
//     Y3 b3 b3 b3 ...
//     Y4 b4 b4 b4 ...
//     ...

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
	uint32_t (*CheckFunc)(void)
);
void GBW_LoopHandler(GBW_Instance_t* Instance);
void GBW_TickInc();
GBW_Tick_t GBW_GetTick();

// Graphic Functions
#define GBW_Draw_Pixel_Fast(Instance, Color, X, Y) (\
{\
	uint32_t Index = ((Y) / GBW_PixelPerData) * ((Instance)->ResX) + (X);\
	if (Color)\
	{\
		((Instance)->FrameBuffer)[Index] |= (1 << ((Y) % GBW_PixelPerData));\
	}\
	else\
	{\
		((Instance)->FrameBuffer)[Index] &= ~(1 << ((Y) % GBW_PixelPerData));\
	}\
})

#define GBW_Draw_Pixel_Safe(Instance, Color, X, Y) (\
{\
	if ((X) >= 0 && (X) < (Instance)->ResX && (Y) >= 0 && (Y) < (Instance)->ResY)\
	{\
		GBW_Draw_Pixel_Fast((Instance), (Color), (X), (Y));\
	}\
})

void GBW_Draw_Fill					(GBW_Instance_t* Instance, GBW_Color_t Color);
void GBW_Draw_SlidRect_Fast			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2);
void GBW_Draw_SlidRect_Safe			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2);
void GBW_Draw_HlowRect_Fast			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2);
void GBW_Draw_HlowRect_Safe			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2);
void GBW_Draw_HLine_Fast			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X1, GBW_CoordElmt_t X2, GBW_CoordElmt_t Y);
void GBW_Draw_HLine_Safe			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X1, GBW_CoordElmt_t X2, GBW_CoordElmt_t Y);
void GBW_Draw_VLine_Fast			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X,  GBW_CoordElmt_t Y1, GBW_CoordElmt_t Y2);
void GBW_Draw_VLine_Safe			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_CoordElmt_t X,  GBW_CoordElmt_t Y1, GBW_CoordElmt_t Y2);
void GBW_Draw_Circle_Fast			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R);
void GBW_Draw_Circle_Safe			(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R);
void GBW_Draw_Disk_Fast				(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R);
void GBW_Draw_Disk_Safe				(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Center, int32_t R);
void GBW_Draw_RoundHlowRect_Fast	(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R);
void GBW_Draw_RoundHlowRect_Safe	(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R);
void GBW_Draw_RoundSlidRect_Fast	(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R);
void GBW_Draw_RoundSlidRect_Safe	(GBW_Instance_t* Instance, GBW_Color_t Color, GBW_Coord_t Corner1, GBW_Coord_t Corner2, int32_t R);


#endif /* GRAPHIC_BW_H */
