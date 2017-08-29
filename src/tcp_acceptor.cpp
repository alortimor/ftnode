#include "../headers/tcp_acceptor.h"

tcp_acceptor::tcp_acceptor(asio::io_service&ios, unsigned short port_num, db_service* _db_service) :
  m_ios(ios),
  m_acceptor(m_ios,
  asio::ip::tcp::endpoint(
  asio::ip::address_v4::any(),
  port_num)),
  m_isStopped(false),
  db_service_{_db_service}  {  }

void tcp_acceptor::Start() {
  m_acceptor.listen();
  InitAccept();
}

void tcp_acceptor::Stop() {
  m_isStopped.store(true);
}

void tcp_acceptor::InitAccept() {
    std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_ios));
    m_acceptor.async_accept(*sock.get(),
                            [this, sock](const boost::system::error_code& error) { onAccept(error, sock);  }
                           );
}

void tcp_acceptor::onAccept(const boost::system::error_code&ec, std::shared_ptr<asio::ip::tcp::socket> sock) {
  if (ec == 0)
    (new tcp_service(sock, db_service_))->StartHandling();
  else
      excep_log("Error code = " + e.code() + ": " + e.what());
      // std::cout<< "Error occured! Error code = "  <<ec.value() << ". Message: " <<ec.message();

  if (!m_isStopped.load())
    InitAccept();
  else
    m_acceptor.close();
}
