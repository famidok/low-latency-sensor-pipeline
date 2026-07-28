import random
import zmq
import time
import struct
import sys

def generate_random_axis_data():
	x = random.uniform(-100, 100)
	y = random.uniform(-100, 100)
	z = random.uniform(-100, 100)

	axis_info = (
		"Random generated axis info: \n"
		"==============================\n"
		f"X-axis: {x}\n"
		f"Y-axis: {y}\n"
		f"Z-axis: {z}\n"
		"==============================\n"
	)

	print(axis_info)
	return (x, y, z)

def run_sensor():
	sleep_time = 0.001

	# Default: 1000 Hz (1000 packet per second)
	if len(sys.argv) >=  2:
		try:
			sleep_time = float(sys.argv[1])
			print(f"[Sensor] Sleep time set from argument: {sleep_time} seconds")
		except ValueError:
			print("[Sensor] ERROR: Invalid number provided! Using default 0.001 seconds.")
			time.sleep(1)

	context = zmq.Context()
	socket = context.socket(zmq.PUB)

	ipc_endpoint = "ipc:///tmp/sensor.ipc"
	socket.bind(ipc_endpoint)

	print(f"[Sensor] Stream ON. Endpoint {ipc_endpoint}")
	print("[Sensor] Waiting for CPP engine (Datas are streaming)...")

	msg_count = 0

	try:
		while True:
			current_time = time.time()
			# <dddd -> 32 byte little-endian for ARM
			# CPP side
			# struct sensordata
			# double timestamp
			# double x
			# double y
			# double z
			# total 32 byte
			payload = struct.pack('<dddd', current_time, *generate_random_axis_data())

			topic = b"3Dsensor"
			socket.send_multipart([topic, payload])
			msg_count += 1

			time.sleep(sleep_time)

	except KeyboardInterrupt:
		print("\n[Sensor] Stopped by user.")
		print(f"[Sensor] {msg_count} packet was successfully transmitted over IPC.")
	finally:
		socket.close()
		context.term()
		print("[Sensor] Sockets are closed.")

def main():
	run_sensor()

if __name__ == "__main__":
	main()
