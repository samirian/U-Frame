import fcntl
from bitarray import bitarray
import os
import struct
import array
from .constants import *


class communication:
	def __init__(self, vid, pid, interface):
		self.vid = vid
		self.pid = pid
		self.interface = interface
		self.root = "/dev/U-Frame/" + self.vid + "/" + self.pid + "/" + self.interface + "/"
		self.interruptWordLength = 8

	def checkForEndPointNodes(self, endPoint):
		for OUTn in range(0, 150, 10):
			if os.path.exists(self.getPath(endPoint, OUTn)) == False:
				break
			print( "OUT" + str( (int)(OUTn/10) ) ) 
		for INn in range(1, 151, 10):
			if os.path.exists(self.getPath(endPoint, INn)) == False:
				break
			print( "IN" + str( (int)(OUTn/10) ) ) 

	def availableEndpoints(self, endPoint):
		if endPoint == 0:
			print("control :")
			self.checkForEndPointNodes(endPoint)
		elif endPoint == 1:
			print("bulk :")
			self.checkForEndPointNodes(endPoint)
		elif endPoint == 2:
			print("interrupt :")
			self.checkForEndPointNodes(endPoint)
		elif endPoint == 3:
			print("isochronous:")
			self.checkForEndPointNodes(endPoint)

	def recive(self, endPoint, INn, request = None, requestType = None, value = None, index = None, size = None, data = None, operation = 1):

		fd = self.getPath(endPoint, INn)
		if request == None:
			if os.path.exists(fd):
				Array = []
				file = open(fd, "r")
				for i in range(self.interruptWordLength):
					Array.append(ord(file.read(1)))
				#data = file.read(self.interruptWordLength)
				file.close()
				#return data
				return Array
			
			else:
				print("no such endpoint ,printing available nodes :")
				self.availableEndpoints(endPoint)
		else :
			file = open(fd, "wb")
			byteBuffer = bytearray(self.formRequestPacket(request, requestType, value, index, size))
			for d in data :
				byteBuffer.append(d)
			print(byteBuffer)
			#tempBuffer = array.array("B", byteBuffer)
			#buffer = array.array("c", tempBuffer.tostring())
			fcntl.ioctl(file, operation, byteBuffer, 1)
			file.close()
			return byteBuffer
				
				
	def send(self, endPoint, OUTn, request = None, requestType = None, value = None, index = None, size = None, data = None):
		fd = self.getPath(endPoint, OUTn)
		messege = bytearray()
		if os.path.exists(fd):
			if request != None:
				messege = bytearray(self.formRequestPacket(request, requestType, value, index, size))
			if data != None:
				messege.append(data)
			file = open(fd, "wb")
			print(messege)
			file.write(messege)
			file.close()
		
		else:
			print("no such endpoint ,printing available nodes :")
			self.availableEndpoints(endPoint)
			
		
	def getPath(self, endPoint, IOn):
		return self.root + endPoint + "/" + "{0:03}".format(IOn)
		
	def formRequestPacket(self, request, requestType, value, index, size):
		return bitarray("{0:08b}".format(request) + "{0:08b}".format(requestType) + "{0:<016b}".format(value) + "{0:<016b}".format(index) + "{0:<016b}".format(size)).tobytes()