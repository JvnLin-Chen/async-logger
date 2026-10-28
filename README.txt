# v1 — 异步日志最小闭环

## 状态
- 版本：v1
- 日期：2026-__-__
- 定位：能用的最小异步日志。单生产者/多生产者 → 单消费者线程 → 文件。
- 不做：有界队列、溢出策略、单例、格式化宏、日志轮转、批量写入。

## 验收结果
| 测试 | 结果 |
|---|---|
| 单线程写 10000 条 | 主线程耗时 _84__ ms，文件 _10000__ 行 |
| 4 线程各 2500 条 | 文件 _10000__ 行，内容  有交错 |
| 退出 | 干净返回，无崩溃无卡死 |
| 空闲 CPU（后台线程等数据时） | 10%-20% |
| 编译警告（/W3 或 -Wall） | [ ] 无警告

## 关键设计决策
| 决策 | 选择 | 理由 |
|---|---|---|
| 消费者线程数 | 1 条 std::thread | 单文件 sink，磁盘是瓶颈，多线程抢句柄更慢 |
| 队列 | 无界 std::queue | 先保证正确性，有界+溢出策略留到 v2 |
| 锁边界 | 只包队列 front/pop/push，写文件在锁外 | 不让磁盘 IO 阻塞业务线程入队 |
| 通知 | 每次 push 后 notify_one（锁外） | 有数据就该叫醒消费者；放锁外避免被唤醒线程立刻撞锁 |
| 停机 | 析构：持锁设 quit → notify_all → join | 保证退出前队列排空，不丢最后几条 |
| 循环退出条件 | data.empty() && quit 才 break | 只判断 quit 会丢队列残留数据 |
| 时间戳 | 每条 localtime_s + strftime | 尚未测成本，先正确性优先 |
| 线程启动 | 构造函数内 std::thread(...)；popData 声明在依赖成员之后 | 避免线程读到未构造的 mutex/queue |

## 踩坑记录
<!-- 每条：症状 → 原因 → 怎么发现/怎么修 -->

1. 症状：程序退出时崩溃（terminate）
   原因：std::thread 既没 join 也没 detach 就被析构
   修复：析构函数持锁设 quit=true → notify_all → joinable 判断后 join

2. 症状：后台线程偶发读垃圾 / 随机崩溃
   原因：成员声明顺序不对，线程先于 mutex/queue 构造就启动
   发现：把成员顺序调整后消失
   修复：把 std::thread 成员声明放到所有依赖成员之后

3. 症状：条件变量谓词编译警告 / 未定义行为
   原因：lambda 只有 if (...) return true; 没有 false 分支
   修复：改成单表达式 return !data.empty() || quit;

4. 症状：日志内容里的 % 导致输出错乱 / 崩溃（FileWriter 里）
   原因：fprintf(file, msg.c_str()) 把用户内容当格式串，还有就是字符串可能会出现 % 这种可能导致的错误
   修复：fprintf(file, "%s", msg.c_str()) 或 fputs

5.症状：程序退出时崩溃
   症状：main 返回后 terminate
   原因：std::thread 既没 join 也没 detach 就被析构
   发现：查 std::thread::~thread 文档
   修复：在程序退出时，在析构函数里面唤醒线程，并且thread.join()，

6.症状：异步没生效，主线程还是卡
   症状：dataPush 耗时忽高忽低
   原因：fileW.addLog 在锁内，业务线程被磁盘 IO 挡住
   发现：把写文件移出锁作用域后 P99 骤降


## 遗留问题（→ v2 输入，按优先级）
1. FileWriter 每条日志 fopen/fclose 一次 → 改成保持打开 + 跨天切换
2. 打开/写失败静默丢日志 → 至少 stderr 报错或 fallback
3. 无界队列 → 有界 + 溢出策略（block / overrun_oldest / discard_new）
4. 每次 wait 只取一条 → 批量 swap 出队列，减少锁竞争
5. 单例化（Meyers Singleton）+ 静态析构顺序处理
6. 变参宏 LOG_INFO("code=%d", code)

## 怎么跑
```bash
# 编译（示例，按你的实际工程改）
g++ -std=c++17 -Wall -Wextra -O2 main.cpp AsyncLogger.cpp FileWriter.cpp -o log_test
./log_test
# 然后去 D:/OFilingApp/YYYY-MM-DD.txt 数行数