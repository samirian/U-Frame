import os
import sys


VID = sys.argv[1]
PID = sys.argv[2]

file = open('uframe.h', 'r')
data = file.readlines()
file.close()
data[5] = '#define VID 0x' + VID + '\n'
data[6] = '#define PID 0x' + PID + '\n'
file = open('uframe.h', 'w')
file.write("".join(data))
file.close()

os.system("make")
os.system("sudo insmod udriver.ko")

interface = 0
os.system("lsusb > lsusb.txt")
file = open('lsusb.txt', 'r')
data = file.readlines()
file.close()
os.system("rm lsusb.txt")
target = "ID " + VID + ":" + PID

deviceDetails = ""
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
	os.system("bash create_node.sh " + " " + VID + " " + PID + " " + str(interface) + " " + "Control" + " " + "{0:03}".format(0) + " " + str(endpointCounter))

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
					os.system("bash create_node.sh " + " " + VID + " " + PID + " " + str(interface) + " " + type + " " + direction + " " + str(endpointCounter))
					break
		counter += 1


else :
	print ("device is not connected")



