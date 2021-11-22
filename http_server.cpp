//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <sstream>

using namespace std;
using boost::asio::ip::tcp;
using boost::asio::io_service;

boost::asio::io_service io_context;

class session
: public std::enable_shared_from_this<session>
{
	public:
		char REQUEST_METHOD[100];
		char REQUEST_URI[100];
		char QUERY_STRING[1024];
		char SERVER_PROTOCOL[100];
		char HTTP_HOST[100];
		char status_str[200] = "HTTP/1.1 200 OK\n";
		std::string server;
		std::string server_port;
		std::string client;
		std::string client_port;
		session(tcp::socket socket)
			: socket_(std::move(socket))
		{
		}
		void BOOST_SETENV(){
			setenv("REQUEST_METHOD",REQUEST_METHOD,1);
			setenv("REQUEST_URI",REQUEST_URI,1);
			setenv("QUERY_STRING",QUERY_STRING,1);
			setenv("SERVER_PROTOCOL",SERVER_PROTOCOL,1);
			setenv("HTTP_HOST",HTTP_HOST,1);
			setenv("SERVER_ADDR",server.c_str(),1);
			setenv("SERVER_PORT",server_port.c_str(),1);
			setenv("REMOTE_ADDR",client.c_str(),1);
			setenv("REMOTE_PORT",client_port.c_str(),1);
		}
		void start()
		{
			do_read();
		}

	private:
		void do_read()
		{
			auto self(shared_from_this());
			socket_.async_read_some(boost::asio::buffer(data_, max_length),
					[this, self](boost::system::error_code ec, std::size_t length){
					if (!ec){
						do_write(length);
					}
					});
		}

		void do_write(std::size_t length)
		{
			auto self(shared_from_this());
			boost::asio::async_write(socket_, boost::asio::buffer(status_str, strlen(status_str)),
					[this, self](boost::system::error_code ec, std::size_t /*length*/){
					if (!ec){
						io_context.notify_fork(io_service::fork_prepare);
						if (fork() != 0) {
							io_context.notify_fork(io_service::fork_parent);
							socket_.close();
						} else {
							io_context.notify_fork(io_service::fork_child);
							/* Parse input env */
							char tmp[1124];
							std::stringstream ss;
							sscanf(data_,"%s %s %s\n\rHost: %s\n",
							REQUEST_METHOD,tmp,SERVER_PROTOCOL,HTTP_HOST);
							boost::asio::ip::tcp::endpoint remote_ep = socket_.remote_endpoint();
							boost::asio::ip::address remote_ad = remote_ep.address();
							client = remote_ad.to_string();
							ss << remote_ep.port();
							client_port = ss.str();
							ss.str("");
							boost::asio::ip::tcp::endpoint server_ep = socket_.local_endpoint();
							boost::asio::ip::address server_ad = server_ep.address();
							server = server_ad.to_string();
							ss << server_ep.port();
							server_port = ss.str();
							sscanf(tmp,"%[^?]?%s",REQUEST_URI,QUERY_STRING);
							BOOST_SETENV();
							/* hadle fd */
							int sock = socket_.native_handle();
							dup2(sock, STDIN_FILENO);
							dup2(sock, STDOUT_FILENO);
							string target_uri = string(REQUEST_URI);
							target_uri = "."+target_uri;
							socket_.close();
							if (target_uri != "./favicon.ico" && execlp(target_uri.c_str(), target_uri.c_str(), NULL) < 0) {
								cout << "Content-type:html/plain\r\n\r\nFAIL";
							}
						}
						do_read();
					}
					});
		}

		tcp::socket socket_;
		enum { max_length = 1024 };
		char data_[max_length];
};

class server
{
	public:
		server(boost::asio::io_context& io_context, short port)
			: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
		{
			do_accept();
		}

	private:
		void do_accept()
		{
			acceptor_.async_accept(
					[this](boost::system::error_code ec, tcp::socket socket)
					{
					if (!ec)
					{
					std::make_shared<session>(std::move(socket))->start();
					}

					do_accept();
					});
		}

		tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: async_tcp_echo_server <port>\n";
			return 1;
		}

		server s(io_context, std::atoi(argv[1]));

		io_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
