#pragma once
#include<queue>
#include<mutex>
#include<functional>

using namespace std;
using Task = function<void()>;//TaskΪһ���ɵ��ö����װ�������������Ϊ��

class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	bool Empty() {
		unique_lock<mutex> lock(mutex_q);
		return task_q.empty();
	}

	size_t GetSize() {
		unique_lock<mutex> lock(mutex_q);
		return task_q.size();
	}

	void AddTask(const Task task) {
		unique_lock<mutex> lock(mutex_q);
		task_q.push(task);
	}

	Task GetTask() {
		unique_lock<mutex> lock(mutex_q);
		if (task_q.empty()) {
			return nullptr;
		}
		Task task = move(task_q.front());
		task_q.pop();
		return task;
	}

private:
	queue<Task> task_q;
	mutex mutex_q;
};

TaskQueue::TaskQueue()
{
}

TaskQueue::~TaskQueue()
{
}

