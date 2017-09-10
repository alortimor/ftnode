#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include "tcp_session.h"

using namespace boost;

class db_service;
class tcp_acceptor {
public:
  tcp_acceptor(asio::io_service& , unsigned short, db_service*);
  // Start accepting incoming connection requests.
  void start();
  // Stop accepting incoming connection requests.
  void stop();

private:
  void initialise();
  void add_session_to_buffer(const boost::system::error_code&, std::shared_ptr<asio::ip::tcp::socket>);

private:
  asio::io_service& m_ios;
  asio::ip::tcp::acceptor m_acceptor;
  std::atomic<bool> m_isStopped;
  db_service* db_service_{nullptr};
  
  std::unique_ptr<tcp_session> session;

};

#endif // ACCEPTOR_H
