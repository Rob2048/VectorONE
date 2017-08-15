#------------------------------------------------------------------------------
# Marker Tracker.
#------------------------------------------------------------------------------
import io
from io import BytesIO
import socket
from socket import *
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
import RPIO.PWM as PWM
import Queue
import traceback
import blobdetect

version = 104

# Current Camera State
camRunning = False
camFps = 40
camSyncTime = False
camMode = 0

# Net Command Request State
setRunCamera = False
setCamFps = 40
setSyncTime = False
setCamMode = 0

# Net
netSend = Queue.Queue()
serverFile = None

#------------------------------------------------------------------------------
# Helper functions.
#------------------------------------------------------------------------------
def GetSerial():
  cpuserial = "00000000"

  try:
    f = open('/proc/cpuinfo','r')
    for line in f:
      if line[0:6]=='Serial':
        cpuserial = line[18:26]
    f.close()
  except:
    pass

  return int(cpuserial, 16)

#------------------------------------------------------------------------------
# RGB LED Setup.
#------------------------------------------------------------------------------
print('Starting RGB LED')
#PWM.set_loglevel(PWM.LOG_LEVEL_DEBUG)
PWM.set_loglevel(PWM.LOG_LEVEL_ERRORS)

PWM.setup(pulse_incr_us=10)
PWM.init_channel(0, subcycle_time_us=10000)
PWM.print_channel(0)

BLUE_LED = 2
RED_LED = 3
GREEN_LED = 4

PWM.add_channel_pulse(0, BLUE_LED, 0, 1000)
PWM.add_channel_pulse(0, RED_LED, 0, 1000)
PWM.add_channel_pulse(0, GREEN_LED, 0, 1000)

def SetRGB(r, g, b):
	PWM.add_channel_pulse(0, RED_LED, 0, int(500 + (1.0 - r) * 500))
	PWM.add_channel_pulse(0, GREEN_LED, 0, int(500 + (1.0 - g) * 500))
	PWM.add_channel_pulse(0, BLUE_LED, 0, int(500 + (1.0 - b) * 500))

#------------------------------------------------------------------------------
# Time Sync.
#------------------------------------------------------------------------------
def SyncTime(ServerSocket, SocketFile):
	try:
		print('Syncing Time')
		SetRGB(0.1, 0.0, 0.0)

		timeErrs = np.zeros((30, 1))
		latencyErrs = np.zeros((30, 1))

		tOrigin = time.time()
		
		for x in range(0, 30):
			sendTime = int((time.time() - tOrigin) * 1000000)
			data = struct.pack('I', 1)

			sendTime = int((time.time() - tOrigin) * 1000000)
			ServerSocket.sendall(data)

			recvData = ServerSocket.recv(4)
			recvTime = int((time.time() - tOrigin) * 1000000)

			rd = struct.unpack('>I', recvData);
			masterTime = int(rd[0])

			rtt = recvTime - sendTime
			latency = rtt / 2
			latencyErrs[x] = latency

			timeError = masterTime - (sendTime + latency)
			timeErrs[x] = timeError
			
			#print('Send Time {}'.format(sendTime))
			print('Data {}us {}us ({}us) {}us Err: {}us'.format(recvTime, sendTime, rtt, masterTime, timeError))
		
		errMed = np.median(timeErrs)
		latMed = np.median(latencyErrs)

		print('Time Err: {} {}'.format(errMed, latMed))

		tOrigin -= errMed / 1000000

		for x in range(0, 1000):
			checkTime = int((time.time() - tOrigin) * 1000000) + latMed
			data = struct.pack('II', 2, checkTime)
			ServerSocket.sendall(data)
			time.sleep(0.05)

		# Final Light Activate After Sync
		for x in range(0, 30):
			t = x / 30.0
			r = 0.1 * (1 - t)
			g = 0.1 * t
			SetRGB(r, g, 0.0)
			time.sleep(1.0 / 30.0)		

		SetRGB(0.0, 0.1, 0.0)

	except Exception,e:
		print "Ex:" + str(e)
		traceback.print_exc()

#------------------------------------------------------------------------------
# Network Command Thread.
#------------------------------------------------------------------------------
class NetworkReaderThread(threading.Thread):
	def __init__(self):
		super(NetworkReaderThread, self).__init__()
		self.daemon = True
		self.broadcastSocket = None
		self.serverIp = None
		self.serverSocket = None
		self.start()

	def FindServer(self):
		# NOTE: We need to drain the socket as old broadcast msgs remain.
		self.broadcastSocket.setblocking(0)
		
		while True:
			try:
				recvData = self.broadcastSocket.recv(1024)
			except:
				break;

		self.broadcastSocket.settimeout(0.5)

		while True:
			try:
				print('Waiting for Server IP')
				msg = self.broadcastSocket.recvfrom(1024)
				
				r = re.search('KineticSynth:(\d+.\d+.\d+.\d+)', msg[0])

				if r != None:		
					print 'Found Server IP: {} from {}'.format(r.group(1), msg[1])
					return r.group(1)
			except:
				continue

	def run(self):
		global setRunCamera
		global netSend
		global serverFile
		global camera
		global setCamFps
		global setCamMode
		global deviceSerial

		print('Starting Network Thread')

		self.broadcastSocket = socket(AF_INET, SOCK_DGRAM)
		self.broadcastSocket.bind(('', 45454))

		while True:
			self.serverSocket = socket()
			self.serverIP = self.FindServer()
			#self.serverIP = '192.168.1.107'

			try:
				self.serverSocket.connect((self.serverIP, 8000))
				self.serverSocket.setsockopt(IPPROTO_TCP, TCP_NODELAY, 1)
			except:
				print('Cant Connect')
				continue

			serverFile = self.serverSocket.makefile('wb')
			# TODO: Need to clear this
			#netSend = Queue.Queue()
			print('Connected to Server')

			while True:
				# TODO: Catch exceptions here.
				try:
					data = serverFile.readline();				
					print('Socket Read ' + data);
					# Check all the things
					if not data:
						print('Socket Dead')
						setRunCamera = False
						break;
					else:
						data = data.rstrip()
						args = data.split(',')
						
						if args[0] == 'sc':
							print('Net start cam')
							setRunCamera = True
						elif args[0] == 'ec':
							print('Net stop cam')
							setRunCamera = False
						elif args[0] == 'pe':
							print('Set exposure ' + args[1])
							camera.shutter_speed = int(args[1])
						elif args[0] == 'pi':
							print('Set ISO ' + args[1])
							camera.iso = int(args[1])
						elif args[0] == 'pf':
							print('Set FPS ' + args[1])
							setCamFps = int(args[1])
						elif args[0] == 'ts':
							setRunCamera = False
							setSyncTime = True
							SyncTime(self.serverSocket, serverFile)
						elif args[0] == 'gi':
							data = struct.pack('III', 4, version, deviceSerial)
							netSend.put(data);
						elif args[0] == 'cm':
							print('Set Mode ' + args[1])
							setCamMode = int(args[1])

				except:
					print('Socket Dead')
					setRunCamera = False
					break;

class NetworkWriterThread(threading.Thread):
	def __init__(self):
		super(NetworkWriterThread, self).__init__()
		self.daemon = True
		self.start()

	def run(self):
		global setRunCamera
		#global netSend
		global serverFile

		print('Starting Network Writer Thread')

		while True:
			data = netSend.get()
			#print('Send Data')
			#print('Got Data')
			try:
				serverFile.write(data)
				serverFile.flush();
			except Exception,e:
				print "Net Write Ex:" + str(e)
				#continue


NetworkReaderThread()
NetworkWriterThread()

#------------------------------------------------------------------------------
# Camera Output H264.
#------------------------------------------------------------------------------
class CamOutput(object):
	def __init__(self):
		self.lastFrameTime = 0
		self.adjusting = 0

	def write(self, s):
		global camera
		global netSend

		frameSize = len(s)		
		frameTime = camera.frame.timestamp
		frameType = int(camera.frame.frame_type)

		if frameTime == None:		
			frameTime = 0

		packetHeader = struct.pack('IIII', 3, frameSize, frameType, frameTime)
		netSend.put(packetHeader)
		netSend.put(s)
		
		print('{} {} {} {}'.format(camera.frame.index, frameType, frameSize, frameTime))
		
		if frameType != 2:
			diff = frameTime - self.lastFrameTime			
			#print('{} {} {}'.format(camera.frame.index, diff, frameSize))
			self.lastFrameTime = frameTime

#------------------------------------------------------------------------------
# Camera Output Markers.
#------------------------------------------------------------------------------
class CamOutputMarkers(object):
	def __init__(self):
		self.lastFrameTime = 0
		
	def write(self, s):
		global camera
		global netSend

		try:
			frameTime = camera.frame.timestamp

			if frameTime == None:		
				frameTime = 0

			diff = frameTime - self.lastFrameTime
			self.lastFrameTime = frameTime

			# NOTE: Will block until previously pushed frame has been completed.
			blobCount = blobdetect.getblobcount()
			blobData = blobdetect.getblobdata()
			# NOTE: Takes about 1ms to copy buffer, should double buffer in C code.
			blobdetect.pushframe(frameTime, s)
			
			packetHeader = struct.pack('II', 5, len(blobData))
			netSend.put(packetHeader)
			netSend.put(blobData)
			
			#print('Markers {} {} {} {}'.format(camera.frame.index, diff, blobCount, len(blobData)))

		except Exception,e:
			print "Ex:" + str(e)
			traceback.print_exc()

#------------------------------------------------------------------------------
# Camera & Main Loop.
#------------------------------------------------------------------------------
print('VectorONE Device Software')

blobdetect.startworkers()

deviceSerial = GetSerial()
print('Serial: {}'.format(deviceSerial))

camera = picamera.PiCamera(sensor_mode = 6, clock_mode = 'reset')
camera.sensor_mode = 6
camera.rotation = 180
camera.resolution = (1024, 704)
camera.framerate = 40
camera.shutter_speed = 7000
camera.iso = 800
camera.awb_mode = 'off'
camera.awb_gains = (1.1, 1.1)
camera.exposure_mode = 'sports'
camera.framerate_delta = 0

def StopCamera():
	print('Stopping Camera')
	camera.stop_recording()
	SetRGB(0.1, 0.1, 0.1)

def StartCamera():
	if camMode == 0:
		print('Starting Camera H264')
		camera.start_recording(CamOutput(), format='h264', quality=25, profile='high', level='4.2', inline_headers=True, intra_period=50)
		SetRGB(0, 0, 0.1);
	elif camMode == 1:
		print('Starting Camera Markers')
		camera.start_recording(CamOutputMarkers(), format='yuv')
		SetRGB(0.1, 0, 0.1);

while True:	
	SetRGB(0.1, 0.1, 0.1)

	while True:
		time.sleep(0.1)

		if camFps != setCamFps:
			if camRunning:
				StopCamera()

			camera.framerate = setCamFps
			camFps = setCamFps
			
			if camRunning:
				StartCamera()

		if camMode != setCamMode:
			if camRunning:
				StopCamera()

			camMode = setCamMode
			
			if camRunning:
				StartCamera()

		if camSyncTime != setSyncTime:
			camSyncTime = setSyncTime

		if camRunning != setRunCamera:			
			if camRunning:
				# TODO: Catch Exceptions
				StopCamera()
			else:
				StartCamera()

			camRunning = setRunCamera

		
