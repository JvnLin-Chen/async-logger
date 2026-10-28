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
	// 【1】构造 AsyncLogger，立刻启动后台线程
	//      从这一刻起，日志文件已经打开（或第一次写时打开）
	AsyncLogger logger;

	// 【2】记录开始时间
	//      steady_clock 是单调时钟，不受系统时间被修改的影响，
	//      专门用来测"经过了多久"
	auto t0 = std::chrono::steady_clock::now();

	// 【3】主线程（生产者）连续 push 10000 条
	//      这一步如果异步真的生效，每次都只是"加锁 + 入队 + 通知"，微秒级
	//      如果异步没生效（锁里写文件），每次都要等磁盘，毫秒级
	for (int i = 0; i < 10000; ++i) {
		logger.dataPush("line " + std::to_string(i));
	}

	// 【4】记录结束时间，算耗时
	auto t1 = std::chrono::steady_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

	// 【5】打印耗时。这是要填进 README 的关键数字
	std::cout << "[Test1] push 10000 条耗时: " << ms << " ms\n";

	// 【6】函数返回时 logger 析构：
	//      析构里做 设 quit → notify_all → join → 后台线程把剩余数据写完
	//      "退出干净"这件事也顺带被验证了：如果析构错了，这里会崩或卡
}

// ============================================================
// Test2：4 个线程各写 2500 条，测"多生产者不丢、不串"
// ============================================================
void Test2_MultiThread() {
	AsyncLogger logger;

	// 【1】4 个线程，每个写 2500 条，总共 10000 条
	//      用常量而不是直接写 4 和 2500，方便以后改成 8 线程
	const int kThreads = 4;
	const int kPerThread = 2500;

	// 【2】存 4 个 std::thread 对象。std::thread 不可拷贝，只能移动，
	//      放 vector 里正合适（emplace_back 是原地构造 + 移动）
	std::vector<std::thread> producers;

	for (int t = 0; t < kThreads; ++t) {
		// 【3】启动第 t 个生产者线程
		//      捕获列表 [&logger, t] 是关键：
		//        &logger —— 引用捕获，4 个线程共用同一个 logger（这是重点！）
		//        t       —— 【值捕获】！绝不能写 &t
		//                   t 是 for 循环的局部变量，引用捕获后线程跑起来时
		//                   t 可能已经变成别的值，或者已经出作用域（悬空引用）
		//                   值捕获保证每个线程拿到自己那一份 t
		producers.emplace_back([&logger, t,kPerThread]() {
			for (int i = 0; i < kPerThread; ++i) {
				// 【4】每条消息带"线程号 + 序号"
				//      格式：T0-0, T0-1, ..., T0-2499, T1-0, ...
				//      为什么带这个？写完之后能按 T0/T1/T2/T3 分组数行数，
				//      直接定位是哪个线程丢了数据
				std::string msg = "T" + std::to_string(t)
					+ "-" + std::to_string(i);
				logger.dataPush(msg);
			}
		});
	}

	// 【5】等所有生产者线程结束
	//      join 之后，10000 条都【已经进入队列】了，
	//      但后台线程可能还没写完——这个 "join vs 后台写完" 的区别很重要
	for (auto& p : producers) p.join();

	// 【6】函数返回时 logger 析构：
	//      持锁设 quit → notify_all → join 后台线程
	//      后台线程醒来后，因为队列可能还没空，会继续取数据直到空，然后退出
	//      这个析构路径就是"不丢最后几条"的关键
}

// ============================================================
// Test3：空闲 10 秒，测"后台线程睡的时候真的不烧 CPU"
// ============================================================
void Test3_IdleCpu() {
	AsyncLogger logger;

	// 【1】先写一条，确保后台线程真的启动了（否则光构造可能还没跑起来）
	std::string msg = "warmup";
	logger.dataPush(msg);

	// 【2】主线程休眠 10 秒
	//      这段时间后台线程应该：
	//        1. 先被 warmup 唤醒，把这条写完
	//        2. 然后回到 cv.wait 里睡
	//        3. 一直睡着，不占 CPU
	//      看任务管理器（Windows）或 top（Linux），CPU 占用应接近 0%
	//      如果持续 10% 以上，说明 wait 没有真睡，可能：
	//        - 谓词写错，每次都返回 true（wait 立刻返回，相当于 while 空转）
	//        - 队列为空但 notify 一直被触发
	//        - 代码里有别的 busy loop
	std::this_thread::sleep_for(std::chrono::seconds(20));

	// 【3】返回，logger 析构，同上
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
	Test3_IdleCpu();

	system("pause");
	return 0;
}