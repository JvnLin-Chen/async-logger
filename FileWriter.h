#pragma once
#include <string>
#include <ctime>
#include <queue>
#include <chrono>
#define MAX_PATH 260

class FileWriter {

public:
	FileWriter();
	~FileWriter();
	std::string GetTime(const char* fmt);
	std::string GetPath();
	//void addLog(std::string str);
	void addLog(std::queue<std::string>& msg);
	void openIfNeeded();

private:
	FILE* file;                      //保存打开文件的文件句柄
	std::string currentDate;        //保存当前时间
	int write_count_since_flush;     // 记录写了多少条
	std::chrono::time_point<std::chrono::steady_clock> last_flush_time;
};