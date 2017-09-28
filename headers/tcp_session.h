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
#include "constants.h"
#include "logger.h"

// Main service between client and server
using namespace boost;

class tcp_session {
public:
  tcp_session(std::shared_ptr<asio::ip::tcp::socket>);
  void start();
  void client_response(const std::string&);
  std::string get_client_msg();
  const long  get_session_id() const;
  
private:
  void action_msg_received(const boost::system::error_code& ec, std::size_t bytes_transferred);
  //void read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred);
  void read_handler();

  std::string req_to_str(std::size_t bytes_transferred);
  std::shared_ptr<asio::ip::tcp::socket> m_sock;

  std::queue<std::string> msg_q;
  std::mutex tcp_sess_mx;
  std::condition_variable cv_sess;

  std::string socket_msg; // socket data received minus ending character
  std::string m_response;
  asio::streambuf m_request;
  bool msg_ready{false};
  static std::atomic<long> atomic_sess_id;
  long session_id;
};

#endif // SERVICE_H
