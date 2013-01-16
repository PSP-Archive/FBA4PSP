
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "burnint.h"
#include "font.h"
#include "psp.h"

PSP_MODULE_INFO(PBPNAME, PSP_MODULE_USER, VERSION_MAJOR, VERSION_MINOR);
PSP_MAIN_THREAD_ATTR( THREAD_ATTR_USER );


//#define MAX_PATH		1024

int nGameStage = 0;
int bGameRunning = 0;
char currentPath[MAX_PATH];

int exit_callback(int arg1, int arg2, void *common) {
	bGameRunning = 0;
	return 0;
}

int CallbackThread(SceSize args, void *argp) {
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

void *video_frame_addr(void *frame, int x, int y)
{
	return (void *)(((unsigned int)frame | 0x44000000) + ((x + (y << 9)) << 1));
}


#define SND_RATE		11025
#define SND_FRAME_SIZE	((SND_RATE * 100 + 3000) / 6000)
short mixbuf[SND_FRAME_SIZE * 2 + 1024];

static unsigned int HighCol16(int r, int g, int b, int  /* i */)
{
	unsigned int t;
	t  = (b << 8) & 0xF800;
	t |= (g << 3) & 0x07E0;
	t |= (r >> 3) & 0x001F;
	return t;
}

int DrvInit(int nDrvNum, bool bRestore);
int DrvExit();
int InpInit();
int InpExit();
int InpMake(unsigned int);
void InpDIP();

extern char szAppRomPath[];

//static unsigned int KeypadData = 0;

int main(int argc, char** argv) {

	SceCtrlData pad;
	
	getcwd(currentPath, MAX_PATH - 1);
	strcat(currentPath, "/");
	
	strcpy(szAppRomPath, currentPath);
	strcat(szAppRomPath, "ROMS/");
	
	int thid = sceKernelCreateThread(PBPNAME, CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0) sceKernelStartThread(thid, 0, 0);
	
	nGameStage = 1;
	init_gui();
	
	
	BurnLibInit();
	
	for (nBurnDrvSelect=0; nBurnDrvSelect<nBurnDrvCount; nBurnDrvSelect++) 
		if ( strcmp("aerofgt", BurnDrvGetText(DRV_NAME)) == 0 )
			break;
	if (nBurnDrvSelect >= nBurnDrvCount) nBurnDrvSelect = ~0U;
	
	bBurnUseASMCPUEmulation = false;
	
	nInterpolation = 3;
	pBurnSoundOut = 0;	//&mixbuf[0];
	nBurnSoundRate = SND_RATE;
	nBurnSoundLen = 0;	//SND_FRAME_SIZE;
	
	//BurnDrvGetFullSize(&VideoBufferWidth, &VideoBufferHeight);
	//printf("%d x %d \n", VideoBufferWidth, VideoBufferHeight);
	nBurnBpp = 2;
	nBurnPitch  = 512 * 2;
	BurnHighCol = HighCol16;
	
	int ret = 0;
	
	//DrvInit(nBurnDrvSelect, false);
	//if (nRet != 0) return 0;

	//BurnRecalcPal();
	//InpInit();
	//InpDIP();
	
	pBurnDraw = (unsigned char *) video_frame_addr(tex_frame, 0, 0);
	
	//szAppRomPath
	//strcat(ui_current_path, "roms");
	
	draw_ui_main();
	bGameRunning = 1;
	
	while( bGameRunning ) {
		sceCtrlReadBufferPositive(&pad, 1); 
		
		if ( nGameStage ) {

			do_ui_key( pad.Buttons );
			
			update_gui();
			sceDisplayWaitVblankStart();
			
		} else {
			
			if ( pad.Buttons & PSP_CTRL_LTRIGGER ) {
				
				scePowerSetClockFrequency(222, 222, 111);
				nGameStage = 1;
				draw_ui_main();
				
				continue;
			}
			
			InpMake(pad.Buttons);
			
			nFramesEmulated++;
			nCurrentFrame++;
			nFramesRendered++;
		
			pBurnDraw = (unsigned char *) video_frame_addr(tex_frame, 0, 0);
			BurnDrvFrame();
			pBurnDraw = NULL;
	
			update_gui();
			
		//sceDisplayWaitVblankStart();

		}

		
		show_frame = draw_frame;
		draw_frame = sceGuSwapBuffers();
	}

	scePowerSetClockFrequency(222, 222, 111);

	exit_gui();
	
	DrvExit();
	BurnLibExit();
	InpExit();
	
	sceKernelExitGame();
}



