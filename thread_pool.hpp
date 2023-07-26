#pragma once
#include<atomic>
#include<future>
#include<condition_variable>
#include<unordered_map>
#include"work_thread.hpp"

using namespace std;

class ThreadPool
{
public:
	ThreadPool(int min = 1, int max = thread::hardware_concurrency()) :shutdown(false), min_(min), max_(max) {
		for (int i = 0; i < min_; i++)
		{
			AddThread();
		}
		managerThread_ = thread([this]() {
			while (!shutdown)
			{
				if (task_q_.GetSize() > 2 * threads_.size() && threads_.size() < max_) {
					AddThread();
				}
				else
				{
					int cnt = 0;
					for (auto& t : threads_) {
						if (t.second->GetState() == WorkThread::STATE_WAIT) ++cnt;
					}
					if (cnt > 2 * task_q_.GetSize() && threads_.size() < min_) {
						DelThread();
					}
				}
				this_thread::sleep_for(chrono::milliseconds(100));
			}
		});
	}

	~ThreadPool() {
		shutdown = true;
		for (auto& t : threads_)
		{
			t.second->Shutdown();
		}
		cond_var_.notify_all();
		if (managerThread_.joinable()) {
			managerThread_.join();
		}
	}

	template<class F, class...Args>
	auto commit(F&& f, Args&&...args)->future<decltype(f(args...))> {
		//std::bind绑定器返回的是一个仿函数类型，得到的返回值可以直接赋值给一个std::function
		//auto func = bind(forward<F>(f), forward<Args>(args)...); 也可以
		//func()即可直接执行传入任务
		//要把func变成输入输出为空的可调用对象包装器
		function<decltype(f(args...))()> func = bind(forward<F>(f), forward<Args>(args)...);
		using RetType = decltype(f(args...));
		//packaged_task<RetType()>  输入为空，输出为RetType
		//packaged_task其实就是对子线程要执行的任务函数进行了包装，和可调用对象包装器的使用方法相同
		//packaged_task<RetType()>与func类型一致
		//智能指针将更方便我们对该std::packaged_task对象进行管理
		auto task_ptr = make_shared<packaged_task<RetType()>>(func);
		//再次利用std::function，将task_ptr指向的std::packaged_task对象取出并包装为void函数
		//task_ptr指向的std::packaged_task对象变成输入输出为空的可调用对象包装器，lambda表达式
		task_q_.AddTask([task_ptr]() {(*task_ptr)(); });
		cond_var_.notify_all();
		return task_ptr->get_future();
	}

	uint64_t Count() { return threads_.size(); }

private:

	void AddThread() {
		auto tdPtr = make_shared<WorkThread>(task_q_, cond_var_, mutex_);
		threads_[tdPtr->GetID()] = tdPtr;
		cout << "add thread " << GetThreadID(tdPtr->GetID()) << endl;
	}

	void DelThread() {
		thread::id tid;
		for (auto &t : threads_)
		{
			if (t.second->GetState() == WorkThread::STATE_WAIT) {
				t.second->Shutdown();
				tid = t.first;
				break;
			}
		}
		threads_.erase(tid);
		cond_var_.notify_all();//如果要删除的线程正在休眠，唤醒，让线程自己析构结束
		cout << "delete thread " << GetThreadID(tid) << endl;
	}

	TaskQueue task_q_;
	condition_variable cond_var_;
	mutex mutex_;
	thread managerThread_;
	atomic_bool shutdown;
	unordered_map<thread::id, ThreadPtr> threads_;
	int min_;
	int max_;
};


