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

#------------------------------------------------------------------------------
# Camera Output.
#------------------------------------------------------------------------------
class CamOutput(object):
	def __init__(self):
		self.size = 0
		self.lastFrameTime = 0
		self.adjusting = 0

	def write(self, s):
		global camera
		global processQueue

		try:
			frameSize = len(s)
			self.size += frameSize
			frameTime = camera.frame.timestamp

			if frameTime == None:
				print('SHOULD NEVER HAPPEN')

			diff = frameTime - self.lastFrameTime
			#print('{} {} {}'.format(camera.frame.index, diff, frameSize))

			t0 = time.time()
			# NOTE: Will block until previously pushed frame has been completed.
			blobCount = blobdetect.getblobcount()
			# NOTE: Takes about 1ms to copy buffer, should double buffer in C code.
			blobdetect.pushframe(s)
			t0 = time.time() - t0
			print('copy {}ms {}'.format(t0 * 1000.0, blobCount))
			
			if diff > 10100:
				print('FRAME SKIPPED {}'.format(diff))

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
camera.framerate = 100
camera.awb_mode = 'off'
camera.awb_gains = (1.1, 1.1)
camera.exposure_mode = 'sports'
camera.framerate_delta = 0
camera.shutter_speed = 7000
camera.iso = 800

while True:
	
	#camera.start_recording(CamOutput(), format='h264', quality=25, profile='high', level='4.2', inline_headers=True, intra_period=50)
	camera.start_recording(CamOutput(), format='yuv')
	while True:
		time.sleep(1.0)

	print('Stopped Camera')
	camera.stop_recording()