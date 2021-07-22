#ifndef RYU_SCHEDULER_HPP
#define RYU_SCHEDULER_HPP


#include <ryulib/base.hpp>
#include <ryulib/SimpleThread.hpp>
#include <ryulib/ThreadQueue.hpp>

using namespace std;
using namespace ryulib;

class TaskOfScheduler
{
public:
	int task;
	string text;
	void* data;
	int size;
	int tag;

	TaskOfScheduler(int t, string txt, void* d, int s, int g) {
		task = t;
		text = txt;
		data = d;
		size = s;
		tag = g;
	}
};

class Scheduler {
public:
	Scheduler()
	{
		thread_ = new SimpleThread(on_thread_execute);
	}

	~Scheduler() 
	{
		stop();
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
		stop();
		thread_->terminateAndWait();
	}

	void start() {
		started_ = true;
	}

	void stop() {
		started_ = false;
	}
	
	void add(int task) {
		TaskOfScheduler* t = new TaskOfScheduler(task, "", nullptr, 0, 0);
		queue_.push(t);
		thread_->wakeUp();
	}

	void add(int task, string text) {
		TaskOfScheduler* t = new TaskOfScheduler(task, text, nullptr, 0, 0);
		queue_.push(t);
		thread_->wakeUp();
	}

	void add(int task, void* data) {
		TaskOfScheduler* t = new TaskOfScheduler(task, "", data, 0, 0);
		queue_.push(t);
		thread_->wakeUp();
	}

	void add(int task, string text, void* data, int size, int tag) {
		TaskOfScheduler* t = new TaskOfScheduler(task, text, data, size, tag);
		queue_.push(t);
		thread_->wakeUp();
	}

	void sleep(int millis)
	{
		thread_->sleep(millis);
	}

	bool is_empty() { return queue_.is_empty(); }

	void setOnTask(const TaskEvent& value) { on_task_ = value; }
	void setOnRepeat(const VoidEvent& value) { on_repeat_ = value; }
	void setOnTerminated(const NotifyEvent& event) { on_terminated_ = event; }

private:
	bool started_ = false;
	ThreadQueue<TaskOfScheduler*> queue_;
	SimpleThread* thread_;

	TaskEvent on_task_ = nullptr;
	VoidEvent on_repeat_ = nullptr;
	NotifyEvent on_terminated_ = nullptr;

	SimpleThreadEvent on_thread_execute = [&](SimpleThread* simpleThread) {
		while (simpleThread->isTerminated() == false) {
			TaskOfScheduler* t = queue_.pop();
			while (t != NULL) {
				if (on_task_ != nullptr) {
					on_task_(t->task, t->text, t->data, t->size, t->tag);
				}
				delete t;

				t = queue_.pop();
			}

			if (started_ && (on_repeat_ != nullptr)) {
				on_repeat_();
			} else {
				thread_->sleep(1);
			}
		}

		if (on_terminated_ != nullptr) on_terminated_(this);
	};
};


#endif  // RYU_SCHEDULER_HPP