#ifndef RYULIB_SOCKETSERVER_HPP
#define RYULIB_SOCKETSERVER_HPP

#include <ryulib/SocketUtils.hpp>
#include <string>
#include <functional>
#include <list>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class Connection;

typedef function<void(Connection*)> ServerSocketEvent;
typedef function<void(Connection*, Packet*)> ServerReceivedEvent;

class SocketServerInterface {

	friend class Connection;

protected:
	virtual void fire_error_event(int code, string msg) {}
	virtual void fire_connected_event(Connection* connection) {}
	virtual void fire_disconnected_event(Connection* connection) {}
	virtual void fire_received_event(Connection* connection, Packet* packet) {}
};

class Connection : public boost::enable_shared_from_this<Connection> {
	
	friend class SocketServer;

public:
	typedef boost::shared_ptr<Connection> pointer;

	static pointer create(SocketServerInterface* server, boost::asio::io_context& io_context) {
		return pointer(new Connection(server, io_context));
	}

	tcp::socket& socket() {
		return socket_;
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

	// Dummy property
	void* user_data = nullptr;

	int idle_count = 0;

	bool is_logined = false;
	string room_id;
	string user_id;
	string user_pw;
	int user_level = 0;

private:
	Connection(SocketServerInterface* server, boost::asio::io_context& io_context)
		: server_(server), socket_(io_context) 
	{
	}

	void start() {
		packet_ = (Packet*) malloc(PACKET_LIMIT * 2);

		server_->fire_connected_event(this);

		boost::asio::async_read(socket_,
			boost::asio::buffer(packet_, HEADER_SIZE),
			boost::bind(&Connection::handle_read_header, shared_from_this(),
				boost::asio::placeholders::error));
	}

	void handle_read_header(const boost::system::error_code& error) {
		if (!error) {
			if (packet_->packet_size > PACKET_LIMIT) {
				server_->fire_error_event(-1, "packet_->packet_size > PACKET_LIMIT");				
				server_->fire_disconnected_event(this);
			}

			boost::asio::async_read(socket_,
				boost::asio::buffer(&packet_->data_start, packet_->getDataSize()),
				boost::bind(&Connection::handle_read_body, shared_from_this(),
					boost::asio::placeholders::error));
		} else {
			server_->fire_disconnected_event(this);
		}
	}

	void handle_read_body(const boost::system::error_code& error) {
		if (!error) {
			server_->fire_received_event(this, packet_);

			boost::asio::async_read(socket_,
				boost::asio::buffer(packet_, HEADER_SIZE),
				boost::bind(&Connection::handle_read_header, shared_from_this(),
					boost::asio::placeholders::error));
		} else {
			server_->fire_disconnected_event(this);
		}
	}

	void handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
		if (error) {
			server_->fire_disconnected_event(this);
		}
	}

	void do_connected(unsigned int id)
	{
		id_ = id;
		is_connected_ = true;

		idle_count = 0;
		is_logined = false;
		room_id = "";
		user_id = "";
		user_pw = "";
		user_level = 0;
	}

	void do_disconnected()
	{
		is_connected_ = false;

		idle_count = 0;
		is_logined = false;
		room_id = "";
		user_id = "";
		user_pw = "";
		user_level = 0;
	}

	void do_send(void* data, int size) {
		boost::asio::async_write(socket_, boost::asio::buffer(data, size),
			boost::bind(&Connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

	SocketServerInterface* server_;
	tcp::socket socket_;
	Packet* packet_;

	unsigned int id_ = 0;
	bool is_connected_ = false;
};

class SocketServer : public SocketServerInterface {
public:
	void start(int port) {
		try {
			acceptor_ = make_unique<tcp::acceptor>(io_context_, tcp::endpoint(tcp::v4(), port));
			start_accept();
			io_context_.run();
		} catch (exception& e) {
			fire_error_event(-1, e.what());
		}
	}

	void sendToAll(Packet* packet)
	{
		for (list<Connection*>::iterator i = connection_list_.begin(); i != connection_list_.end(); ++i){
			(*i)->send(packet);
		}
	}

	void sendToOther(Connection* connection, Packet* packet)
	{
		for (list<Connection*>::iterator i = connection_list_.begin(); i != connection_list_.end(); ++i){
			if (connection != (*i)) (*i)->send(packet);
		}
	}

	void sendToUserID(string user_id, Packet* packet)
	{
		for (list<Connection*>::iterator i = connection_list_.begin(); i != connection_list_.end(); ++i){
			if ((*i)->user_id == user_id) {
				(*i)->send(packet);
				break;
			}
		}
	}

	void sendToID(int id, Packet* packet)
	{
		// TODO:
	}

	void setOnError(const SocketErrorEvent &value) { on_error_ = value; }
	void setOnConnected(const ServerSocketEvent &value) { on_connected_ = value; }
	void setOnDisconnected(const ServerSocketEvent &value) { on_disconnected_ = value; }
	void setOnReceived(const ServerReceivedEvent &value) { on_received_ = value; }

protected:
	virtual void fire_error_event(int code, string msg)
	{
		if (on_error_ != nullptr) on_error_(code, msg);
	}

	virtual void fire_connected_event(Connection* connection)
	{
		connection->do_connected(id_++);
		connection_list_.push_back(connection);
		if (on_connected_ != nullptr) on_connected_(connection);
	}

	virtual void fire_disconnected_event(Connection* connection)
	{
		connection->do_disconnected();
		connection_list_.remove(connection);
		if (on_disconnected_ != nullptr) on_disconnected_(connection);
	}

	virtual void fire_received_event(Connection* connection, Packet* packet)
	{
		if (on_received_ != nullptr) on_received_(connection, packet);
	}

private:
	void start_accept() {
		Connection::pointer new_connection = Connection::create(this, io_context_);
		acceptor_->async_accept(new_connection->socket(),
			boost::bind(&SocketServer::handle_accept, this, new_connection,
				boost::asio::placeholders::error));
	}

	void handle_accept(Connection::pointer new_connection,
		const boost::system::error_code& error) {
		if (!error) {
			new_connection->start();
		}

		start_accept();
	}

	unsigned int id_ = 0;
	boost::asio::io_context io_context_;
	unique_ptr<tcp::acceptor> acceptor_;
	list<Connection*> connection_list_;

	SocketErrorEvent on_error_ = nullptr;
	ServerSocketEvent on_connected_ = nullptr;
	ServerSocketEvent on_disconnected_ = nullptr;
	ServerReceivedEvent on_received_ = nullptr;
};

#endif  // RYULIB_SOCKETSERVER_HPP