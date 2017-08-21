#------------------------------------------------------------------------------
# Marker Tracker.
#------------------------------------------------------------------------------
import io
from io import BytesIO
import struct
import time
import picamera
import sys
import select
import threading
import pprint
import numpy as np
import os
import re
import subprocess
import zlib
import Queue
import socket
from socket import *
import multiprocessing
import blobdetect

#------------------------------------------------------------------------------
# Processing thread.
#------------------------------------------------------------------------------
'''
processQueue = Queue.Queue()

class DetectorThread(threading.Thread):
	def __init__(self):
		super(DetectorThread, self).__init__()
		self.daemon = True
		self.start()

	def run(self):
		print('Starting detector thread.')

		while True:
			processQueue.get()
			#print('Got Frame {}'.format(processQueue.qsize()))

			try:
				t0 = time.time()
				#time.sleep(0.008)
				for x in range (0, 40000):
					pass

				#blobdetect.detect()
				t0 = time.time() - t0
				print('Process Time {}'.format(t0 * 1000))
			except:
				continue

DetectorThread()
'''

'''
wpQueue = multiprocessing.Queue()
wp = multiprocessing.Process(target=worker.worker, args=(wpQueue,))
wp.daemon = True
wp.start()
'''

broadcastSocket = socket(AF_INET, SOCK_DGRAM)
#broadcastSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
broadcastSocket.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)

#------------------------------------------------------------------------------
# Camera Output.
#------------------------------------------------------------------------------
class CamOutput(object):
	def __init__(self):
		self.size = 0
		self.lastFrameTime = 0
		self.adjusting = 0
		self.average = 0
		self.averageCount = 0
		self.ca = 0

	def write(self, s):
		global camera
		global processQueue
		global broadcastSocket
		global camTimeOffset

		try:
			frameSize = len(s)
			self.size += frameSize
			frameTime = camera.frame.timestamp

			if frameTime == None:
				frameTime = 0
			else:
				frameTime -= camTimeOffset

			diff = frameTime - self.lastFrameTime

			#err = int(frameTime / 9970.72 + 0.5)
			err = frameTime / 9970.74175
			print('{} {} {} {} {}'.format(camera.frame.index, diff, frameTime, err, self.ca))

			if diff > 0 and diff < 10100:
				self.average += float(diff)
				self.averageCount += 1
				self.ca = self.average / float(self.averageCount)

			#broadcastSocket.sendto('test', ('192.168.1.255', 45455))
			broadcastSocket.sendto('{}'.format(frameTime), ('255.255.255.255', 45455))

			#if diff > 10100:
				#print('FRAME SKIPPED {}'.format(diff))

			self.lastFrameTime = frameTime
			
		except Exception,e:
			print "Ex:" + str(e)
			traceback.print_exc()

	def flush(self):
		print('{} Bytes Written'.format(self.size))

#------------------------------------------------------------------------------
# Camera & Main Loop.
#------------------------------------------------------------------------------
print('High FPS Capture')


blobdetect.startworkers()

camera = picamera.PiCamera(sensor_mode = 6, clock_mode = 'raw')
camera.sensor_mode = 6
camera.rotation = 180
camera.resolution = (1024, 704)
camera.framerate = 40
camera.awb_mode = 'off'
camera.awb_gains = (1.1, 1.1)
camera.exposure_mode = 'sports'
camera.framerate_delta = 0
camera.shutter_speed = 7000
camera.iso = 800

camTimeOffset = camera.timestamp

print(str(camTimeOffset))

while True:
	
	#camera.start_recording(CamOutput(), format='h264', quality=25, profile='high', level='4.2', inline_headers=True, intra_period=50)
	camera.start_recording(CamOutput(), format='yuv')
	#print('Start Time {0:.2f}ms'.format(t))

	while True:
		time.sleep(1.0)

	print('Stopped Camera')
	camera.stop_recording()