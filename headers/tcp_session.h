#ifndef SERVICE_H
#define SERVICE_H

#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <queue>
#include <iostream>
#include "logger.h"

using namespace boost;

class tcp_session {
public:
  tcp_session(std::shared_ptr<asio::ip::tcp::socket>);
  void start();
  void stop();
  void client_response(const std::string&);
  std::string get_client_msg();

private:
  void onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_transferred);
  void onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred);

  // onFinish performs the cleanup.
  void onFinish();

  //std::string ProcessRequest(const std::string& buf_str);
  std::string req_to_str(std::size_t bytes_transferred);
  std::shared_ptr<asio::ip::tcp::socket> m_sock;

  std::queue<std::string> q;
  std::mutex tcp_sess_mx;
  std::condition_variable cv_sess;

  std::string socket_msg; // socket data received minus ending character
  std::string m_response;
  asio::streambuf m_request;
  bool msg_ready{false};
  // db_service* db_service_{nullptr};
};

#endif // SERVICE_H
