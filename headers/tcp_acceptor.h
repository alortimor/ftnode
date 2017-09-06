#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include "tcp_service.h"

using namespace boost;

class db_service;
class tcp_acceptor {
public:
  tcp_acceptor(asio::io_service&ios, unsigned short port_num);
  // Start accepting incoming connection requests.
  void start();
  // Stop accepting incoming connection requests.
  void stop();

private:
  void init_accept();
  void on_accept(const boost::system::error_code&ec,  std::shared_ptr<asio::ip::tcp::socket> sock);

private:
  asio::io_service& m_ios;
  asio::ip::tcp::acceptor m_acceptor;
  std::atomic<bool> inactive;

};

#endif // ACCEPTOR_H
