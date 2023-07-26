#pragma once
#include<thread>
#include<atomic>
#include<memory>
#include<condition_variable>
#include<sstream>
#include <iostream>
#include"task_queue.hpp"


using namespace std;

inline unsigned long long GetThreadID(thread::id tid) {
	stringstream ss;
	ss << tid;
	unsigned long long id;
	ss >> id;
	return id;
}
inline unsigned long long GetThreadID() {
	return GetThreadID(this_thread::get_id());
}
inline unsigned long long GetThreadID(thread &t) {
	return GetThreadID(t.get_id());
}

class WorkThread
{
public:
	constexpr static int STATE_WAIT = 1;
	constexpr static int STATE_WORK = 2;
	constexpr static int STATE_EXIT = 3;

	WorkThread(TaskQueue& task_q, condition_variable& cond_var, mutex& mut) :task_q_(task_q), cond_var_(cond_var), mutex_(mut)
	{
		this->thread_ = std::thread([this]() {
			while (!shutdown)
			{
				state = STATE_WAIT;
				unique_lock<mutex> lk(mutex_);
				//只有在队列为空且线程池不关闭的条件下，返回为0，线程阻塞
				cond_var_.wait(lk, [&]() {return !task_q_.Empty() || shutdown; });
				if (shutdown) break;
				Task task = task_q_.GetTask();
				if (task != nullptr) {
					state = STATE_WORK;
					task();//仿函数使用，绑定器用法
				}
			}
			state = STATE_EXIT;
		});
		cout << "thread " << GetThreadID(thread_) << " is start" << endl;
	}

	~WorkThread() {
		cout << "thread " << GetThreadID(thread_) << " is end" << endl;
		if (thread_.joinable()) {
			thread_.join();
		}
	}

	int GetState() {
		return state;
	}

	void Shutdown() {
		shutdown = true;
	}

	thread::id GetID() {
		return thread_.get_id();
	}

	thread& GetThread() {
		return thread_;
	}

private:
	TaskQueue& task_q_;//todo
	condition_variable& cond_var_;
	mutex& mutex_;
	atomic_int state;
	atomic_bool shutdown;
	thread thread_;

};

using ThreadPtr = shared_ptr<WorkThread>;