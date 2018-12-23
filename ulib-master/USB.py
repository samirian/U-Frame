import Communication
import threading
import time


# 0 -> control (SETUP) endpoint
# 1 -> IN endpoint
# 0 -> OUT endpoint

CONTROL = "Control"
BULK = "Bulk"
INTERRUPT = "Interrupt"
ISOCHRONOUS = "Isochronous"

IN0 = 1
IN1 = 11
IN2 = 21
IN3 = 31
IN4 = 41
IN5 = 51
IN6 = 61
IN7 = 71
IN8 = 81
IN9 = 91
IN10 = 101
IN11 = 111
IN12 = 121
IN13 = 131
IN14 = 141
IN15 = 151

OUT0 = 0
OUT1 = 10
OUT2 = 20
OUT3 = 30
OUT4 = 40
OUT5 = 50
OUT6 = 60
OUT7 = 70
OUT8 = 80
OUT9 = 90
OUT10 = 100
OUT11 = 110
OUT12 = 120
OUT13 = 130
OUT14 = 140
OUT15 = 150

IOCTL_INTERRUPT_INTERVAL = 0
IOCTL_INTERRUPT_LENGTH = 0


class USB:
	def __init__(self, vid, pid, interface):

		self.vid = vid
		self.pid = pid
		self.interface = interface
		self.comm = Communication.communication(vid, pid, interface)
		#self.interruptInterval = self.comm.ioctl(vid, pid, interface, 0, 0, 0, 0, 0, 0, IN0, IOCTL_INTERRUPT_INTERVAL)
		self.interruptInterval = 500

	def writeInterruptHandler(self, OUTn, interval = None, callBackFunction = None):
		if interval == None:
			interval = self.interruptInterval
		thread = threading.Thread(target = self.writeInterruptCaller, args = (INn, interval, callBackFunction,))
		thread.start()
		return thread
		
		
	def writeInterruptCaller(self, OUTn, interval, callBackFunction = None):
		while True :
			data = callBackFunction()
			self.writeInterrupt(len(data), data, OUTn)
			time.sleep(interval/1000)


	def writeInterrupt(self, size, data, OUTn):
	    	self.comm.send(INTERRUPT, OUTn, size, data)


	def readInterruptHandler(self, INn, interval = None, callBackFunction = None):
		if interval == None:
			interval = self.interruptInterval
		thread = threading.Thread(target = self.readInterruptCaller, args = (INn, interval, callBackFunction,))
		thread.start()
		return thread


	def readInterruptCaller(self, INn, interval, callBackFunction = None):
		while True :
			data = self.readInterrupt(INn)
			callBackFunction(data)
			time.sleep(interval/1000.0)


	def readInterrupt(self, INn):
	    	data = self.comm.recive(INTERRUPT, INn)
	    	return data

	
	def writeBulk(self, OUTn, size, data):
	    	self.comm.sendData(BULK, OUTn, size, data)


	def readBulk(self, INn):
	    	data = self.comm.recive(BULK, INn)
	    	return data


	def writeControl(self, request, requestType, value, index, size, data = None):
		self.comm.send(CONTROL, 1, request, requestType, value, index, size, data)
		
	def readControl(self, request, requestType, value, index, size, data):
		return self.comm.recive(CONTROL, 0, request, requestType, value, index, size, data)
	
	
def callback1(data):
	print("2")


def callback2(data):
	print("1")


if __name__ == "__main__":
	usb = USB(10, 10, 15)
	usb.writeBulk(OUT15, 8, 65)
	print(usb.readBulk(IN1))
	thread1 = usb.readInterruptHandler(IN1, callBackFunction = callback1)
	thread2 = usb.readInterruptHandler(IN1, callBackFunction = callback2)

	time.sleep(2)
