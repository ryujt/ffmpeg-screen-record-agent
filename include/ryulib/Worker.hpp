#ifndef RYU_WORKER_HPP
#define RYU_WORKER_HPP

#include <ryulib/base.hpp>
#include <ryulib/SimpleThread.hpp>
#include <ryulib/SuspensionQueue.hpp>

using namespace std;
using namespace ryulib;

const int TASK_STRING = -1;

class TaskOfWorker
{
public:
	int task;
	void* data;
	int size;
	int tag;
	string text;

	TaskOfWorker(int t, string txt, void* d, int s, int g) {
		task = t;
		text = txt;
		data = d;
		size = s;
		tag = g;
	}
};

class Worker {
public:
	Worker() {
		thread_ = new SimpleThread(on_thread_execute);
	}

	void terminate()
	{
		thread_->terminate();
	}

	void terminateNow()
	{
		thread_->terminateNow();
		if (on_terminated_ != nullptr) on_terminated_(this);
	}

	void terminateAndWait()
	{
		thread_->terminateAndWait();
	}

	void add(int task) {
		TaskOfWorker* t = new TaskOfWorker(task, "", nullptr, 0, 0);
		queue_.push(t);
	}

	void add(string text) {
		TaskOfWorker* t = new TaskOfWorker(TASK_STRING, text, nullptr, 0, 0);
		queue_.push(t);
	}

	void add(int task, void* data) {
		TaskOfWorker* t = new TaskOfWorker(task, "", data, 0, 0);
		queue_.push(t);
	}

	void add(int task, void* data, int size, int tag) {
		TaskOfWorker* t = new TaskOfWorker(task, "", data, size, tag);
		queue_.push(t);
	}

	bool is_empty() { return queue_.is_empty(); }

	void setOnTask(const TaskEvent& event) { on_task_ = event; }
	void setOnTerminated(const NotifyEvent& event) { on_terminated_ = event; }

private:
	bool started_ = false;
	SuspensionQueue<TaskOfWorker*> queue_;

	TaskEvent on_task_ = nullptr;
	NotifyEvent on_terminated_ = nullptr;

	SimpleThread* thread_;
	SimpleThreadEvent on_thread_execute = [&](SimpleThread * simpleThread) {
		while (simpleThread->isTerminated() == false) {
			TaskOfWorker* t = queue_.pop();
			if (on_task_ != nullptr) {
				on_task_(t->task, t->text, t->data, t->size, t->tag);
			}

			delete t;
		}

		if (on_terminated_ != nullptr) on_terminated_(this);
	};
};

#endif  // RYU_WORKER_HPP