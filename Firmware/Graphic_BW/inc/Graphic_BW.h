#ifndef GRAPHIC_BW_H_
#define GRAPHIC_BW_H_

// Include
#include <stdint.h>

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
typedef struct
{
	uint32_t ResX;
	uint32_t ResY;
	uint8_t* Data;
} GBW_FrameBuffer_t;

typedef enum
{
	GBW_Color_0 = 0,
	GBW_Color_1 = 1
} GBW_Color_t;

// Functions
void GBW_SetBufferRes		(GBW_FrameBuffer_t* Buffer, uint32_t X, uint32_t Y);
void GBW_Fill				(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color);
void GBW_Draw_Pixel_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X, int32_t Y);
void GBW_Draw_Pixel_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X, int32_t Y);
void GBW_Draw_SlidRect_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y1, int32_t Y2);
void GBW_Draw_SlidRect_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y1, int32_t Y2);
void GBW_Draw_HLine_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y);
void GBW_Draw_HLine_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X1, int32_t X2, int32_t Y);
void GBW_Draw_VLine_Fast	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X,  int32_t Y1, int32_t Y2);
void GBW_Draw_VLine_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X,  int32_t Y1, int32_t Y2);
void GBW_Draw_Circle_Safe	(GBW_FrameBuffer_t* Buffer, GBW_Color_t Color, int32_t X, int32_t Y, int32_t R);

#endif /* GRAPHIC_BW_H */
