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


Harika bir README oluyor dostum. Profesyonel bir dille yazılmış, tam bir sistem programlama projesine yakışacak kalitede.

Yazdığın o ilk kısımla mükemmel uyum sağlayacak şekilde, C++ motorunun (Processing Engine) mimarisini, çıktı örneklerini ve derleme/çalıştırma adımlarını hazırladım. Doğrudan kopyalayıp mevcut README dosyanın altına yapıştırabilirsin:

---

### 2. Core Processing Engine (C++)

Located in `src/main.cpp`, this is the high-performance heart of the pipeline. It reads the raw byte stream from the Linux kernel and processes it with minimal CPU overhead.

* **Zero-Copy Deserialization:** Employs `#pragma pack(push, 1)` to enforce strict memory alignment, allowing the engine to map incoming 32-byte ZeroMQ frames directly into a C++ `struct` using `std::memcpy`. This completely eliminates the deserialization penalty.
* **Lock-Free Concurrency:** Uses `std::atomic` variables with `std::memory_order_relaxed` to share telemetry metrics between the high-speed ingestion loop and the HTTP server. This prevents Data Tearing and thread contention without using expensive mutex locks.
* **Microsecond Latency Tracking:** Compares the payload's origin timestamp against the C++ `std::chrono::system_clock` upon arrival, accurately tracking the end-to-end IPC latency (typically yielding ~0.15 milliseconds).
* **Asynchronous Metric API:** Spawns a detached `std::thread` to run a lightweight HTTP server. It serves the latest ingested telemetry and system throughput as a JSON endpoint (`/metrics`) without blocking the main `while(true)` event loop.

### Example Outputs

**Python Sensor Simulator (`mock_sensor.py`):**

```text
Random generated axis info:
==============================
X-axis: 77.51793416649247
Y-axis: -79.89567180772134
Z-axis: -96.15395469849808
==============================
```

**C++ Processing Engine (`./processing_engine`):**

```text
./processing_engine
[Core] Data labeled '3Dsensor' is expected over IPC...
[HTTP] Metric server started at http://localhost:8080/metrics.
[Core] Packet: 2000 | Latency: 0.137806 ms | X: -99.7572 Y: -31.6741 Z: 34.8674
[Core] Packet: 4000 | Latency: 0.166655 ms | X: 71.9487 Y: -58.5282 Z: -91.1046
[Core] Packet: 6000 | Latency: 0.169039 ms | X: 67.0638 Y: -57.7211 Z: 70.0935
[Core] Packet: 8000 | Latency: 0.157118 ms | X: -42.415 Y: -38.9086 Z: -95.3339
[Core] Packet: 10000 | Latency: 0.155926 ms | X: 24.3002 Y: 55.7363 Z: 29.4392
```

### Build and Run Instructions

#### Prerequisites

* **Python 3.x**
* **C++17 Compatible Compiler** (GCC/Clang)
* **CMake** (3.10+)
* **ZeroMQ Library** (e.g., `libzmq3-dev` on Debian/Ubuntu systems)

#### 1. Python Environment Setup

The Python script requires the `pyzmq` package to interface with ZeroMQ. The `time` and `struct` modules are built into the standard library.

```bash
# Install the ZeroMQ Python binding
pip install pyzmq
```

#### 2. Building the C++ Engine

We use an out-of-source build pattern via CMake to keep the project directory clean.

```bash
# Navigate to the project root, create a build directory
mkdir build
cd build/

# Generate Makefiles and compile the engine
cmake ..
make
```

#### 3. Running the Pipeline

For the best experience, run these in two separate terminal sessions. It is recommended to start the C++ engine first so it is ready to catch the initial data frames without dropping packets.

**Terminal 1 (Start the Core Engine):**

```bash
cd build/
./processing_engine
```

**Terminal 2 (Start the Sensor Stream):**

```bash
cd scripts/
# Optionally pass the delay between packets in seconds (e.g., 0.001 for 1000 Hz)
python mock_sensor.py 0.001
```
