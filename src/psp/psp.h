#ifndef _H_PSP_
#define _H_PSP_


#define PSP_LINE_SIZE	512
#define SCREEN_WIDTH	480
#define SCREEN_HEIGHT	272

#define GU_FRAME_ADDR(frame)		(unsigned short *)((unsigned int)frame | 0x44000000)

/* main.cpp */

extern int nGameStage;
extern int bGameRunning;
extern char currentPath[];

struct Vertex
{
	unsigned short u, v;
	unsigned short color;
	signed short x, y, z;
};


/* ui.cpp */
#define UI_COLOR	R8G8B8_to_B5G6R5(0xffc090)

extern char ui_current_path[];

int do_ui_key(unsigned int key);

void draw_ui_main();
void draw_ui_browse(bool rebuiltlist);

/* roms.cpp */

int findRomsInDir(bool force);
char * getRomsFileName(int idx);
int getRomsFileStat(int idx);

/* gui.cpp */

extern void * show_frame;
extern void * draw_frame;
extern void * work_frame;
extern void * tex_frame;

void init_gui();
void exit_gui();
void update_gui();

/* bzip */
extern char szAppRomPath[];


/*  */
int DrvInit(int nDrvNum, bool bRestore);
int DrvExit();
int InpInit();
int InpExit();
void InpDIP();

#endif	// _H_PSP_
