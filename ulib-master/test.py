import USB

import USB
import sys
import time
from bitarray import bitarray


def callback(data):
	print(data)


usb = USB.USB(10, 10, 15)
temp = bytearray()
#control test

#turn on led
usb.writeControl(1,0xc0,0,0,16,temp)
time.sleep(2)

#turn off led
usb.writeControl(0,0xc0,0,0,16,temp)
time.sleep(1)

#read data
temp = bytearray("                ")
print(usb.readControl(2,0xC0,0,0,16,temp))

#write data in value and index
#usb.writeControl(3, 0xc0, ord("e") + (ord("t")<<8), ord("t") + (ord("s")<<8), 16)

#read data
print(usb.readControl(2,0xC0,0,0,16,temp))

#write new data
data = bytearray("wla wla         ")
usb.writeControl(4,0x40,0,0,16,data)
		
#read data
temp = bytearray("                ")
print(usb.readControl(2,0xC0,0,0,16,temp))


