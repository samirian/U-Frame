import os
from ulib.USB import *

vid = '17ef'
pid = '6078'
interface = '0'

usb = USB(vid, pid, interface)

while True:
	try:
		data = usb.readInterrupt(IN1)
		if data[1] == 4:
			#if the data in the second byte = 4, then the middle button is clicked
			os.system("gedit test.txt")
			break;
	except:
		print('connection timed out')