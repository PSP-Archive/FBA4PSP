
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>

#include <stdarg.h>

#include "burnint.h"
#include "psp.h"
#include "font.h"

static unsigned char __attribute__((aligned(16))) list[32*1024];

void * show_frame;
void * draw_frame;
void * work_frame;
void * tex_frame;

static int nPrevStage;

static int VideoBufferWidth, VideoBufferHeight;

static int myProgressRangeCallback(double dProgressRange)
{
	
	return 0;
}

static int myProgressUpdateCallback(double dProgress, const char* pszText, bool bAbs)
{
	bprintf(PRINT_IMPORTANT, "%s %f %d", pszText, dProgress, bAbs);
	return 0;
}

static int AppDebugPrintf(int nStatus, char* pszFormat, ...)
{
	va_list vaFormat;
	va_start(vaFormat, pszFormat);
	char buf[256];
	
	sprintf(buf, pszFormat, vaFormat);
	
	unsigned short fc;
	switch (nStatus) {
	case PRINT_UI:			fc = R8G8B8_to_B5G6R5(0x404040); break;
	case PRINT_IMPORTANT:	fc = R8G8B8_to_B5G6R5(0x600000); break;
	case PRINT_ERROR:		fc = R8G8B8_to_B5G6R5(0x606000); break;
	default:				fc = R8G8B8_to_B5G6R5(0x404040); break;
	}
	
	drawRect(GU_FRAME_ADDR(work_frame), 0, 0, 480, 20, fc);
	drawString(buf, GU_FRAME_ADDR(work_frame), 4, 4, R8G8B8_to_B5G6R5(0xffffff));

	drawRect(GU_FRAME_ADDR(tex_frame), 0, 0, 480, 20, fc);
	drawString(buf, GU_FRAME_ADDR(tex_frame), 4, 4, R8G8B8_to_B5G6R5(0xffffff));
	
	va_end(vaFormat);
	
	update_gui();
	
	return 0;
}


void init_gui()
{
	show_frame = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 0);
	draw_frame = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 1);
	work_frame = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 2);
	tex_frame  = (void *)(PSP_LINE_SIZE * SCREEN_HEIGHT * 2 * 3);

	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, show_frame, PSP_LINE_SIZE);
	sceGuClutMode(GU_PSM_5650, 0, 0xff, 0);
	sceGuDrawBuffer(GU_PSM_5650, draw_frame, PSP_LINE_SIZE);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	
	sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
	sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);

	sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	
//	sceGuFrontFace(GU_CW);

	sceGuDisable(GU_ALPHA_TEST);
	sceGuAlphaFunc(GU_LEQUAL, 0, 0x01);

	sceGuDisable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthRange(65535, 0);	//sceGuDepthRange(0xc350, 0x2710);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuDepthMask(GU_TRUE);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_5650, 0, 0, GU_FALSE);
	sceGuTexScale(1.0f / PSP_LINE_SIZE, 1.0f / PSP_LINE_SIZE);
	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);

	sceGuTexImage(0, 512, 512, 512, GU_FRAME_ADDR(work_frame));
//	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);

	sceGuDisable(GU_DITHER);
	
	sceGuClearDepth(0);
	sceGuClearColor(0);

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);
	sceGuDisplay(GU_TRUE);	
	
	nPrevStage = nGameStage;
	//bprintf = AppDebugPrintf;
	
	
	BurnExtProgressRangeCallback = myProgressRangeCallback;
	BurnExtProgressUpdateCallback = myProgressUpdateCallback;
}

void exit_gui()
{
	sceGuDisplay(GU_FALSE);
	sceGuTerm();
}

void update_gui()
{
	sceGuStart(GU_DIRECT, list);
	sceGuClear(GU_COLOR_BUFFER_BIT);

	sceGuDrawBufferList(GU_PSM_5650, draw_frame, PSP_LINE_SIZE);
	
//	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
//	sceGuDisable(GU_ALPHA_TEST);
//	sceGuTexMode(GU_PSM_5650, 0, 0, GU_FALSE);
//	sceGuTexImage(0, 512, 512, 512, GU_FRAME_ADDR(tex_frame));
//	sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	if ( nPrevStage != nGameStage ) {
		if ( nGameStage ) {
			sceGuTexImage(0, 512, 512, 512, GU_FRAME_ADDR(work_frame));
			sceGuTexFilter(GU_NEAREST, GU_NEAREST);
		} else {
			BurnDrvGetFullSize(&VideoBufferWidth, &VideoBufferHeight);
			sceGuTexImage(0, 512, 512, 512, GU_FRAME_ADDR(tex_frame));
			sceGuTexFilter(GU_LINEAR, GU_LINEAR);
		}
		nPrevStage = nGameStage;
	}

	struct Vertex* vertices = (struct Vertex*)sceGuGetMemory(2 * sizeof(struct Vertex));
	vertices[0].u = 0; 
	vertices[0].v = 0;
	vertices[0].x = 0; 
	vertices[0].y = 0; 
	vertices[1].x = SCREEN_WIDTH; 
	vertices[1].y = SCREEN_HEIGHT; 

	if ( nGameStage ) {
		vertices[1].u = 480;	//384;
		vertices[1].v = 272;	//224;
	} else {
		vertices[1].u = VideoBufferWidth;	//384;
		vertices[1].v = VideoBufferHeight;	//224;
	}

	sceGuDrawArray(	GU_SPRITES, 
					GU_TEXTURE_16BIT | GU_COLOR_5650 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 
					2, 0, vertices);

	sceGuFinish();
	sceGuSync(0, GU_SYNC_FINISH);	
	
}

