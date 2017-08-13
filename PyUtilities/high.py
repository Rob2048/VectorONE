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

import socket
from socket import *

runCamera = False
serverSocket = None
serverFile = None

#------------------------------------------------------------------------------
# Camera Output.
#------------------------------------------------------------------------------
class CamOutput(object):
	def __init__(self):
		self.size = 0
		self.lastFrameTime = 0
		self.adjusting = 0

	def write(self, s):
		global runCamera
		global serverFile
		global camera

		frameSize = len(s)
		self.size += frameSize
		frameTime = camera.frame.timestamp

		try:
			serverFile.write(s)
		except:
			print('Lost Server Connection')			
			runCamera = False
			serverFile = None
			
		if frameTime != None:
			diff = frameTime - self.lastFrameTime
			print('{} {} {}'.format(camera.frame.index, diff, frameSize))
			self.lastFrameTime = frameTime

	def flush(self):
		print('{} Bytes Written'.format(self.size))

#------------------------------------------------------------------------------
# Camera & Main Loop.
#------------------------------------------------------------------------------
print('High FPS Capture')
print('Searching For Server')

broadcastSocket = socket(AF_INET, SOCK_DGRAM)
broadcastSocket.bind(('', 45454))

def FindServer():

	return '192.168.1.100'

	# NOTE: We need to drain the socket as old broadcast msgs remain.
	broadcastSocket.setblocking(0)
	
	while True:
		try:
			recvData = broadcastSocket.recv(1024)
		except:
			break;
	
	broadcastSocket.settimeout(1)

	while True:
		try:
			print('Waiting for Server IP')
			msg = broadcastSocket.recvfrom(1024)
			r = re.search('KineticSynth:(\d+.\d+.\d+.\d+)', msg[0])

			if r != None:		
				print 'Found Server IP: {} from {}'.format(r.group(1), msg[1])
				return r.group(1)
		except:			
			continue

camera = picamera.PiCamera(sensor_mode = 6, clock_mode = 'raw')
camera.sensor_mode = 6
camera.rotation = 180
camera.resolution = (1000, 700)
camera.framerate = 100
camera.awb_mode = 'off'
camera.awb_gains = (1.1, 1.1)
camera.exposure_mode = 'sports'
camera.framerate_delta = 0
camera.shutter_speed = 7000
camera.iso = 800
#camera.shutter_speed = 23000
#camera.iso = 800

while True:
	serverIP = FindServer()
	serverSocket = socket()

	try:
		serverSocket.connect((serverIP, 8000))
		serverSocket.setsockopt(IPPROTO_TCP, TCP_NODELAY, 1)
	except:
		print('Cant Connect')
		continue
	
	serverFile = serverSocket.makefile('wb')
	print('Connected to Server')
	
	runCamera = True
	camera.start_recording(CamOutput(), format='h264', quality=25, profile='high', level='4.2')
	while runCamera:
		time.sleep(1.0)

	print('Stopped Camera')
	camera.stop_recording()