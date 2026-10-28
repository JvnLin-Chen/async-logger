#include "FileWriter.h"

std::string FileWriter::GetTime(const char* fmt) {
	time_t t = time(0);
	struct tm tm_buf;
	localtime_s(&tm_buf, &t);
	char buf[32];
	strftime(buf, sizeof(buf), fmt, &tm_buf);
	return std::string(buf);
}


std::string FileWriter::GetPath() {
	char modulePath[MAX_PATH] = "D:\\OFilingApp";   // 或者 "D:/OFilingApp"
													//GetModuleFileNameA(NULL, modulePath, MAX_PATH);
	return std::string(modulePath);
}


//
void FileWriter::addLog(std::string str) {
	//打开文件
	std::string date = GetTime("%Y-%m-%d");
	std::string time = GetTime("%Y-%m-%d %H:%M:%S");
	std::string path = GetPath();
	char FilePath[512] = { 0 };
	std::snprintf(FilePath, sizeof(FilePath), "%s\\%s.txt", path.c_str(), date.c_str());
	FILE* file;

	errno_t err = fopen_s(&file, FilePath, "a");
	if (err != 0 || file == nullptr) return;

	std::string timeWithstr = "[" + time + "]--" + str + "\n";
	//写入日志内容
	fprintf(file, "%s", timeWithstr.c_str());

	//关闭文件
	fclose(file);
}