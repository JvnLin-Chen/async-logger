#pragma once
#include <string>
#include <ctime>
#define MAX_PATH 260

class FileWriter {

public:
	FileWriter();
	~FileWriter();
	std::string GetTime(const char* fmt);
	std::string GetPath();
	void addLog(std::string str);
	void openIfNeeded();

private:
	FILE* file;                      //保存打开文件的文件句柄
	std::string currentDate;        //保存当前时间
};