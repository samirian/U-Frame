import fcntl
from bitarray import bitarray
import os
import struct
import array
import threading
import time
from helper import Helper
from config import *


class Communication:
    def __init__(self, path):
        if os.path.exists(path):
            self.path = path
        else:
            raise ValueError("Path does not exist")

    def send(self, data):
        file = open(self.path, "w")
        file.write(data)
        file.close()

    def receive(self, size=0):
        try:
            file = open(self.path, "r")
            fd = file.fileno()
            data = os.read(fd, 16)
            file.close()
            return data
        except Exception as e:
            print(e)
            return None

    def device_control(self, operation, buffer):
        file = open(self.path, "r")
        fcntl.ioctl(file, operation, buffer, 1)
        file.close()
        return buffer


class Bulk(Communication):
    def __init__(self, vid, pid, interface, endpoint, endpoint_type="Bulk"):
        self.path = "/home/sayed/dev/" + vid + "/" + pid + "/" + interface + "/" + endpoint_type + "/" + "{0:03}".format(
            endpoint)
        Communication.__init__(self, self.path)
        buffert = array.array("i", struct.pack("iiiii", 0, 0, 0, 0, 0))
        buffer = self.device_control(3, buffert)
        self.type = buffer[0]
        print(self.type)
        self.direction = buffer[1]
        print(self.direction)
        self.endpoint_address = buffer[2]
        print(self.endpoint_address)
        self.interval = buffer[3]
        print(self.interval)
        self.buffer_size = buffer[4]
        print(self.buffer_size)


class Interrupt(Bulk):
    def __init__(self, vid, pid, interface, endpoint):
        Bulk.__init__(self, vid, pid, interface, endpoint, "Interrupt")

    def write_interrupt_handler(self, callback_function=None):
        thread = threading.Thread(target=self.__write_interrupt_caller, args=(self.path, self.interval, callback_function,))
        thread.start()
        return thread

    def __write_interrupt_caller(self, callback_function=None):
        while True:
            data = callback_function()
            self.send(data)
            time.sleep(self.interval / 1000.0)

    def read_interrupt_handler(self, callback_function=None):
        thread = threading.Thread(target=self.__read_interrupt_caller, args=(callback_function,))
        thread.start()
        return thread

    def __read_interrupt_caller(self, callback_function=None):
        while True:
            data = self.receive()
            callback_function(data)
            time.sleep(self.interval / 1000.0)


class Control(Communication):
    def __init__(self, vid, pid, interface, endpoint):
        self.path = "/home/sayed/dev/" + vid + "/" + pid + "/" + interface + "/" + "Control" + "/" + "{0:03}".format(
            endpoint)
        Communication.__init__(self, self.path)

    def form_request_packet(self, request, request_type, value, index, size):

        return bitarray(
            "{0:08b}".format(request) + "{0:08b}".format(request_type) + "{0:<016b}".format(value) +
            "{0:<016b}".format(index) + "{0:<016b}".format(size)).tobytes()

    def send(self, request, request_type, value, index, size, data):
        messege = ""
        req = self.form_request_packet(request, request_type, value, index, size)
        messege = messege + str(req)
        messege = messege + data
        Communication.send(self, messege)

    def receive(self, request, request_type, value, index, size, data):
        buffer = array.array("c", struct.pack("BBHHH{width}s".format(width=size), request, request_type, value, index, size, data))
        return Communication.device_control(self, IOCTL_CONTROL_READ, buffer)[8:]


