#include "tcp_acceptor.h"
#include "logger.h"
#include "db_service.h"

// Accepts network connections and moves socket to TCP Service, which forms part of the request
// that lives in the buffer

extern logger exception_log;

tcp_acceptor::tcp_acceptor(asio::io_service& ios, unsigned short port, db_service* _db_service) :
     m_ios(ios)
    ,acc(m_ios, asio::ip::tcp::endpoint(asio::ip::address_v4::any(), port))
    ,not_accepting(false)
    ,db_service_{_db_service} {  }

void tcp_acceptor::start() {
  acc.listen();
  initialise();
}

void tcp_acceptor::stop() {
  not_accepting.store(true);
}

void tcp_acceptor::initialise() {
  std::shared_ptr<asio::ip::tcp::socket> sk {std::make_shared<asio::ip::tcp::socket>(m_ios) };
  acc.async_accept(*sk.get(), [this, sk](const boost::system::error_code& ec) { add_session_to_buffer(ec, sk); });
}

void tcp_acceptor::add_session_to_buffer(const boost::system::error_code&ec, std::shared_ptr<asio::ip::tcp::socket> sock) {
  if (ec == 0) {
    session = std::make_unique<tcp_session>(sock);
    db_service_->add_request(std::move(session)); // move to db buffer
  }
  else
      excep_log("Error code = " + std::to_string(ec.value()) + ": " + ec.message());

  if (!not_accepting.load())
    initialise();
  else
    acc.close();
}
