// Z80 (Zed Eight-Ty) Interface
#include "burnint.h"


#ifdef EMU_MAME_Z80
 static struct ZetExt * ZetCPUContext = NULL;
 static struct ZetExt * lastZetCPUContext = NULL;
#elif defined EMU_CZ80
 static cz80_struc* ZetCPUContext = NULL;
 static cz80_struc* lastZetCPUContext = NULL;
#endif // EMU_DOZE

static int nOpenedCPU = -1;
static int nCPUCount = 0;

unsigned char __fastcall ZetDummyReadHandler(unsigned short) { return 0; }
void __fastcall ZetDummyWriteHandler(unsigned short, unsigned char) { }
unsigned char __fastcall ZetDummyInHandler(unsigned short) { return 0; }
void __fastcall ZetDummyOutHandler(unsigned short, unsigned char) { }

#ifdef EMU_MAME_Z80

unsigned char __fastcall ZetReadIO(unsigned int a)
{
	return lastZetCPUContext->ZetIn(a);
}

void __fastcall ZetWriteIO(unsigned int a, unsigned char d)
{
	lastZetCPUContext->ZetOut(a, d);
}

unsigned char __fastcall ZetReadProg(unsigned int a)
{
	unsigned char *pr = lastZetCPUContext->pZetMemMap[0x000 + (a >> 8)];
	if (pr != NULL) return pr[a & 0xff];
	
	return lastZetCPUContext->ZetRead(a);
}

void __fastcall ZetWriteProg(unsigned int a, unsigned char d)
{
	unsigned char *pr = lastZetCPUContext->pZetMemMap[0x100 + (a >> 8)];
	if (pr != NULL) {
		pr[a & 0xff] = d;
		return;
	}
	lastZetCPUContext->ZetWrite(a, d);	
//	bprintf(PRINT_NORMAL, _T("Write %x, %x\n"), a, d);
}

unsigned char __fastcall ZetReadOp(unsigned int a)
{
	unsigned char *pr = lastZetCPUContext->pZetMemMap[0x200 + (a >> 8)];
	if (pr != NULL) return pr[a & 0xff];

	bprintf(PRINT_NORMAL, _T("Op Read %x\n"), a);
	return 0;
}

unsigned char __fastcall ZetReadOpArg(unsigned int a)
{
	unsigned char *pr = lastZetCPUContext->pZetMemMap[0x300 + (a >> 8)];
	if (pr != NULL) return pr[a & 0xff];
	
	bprintf(PRINT_NORMAL, _T("Op Arg Read %x\n"), a);	
	return 0;
}
#endif


#ifdef EMU_MAME_Z80
void ZetSetReadHandler(unsigned char (__fastcall *pHandler)(unsigned short))
{
	lastZetCPUContext->ZetRead = pHandler;
}

void ZetSetWriteHandler(void (__fastcall *pHandler)(unsigned short, unsigned char))
{
	lastZetCPUContext->ZetWrite = pHandler;
}

void ZetSetInHandler(unsigned char (__fastcall *pHandler)(unsigned short))
{
	lastZetCPUContext->ZetIn = pHandler;
}

void ZetSetOutHandler(void (__fastcall *pHandler)(unsigned short, unsigned char))
{
	lastZetCPUContext->ZetOut = pHandler;
}
#endif

#if defined EMU_CZ80
void ZetSetReadHandler(unsigned char (__fastcall *pHandler)(unsigned short))
{
	CZ80->Read_Byte = (CZ80_READ*)pHandler;
}

void ZetSetWriteHandler(void (__fastcall *pHandler)(unsigned short, unsigned char))
{
	CZ80->Write_Byte = (CZ80_WRITE*)pHandler;
}

void ZetSetInHandler(unsigned char (__fastcall *pHandler)(unsigned short))
{
	CZ80->IN_Port = (CZ80_READ*)pHandler;
}

void ZetSetOutHandler(void (__fastcall *pHandler)(unsigned short, unsigned char))
{
	CZ80->OUT_Port = (CZ80_WRITE*)pHandler;
}
#endif

void ZetNewFrame()
{


#ifdef EMU_MAME_Z80
	for (int i = 0; i < nCPUCount; i++) {
		ZetCPUContext[i].nCyclesTotal = 0;
	}
#elif defined EMU_CZ80
	for (int i = 0; i < nCPUCount; i++) {
		ZetCPUContext[i].CycleTotal = 0;
	}
#endif
}

int ZetInit(int nCount)
{


#ifdef EMU_MAME_Z80
	ZetCPUContext = (struct ZetExt *) malloc(nCount * sizeof(struct ZetExt));
	if (ZetCPUContext == NULL) return 1;
	memset(ZetCPUContext, 0, nCount * sizeof(struct ZetExt));
	
	Z80Init();
	
	for (int i = 0; i < nCount; i++) {
		ZetCPUContext[i].ZetIn = ZetDummyInHandler;
		ZetCPUContext[i].ZetOut = ZetDummyOutHandler;
		ZetCPUContext[i].ZetRead = ZetDummyReadHandler;
		ZetCPUContext[i].ZetWrite = ZetDummyWriteHandler;
		
		// TODO: Z80Init() will set IX IY F regs with default value, so get them ...
		Z80GetContext(& (ZetCPUContext[i].reg) );
	}
	
	Z80SetIOReadHandler(ZetReadIO);
	Z80SetIOWriteHandler(ZetWriteIO);
	Z80SetProgramReadHandler(ZetReadProg);
	Z80SetProgramWriteHandler(ZetWriteProg);
	Z80SetCPUOpReadHandler(ZetReadOp);
	Z80SetCPUOpArgReadHandler(ZetReadOpArg);
	
	ZetOpen(0);
	
	nCPUCount = nCount;
#elif defined EMU_CZ80
	ZetCPUContext = (cz80_struc*)malloc(nCount * sizeof(cz80_struc));
	if (ZetCPUContext == NULL) {
		return 1;
	}

	Cz80_InitFlags();

	memset(ZetCPUContext, 0, nCount * sizeof(cz80_struc));

	for (int i = 0; i < nCount; i++) {
		ZetCPUContext[i].InterruptLatch = -1;

		ZetCPUContext[i].Read_Byte = (CZ80_READ*)ZetDummyReadHandler;
		ZetCPUContext[i].Write_Byte = (CZ80_WRITE*)ZetDummyWriteHandler;
		ZetCPUContext[i].IN_Port = (CZ80_READ*)ZetDummyInHandler;
		ZetCPUContext[i].OUT_Port = (CZ80_WRITE*)ZetDummyOutHandler;
	}

	ZetOpen(0);

	nCPUCount = nCount;
#endif

	return 0;
}

void ZetClose()
{


#ifdef EMU_MAME_Z80
	// Set handlers here too
	if (nOpenedCPU >= 0)
		Z80GetContext(&(ZetCPUContext[nOpenedCPU].reg));
#endif

	nOpenedCPU = -1;
}

int ZetOpen(int nCPU)
{


#ifdef EMU_MAME_Z80
	// Set handlers here too
	Z80SetContext(&ZetCPUContext[nCPU].reg);
	lastZetCPUContext = &ZetCPUContext[nCPU];
#elif defined EMU_CZ80
//	nOpenedCPU = nCPU;
	CZ80 = &ZetCPUContext[nCPU];
	lastZetCPUContext = &ZetCPUContext[nCPU];
#endif

	nOpenedCPU = nCPU;

	return 0;
}

int ZetRun(int nCycles)
{


#ifdef EMU_MAME_Z80
	if (nCycles <= 0) return 0;
	lastZetCPUContext->nCyclesTotal += nCycles;
	lastZetCPUContext->nCyclesSegment = nCycles;
	lastZetCPUContext->nCyclesLeft = nCycles;
	
	nCycles = Z80Execute(nCycles);
	
	lastZetCPUContext->nCyclesLeft = lastZetCPUContext->nCyclesLeft - nCycles;
	lastZetCPUContext->nCyclesTotal -= lastZetCPUContext->nCyclesLeft;
	lastZetCPUContext->nCyclesLeft = 0;
	lastZetCPUContext->nCyclesSegment = 0;
	
	return nCycles;
#elif defined EMU_CZ80

	if (nCycles <= 0) {
		return 0;
	}

	CZ80->CycleTotal += nCycles;
	CZ80->CycleSegment = nCycles;
	CZ80->CycleToDo = nCycles;
	//CZ80->CycleIO = nCycles;

	nCycles = Cz80_Exec();
	//nCycles = Doze.nCyclesSegment - Doze.nCyclesLeft;

	CZ80->CycleTotal -= CZ80->CycleIO;
	CZ80->CycleIO = 0;
	CZ80->CycleSegment = 0;

	return nCycles;
#else
	return 1;
#endif 
}

void ZetRunAdjust(int nCycles)
{


#ifdef EMU_MAME_Z80
	if (nCycles < 0 && lastZetCPUContext->nCyclesLeft < -nCycles) {
		nCycles = 0;
	}

	lastZetCPUContext->nCyclesTotal += nCycles;
	lastZetCPUContext->nCyclesSegment += nCycles;
	lastZetCPUContext->nCyclesLeft += nCycles;
#elif defined EMU_CZ80
	if (nCycles < 0 && (int)CZ80->CycleIO < -nCycles) {
		nCycles = 0;
	}

	CZ80->CycleTotal += nCycles;
	CZ80->CycleSegment += nCycles;
	CZ80->CycleIO += nCycles;
#endif
}

void ZetRunEnd()
{

#ifdef EMU_MAME_Z80
	lastZetCPUContext->nCyclesTotal -= lastZetCPUContext->nCyclesLeft;
	lastZetCPUContext->nCyclesSegment -= lastZetCPUContext->nCyclesLeft;
	lastZetCPUContext->nCyclesLeft = 0;
#elif defined EMU_CZ80
	CZ80->CycleTotal -= CZ80->CycleIO;
	CZ80->CycleSegment -= CZ80->CycleIO;
	CZ80->CycleIO = 0;
#endif
}

// This function will make an area callback ZetRead/ZetWrite
int ZetMemCallback(int nStart, int nEnd, int nMode)
{

#ifdef EMU_MAME_Z80
	unsigned char cStart = (nStart >> 8);
	unsigned char **pMemMap = lastZetCPUContext->pZetMemMap;

	for (unsigned short i = cStart; i <= (nEnd >> 8); i++) {
		switch (nMode) {
			case 0:
				pMemMap[0     + i] = NULL;
				break;
			case 1:
				pMemMap[0x100 + i] = NULL;
				break;
			case 2:
				pMemMap[0x200 + i] = NULL;
				//pMemMap[0x300 + i] = NULL;
				break;
		}
	}
#elif defined EMU_CZ80
	nStart >>= CZ80_FETCH_SFT;
	nEnd += CZ80_FETCH_BANK - 1;
	nEnd >>= CZ80_FETCH_SFT;

	// Leave the section out of the memory map, so the callback with be used
	for (int i = nStart; i < nEnd; i++) {
		switch (nMode) {
			case 0:
				CZ80->Read[i] = NULL;
				break;
			case 1:
				CZ80->Write[i] = NULL;
				break;
			case 2:
				CZ80->Fetch[i] = NULL;
				break;
		}
	}
#endif

	return 0;
}

int ZetMemEnd()
{
	return 0;
}

void ZetExit()
{


#ifdef EMU_MAME_Z80
	Z80Exit();
	free( ZetCPUContext );
	ZetCPUContext = NULL;
	lastZetCPUContext = NULL;
#elif defined EMU_CZ80
	free(ZetCPUContext);
	ZetCPUContext = NULL;
	CZ80 = NULL;
#endif

	nCPUCount = 0;
}


int ZetMapArea(int nStart, int nEnd, int nMode, unsigned char *Mem)
{


#ifdef EMU_MAME_Z80
	unsigned char cStart = (nStart >> 8);
	unsigned char **pMemMap = lastZetCPUContext->pZetMemMap;

	for (unsigned short i = cStart; i <= (nEnd >> 8); i++) {
		switch (nMode) {
			case 0: {
				pMemMap[0     + i] = Mem + ((i - cStart) << 8);
				break;
			}
		
			case 1: {
				pMemMap[0x100 + i] = Mem + ((i - cStart) << 8);
				break;
			}
			
			case 2: {
				pMemMap[0x200 + i] = Mem + ((i - cStart) << 8);
				pMemMap[0x300 + i] = Mem + ((i - cStart) << 8);
				break;
			}
		}
	}
#elif defined EMU_CZ80
	int s = nStart >> CZ80_FETCH_SFT;
	int e = (nEnd + CZ80_FETCH_BANK - 1) >> CZ80_FETCH_SFT;

	// Put this section in the memory map, giving the offset from Z80 memory to PC memory
	for (int i = s; i < e; i++) {
		switch (nMode) {
			case 0:
				CZ80->Read[i] = Mem - nStart;
				break;
			case 1:
				CZ80->Write[i] = Mem - nStart;
				break;
			case 2:
				CZ80->Fetch[i] = Mem - nStart;
				CZ80->FetchData[i] = Mem - nStart;
				break;
		}
	}
	if (nMode == 2) {
		s = CZ80->PC - CZ80->BasePC;
		e = s >> CZ80_FETCH_SFT;
		CZ80->BasePC = (u32) CZ80->Fetch[e];
		CZ80->BasePCData = (u32) CZ80->FetchData[e];
		CZ80->PC = s + CZ80->BasePC;
		CZ80->PCData = s + CZ80->BasePCData;
	}
#endif

	return 0;
}

int ZetMapArea(int nStart, int nEnd, int nMode, unsigned char *Mem01, unsigned char *Mem02)
{


#ifdef EMU_MAME_Z80
	unsigned char cStart = (nStart >> 8);
	unsigned char **pMemMap = lastZetCPUContext->pZetMemMap;
	
	if (nMode != 2) {
		return 1;
	}
	
	for (unsigned short i = cStart; i <= (nEnd >> 8); i++) {
		pMemMap[0x200 + i] = Mem01 + ((i - cStart) << 8);
		pMemMap[0x300 + i] = Mem02 + ((i - cStart) << 8);
	}
#elif defined EMU_CZ80
	int s = nStart >> CZ80_FETCH_SFT;
	int e = (nEnd + CZ80_FETCH_BANK - 1) >> CZ80_FETCH_SFT;

	if (nMode != 2) {
		return 1;
	}

	// Put this section in the memory map, giving the offset from Z80 memory to PC memory
	for (int i = s; i < e; i++) {
		CZ80->Fetch[i] = Mem01 - nStart;
		CZ80->FetchData[i] = Mem02 - nStart;
	}
	s = CZ80->PC - CZ80->BasePC;
	e = s >> CZ80_FETCH_SFT;
	CZ80->BasePC = (u32) CZ80->Fetch[e];
	CZ80->BasePCData = (u32) CZ80->FetchData[e];
	CZ80->PC = s + CZ80->BasePC;
	CZ80->PCData = s + CZ80->BasePCData;
#endif
	return 0;
}

int ZetReset()
{

#ifdef EMU_MAME_Z80
	Z80Reset();
#elif defined EMU_CZ80
	Cz80_Reset();
#endif
	return 0;
}

int ZetPc(int n)
{


#ifdef EMU_MAME_Z80
	
	if (n < 0) {
		return lastZetCPUContext->reg.pc.w.l;
	} else {
		return ZetCPUContext[n].reg.pc.w.l;
	}

#elif defined EMU_CZ80
	if (n < 0) {
		return Cz80_Get_PC(CZ80);
	} else {
		return Cz80_Get_PC(&ZetCPUContext[n]);
	}
#else
	return 0;
#endif
}

int ZetBc(int n)
{


#ifdef EMU_MAME_Z80
	if (n < 0) {
		return lastZetCPUContext->reg.bc.w.l;
	} else {
		return ZetCPUContext[n].reg.bc.w.l;
	}
#elif defined EMU_CZ80
	if (n < 0) {
		return Cz80_Get_BC(CZ80);
	} else {
		return Cz80_Get_BC(&ZetCPUContext[n]);
	}
#endif
}

int ZetHL(int n)
{

#ifdef EMU_MAME_Z80

	if (n < 0) {
		return lastZetCPUContext->reg.hl.w.l;
	} else {
		return ZetCPUContext[n].reg.hl.w.l;
	}

#elif defined EMU_CZ80
	if (n < 0) {
		return Cz80_Get_HL(CZ80);
	} else {
		return Cz80_Get_HL(&ZetCPUContext[n]);
	}
#endif
}

int ZetScan(int nAction)
{
	if ((nAction & ACB_DRIVER_DATA) == 0) {
		return 0;
	}
#if defined EMU_CZ80
	char szText[] = "Z80 #0";

	for (int i = 0; i < nCPUCount; i++) {
		szText[5] = '1' + i;

		ScanVar(&ZetCPUContext[i], (u32)(&(ZetCPUContext[i].BasePC)) - (u32)(&(ZetCPUContext[i].BC)), szText);
	}
#endif


	return 0;
}

void ZetSetIRQLine(const int line, const int status)
{


#ifdef EMU_MAME_Z80

#if 0
	if (status == ZET_IRQSTATUS_ACK) {
		Z80SetIrqLine(line, 1);
		Z80Execute(0);
	} else {
		if (status == ZET_IRQSTATUS_NONE) {
			Z80SetIrqLine(0, 0);
			Z80Execute(0);
		} else {
			if (status == ZET_IRQSTATUS_AUTO) {
				Z80SetIrqLine(line, 1);
				Z80Execute(0);
				Z80SetIrqLine(0, 0);
				Z80Execute(0);
			}
		}
	}

#else

	switch ( status ) {
	case ZET_IRQSTATUS_NONE:
		Z80SetIrqLine(0, 0);
		break;
	case ZET_IRQSTATUS_ACK: 	
		Z80SetIrqLine(line, 1);
		break;
	case ZET_IRQSTATUS_AUTO:
		Z80SetIrqLine(line, 1);
		Z80Execute(0);
		Z80SetIrqLine(0, 0);
		break;
	}

#endif


#elif defined EMU_CZ80
	if (status) {
		CZ80->InterruptLatch = status;
		CZ80->IntVect = line;
		CZ80->Status |= CZ80_HAS_INT;
		CZ80->CycleSup = CZ80->CycleIO;
		CZ80->CycleIO = 0;
	} else {
		CZ80->Status &= ~CZ80_HAS_INT;
	}
#endif
}

int ZetNmi()
{


#ifdef EMU_MAME_Z80
	
	Z80SetIrqLine(Z80_INPUT_LINE_NMI, 1);
	Z80Execute(0);
	Z80SetIrqLine(Z80_INPUT_LINE_NMI, 0);
	//Z80Execute(0);

	int nCycles = 12;
	lastZetCPUContext->nCyclesTotal += nCycles;
	
#elif defined EMU_CZ80
	// Taking an NMI requires 12 cycles
	int nCycles = 12;

	CZ80->Status |= CZ80_HAS_NMI;
	CZ80->CycleSup = CZ80->CycleIO;
	CZ80->CycleIO = 0;
	CZ80->CycleTotal += nCycles;
#else
	// Taking an NMI requires 12 cycles
	int nCycles = 12;
#endif

	return nCycles;
}

int ZetIdle(int nCycles)
{


#ifdef EMU_MAME_Z80
	lastZetCPUContext->nCyclesTotal += nCycles;
#elif defined EMU_CZ80
	CZ80->CycleTotal += nCycles;
#endif

	return nCycles;
}

int ZetSegmentCycles()
{


#ifdef EMU_MAME_Z80
	return lastZetCPUContext->nCyclesSegment - lastZetCPUContext->nCyclesLeft;
#elif defined EMU_CZ80
	return CZ80->CycleSegment - CZ80->CycleIO - CZ80->CycleSup;
#endif
}

int ZetTotalCycles()
{


#ifdef EMU_MAME_Z80
	return lastZetCPUContext->nCyclesTotal - lastZetCPUContext->nCyclesLeft;
#elif defined EMU_CZ80
	return CZ80->CycleTotal - CZ80->CycleIO - CZ80->CycleSup;
#endif
}
