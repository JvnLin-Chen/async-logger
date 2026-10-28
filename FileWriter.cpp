#include "FileWriter.h"

FileWriter::FileWriter() {
	currentDate = GetTime("%Y-%m-%d");

	std::string path = GetPath();
	char FilePath[512] = { 0 };
	std::snprintf(FilePath, sizeof(FilePath), "%s\\%s.txt", path.c_str(), currentDate.c_str());
	fopen_s(&file, FilePath, "a");
}

FileWriter::~FileWriter() {
	//关闭文件
	if(file != nullptr)
	   fclose(file);
}

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

void FileWriter::openIfNeeded() {
	std::string date = GetTime("%Y-%m-%d");
	if (date == currentDate && file != nullptr)
		return;
	else {
		//file之前也可能是空指针，所以一样要进行空指针判断，或者说之前没办法打开文件,导致file变成空指针，这个也要检查
		FILE* newFile = nullptr;
		std::string path = GetPath();
		char FilePath[512] = { 0 };
		std::snprintf(FilePath, sizeof(FilePath), "%s\\%s.txt", path.c_str(), date.c_str());
		errno_t err = fopen_s(&newFile, FilePath, "a");
		if (err != 0 || newFile == nullptr)    //文件没办法打开
		{
			printf("Error: 无法打开日志文件 %s\n", FilePath);
			return;
		}
		else
		{
			fclose(file);     //成功打开后就关闭旧文件，获取新文件句柄
		}
		file = newFile;
		currentDate = date;
	}
}

//
void FileWriter::addLog(std::string str) {

	openIfNeeded();
	//空指针判断
	if (file == nullptr) return;

	std::string time = GetTime("%Y-%m-%d %H:%M:%S");

	std::string timeWithstr = "[" + time + "]--" + str + "\n";
	//写入日志内容
	fprintf(file, "%s", timeWithstr.c_str());
}