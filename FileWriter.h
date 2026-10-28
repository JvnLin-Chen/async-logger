#pragma once
#include <string>
#include <ctime>
#define MAX_PATH 260

class FileWriter {

public:
	std::string GetTime(const char* fmt);
	std::string GetPath();
	void addLog(std::string str);
};