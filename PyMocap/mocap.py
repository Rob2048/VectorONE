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
from fractions import Fraction
import json

version = 110

# Current Camera State
camRunning = False
camFps = 50
camMode = 0

# Net Command Request State
setRunCamera = camRunning
setCamFps = camFps
setCamMode = camMode

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
# Config.
#------------------------------------------------------------------------------
tempMask = ''
for i in range(0, 64 * 44):
	tempMask += '1'

config = {
	'name' : 'Unnamed',
	'mask' : tempMask
}

def SaveConfig():
	with open('config.json', 'w') as f:
		json.dump(config, f, indent=4)

def LoadConfig():
	try:
		with open('config.json', 'r') as f:
			config.update(json.load(f))
	except Exception,e:
		print 'Config file: ' + str(e)
	
	SaveConfig()

LoadConfig()

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
				break

		self.broadcastSocket.settimeout(0.5)

		while True:
			try:
				#print('Waiting for Server IP')
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
			blobdetect.masterconnectionmade(self.serverIP)
			#self.serverIP = '192.168.1.106'

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
					data = serverFile.readline()
					#print('Socket Read ' + data)
					# Check all the things
					if not data:
						blobdetect.masterconnectionlost()
						print('Socket Dead')						
						setRunCamera = False
						serverFile = None						
						break
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
						elif args[0] == 'gi':
							configStr = json.dumps(config)
							data = struct.pack('IIII', 4, version, deviceSerial, len(configStr))
							netSend.put(data)
							netSend.put(configStr)
						elif args[0] == 'cm':
							print('Set camera mode ' + args[1])
							setCamMode = int(args[1])
						elif args[0] == 'sm':
							print('Set mask')
							config['mask'] = args[1]
							blobdetect.setmask(args[1])
							SaveConfig()
						elif args[0] == 'sp':
							print('Set properties')
							config.update(json.loads(data[3:]))
							blobdetect.setmask(config['mask'])
							SaveConfig()

				except Exception,e:
					blobdetect.masterconnectionlost()
					print('Socket Dead: ' + str(e))
					setRunCamera = False
					serverFile = None
					break

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

			try:
				if serverFile != None:
					serverFile.write(data)
					serverFile.flush()
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
		
	def write(self, s):
		global camera
		global netSend
		
		try:
			frameSize = len(s)		
			frameTime = camera.frame.timestamp
			frameType = int(camera.frame.frame_type)

			if frameTime == None:		
				frameTime = 0

			#print('{} {} {} {}'.format(camera.frame.index, frameType, frameSize, frameTime))

			avgHostOffset = 0.0
			currentFrameMasterId = 0
			
			if frameType != 2:
				diff = frameTime - self.lastFrameTime
				self.lastFrameTime = frameTime

				masterTime, avgHostOffset = blobdetect.getmastertime()
				camToMasterTime = masterTime - camera.timestamp

				fd = 11080

				if camFps == 50:
					fd = 19941
				elif camFps == 60:
					fd = 16611
				elif camFps == 70:
					fd = 14235
				elif camFps == 80:
					fd = 12463
				elif camFps == 90:
					fd = 11080
				elif camFps == 100:
					fd = 9970

				fId = (frameTime + camToMasterTime) / fd
				frameTarget = (fId) * fd
				frameErr = frameTarget - (frameTime + camToMasterTime)
				
				if frameErr < -(fd / 2):
					frameErr = (fd + frameErr)

				fIdDiff = (frameTime + camToMasterTime) - frameTarget
				if fIdDiff < 3000:
					currentFrameMasterId = fId
				elif fIdDiff > fd - 3000:
					currentFrameMasterId = fId + 1
				
				correction = 0.0
				updateIndex = int(70000 / fd)

				if camera.frame.index % updateIndex == 0:
					if abs(frameErr) > 2000:
						correction = 10
					elif abs(frameErr) > 1000:
						correction = 6
					elif abs(frameErr) > 200:
						correction = 0.6
					elif abs(frameErr) > 100:
						correction = 0.1

					if frameErr > 0:
						correction *= -1
					
					camera.framerate_delta = correction
				else:
					camera.framerate_delta = 0

				print('{:>6} {:>6} {:>6} {:>10} {:>8.2f}'.format(diff, frameErr, correction, currentFrameMasterId, avgHostOffset))

			packetHeader = struct.pack('IIIfq', 3, frameSize, frameType, avgHostOffset, currentFrameMasterId)
			netSend.put(packetHeader)
			netSend.put(s)

		except Exception,e:
			print "Ex:" + str(e)
			traceback.print_exc()

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
				return

			diff = frameTime - self.lastFrameTime
			self.lastFrameTime = frameTime

			currentFrameMasterId = 0
			masterTime, avgHostOffset = blobdetect.getmastertime()
			camToMasterTime = masterTime - camera.timestamp

			fd = 11080

			if camFps == 50:
				fd = 19941
			elif camFps == 60:
				fd = 16611
			elif camFps == 70:
				fd = 14235
			elif camFps == 80:
				fd = 12463
			elif camFps == 90:
				fd = 11080
			elif camFps == 100:
				fd = 9970

			fId = (frameTime + camToMasterTime) / fd
			frameTarget = (fId) * fd
			frameErr = frameTarget - (frameTime + camToMasterTime)
			
			if frameErr < -(fd / 2):
				frameErr = (fd + frameErr)

			fIdDiff = (frameTime + camToMasterTime) - frameTarget
			if fIdDiff < 3000:
				currentFrameMasterId = fId
			elif fIdDiff > fd - 3000:
				currentFrameMasterId = fId + 1
			
			correction = 0.0
			updateIndex = int(70000 / fd)

			if camera.frame.index % updateIndex == 0:
				if abs(frameErr) > 2000:
					correction = 10
				elif abs(frameErr) > 1000:
					correction = 6
				elif abs(frameErr) > 200:
					correction = 0.6
				elif abs(frameErr) > 100:
					correction = 0.1

				if frameErr > 0:
					correction *= -1
				
				camera.framerate_delta = correction
			else:
				camera.framerate_delta = 0

			print('{:>6} {:>6} {:>6} {:>10} {:>8.2f}'.format(diff, frameErr, correction, currentFrameMasterId, avgHostOffset))

			blobStatus = blobdetect.getstatus()
			if blobStatus == 0 or blobStatus == 3:
				if blobStatus == 3:
					blobData = blobdetect.getblobdata()
					packetHeader = struct.pack('II', 5, len(blobData))
					netSend.put(packetHeader)
					netSend.put(blobData)
					
				# NOTE: Takes about 1ms to copy buffer, should double buffer in C code.
				blobdetect.pushframe(currentFrameMasterId, avgHostOffset, s)
			
			#print('Markers {} {} {} {}'.format(camera.frame.index, diff, blobCount, len(blobData)))

		except Exception,e:
			print "Ex:" + str(e)
			traceback.print_exc()

#------------------------------------------------------------------------------
# Camera & Main Loop.
#------------------------------------------------------------------------------
print('VectorONE Device Software')

blobdetect.startworkers()
blobdetect.setmask(config['mask'])

# TODO: Set all the things from config.

deviceSerial = GetSerial()
print('Serial: {}'.format(deviceSerial))

camera = picamera.PiCamera(sensor_mode = 6, clock_mode = 'raw')
camera.sensor_mode = 6
camera.rotation = 180
camera.resolution = (1024, 704)
camera.framerate = camFps
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
		SetRGB(0, 0, 0.1)
	elif camMode == 1:
		print('Starting Camera Markers')
		camera.start_recording(CamOutputMarkers(), format='yuv')
		SetRGB(0.1, 0, 0.1)

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

		if camRunning != setRunCamera:			
			if camRunning:
				# TODO: Catch Exceptions
				StopCamera()
			else:
				StartCamera()

			camRunning = setRunCamera
		