import os
import sys
import csv

class Device:
	def __init__(self):
		self.VID = 0
		self.PID = 0
		self.minor = 0

def readCSV(filename):
	csvfile = open(filename, newline='')
	return csv.reader(csvfile, delimiter=':', quotechar='|')

tempDevicesArray = readCSV('userDefinedDevices.txt')

devicesArray = []
device_counter = 0;
for row in tempDevicesArray:
	devicesArray.append(Device())
	devicesArray[device_counter].VID = row[0]
	devicesArray[device_counter].PID = row[1]
	devicesArray[device_counter].minor = device_counter
	device_counter += 1

print("Number of devices : " + str(device_counter))

device_counter = 0

os.system("lsusb > lsusb.txt")

file = open('udriver.h', 'r')
data = file.readlines()
file.close()

appended_part = []
appended_part.append('static struct usb_device_id usb_table [] =')
appended_part.append('{')
for dev in devicesArray:
	appended_part.append('\t{USB_DEVICE(0x' + dev.VID +', 0x' + dev.PID + ')},')
appended_part.append('{}};\n')
appended_part.append('\nstatic int devCnt = ' + str(len(devicesArray)) + ';\n')
appended_part.append('\nstatic struct uframe_dev uframe_devices [' + str(len(devicesArray)) + '] = {')

for dev in devicesArray:
	appended_part.append('{.VID = 0x' + str(dev.VID) + ', .PID = 0x' + str(dev.PID) +'}, ')
appended_part.append('};\n')

file = open('uframe.h', 'w')
file.write("".join(data[0:76]))
file.write("".join(appended_part))
file.write("".join(data[81:]))
file.close()

os.system("make")
os.system("sudo insmod udriver.ko")

interface = 0



for dev in devicesArray:
	VID = dev.VID
	PID = dev.PID
	print('--------------------------------')
	print("VID : " + VID +  ", PID : "+ PID)
	target = "ID " + VID + ":" + PID

	deviceDetails = ""
	file = open('lsusb.txt', 'r')
	data = file.readlines()
	file.close()
	for line in data:
		if target in line:
			deviceDetails = line
			break

	if deviceDetails != "":
		bus = deviceDetails[4:7]
		device = deviceDetails[15:18]

		print("bus: " + bus)
		print("device: " + device)
		
		os.system("lsusb -D /dev/bus/usb/" + bus + "/" + device + " > deviceDiscriptor.txt")
		
		file = open("deviceDiscriptor.txt", "r")
		data = file.readlines()
		file.close()
		#os.system("rm deviceDiscriptor.txt")

		target = "Endpoint Descriptor:"
		ep = open("endpoints.txt", "w")
		ep.write(target + "\n")
		deviceDetails = ""
		counter = 0
		for line in data:
			if target in line:
				for i in range(counter + 1, len(data)-1) :
					ep.write(data[i])
				ep.close()
				break
			counter += 1

		
		endpointCounter = 0 
		type = ""
		direction = ""
		#creating control endpoint
		os.system("bash create_node.sh " + " " + VID + " " + PID + " " + str(interface) + " " + "Control" + " " + "{0:03}".format(0) + " " + str(dev.minor))
		target = "Endpoint Descriptor:"
		ep = open("endpoints.txt", "r")
		data = ep.readlines()
		counter = 0
		for line in data:
			if target in line:
				type = ""
				direction = ""
				for i in range(counter + 1, len(data)-1) :
					if "bEndpointAddress" in data[i]:
						endpointCounter += 1
						if "IN" in data[i]:
							direction = "{0:03}".format(1 + endpointCounter*10) #IN
						else:
							direction = "{0:03}".format(0 + endpointCounter*10) #OUT
					if "Transfer Type" in data[i]:
						if "Interrupt":
							type = "Interrupt"
						elif "Bulk":
							type = "Bulk"
						elif "Isochronous":
							type = "Isochronous"
					if type != "" and direction != "":
						os.system("bash create_node.sh " + " " + VID + " " + PID + " " + str(interface) + " " + type + " " + direction + " " + str(dev.minor))
						break

	else :
		print ("device is not connected")



os.system("rm lsusb.txt")
