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
//也就是说其实每一个线程都是复制一份代码，然后都是调用对应的函数，如果是在这个类里面的话，那么这个类里面的成员就是公共资源，而函数内部里面的变量就是私有资源
//所以多个线程运行之后改变局部变量，都是自己的，可以根据自己的私有资源来修改公共资源
void AsyncLogger::dataPush(std::string msg) {
	bool is_drop = false;           //局部变量，那么这个是每一个线程上面都会存在有这个变量，而且是互不干扰的变量
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (data.size() >= queue_max_size)
		{
			data.pop();
			is_drop = true;
		}
		data.push(std::move(msg));
	}
	if (is_drop)
		drop_count++;
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
		//测试
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

int AsyncLogger::getDropCount() const{
	return drop_count.load();
}