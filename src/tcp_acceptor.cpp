#include "tcp_acceptor.h"
#include "logger.h"
#include "db_service.h"

extern logger exception_log;

tcp_acceptor::tcp_acceptor(asio::io_service& ios, unsigned short port, db_service* _db_service) :
  m_ios(ios),
  m_acceptor(m_ios, asio::ip::tcp::endpoint( asio::ip::address_v4::any(), port)),  m_isStopped(false),
  db_service_{_db_service} {  
}

void tcp_acceptor::start() {
  m_acceptor.listen();
  initialise();
}

void tcp_acceptor::stop() {
  m_isStopped.store(true);
}

void tcp_acceptor::initialise() {
  std::shared_ptr<asio::ip::tcp::socket> sk {std::make_shared<asio::ip::tcp::socket>(m_ios) };
  m_acceptor.async_accept(*sk.get(), [this, sk](const boost::system::error_code& ec) { add_session_to_buffer(ec, sk); });
}

void tcp_acceptor::add_session_to_buffer(const boost::system::error_code&ec, std::shared_ptr<asio::ip::tcp::socket> sock) {
  if (ec == 0) {
    session = std::make_unique<tcp_session>(sock);
    db_service_->add_request(std::move(session)); // move to db buffer
  }
  else
      excep_log("Error code = " + std::to_string(ec.value()) + ": " + ec.message());

  if (!m_isStopped.load())
    initialise();
  else
    m_acceptor.close();
}
