#include <iostream>
#include <zmq.hpp>
#include "httplib.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstring>

// We don't want padding
#pragma pack(push, 1)
struct SensorPayload {
	double timestamp;
	double x;
	double y;
	double z;
};
#pragma pack(pop)

struct SensorMetrics {
	std::atomic<uint64_t> total_messages{0};
	std::atomic<double> latest_latency_ms{0.0};
	std::atomic<double> latest_x{0.0};
	std::atomic<double> latest_y{0.0};
	std::atomic<double> latest_z{0.0};
};

void run_http_server(SensorMetrics& metrics) {
	httplib::Server svr;

	svr.Get("/metrics", [&metrics](const httplib::Request&, httplib::Response& res) {
		uint64_t count = metrics.total_messages.load(std::memory_order_relaxed);
		double latency = metrics.latest_latency_ms.load(std::memory_order_relaxed);
		double x = metrics.latest_x.load(std::memory_order_relaxed);
		double y = metrics.latest_y.load(std::memory_order_relaxed);
		double z = metrics.latest_z.load(std::memory_order_relaxed);

		std::string json = "{\n";
        json += "  \"total_messages\": " + std::to_string(count) + ",\n";
        json += "  \"latency_ms\": " + std::to_string(latency) + ",\n";
        json += "  \"payload\": {\n";
        json += "    \"x\": " + std::to_string(x) + ",\n";
        json += "    \"y\": " + std::to_string(y) + ",\n";
        json += "    \"z\": " + std::to_string(z) + "\n";
        json += "  }\n";
        json += "}";

		res.set_content(json, "application/json");
	});

	std::cout << "[HTTP] Metric server started at http://localhost:8080/metrics.\n";
	svr.listen("0.0.0.0", 8080);
}


int main() {
	SensorMetrics metrics;

	zmq::context_t context(1);
	zmq::socket_t subscriber(context, zmq::socket_type::sub);

	const std::string ipc_endpoint = "ipc:///tmp/sensor.ipc";

	try {
		subscriber.connect(ipc_endpoint);
	} catch (const zmq::error_t& e) {
		std::cerr << "[Core] IPC connection error: " << e.what() << "\n";
		return 1;
	}

	// Set filter as 3Dsensor
	subscriber.set(zmq::sockopt::subscribe, "3Dsensor");
	std::cout << "[Core] Data labeled '3Dsensor' is expected over IPC...\n";

	std::thread http_thread(run_http_server, std::ref(metrics));
	http_thread.detach();

	while (true) {
		zmq::message_t topic_msg;

		auto result_topic = subscriber.recv(topic_msg, zmq::recv_flags::none);

		if (result_topic && topic_msg.more()) {
			zmq::message_t payload_msg;

			auto result_payload = subscriber.recv(payload_msg, zmq::recv_flags::none);

			if (result_payload && payload_msg.size() == sizeof(SensorPayload)) {
				SensorPayload payload;

				std::memcpy(&payload, payload_msg.data(), sizeof(SensorPayload));

				auto now = std::chrono::system_clock::now();
				double current_time_sec = std::chrono::duration<double>(now.time_since_epoch()).count();
				double latency_ms = (current_time_sec - payload.timestamp) * 1000.0;

				metrics.total_messages.fetch_add(1, std::memory_order_relaxed);
				metrics.latest_latency_ms.store(latency_ms, std::memory_order_relaxed);
				metrics.latest_x.store(payload.x, std::memory_order_relaxed);
				metrics.latest_y.store(payload.y, std::memory_order_relaxed);
				metrics.latest_z.store(payload.z, std::memory_order_relaxed);

				uint64_t current_count = metrics.total_messages.load(std::memory_order_relaxed);
                if (current_count % 2000 == 0) {
                    std::cout << "[Core] Packet: " << current_count
                              << " | Latency: " << latency_ms << " ms"
                              << " | X: " << payload.x << " Y: " << payload.y << " Z: " << payload.z << "\n";
                }
		  }
		}
	}
	return 0;
}

