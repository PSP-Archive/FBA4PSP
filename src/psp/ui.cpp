#include <pspkernel.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <psppower.h>

#include <stdio.h>
#include <string.h>

#include "psp.h"
#include "font.h"
#include "burnint.h"

#define UI_COLOR	R8G8B8_to_B5G6R5(0xffc090)
#define UI_BGCOLOR	R8G8B8_to_B5G6R5(0x102030)
#define find_rom_list_cnt	10

static int find_rom_count = 0;
static int find_rom_select = 0;
static int find_rom_top = 0;
char ui_current_path[MAX_PATH];

static unsigned int nPrevGame = ~0U;

static int ui_mainmenu_select = 0;

static unsigned short set_ui_main_menu_item_color(int i)
{
	if (ui_mainmenu_select == i) return UI_BGCOLOR;	
	return UI_COLOR;
}

static struct {
	int cpu, bus; 
} cpu_speeds[] = { { 222, 111}, { 266, 133}, { 300, 150}, { 333, 166} };

static int cpu_speeds_select = 0;

static char *ui_main_menu[] = {
	"Select ROM",
	"Load Game",
	"Save Game",
	"DIP Settings",
	"Reset Game",
	"Game Control",
	"< Wide Screen 16:9 480x272 >",
	"< CPU Speed %dMHz >",
	"Return to Game",
	"Exit FinaBurn Alpha",
};

void draw_ui_main()
{
	char buf[320];
	drawRect(GU_FRAME_ADDR(work_frame), 0, 0, 480, 272, UI_BGCOLOR);
	drawString("FinalBurn Alpha for PSP (ver: 0.01)", GU_FRAME_ADDR(work_frame), 10, 10, UI_COLOR);
	unsigned int kdv = sceKernelDevkitVersion();
	sprintf(buf, "FW: %X.%X%X.%02X", kdv >> 24, (kdv&0xf0000)>>16, (kdv&0xf00)>>8, kdv&0xff);
	drawString(buf, GU_FRAME_ADDR(work_frame), 470 - getDrawStringLength(buf), 10, UI_COLOR);
    drawRect(GU_FRAME_ADDR(work_frame), 8, 28, 464, 1, UI_COLOR);
    
    drawRect(GU_FRAME_ADDR(work_frame), 10, 40+ui_mainmenu_select*18, 460, 18, UI_COLOR);
    
    for(int i=0; i<10; i++)  {
	    unsigned short fc = set_ui_main_menu_item_color(i);
	    
	    switch ( i ) {
//	    case 6:
	    	
	    case 7:
	    	sprintf( buf, ui_main_menu[i], cpu_speeds[cpu_speeds_select].cpu );
	    	drawString(buf, 
	    			GU_FRAME_ADDR(work_frame), 
	    			240 - getDrawStringLength(ui_main_menu[i]) / 2,
	    			44 + i * 18, fc);
			break;
	    default:
	    	drawString(ui_main_menu[i], 
	    			GU_FRAME_ADDR(work_frame), 
	    			240 - getDrawStringLength(ui_main_menu[i]) / 2,
	    			44 + i * 18, fc);
    	}
	}
	
    drawRect(GU_FRAME_ADDR(work_frame), 8, 230, 464, 1, UI_COLOR);
    drawString("FB Alpha contains parts of MAME & Final Burn. (C) 2004, Team FB Alpha.", GU_FRAME_ADDR(work_frame), 10, 238, UI_COLOR);
    drawString("FinalBurn Alpha for PSP (C) 2008, OopsWare.", GU_FRAME_ADDR(work_frame), 10, 255, UI_COLOR);
}

void draw_ui_browse(bool rebuiltlist)
{
	unsigned int bds = nBurnDrvSelect;
	char buf[1024];
	drawRect(GU_FRAME_ADDR(work_frame), 0, 0, 480, 272, UI_BGCOLOR);

	find_rom_count = findRomsInDir( rebuiltlist );

	strcpy(buf, "PATH: ");
	strcat(buf, ui_current_path);
	
	drawString(buf, GU_FRAME_ADDR(work_frame), 10, 10, UI_COLOR, 460);
    drawRect(GU_FRAME_ADDR(work_frame), 8, 28, 464, 1, UI_COLOR);
	
	for(int i=0; i<find_rom_list_cnt; i++) {
		char *p = getRomsFileName(i+find_rom_top);
		unsigned short fc, bc;
		
		if ((i+find_rom_top) == find_rom_select) {
			bc = UI_COLOR;
			fc = UI_BGCOLOR;
		} else {
			bc = UI_BGCOLOR;
			fc = UI_COLOR;
		}
		
		drawRect(GU_FRAME_ADDR(work_frame), 10, 40+i*18, 230, 18, bc);
		if (p) {
			switch( getRomsFileStat(i+find_rom_top) ) {
			case -2: // unsupport
			case -3: // not working
				drawString(p, GU_FRAME_ADDR(work_frame), 12, 44+i*18, R8G8B8_to_B5G6R5(0x808080), 180);
				break;
			case -1: // directry
				drawString("<DIR>", GU_FRAME_ADDR(work_frame), 194, 44 + i*18, fc);
			default:
				drawString(p, GU_FRAME_ADDR(work_frame), 12, 44+i*18, fc, 180);
			}
		}
		
		if ( find_rom_count > find_rom_list_cnt ) {
			drawRect(GU_FRAME_ADDR(work_frame), 242, 40, 5, 18 * find_rom_list_cnt, R8G8B8_to_B5G6R5(0x807060));
		
			drawRect(GU_FRAME_ADDR(work_frame), 242, 
					40 + find_rom_top * 18 * find_rom_list_cnt / find_rom_count , 5, 
					find_rom_list_cnt * 18 * find_rom_list_cnt / find_rom_count, UI_COLOR);
		} else
			drawRect(GU_FRAME_ADDR(work_frame), 242, 40, 5, 18 * find_rom_list_cnt, UI_COLOR);

	}
	
    drawRect(GU_FRAME_ADDR(work_frame), 8, 230, 464, 1, UI_COLOR);

	nBurnDrvSelect = getRomsFileStat(find_rom_select);

	strcpy(buf, "Game Info: ");
	if ( nBurnDrvSelect < nBurnDrvCount)
		strcat(buf, BurnDrvGetTextA( DRV_FULLNAME ) );
    drawString(buf, GU_FRAME_ADDR(work_frame), 10, 238, UI_COLOR, 460);

	strcpy(buf, "Released by: ");
	if ( nBurnDrvSelect < nBurnDrvCount ) {
		strcat(buf, BurnDrvGetTextA( DRV_MANUFACTURER ));
		strcat(buf, " (");
		strcat(buf, BurnDrvGetTextA( DRV_DATE ));
		strcat(buf, ", ");
		strcat(buf, BurnDrvGetTextA( DRV_SYSTEM ));
		strcat(buf, " hardware)");
	}
    drawString(buf, GU_FRAME_ADDR(work_frame), 10, 255, UI_COLOR, 460);
   
    nBurnDrvSelect = bds;
}



static void process_key( int key, int down, int repeat )
{
	if ( !down ) return ;
	switch( nGameStage ) {
	/* ---------------------------- Main Menu ---------------------------- */
	case 1:		
		//ui_mainmenu_select
		switch( key ) {
		case PSP_CTRL_UP:
			if (ui_mainmenu_select <= 0) break;
			ui_mainmenu_select--;
			draw_ui_main();
			break;
		case PSP_CTRL_DOWN:
			if (ui_mainmenu_select >=9 ) break;
			ui_mainmenu_select++;
			draw_ui_main();
			break;

		case PSP_CTRL_LEFT:
			switch(ui_mainmenu_select) {
			case 7:
				if ( cpu_speeds_select > 0 ) {
					cpu_speeds_select--;
					draw_ui_main();
				}
				break;
			}
			break;
		case PSP_CTRL_RIGHT:
			switch(ui_mainmenu_select) {
			case 7:
				if ( cpu_speeds_select < 3 ) {
					cpu_speeds_select++;
					draw_ui_main();
				}
				break;
			}
			break;
			
		case PSP_CTRL_CIRCLE:
			switch( ui_mainmenu_select ) {
			case 0:
				nGameStage = 2;
				strcpy(ui_current_path, szAppRomPath);
				//ui_current_path[strlen(ui_current_path)-1] = 0;
				draw_ui_browse(true);
				break;
			case 8: // Return to Game
				if ( nPrevGame < nBurnDrvCount ) {
					scePowerSetClockFrequency(
								cpu_speeds[cpu_speeds_select].cpu, 
								cpu_speeds[cpu_speeds_select].cpu, 
								cpu_speeds[cpu_speeds_select].bus );
					nGameStage = 0;
				}
				break;	
			case 9:	// Exit
				bGameRunning = 0;
				break;
				
			}
			break;
		}
		break;
	/* ---------------------------- Rom Browse ---------------------------- */
	case 2:		
		switch( key ) {
		case PSP_CTRL_UP:
			if (find_rom_select == 0) break;
			if (find_rom_top >= find_rom_select) find_rom_top--;
			find_rom_select--;
			draw_ui_browse(false);
			break;
		case PSP_CTRL_DOWN:
			if ((find_rom_select+1) >= find_rom_count) break;
			find_rom_select++;
			if ((find_rom_top + find_rom_list_cnt) <= find_rom_select) find_rom_top++;
			draw_ui_browse(false);
			break;
		case PSP_CTRL_CIRCLE:
			switch( getRomsFileStat(find_rom_select) ) {
			case -1:	// directry
				{		// printf("change dir %s\n", getRomsFileName(find_rom_select) );
					char * pn = getRomsFileName(find_rom_select);
					if ( strcmp("..", pn) ) {
						strcat(ui_current_path, getRomsFileName(find_rom_select));
						strcat(ui_current_path, "/");
					} else {
						if (strlen(strstr(ui_current_path, ":/")) == 2) break;	// "ROOT:/"
						for(int l = strlen(ui_current_path)-1; l>1; l-- ) {
							ui_current_path[l] = 0;
							if (ui_current_path[l-1] == '/') break;
						}
					}
					//printf("change dir to %s\n", ui_current_path );
					find_rom_count = 0;
					find_rom_select = 0;
					find_rom_top = 0;
					draw_ui_browse(true);
				}
				break;
			default: // rom zip file
				{
					nBurnDrvSelect = (unsigned int)getRomsFileStat( find_rom_select );
					if (nBurnDrvSelect <= nBurnDrvCount && BurnDrvIsWorking() ) {

						if ( DrvInit( nBurnDrvSelect, false ) == 0 ) {
							
							BurnRecalcPal();
							InpInit();
							InpDIP();

							scePowerSetClockFrequency(
								cpu_speeds[cpu_speeds_select].cpu, 
								cpu_speeds[cpu_speeds_select].cpu, 
								cpu_speeds[cpu_speeds_select].bus );
							nGameStage = 0;
							
						} else
							nBurnDrvSelect = ~0U; 

					} else
						nBurnDrvSelect = ~0U; 

					nPrevGame = nBurnDrvSelect;
											
					//if (nBurnDrvSelect == ~0U) {
					//	bprintf(0, "unkown rom %s", getRomsFileName(find_rom_select));
					//}
				}
				
				
				
			}
			break;
		case PSP_CTRL_CROSS:	// cancel
			nGameStage = 1;
			draw_ui_main();
			break;
		}
		break;
	/* ---------------------------- DIP Setting ---------------------------- */
	case 3:		
		
		break;

	}
}

int do_ui_key(unsigned int key)
{
	// mask keys
	key &= PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT | PSP_CTRL_CIRCLE | PSP_CTRL_CROSS;
	static int prvkey = 0;
	static int repeat = 0;
	static int repeat_time = 0;
	
	if (key != prvkey) {
		int def = key ^ prvkey;
		repeat = 0;
		repeat_time = 0;
		process_key( def, def & key, 0 );
		if (def & key) {
			// auto repeat up / down only
			repeat = def & (PSP_CTRL_UP | PSP_CTRL_DOWN);
		} else repeat = 0;
		prvkey = key;
	} else {
		if ( repeat ) {
			repeat_time++;
			if ((repeat_time >= 32) && ((repeat_time & 0x3) == 0))
				process_key( repeat, repeat, repeat_time );
		}
	}
	return 0;
}


