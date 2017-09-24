#include "tcp_server.h"
#include "logger.h"

// TCP Server, manager over acceptor

tcp_server::tcp_server(db_service* _db_service) : db_service_{_db_service} {
  m_work.reset(new asio::io_service::work(m_ios));
}

void tcp_server::start(unsigned short port, unsigned short thread_pool_size) {
  assert(thread_pool_size > 0);

  // Create accept object and start accepting new connections.
  acc.reset(new tcp_acceptor(m_ios, port, db_service_));
  acc->start();

  // Create specified number of threads and add them to the pool.
  for (unsigned int i = 0; i < thread_pool_size; i++) {
    std::unique_ptr<std::thread> th( new std::thread([this]() { m_ios.run(); } ) );
    m_thread_pool.push_back(std::move(th));
  }
  // the server is almost ready so print a log message
  log_1("Server started");
}

void tcp_server::stop() {
  acc->stop();
  m_ios.stop();
  for (auto& th : m_thread_pool)
    th->join();
}
