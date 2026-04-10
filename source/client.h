#pragma once

#include "asio.hpp"
#include <string>
#include <memory>
#include <mutex>

class client : public std::enable_shared_from_this<client> {
public:
	enum Reason {
		Clean,
		Timeout,
		Reset,
		Error,
		Manual
	};
	using Disconnect = std::function<void(Reason, std::string)>;

	client(asio::ip::tcp::socket socket);
	~client();

	void start(Disconnect disconnect);
	void close(Reason reason);
	void write(const std::string& message);

private:
	void do_read();
	void do_send(const std::string& message);
	void do_disconnect(const asio::error_code& error);

private:
	asio::ip::tcp::socket m_socket;
	std::array<uint8_t, 1024> m_buffer;
	uint16_t m_port{};
	std::string m_address{};
	std::string m_number{};
	bool m_connected{};
	Disconnect m_disconnect{};
};
