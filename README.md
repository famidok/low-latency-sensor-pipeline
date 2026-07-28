# Low-Latency Sensor Data Pipeline

This project is a high-performance proof-of-concept designed to simulate real-time data ingestion and processing from industrial 3D sensors. Built with a focus on microsecond-level latency and system reliability, it demonstrates how to efficiently stream high-frequency spatial data across system processes.

## Project Overview

In industrial robotics, controllers must process high-rate sensor telemetry (e.g., 1000+ Hz) with absolute determinism. This pipeline architecture solves the inter-process communication bottleneck by eliminating heavy network protocols and text-based parsing. 

Key architectural decisions include:

* **ZeroMQ via IPC (Unix Domain Sockets):** Ensures lock-free, ultra-fast data transfer between the sensor driver and the processing engine using Kernel memory buffers, completely bypassing the TCP/IP stack.
* **Zero-Parsing Serialization:** Utilizes raw binary payload packing (Little-endian struct) instead of JSON. This eliminates parsing overhead, allowing the C++ engine to map the data directly into memory via zero-copy principles.
* **Asynchronous C++ Core:** The backend engine continuously ingests and filters the 3D point cloud data without blocking, while simultaneously running a lightweight HTTP server to expose real-time metrics (throughput and latency).

## System Components

### 1. High-Speed Sensor Simulator (Python)
Located in `scripts/mock_sensor.py`, this component acts as a hardware abstraction layer, simulating a 3D industrial camera generating continuous point cloud data.

* **High-Frequency Telemetry:** Generates continuous X, Y, Z spatial coordinates along with microsecond-precision UNIX timestamps at configurable rates (defaulting to 1000 Hz).
* **Binary Serialization (`struct.pack`):** Instead of CPU-heavy text formats like JSON, it packs the telemetry data into a raw 32-byte little-endian binary payload (`<dddd`). This ensures memory-alignment compatibility with C++ structs.
* **Kernel-Level Routing:** Broadcasts the payloads over a ZeroMQ PUB socket bound to a Unix Domain Socket (`ipc:///tmp/sensor.ipc`), dropping the data directly into the Linux Kernel buffer for ultra-fast consumption.
* **Fault-Tolerant CLI:** Includes graceful fallback mechanisms for command-line arguments to prevent system crashes from invalid operator inputs.
