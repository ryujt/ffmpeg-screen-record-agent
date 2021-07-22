#ifndef RYU_TCP_CLIENT_HPP
#define RYU_TCP_CLIENT_HPP

#include <ryulib/SocketUtils.hpp>
#include <ryulib/base.hpp>
#include <ryulib/debug_tools.hpp>
#include <ryulib/Scheduler.hpp>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

enum TaskType {ttConnect, ttDisconnect, ttSendData, ttSendText, ttReceive};

typedef function<void()> SocketEvent;
typedef function<void(void*, int)> ReceivedEvent;
typedef function<void(int, string)> SocketErrorEvent;

class TcpClient
{
public:
	TcpClient() {
		scheduler_.setOnRepeat(
			[&]() {
				if (do_receive()) {
                    idle_count_ = 0;
				} else {
                    idle_count_ = idle_count_ + 5;
					if (on_repeat_ != nullptr) on_repeat_();
					scheduler_.sleep(5);
				}
			}
		);
		scheduler_.setOnTask(
			[&](int task, const string text, const void* data, int size, int tag) {
				switch (task) {
                    case ttConnect: {
						do_disconnect();
                        do_connect(text, size);
					} break;

					case ttDisconnect: do_disconnect(); break;
                        
                    case ttSendData: {
						do_sendData(data, size);
                        free((void*) data);
                    } break;

                    case ttSendText: {
                        string* str = (string*) data;
                        do_sendData((char*) str->c_str(), str->size());
                        delete str;
                    } break;
				}
			}
		);
		scheduler_.start();
	}

	void terminate()
	{
		disconnect();
		scheduler_.terminateAndWait();
	}

	void connect(string host, int port) {
		scheduler_.add(ttConnect, host, nullptr, port, 0);
	}

	void disconnect() {
		scheduler_.add(ttDisconnect);
	}

    void sendData(const void* data, int size) {
        if (size > PACKET_LIMIT) {
            if (on_error_ != nullptr) on_error_(this, -4, "Packet size is limited to exec(_codes_) bytes.");
            return;
        }
        
        void *buffer = new char[PACKET_LIMIT];
        memcpy(buffer, data, size);
        scheduler_.add(ttSendData, "", buffer, size, 0);
    }
    
    void sendText(const char* text) {
        string* str = new string(text);
        scheduler_.add(ttSendText, "", str, 0, 0);
    }
    
	bool isConnected() { return socket_ != 0;  }
	int getIdleCount() { return idle_count_; }

	void setOnConnected(SocketEvent event) { on_connected_ = event; }
	void setOnDisconnected(SocketEvent event) { on_disconnected_ = event; }
	void setOnReceived(ReceivedEvent event) { on_received_ = event; }
	void setOnError(ErrorEvent event) { on_error_ = event; }
	void setOnRepeat(const VoidEvent& value) { on_repeat_ = value; }

protected:
    int idle_count_ = 0;
	int socket_ = 0;

private:
	fd_set fd_read_;
	Scheduler scheduler_;

	SocketEvent on_connected_ = nullptr;
	SocketEvent on_disconnected_ = nullptr;
	ReceivedEvent on_received_ = nullptr;
	ErrorEvent on_error_ = nullptr;
	VoidEvent on_repeat_ = nullptr;

	void do_connect(string host, int port) {
		struct sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(port);

		// TODO
		if ((host.at(0) >= '0') && (host.at(0) <= '9')) {
			address.sin_addr.s_addr = inet_addr(host.c_str());
		} else {
			struct hostent* hostent = gethostbyname(host.c_str());
			if (hostent == NULL) {
				if (on_error_ != nullptr) on_error_(this, -1, "gethostbyname, can't find domain.");
				return;
			}
			address.sin_addr = *((struct in_addr*) hostent->h_addr_list[0]);
		}
        
		socket_ = socket(PF_INET, SOCK_STREAM, 0);
		if (socket_ < 0) {
			if (on_error_ != nullptr) on_error_(this, -2, "Can't open socket stream.");
			return;
		}

		FD_SET(socket_, &fd_read_); 

		long arg = fcntl(socket_, F_GETFL, NULL);
		arg = arg | O_NONBLOCK;
		fcntl(socket_, F_SETFL, arg);

		bool connected = ::connect(socket_, (struct sockaddr*) & address, sizeof(address)) >= 0;
		if ((!connected) && (errno == EINPROGRESS)) {
			struct timeval timeout;
			timeout.tv_sec = 5000;
			timeout.tv_usec = 0;
			connected = select(socket_ + 1, NULL, &fd_read_, NULL, &timeout) > 0;
		}        
		if (!connected) {
			do_disconnect();
			if (on_error_ != nullptr) on_error_(this, -3, "can't connect to server.");
		} else {
			FD_ZERO(&fd_read_);
			if (on_connected_ != nullptr) on_connected_();
		}
	}

	void do_disconnect() {
		if (socket_ == 0) return;

		shutdown(socket_, SHUT_RDWR);
		//close(socket_);
		socket_ = 0;
	}

	void do_sendData(const void* data, int size) {
		if (socket_ == 0) return;

		if (send(socket_, data, size, 0) <= 0) {
            do_disconnect();
            if (on_disconnected_ != nullptr) on_disconnected_();
        }
	}

	bool do_receive() {
		if (socket_ == 0) return false;

		// add 4 bytes more for safe
		char buffer[4096 + 4];
		int received_bytes = recv(socket_, buffer, 4096, MSG_DONTWAIT);
		if (received_bytes <= 0) return false;

		while (received_bytes > 0) {
			if (on_received_ != nullptr) on_received_(buffer, received_bytes);
			received_bytes = recv(socket_, buffer, 4096, MSG_DONTWAIT);
		}

		return true;
	}
};

#endif  // RYU_TCP_CLIENT_HPP
