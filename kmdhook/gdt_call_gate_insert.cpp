/*
	This function gives us the indication 
	that we made to ring 0, we just print out
	a debug message that the function was called
*/
void say_something()
{
	DbgPrint("We are running in ring0 from ring3.....congrats");
	return;
}

/*
	This function will be us to call our call
	gate in the GDT. Some things to note here:
	1) We are going to set up our own prologue, and 
	   epilogue, since we are making this like a system
	   interrupt, we don't want the compiler messing with
	   things
*/
void __declspec(naked) callgateproc()
{
	//PROLOGUE code
	__asm
	{
		pushad;		//push EAX, ECX, EBX, EDX, EBP, ESP, ESI, EDI
		pushfd;		//push EFLAGS
		cli;		//disable interrupts
		push fs;	//save fs
		
		//save 0x30 to fs
		mov bx, 0x30;
		mov fs, bx;

		push ds;
		push es;

		call say_something;	//call our special function to let us know we made it into ring0
	}

	//EPILOGUE code
	__asm
	{
		//restore the registers and enable the interrrupts
		pop es;
		pop ds;
		pop fs;
		sti;	//enable interrupts
		popfd;
		popad;
		retf;		//note we would retf x, if we passed arguments(x == # of args)
	}
}


/*
	return the base address of the GDTR
*/

PSEG_DESCRIPTOR getgdtbaseaddress()
{
	GDTR gdtr;
	__asm
	{
		SGDT gdtr;	//this function returns the contents of gdt
	}
	return (PSEG_DESCRIPTOR)gdtr.baseaddress;
}


/*
	return the number of entries in the gdt
*/
DWORD32 getnumberofgdtentries()
{
	GDTR gdtr;
	__asm
	{
		SGDT gdtr;	//this function returns the contents of gdt
	}
	return gdtr.nbytes/8;	//return the number of entries in the gdt, each entry is 8 bytes
}


/*
	Build the call gate to insert into the GDT
*/
CALL_GATE_DESCRIPTOR buildcallgate(BYTE *procaddr)
{
	DWORD32 address;
	CALL_GATE_DESCRIPTOR cg;

	address = (DWORD32)procaddr;

	//assemble our crafted call gate
	cg.selector = KGDT_R0_CODE;	// we are going to make this a ring0 call gate
	cg.argcount = 0;	//our function will not have any agruments
	cg.zeroes = 0;
	cg.type = 0xC;	//the descriptor type is going to be a 32 bit call gate
	cg.sflag = 0;	//0 = system descriptor
	cg.dp1 = 0x3;		//can be called by ring3 code
	cg.pflag = 0;		//code is always in memory
	cg.offset_00_15 = (WORD)(0x0000FFFF & address);	//set the bottom 16 bits of the address
	address = address >> 16;	//right shift by 16 bits to store the top 16 bits
	cg.offset_16_31 = address;	//store the top 16 bits

	return cg;
}


/*
	Inject the call gate into the GDT
*/
CALL_GATE_DESCRIPTOR injectcallgate(CALL_GATE_DESCRIPTOR cg)
{
	PSEG_DESCRIPTOR gdt;
	PSEG_DESCRIPTOR gdtentry;
	PCALL_GATE_DESCRIPTOR oldcgptr;
	CALL_GATE_DESCRIPTOR oldcg;

	gdt = getgdtbaseaddress();	//return the base addr of the gdt
	oldgdtptr = (PCALL_GATE_DESCRIPTOR)&(gdt[100]);	//set the oldptr to the 100th index in the gdt
	oldcg = *oldgdtptr;	//save the contents of the oldgdt
	gdtentry = (PSEG_DESCRIPTOR)&cg;	//set the new gdt enrty
	gdt[100] = *gdtentry;	//inject our crafted call gate

	return oldcg;
}
