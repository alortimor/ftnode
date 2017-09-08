#ifndef SERVICE_H
#define SERVICE_H

#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>


using namespace boost;

class db_service;

class tcp_session {
public:
  tcp_session(std::shared_ptr<asio::ip::tcp::socket> sock, db_service* _db_service);
  void start();
  void stop();

  void client_response(const std::string& msg);
  std::shared_ptr<asio::ip::tcp::socket> m_sock;

private:
  void onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_transferred);

  void onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred);

  // onFinish performs the cleanup.
  void onFinish();

  //std::string ProcessRequest(asio::streambuf& req);
  std::string ProcessRequest(const std::string& buf_str);
  std::string req_to_str(asio::streambuf& req);

private:
  std::string socket_msg; // socket data received minus ending character
  std::string m_response;
  asio::streambuf m_request;
  int bytes_received;
  db_service* db_service_{nullptr};
};

#endif // SERVICE_H
