#include "AsyncLogger.h"


AsyncLogger::AsyncLogger() {
	popData = std::thread(&AsyncLogger::workloop, this);   //启动线程，但是等待唤醒
	//workloop();
}
AsyncLogger::~AsyncLogger() {
	//退出的时候将缓存区里面所有的数据全部清理掉
	{
		std::lock_guard <std::mutex> lock(mtx);
		quit = true;
	}
	cv.notify_all();
	if (popData.joinable()) popData.join();
}

//提供到外部，主线程不断调用该函数去添加日志数据
void AsyncLogger::dataPush(std::string& msg) {
	{
		std::lock_guard<std::mutex> lock(mtx);
		data.push(msg);
	}
	cv.notify_one();
}

//还需要一个函数，是后台线程不断等待数据，
//等待条件达到的时候就会唤醒后台线程
//同时该函数是在放在构造函数里面，一旦创建对象，就开始调用

void AsyncLogger::workloop() {
	while (true) {
		std::string  msg;
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [&] {
				return !data.empty() || quit;
			});
			//popData.join();

			if (data.empty() && quit)    // 队列清空且该退出了 → 退出循环
				break;

			msg = std::move(data.front());
			data.pop();
		}
		fileW.addLog(msg);
	}
}