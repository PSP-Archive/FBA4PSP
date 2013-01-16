#include "burner.h"

struct GameInp {
	unsigned char *pVal;  // Destination for the Input Value
	unsigned char nType;  // 0=binary (0,1) 1=analog (0x01-0xFF) 2=dip switch
	unsigned char nConst;
	unsigned char nop[2]; // 
	int nBit;   // bit offset of Keypad data
};

// Mapping of PC inputs to game inputs
struct GameInp *GameInp = NULL;
unsigned int nGameInpCount = 0;
static bool bInputOk = false;

int DoInputBlank(int bDipSwitch)
{
  unsigned int i=0; 
  struct GameInp *pgi = NULL;
  // Reset all inputs to undefined (even dip switches, if bDipSwitch==1)
  if (GameInp==NULL) return 1;
  
  int bVert = BurnDrvGetFlags() & BDF_ORIENTATION_VERTICAL;

  // Get the targets in the library for the Input Values
  for (i=0,pgi=GameInp; i<nGameInpCount; i++,pgi++)
  {
    struct BurnInputInfo bii;
    memset(&bii,0,sizeof(bii));
    BurnDrvGetInputInfo(&bii,i);
    
    pgi->nType = bii.nType; // store input type
    pgi->pVal  = bii.pVal;  // store input pointer to value
    pgi->nBit  = -1;

    //if ( (bDipSwitch==0) && (bii.nType==2) ) {
    //	continue; // Don't blank the dip switches
    //}

    if (pgi->nType == BIT_DIGITAL) 
    {  // map my keypad def
	    if (strcmp(bii.szInfo, "p1 coin") == 0)		// PSP_CTRL_SELECT = 0x000001,
	    	pgi->nBit  = 0;
	    else
	    if (strcmp(bii.szInfo, "p1 start") == 0)	// PSP_CTRL_START = 0x000008,
	    	pgi->nBit  = 3;
	    else
	    if (strcmp(bii.szInfo, "p1 up") == 0)		//PSP_CTRL_UP = 0x000010,
	    	pgi->nBit  = (bVert) ? 5 : 4;
	    else
	    if (strcmp(bii.szInfo, "p1 down") == 0)		// PSP_CTRL_DOWN = 0x000040,
	    	pgi->nBit  = (bVert) ? 7 : 6;
	    else
	    if (strcmp(bii.szInfo, "p1 left") == 0)		// PSP_CTRL_LEFT = 0x000080,
	    	pgi->nBit  = (bVert) ? 4 : 7;
	    else
	    if (strcmp(bii.szInfo, "p1 right") == 0)	// PSP_CTRL_RIGHT = 0x000020,
	    	pgi->nBit  = (bVert) ? 6 : 5;
	    else
	    if (strcmp(bii.szInfo, "p1 fire 1") == 0)	// PSP_CTRL_CROSS = 0x004000,
	    	pgi->nBit  = 14;
	    else
	    if (strcmp(bii.szInfo, "p1 fire 2") == 0)	// PSP_CTRL_CIRCLE = 0x002000,
	    	pgi->nBit  = 13;
	    else
	    if (strcmp(bii.szInfo, "p1 fire 3") == 0)	// PSP_CTRL_SQUARE = 0x008000
	    	pgi->nBit  = 15;
	    else
	    if (strcmp(bii.szInfo, "p1 fire 4") == 0)	// PSP_CTRL_TRIANGLE = 0x001000,
	    	pgi->nBit  = 12;
	    else
	    if (strcmp(bii.szInfo, "p1 fire 5") == 0)	// PSP_CTRL_RTRIGGER = 0x000200
	    	pgi->nBit  = 9;
	    else
	    if (strcmp(bii.szInfo, "p1 fire 6") == 0)	// PSP_CTRL_LTRIGGER = 0x000100,
	    	pgi->nBit  = 8;
	    	

	} else
	if (pgi->nType == BIT_DIPSWITCH) 
	{  // black DIP switch
	
	//	if (pgi->pVal)
	//		* (pgi->pVal) = 0;
	
    }

#if 0
if (pgi->pVal != NULL)
	printf("GI(%02d): %-12s 0x%02x 0x%02x %-12s, [%d]\n", i, bii.szName, bii.nType, *(pgi->pVal), bii.szInfo, pgi->nBit );
else
	printf("GI(%02d): %-12s 0x%02x N/A  %-12s, [%d]\n", i, bii.szName, bii.nType, bii.szInfo, pgi->nBit );
#endif
   
  }
  return 0;
}

int InpInit()
{
  unsigned int i=0; 
  int nRet=0;
  bInputOk = false;
  // Count the number of inputs
  nGameInpCount=0;
  for (i=0;i<0x1000;i++) {
    nRet = BurnDrvGetInputInfo(NULL,i);
    if (nRet!=0) {   // end of input list
    	nGameInpCount=i; 
    	break; 
    }
  }
  
  // Allocate space for all the inputs
  GameInp = (struct GameInp *)malloc(nGameInpCount * sizeof(struct GameInp));
  if (!GameInp) return 1;
  memset(GameInp, 0, nGameInpCount * sizeof(struct GameInp));
 
  DoInputBlank(1);
  bInputOk = true;
  
  return 0;
}

int InpExit()
{
  if (GameInp!=NULL) free(GameInp);
  GameInp = NULL;
  nGameInpCount = 0;
  bInputOk = false;
  return 0;
}

int InpMake(unsigned int key)
{
	if (!bInputOk) return 1;

#if 0	
	static int skip = 0;
	skip ++;
	if (skip > 2) skip = 0;
	if (skip != 1) return 1;
#endif
	
	unsigned int i=0; 
	unsigned int down = 0;
	struct GameInp *pgi;
	for (i=0,pgi=GameInp; i<nGameInpCount; i++,pgi++) {
		if (pgi->pVal == NULL) continue;
		
		if ( pgi->nBit >= 0 ) {
			down = key & (1U << pgi->nBit);
			
			if (pgi->nType!=1) {
				// Set analog controls to full
				if (down) *(pgi->pVal)=0xff; else *(pgi->pVal)=0x01;
			} else {
				// Binary controls
				if (down) *(pgi->pVal)=1;    else *(pgi->pVal)=0;
			}

			//if ( *(pgi->pVal) != 0 ) printf("down key %08x (%d %d)\n", key, i, *(pgi->pVal));
		} else {
			// dip switch ...
			
			*(pgi->pVal) = pgi->nConst;
		}
	}
	
	return 0;
}

void InpDIPSetOne(int dipOffset, struct BurnDIPInfo * bdi)
{
	struct GameInp* pgi = GameInp + bdi->nInput + dipOffset;
	
	printf( "0x%p -> 0x%02x  \n", pgi->pVal, *( pgi->pVal) );
	if ( pgi->pVal ) {
		*(pgi->pVal) &= ~(bdi->nMask);
		*(pgi->pVal) |= (bdi->nSetting & bdi->nMask);
	}
	printf( "0x%p -> 0x%02x  \n", pgi->pVal, *( pgi->pVal) );
	//struct BurnInputInfo bii;
	//memset(&bii,0,sizeof(bii));
    //if (BurnDrvGetInputInfo(&bii, bdi->nInput + dipOffset) == 0) {
    	//printf("%s\n", bii.szName);
    //}
	//printf(" DIP Offset 0x%02x\n", bdi->nInput + dipOffset);
}

static unsigned char hex2int(char c)
{
	switch( c ){
	case '1': return  1;
	case '2': return  2;
	case '3': return  3;
	case '4': return  4;
	case '5': return  5;
	case '6': return  6;
	case '7': return  7;
	case '8': return  8;
	case '9': return  9;
	case 'a': 
	case 'A': return 10;
	case 'b': 
	case 'B': return 11;
	case 'c': 
	case 'C': return 12;
	case 'd': 
	case 'D': return 13;
	case 'e': 
	case 'E': return 14;
	case 'f': 
	case 'F': return 15;
//	case '0': return  0;
	default : return  0;
	}
}

void InpDIP()
{
	
	struct BurnDIPInfo bdi;
	struct GameInp* pgi;
	struct BurnInputInfo bii;
	
	int i;
	
	// get dip switch offset 
	int nDIPOffset = 0;
	for (i = 0; BurnDrvGetDIPInfo(&bdi, i) == 0; i++)
		if (bdi.nFlags == 0xF0) {
			nDIPOffset = bdi.nInput;
			break;
		}
	//printf("nDIPOffset = %d\n", nDIPOffset);
	
	char InputDIPConfigFileName[256];
	sprintf(InputDIPConfigFileName, "roms/%s.ini", BurnDrvGetTextA(DRV_NAME));
	FILE * fp = fopen(InputDIPConfigFileName, "r");
	
	char IniLine[256];
	char FindName[64];

	i = 0;
	while (BurnDrvGetDIPInfo(&bdi, i) == 0) {
		if (bdi.nFlags == 0xFF) {
			pgi = GameInp + bdi.nInput + nDIPOffset;
			pgi->nConst = (pgi->nConst & ~bdi.nMask) | (bdi.nSetting & bdi.nMask);
			
			BurnDrvGetInputInfo(&bii, bdi.nInput + nDIPOffset);
			sprintf(FindName, "\"%s\"", bii.szName);
			
			if (fp) {
				fseek(fp, 0, SEEK_SET);
				while ( fgets( IniLine, 255, fp ) )
					if (strstr(IniLine, FindName)) {
						char *p = strstr(IniLine, "constant 0x");
						if ( p )
							pgi->nConst = ((hex2int(p[11]) << 4) | hex2int(p[12])) & bdi.nMask;
					}
			}
			
		}
		i++;
	}
	
	if (fp) fclose(fp);
	
	InpMake( 0 );

#if 0	
	bool bContinue = false;
	bool bFreePlay = false;
	
	// find dip need set to default
	i = 0;
	while (BurnDrvGetDIPInfo(&bdi, i) == 0) {
		if ((bdi.nFlags & 0xF0) == 0xF0) {
		   	if (bdi.nFlags == 0xFE || bdi.nFlags == 0xFD) {
		   		if (bdi.szText) {
		   			
		   			printf("DIP: %3d %s\n", i, bdi.szText);
		   			
		   			if (strcmp(bdi.szText, "Continue") == 0)
		   				//bContinue = true;
		   				;
		   			else
		   			if (strcmp(bdi.szText, "Free play") == 0)
		   				// neogeo free play disable
		   				//bFreePlay = true;
		   				;
		   			else
		   			;
		   		} else printf("DIP: %3d [null]\n", i);
			}
			i++;
		} else {
			if ( bdi.szText ) {
				
				printf("     %3d %s", i, bdi.szText);
				
				struct GameInp* pgi = GameInp + bdi.nInput + nDIPOffset;
				
				if ( pgi->pVal ) {
					
					if ((*(pgi->pVal) & bdi.nMask) == bdi.nSetting )
						printf(" <<<");
				
				}
				
				
				printf("\n");
				
				if (bFreePlay && (strcmp(bdi.szText, "Off") == 0)) {
					// printf(" --- Free play set to 'Off'\n");
					InpDIPSetOne( nDIPOffset, &bdi );
					bFreePlay = false;
				} else
				if (bContinue && (strcmp(bdi.szText, "On") == 0)) {
					// printf(" --- Continue set to 'On'\n");
					InpDIPSetOne( nDIPOffset, &bdi );
					bContinue = false;
				}
				
			}
		
			i += (bdi.nFlags & 0x0F);
		}
	}
#endif
}

