#------------------------------------------------------------------------------
# Car Sense
# Version: 0.1.0
#------------------------------------------------------------------------------
# Created by Robert Gowans 2017
#------------------------------------------------------------------------------

import io
from io import BytesIO
import socket
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

print('Car Sense Single Capture')

#------------------------------------------------------------------------------
# Camera & Main Loop.
#------------------------------------------------------------------------------

with picamera.PiCamera(sensor_mode=5) as camera:

	camera.sensor_mode = 5
	camera.rotation = 180
	camera.resolution = (1640, 922)
	camera.framerate = 30
	#camera.shutter_speed = 200000
	# camera.meter_mode = 'backlit'
	
	imgNum = 0

	cameraStartTime = time.time()
	time.sleep(2)

	try:
		i = 0;
		while True:
			camera.capture('cap' + str(i) + '.jpg', use_video_port=True, resize=(1640, 922), quality=100)
			#camera.capture('/home/pi/preview.jpg', use_video_port=True, resize=(960, 540), quality=10)
			time.sleep(1)
			print('Captured')
			i += 1;
		
	finally:
		print('Car Sense Terminated')
