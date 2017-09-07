#include "tcp_acceptor.h"
#include "logger.h"

extern logger exception_log;

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

tcp_service* jj{nullptr};

void tcp_acceptor::InitAccept() {
  std::cout<< "tcp_acceptor::InitAccept" << std::endl;
  /*if(jj)
  {
      //if (!jj->m_sock->is_open() )
        //std::cout << "*** socket not open!!\n";
    jj->StartHandling();
    //std::shared_ptr<asio::ip::tcp::socket>& sock{jj->m_sock};
    //m_acceptor.async_accept(*sock.get(),
                           // [this, sock](const boost::system::error_code& error) { onAccept(error, sock);  }
                           //);   
  }
  else*/
  {
    std::shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(m_ios));
    m_acceptor.async_accept(*sock.get(),
                            [this, sock](const boost::system::error_code& error) { onAccept(error, sock);  }
                           );
  }
}

void tcp_acceptor::onAccept(const boost::system::error_code&ec, std::shared_ptr<asio::ip::tcp::socket> sock) {
  std::cout<< "tcp_acceptor::onAccept" << std::endl;
  if (ec == 0)
  {
    //if(!jj)
      jj = new tcp_service(sock, db_service_);
      // move to db buffer
    (jj)->StartHandling(); // make part of make_active
  }
  else
      excep_log("Error code = " + std::to_string(ec.value()) + ": " + ec.message());
      // std::cout<< "Error occured! Error code = "  <<ec.value() << ". Message: " <<ec.message();

  if (!m_isStopped.load())
    InitAccept();
  else
    m_acceptor.close();
}
