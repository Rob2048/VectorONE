#------------------------------------------------------------------------------
# RGB LED Test App.
#------------------------------------------------------------------------------
import io
import time
import math
import RPIO.PWM as PWM

print('Starting RGB LED Test')
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

startTime = time.time()

speed = 4.0
brightness = 100
r = 0.0
g = 0.0
b = 0.0

while True:
	time.sleep(1.0 / 30.0)
	t = math.sin(float(time.time() - startTime) * speed) * 0.5 + 0.5
	tg = math.sin(float(time.time() - startTime) * speed + 1) * 0.5 + 0.5
	tb = math.sin(float(time.time() - startTime) * speed + 2) * 0.5 + 0.5

	r = t * 3 - 2
	if r < 0:
		r = 0

	g = tg * 3 - 2
	if g < 0:
		g = 0

	b = tb * 3 - 2
	if b < 0:
		b = 0

	# Update LEDs
	pulse = int((1.0 - r) * brightness) + 1000 - brightness
	PWM.add_channel_pulse(0, RED_LED, 0, pulse)
	pulse = int((1.0 - g) * brightness) + 1000 - brightness
	PWM.add_channel_pulse(0, GREEN_LED, 0, pulse)
	pulse = int((1.0 - b) * brightness) + 1000 - brightness
	PWM.add_channel_pulse(0, BLUE_LED, 0, pulse)


