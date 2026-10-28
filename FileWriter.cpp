#include "FileWriter.h"

FileWriter::FileWriter() {
	currentDate = GetTime("%Y-%m-%d");

	std::string path = GetPath();
	char FilePath[512] = { 0 };
	std::snprintf(FilePath, sizeof(FilePath), "%s\\%s.txt", path.c_str(), currentDate.c_str());
	fopen_s(&file, FilePath, "a");

	last_flush_time = std::chrono::steady_clock::now();
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
void FileWriter::addLog(std::queue<std::string>& msg) {

	openIfNeeded();
	//空指针判断
	if (file == nullptr) return;

	std::string time = GetTime("%Y-%m-%d %H:%M:%S");
	std::string big_buffer;
	big_buffer.reserve(msg.size() * 100);     //扩容，预留空间

	size_t batch_count = msg.size(); // 先存下来

	while (!msg.empty()) {
		big_buffer.append("[").append(time).append("]--").append(msg.front()).append("\n");
		msg.pop();
	}

	fwrite(big_buffer.c_str(), 1, big_buffer.length(), file);

	// 刷盘节奏（累加条数）
	write_count_since_flush += batch_count;   // 记录写了多少条

	 // 3. 检查是否需要强制刷盘（写入物理磁盘）
	auto now = std::chrono::steady_clock::now();
	auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(now - last_flush_time).count();

	// 条件A：攒够 1000 条了（防止大批量数据滞留）
	// 条件B：距离上次刷盘超过 2 秒了（防止低峰期数据滞留）
	if (write_count_since_flush >= 1000 || elapsed_sec >= 2) {
		fflush(file); // 真正的物理落盘！
		write_count_since_flush = 0;
		last_flush_time = now;
	}

	//std::string timeWithstr = "[" + time + "]--" + msg + "\n";
	//写入日志内容

}