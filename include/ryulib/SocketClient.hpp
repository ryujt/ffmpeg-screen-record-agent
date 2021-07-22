#ifndef RYULIB_SOCKETCLIENT_HPP
#define RYULIB_SOCKETCLIENT_HPP

#include <ryulib/SocketUtils.hpp>
#include <string>
#include <functional>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <ryulib/SimpleThread.hpp>

using namespace std;
using boost::asio::ip::tcp;

typedef function<void()> ClientSocketEvent;
typedef function<void(Packet*)> ClientReceivedEvent;

class SocketClient {
public:
	SocketClient()
	{
		receive_buffer_ = (Packet*) malloc(PACKET_LIMIT * 2);
	}

	~SocketClient()
	{
		disconnect();
		if (receive_buffer_ != nullptr) delete receive_buffer_;
	}

	void connect(string host, int port)
	{
		disconnect();

		io_ = make_unique<boost::asio::io_context>();
		socket_ = make_unique<tcp::socket>(*io_);

		tcp::endpoint endpoint(boost::asio::ip::address::from_string(host), port);
		socket_->async_connect(endpoint, boost::bind(&SocketClient::connect_handler, this, boost::asio::placeholders::error));

		simple_thread_ = make_unique<SimpleThread>([&](SimpleThread* simple_thread){ 
			io_->run();
		});
	}

	void disconnect()
	{
		if (simple_thread_ != nullptr) {
			simple_thread_->terminateNow();
		}

		if (socket_ != nullptr) {
			socket_->close();
		}
	}

	void send(Packet* packet)
	{
		do_send(packet, packet->packet_size);
	}

	void send(int packet_type, const void* data, int size)
	{
		Packet* packet = create_packet(packet_type, data, size);
		send(packet);
		free(packet);
	}

	void send(char packet_type, string text)
	{
		send(packet_type, (void*) text.c_str(), text.size());
	}

	void setOnError(SocketErrorEvent event) { on_error_ = event; }
	void setOnConnected(ClientSocketEvent event) { on_connected_ = event; }
	void setOnDisconnected(ClientSocketEvent event) { on_disconnected_ = event; }
	void setOnReceived(ClientReceivedEvent event) { on_received_ = event; }

private:
	Packet* receive_buffer_ = nullptr;

	unique_ptr<boost::asio::io_context> io_ = nullptr;
	unique_ptr<tcp::socket> socket_ = nullptr;
	unique_ptr<SimpleThread> simple_thread_ = nullptr;

	SocketErrorEvent on_error_ = nullptr;
	ClientSocketEvent on_connected_ = nullptr;
	ClientSocketEvent on_disconnected_ = nullptr;
	ClientReceivedEvent on_received_ = nullptr;

	void do_disconnect()
	{
		socket_->close();
		if (on_disconnected_ != nullptr) on_disconnected_();
	}

	void do_send(void* data, int size) {
		boost::asio::async_write(*socket_, 
			boost::asio::buffer(data, size), 
			boost::bind(&SocketClient::handle_write, this, boost::asio::placeholders::error)
		);
	}

	void connect_handler(const boost::system::error_code& error)
	{
		if (error) {
			if (on_error_ != nullptr) on_error_(ERROR_CONNECT, error.message());
		} else {
			if (on_connected_ != nullptr) on_connected_();

			boost::asio::async_read(*socket_,
				boost::asio::buffer(receive_buffer_, sizeof(PacketHeader)),
				boost::bind(&SocketClient::handle_read_header, this, boost::asio::placeholders::error)
			);
		}
	}

	void handle_read_header(const boost::system::error_code& error)
	{
		if (!error) {
			boost::asio::async_read(*socket_,
				boost::asio::buffer(&receive_buffer_->data_start, receive_buffer_->getDataSize()),
				boost::bind(&SocketClient::handle_read_body, this, boost::asio::placeholders::error)
			);
		} else {
			if (on_error_ != nullptr) on_error_(ERROR_READ, error.message());
			do_disconnect();
		}
	}

	void handle_read_body(const boost::system::error_code& error)
	{
		if (!error)
		{
			if (on_received_ != nullptr) on_received_(receive_buffer_);

			boost::asio::async_read(*socket_,
				boost::asio::buffer(receive_buffer_, sizeof(PacketHeader)),
				boost::bind(&SocketClient::handle_read_header, this, boost::asio::placeholders::error)
			);
		} else {
			if (on_error_ != nullptr) on_error_(ERROR_READ, error.message());
			do_disconnect();
		}
	}

	void handle_write(const boost::system::error_code& error)
	{
		if (error) {
			if (on_error_ != nullptr) on_error_(ERROR_WRITE, error.message());
			do_disconnect();
		}
	}

};

#endif  // RYULIB_SOCKETCLIENT_HPP