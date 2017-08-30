#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include "tcp_acceptor.h"

using namespace boost;

class db_service;
class tcp_server {
public:
  tcp_server(db_service* _db_service);
  // Start the server.
  void Start(unsigned short port_num, unsigned int thread_pool_size);
  // Stop the server.
  void Stop();

private:
  asio::io_service m_ios;
  std::unique_ptr<asio::io_service::work> m_work;
  std::unique_ptr<tcp_acceptor> acc;
  std::vector<std::unique_ptr<std::thread>> m_thread_pool;
  db_service* db_service_{nullptr};
};

#endif // SERVER_H
