#pragma once
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include "FileWriter.h"

/*
需要一个线程变量，消费者线程
需要一个条件变量，用来唤醒消费者线程
需要一个队列缓存区，用来缓存生产者生产的数据
需要一个类变量，属于后台线程同步插入数据
*/
class AsyncLogger {
public:
	AsyncLogger();
	~AsyncLogger();
	void dataPush(std::string msg);         //生产者入队
	int getDropCount() const;                      //返回丢掉的日志数目

private:
	//std::thread pushData;
	std::thread popData;
	std::condition_variable cv;
	std::queue<std::string> data;
	FileWriter fileW;
	std::mutex mtx;
	bool quit = false;
	const size_t queue_max_size = 10000;       //队列最大容量
	std::atomic<int> drop_count{ 0 };


	void workloop();         //工作函数
};
