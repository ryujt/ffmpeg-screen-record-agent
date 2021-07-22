#pragma once

#include "ryulib/SuspensionQueue.hpp"
#include "ryulib/SimpleThread.hpp"

namespace BroadCastUDP {
	class Socket
	{
	public:
		void open(int port)
		{
			// TODO:
		}

		void close()
		{
			// TODO:
		}

		void send(string text)
		{
			// TODO:
		}

	};
}

class BroadcastUDP
{
public:
	BroadcastUDP() 
	{
		thread = new SimpleThread(
			[&](SimpleThread *athread) {
				string text = queue.pop();
				socket.send(text);
			}
		);
	}

	void open(int port)
	{
		socket.open(port);
	}

	void close()
	{
		socket.close();
	}

	void send(string text)
	{
		queue.push(text);
	}
private:
	BroadCastUDP::Socket socket;
	SuspensionQueue<string> queue;
	SimpleThread *thread;
};
