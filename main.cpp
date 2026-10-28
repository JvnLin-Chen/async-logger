#include "AsyncLogger.h"

#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>

// ============================================================
// Test1：单线程写 10000 条，测"主线程被阻塞了多久"
// ============================================================
void Test1_SingleThread() {

	AsyncLogger logger;


	auto t0 = std::chrono::steady_clock::now();


	for (int i = 0; i < 10000; ++i) {
		logger.dataPush("line " + std::to_string(i));
	}


	auto t1 = std::chrono::steady_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

	std::cout << "[Test1] push 10000 条耗时: " << ms << " ms\n";


}

// ============================================================
// Test2：4 个线程各写 2500 条，测"多生产者不丢、不串"
// ============================================================
void Test2_MultiThread() {
	AsyncLogger logger;


	const int kThreads = 4;
	const int kPerThread = 2500;


	std::vector<std::thread> producers;

	for (int t = 0; t < kThreads; ++t) {

		producers.emplace_back([&logger, t,kPerThread]() {
			for (int i = 0; i < kPerThread; ++i) {

				std::string msg = "T" + std::to_string(t)
					+ "-" + std::to_string(i);
				logger.dataPush(msg);
			}
		});
	}


	for (auto& p : producers) p.join();


}

// ============================================================
// Test3：空闲 10 秒，测"后台线程睡的时候真的不烧 CPU"
// ============================================================
void Test3_IdleCpu() {
	AsyncLogger logger;

	std::string msg = "warmup";
	logger.dataPush(msg);

	std::this_thread::sleep_for(std::chrono::seconds(20));

}

// ============================================================
// Test4：主线程连续调用 dataPush 写 50000 条日志,测试队列上限
// ============================================================
void Test4_SingleThread() {

	AsyncLogger logger;

	for (int i = 0; i < 50000; ++i) {
		logger.dataPush("line " + std::to_string(i));
	}
	//等待两秒钟
	std::this_thread::sleep_for(std::chrono::seconds(10));
	std::cout << "丢失日志总数: " << logger.getDropCount() << std::endl;
	
}

// ============================================================
// main：根据命令行参数选跑哪个测试
// ============================================================
int main(int argc, char** argv) {
	//// 默认跑 Test1；也可以 ./log_test 1 / 2 / 3 选
	//std::string mode = (argc > 1) ? argv[1] : "1";

	//if (mode == "1")      Test1_SingleThread();
	//else if (mode == "2") Test2_MultiThread();
	//else if (mode == "3") Test3_IdleCpu();
	//else std::cout << "用法: " << argv[0] << " [1|2|3]\n";
	//Test2_MultiThread();
	//Test3_IdleCpu();

	Test4_SingleThread();

	system("pause");
	return 0;
}