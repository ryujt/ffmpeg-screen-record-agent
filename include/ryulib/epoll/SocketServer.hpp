#ifndef RYULIB_SOCKETSERVER_HPP
#define RYULIB_SOCKETSERVER_HPP

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <functional>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <ryulib/base.hpp>
#include <ryulib/SimpleThread.hpp>
#include <ryulib/ThreadQueue.hpp>

using namespace std;

#define PACKET_SIZE 4096
#define MAXEVENTS 32768

static int create_and_bind(int aport) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, sfd;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE; /* All interfaces */

	s = getaddrinfo(NULL, std::to_string(aport).c_str(), &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) continue;

		s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if (s == 0) break;

		close(sfd);
	}

	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		return -1;
	}

	freeaddrinfo(result);

	return sfd;
}

static int set_socket_options(int sfd) {
	int flags, s;

	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	s = fcntl(sfd, F_SETFL, flags);
	if (s == -1) {
		perror("fcntl");
		return -1;
	}

	struct linger solinger = { 1, 0 };
	s = setsockopt(sfd, SOL_SOCKET, SO_LINGER, &solinger, sizeof(struct linger));
	if (s == -1) {
		perror("fcntl");
		return -1;
	}

	return 0;
}

#define TASK_START 1
#define TASK_STOP 2

class Task {
public:
	int task_type;
	int int_data;

	Task(int atype, int aint_data) {
		task_type = atype;
		int_data = aint_data;
	}
};

class SocketServer {
public:
	SocketServer() {
		events = (epoll_event *) calloc(MAXEVENTS, sizeof event);
		tasks_ = new ThreadQueue<Task*>();
		thread_ = new SimpleThread(thread_on_execute);

		efd = epoll_create1(0);
		if (efd == -1) perror("epoll_create");
	}

	~SocketServer() {
//		free(events);
//		delete tasks_;
//		delete thread_;
	}

	void start(int aport) {
		tasks_->push(new Task(TASK_START, aport));
	}

	void stop() {
		tasks_->push(new Task(TASK_STOP, 0));
	}

public:
	bool is_started_ = false;
	IntegerEvent on_connected_ = nullptr;
	IntegerEvent on_disconnected_ = nullptr;
	HanldeDataEvent on_received_ = nullptr;
	bool is_started() { return is_started_; }
	void setOnConnected(const IntegerEvent &value) { on_connected_ = value; }
	void setOnDisconnected(const IntegerEvent &value) { on_disconnected_ = value; }
	void setOnReceived(const HanldeDataEvent &value) { on_received_ = value; }

private:
	int sfd, s;
	int efd;
	struct epoll_event event;
	struct epoll_event *events;
	ThreadQueue<Task*> *tasks_;

private:
	SimpleThread *thread_;
	SimpleThreadEvent thread_on_execute = [&](SimpleThread *thread) {
		while (thread->isTerminated() == false) {
			Task *task = tasks_->pop();
			if (task == NULL) {
				if (is_started_) process_event();
				else thread->Sleep(5);

				continue;
			}

			printf("task_type: %d, port: %d \n", task->task_type, task->int_data);

			switch(task->task_type) {
				case TASK_START: do_task_start(task->int_data); break;
				case TASK_STOP: do_task_stop(); break;
			}

			delete task;
		}
	};

	void process_event() {
		int n, i;

		n = epoll_wait(efd, events, MAXEVENTS, 5);
		for (i = 0; i < n; i++) {
			// error or not ready
			bool is_error =
					(events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP) ||
					(!(events[i].events & EPOLLIN));
			if (is_error) {
				close_connection(events[i].data.fd);
				continue;
			}

			if (sfd == events[i].data.fd) {
				have_new_connection();
			} else {
				received(i);
			}
		}
	}

	void do_task_start(int aport) {
		if (is_started_) return;
		is_started_ = true;

		sfd = create_and_bind(aport);
		if (sfd == -1) abort();

		s = set_socket_options(sfd);
		if (s == -1) abort();

		s = listen(sfd, SOMAXCONN);
		if (s == -1) {
			perror("listen");
			abort();
		}

		event.data.fd = sfd;
		event.events = EPOLLIN | EPOLLET;
		s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
		if (s == -1) {
			perror("epoll_ctl");
			abort();
		}
	}

	void do_task_stop() {
		if (is_started_ == false) return;
		is_started_ = false;

		close(sfd);
	}

	void have_new_connection() {
		while (1) {
			struct sockaddr in_addr;
			socklen_t in_len;
			int infd;
			char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

			in_len = sizeof in_addr;
			infd = accept(sfd, &in_addr, &in_len);
			if (infd == -1) {
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
					break;
				} else {
					// TODO: make an event handler
					perror("accept");
					break;
				}
			}

//			s = getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);
//			if (s == 0) {
//				printf("Accepted connection on descriptor %d (host=%s, port=%s)\n", infd, hbuf, sbuf);
//			}

			s = set_socket_options(infd);
			if (s == -1) abort();

			event.data.fd = infd;
			event.events = EPOLLIN | EPOLLET;
			s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
			if (s == -1) {
				perror("epoll_ctl");
				abort();
			}

			if (on_connected_ != nullptr) on_connected_(this, infd);
		}
	}

	void close_connection(int socket) {
		if (on_disconnected_ != nullptr) on_disconnected_(this, socket);
		close(socket);
	}

	void received(int index) {
		int done = 0;

		while (1) {
			ssize_t count;
			char buf[PACKET_SIZE];

			count = read(events[index].data.fd, buf, PACKET_SIZE);
			if (count == -1) {
				if (errno != EAGAIN) {
					perror("read");
					done = 1;
				}

				done = 0;
				break;
			} else if (count == 0) {
				done = 1;
				break;
			}

			if (on_received_ != nullptr) on_received_(this, events[index].data.fd, buf, count);
		}

		if (done) {
			close_connection(events[index].data.fd);
		}
	}
};

#endif  // RYULIB_SOCKETSERVER_HPP
