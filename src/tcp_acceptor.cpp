#include "tcp_acceptor.h"
#include "logger.h"

extern logger exception_log;

tcp_acceptor::tcp_acceptor(asio::io_service& ios, unsigned short port_num) :
                   m_ios(ios)
                  ,m_acceptor(m_ios, asio::ip::tcp::endpoint( asio::ip::address_v4::any(), port_num))
                  ,inactive(false) {  }

void tcp_acceptor::start() {
  m_acceptor.listen();
  init_accept();
}

void tcp_acceptor::stop() {
  inactive.store(true);
}

void tcp_acceptor::init_accept() {
    std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_ios));
    m_acceptor.async_accept(*sock.get(),
                            [this, sock](const boost::system::error_code& error) { on_accept(error, sock);  }
                           );
}

void tcp_acceptor::on_accept(const boost::system::error_code&ec, std::shared_ptr<asio::ip::tcp::socket> sock) {
  if (ec == 0)
    (new tcp_service(sock, db_service_))->start_handling();
  else
      excep_log("Error code = " + std::to_string(ec.value()) + ": " + ec.message());
      // std::cout<< "Error occured! Error code = "  <<ec.value() << ". Message: " <<ec.message();

  if (!inactive.load())
    init_accept();
  else
    m_acceptor.close();
}
