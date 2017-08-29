#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include "tcp_service.h"

using namespace boost;

class db_service;
class tcp_acceptor {
public:
  tcp_acceptor(asio::io_service&ios, unsigned short port_num, db_service* _db_service);
  // Start accepting incoming connection requests.
  void Start();
  // Stop accepting incoming connection requests.
  void Stop();

private:
  void InitAccept();
  void onAccept(const boost::system::error_code&ec,  std::shared_ptr<asio::ip::tcp::socket> sock);

private:
  asio::io_service& m_ios;
  asio::ip::tcp::acceptor m_acceptor;
  std::atomic<bool> m_isStopped;
  db_service* db_service_{nullptr};

};

#endif // ACCEPTOR_H
