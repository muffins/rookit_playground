
//---------------MACROS------------------------------
#define KGDT_R0_CODE 0x8 // address to the selector to windows code segment at ring0
#define INDEX_NUMBER_INSERT 100	//this will be the index into the gdt where we will 
								//place our crafted call gate

//-----------------variables--------------------------



//------------------structures-------------------------
#pragma pack(1)	//We use this compiler directive to align the structure on a 1 bit boundary, 
//instead of a 4 byte boundary (data bus is 32 bits)
typedef struct _GDTR
{
	WORD nbytes;	//the number of bytes in the GDT, to find the # of selectors divide this # by 8
	DWORD32 baseaddr;	//this is the base address of the GDT
}GDTR;
#pragma pack()

#pragma pack(1)	//We use this compiler directive to align the structure on a 1 bit boundary, 
//instead of a 4 byte boundary (data bus is 32 bits)

//Define our seg selector structure, which points to a seg_selector in the gdt
typedef struct _SELECTOR
{
	WORD rpl:2;	//Request privilege level 0 = ring0
	WORD t1:1;	//table indicator 0 = gdt
	WORD index:13;	//array index into the table
}SELECTOR;
#pragma pack()

#pragma pack(1)	//We use this compiler directive to align the structure on a 1 bit boundary, 
//instead of a 4 byte boundary (data bus is 32 bits)

//define a segment selector in the gdt
typedef struct _SEG_DESCRIPTOR
{
	WORD size_00_15;	//segment size, first 15 bits
	WORD baseaddress_00_15;	//linear base addr of gdt, first 15 bits
	WORD baseaddress_16_23:8;	//base addr of the gdt, next 8 bits
	WORD type:4;	//descriptor type, code or data
	WORD sflag:1;	//0 = system segment, 1 = code/data
	WORD dpl:2;		//descriptor privilege level = 0x0-0x3
	WORD pflag:1;	//1 = segment present in memory
	WORD size_16_19:4;	//seg size, part 2
	WORD notused:1;		//not used(0)
	WORD lflag:1;		//l flag(0)
	WORD db:1;		//default size for operands and addresses
	WORD gflag:1;	//granularity (1=4kb, 0=1 byte)
	WORD baseaddress_24_31:8;	//base address part 3
}SEG_DESCRIPTOR, *PSEG_DESCRIPTOR;
#pragma pack()

#pragma pack(1)	//We use this compiler directive to align the structure on a 1 bit boundary, 
//instead of a 4 byte boundary (data bus is 32 bits)

//this will be our call gate descriptor, which is a special kind of seg descriptor for code
typedef struct _CALL_GATE_DESCRIPTOR
{
	WORD offset_00_15;	//procedure address
	WORD selector;		//specify the code segment, for this we will use
						//KGDT_R0_CODE
	WORD argcount:5;	//the arguments into the procedure the code selector will describe
	WORD zeroes:3;		//
	WORD type:4;		//descriptor type, 32 bit call gate (in binary that's 1100)
	WORD sflag:1;		//0 = system segment
	WORD dp1:2;			//dpl required by caller through gate (11 = 0x3)
	WORD pflag:1;		//pflag, 1=segment in memory
	WORD offset_16_31;	//procedure address, high word
}CALL_GATE_DESCRIPTOR, *PCALL_GATE_DESCRIPTOR;
#pragma pack()