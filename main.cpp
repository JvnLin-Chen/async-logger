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

//
// ============================================================
// Test 1: 基础并发与正确性回归（验证不丢数据、不串行）
// ============================================================
void V2_Test1_Concurrency() {
	std::cout << "[Test1] 开始 4 线程并发压测，每线程 25000 条，共 100000 条...\n";
	AsyncLogger logger;
	const int kThreads = 4;
	const int kPerThread = 25000;
	std::vector<std::thread> producers;

	for (int t = 0; t < kThreads; ++t) {
		producers.emplace_back([&logger, t,&kPerThread]() {
			for (int i = 0; i < kPerThread; ++i) {
				logger.dataPush("T" + std::to_string(t) + "-" + std::to_string(i));
			}
		});
	}
	for (auto& p : producers) p.join();

	std::cout << "[Test1] 写入完成。请检查日志文件：\n";
	std::cout << "  1. 总行数应为 100000 行\n";
	std::cout << "  2. 搜索 T0-, T1-, T2-, T3- 均应为 25000 条\n";

}

// ============================================================
// Test 2: 刷盘节奏（验证时间阈值 2 秒是否生效）
// ============================================================
void V2_Test2_FlushTimer() {
	std::cout << "[Test2] 开始刷盘节奏测试。只写 1 条，然后主线程等待 3 秒...\n";
	std::cout << "【请在程序睡着时（3秒内），立刻去打开日志文件查看】\n";
	{
		AsyncLogger logger;
		logger.dataPush("Test2-Flush-Timeout-Check");
		// 此时在后台，最多等 2 秒，fflush 就会触发
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
	std::cout << "[Test2] 睡眠结束。如果你在 3 秒内去文件里看到了那条日志，说明刷盘生效了。\n";
}

// ============================================================
// Test 3: 退出时的“临终关怀”（验证析构是否强制刷盘）
// ============================================================
void V2_Test3_GracefulExit() {
	std::cout << "[Test3] 开始瞬间退出测试。写入 5000 条后立刻退出作用域...\n";
	{
		AsyncLogger logger;
		for (int i = 0; i < 5000; ++i) {
			logger.dataPush("FastExit-" + std::to_string(i));
		}
		// 注意：这里没有任何 sleep，直接退出作用域！
		// 这一步专门用来测你的析构函数里有没有强制 fflush(file) 和 fclose(file)
	}
	std::cout << "[Test3] 已退出作用域。请检查日志文件行数，应为恰好 5000 行（少一行就是退出没刷盘）。\n";
}

// ============================================================
// Test 4: 验证批量写入的性能（主线程耗时）
// ============================================================
void V2_Test4_Performance() {
	std::cout << "[Test4] 开始性能测试。写入 100000 条，测试主线程耗时...\n";
	{
		AsyncLogger logger;
		auto start = std::chrono::steady_clock::now();

		for (int i = 0; i < 100000; ++i) {
			logger.dataPush("Perf-Test-" + std::to_string(i));
		}

		auto end = std::chrono::steady_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		std::cout << "[Test4] 主线程 push 100000 条耗时: " << ms << " ms\n";
		std::cout << "【期望值】: 耗时应该非常小（几十到几百毫秒），说明完全没被磁盘 I/O 卡住。\n";
	}
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

	//Test4_SingleThread();
	//V2_Test1_Concurrency();
	//V2_Test2_FlushTimer();
	V2_Test3_GracefulExit();
	//V2_Test4_Performance();

	system("pause");
	return 0;
}