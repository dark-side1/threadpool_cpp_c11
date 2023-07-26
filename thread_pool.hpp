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
		//std::bind�������ص���һ���º������ͣ��õ��ķ���ֵ����ֱ�Ӹ�ֵ��һ��std::function
		//auto func = bind(forward<F>(f), forward<Args>(args)...); Ҳ����
		//func()����ֱ��ִ�д�������
		//Ҫ��func����������Ϊ�յĿɵ��ö����װ��
		function<decltype(f(args...))()> func = bind(forward<F>(f), forward<Args>(args)...);
		using RetType = decltype(f(args...));
		//packaged_task<RetType()>  ����Ϊ�գ����ΪRetType
		//packaged_task��ʵ���Ƕ����߳�Ҫִ�е������������˰�װ���Ϳɵ��ö����װ����ʹ�÷�����ͬ
		//packaged_task<RetType()>��func����һ��
		//����ָ�뽫���������ǶԸ�std::packaged_task������й���
		auto task_ptr = make_shared<packaged_task<RetType()>>(func);
		//�ٴ�����std::function����task_ptrָ���std::packaged_task����ȡ������װΪvoid����
		//task_ptrָ���std::packaged_task�������������Ϊ�յĿɵ��ö����װ����lambda���ʽ
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
		cond_var_.notify_all();//���Ҫɾ�����߳��������ߣ����ѣ����߳��Լ���������
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


