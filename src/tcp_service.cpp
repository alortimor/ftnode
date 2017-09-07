#include "tcp_service.h"
#include "db_service.h"
#include "tcp_msg_consts.h"

tcp_service::tcp_service(std::shared_ptr<asio::ip::tcp::socket> sock, db_service* _db_service) :
    m_sock{sock}, db_service_{_db_service} { }

void tcp_service::StartHandling() {
    asio::async_read_until(*m_sock.get(),
        m_request,
        '\n',
        [this](const boost::system::error_code& ec,
            std::size_t bytes_transferred)
            {
                onRequestReceived(ec, bytes_transferred);
            }
    );
    

    
   /* asio::async_read_until(*m_sock.get(),
        m_request,
        '\n',
        //StartHandling, this
        [this](const boost::system::error_code& ec,
            std::size_t bytes_transferred)
            {
                onRequestReceived(ec, bytes_transferred);
            }
    );   */      
    
    //onFinish();
}

void tcp_service::client_response(const std::string& msg) {
  // Initiate asynchronous write operation.
  std::string msg_suff = msg + "\n"; // needs to have \n at the end - message format
  asio::async_write(*m_sock.get(),
      asio::buffer(msg_suff),
      [this](
      const boost::system::error_code& ec,
      std::size_t bytes_transferred) {
        onResponseSent(ec, bytes_transferred);
      });
}

std::string tcp_service::req_to_str(asio::streambuf& req) {
  boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  std::string buf_str(boost::asio::buffers_begin(bufs),
      boost::asio::buffers_begin(bufs) + m_request.size() - 1);
      
  //std::string kk = buf_str.substr(0, buf_str.size()-1);
  //std::string kk2 ;
  //std::cout << "ZZ::" << std::string{boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + (m_request.size() - 1)} << "::ZZ\n";      
      
  return buf_str;
}

void tcp_service::onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0) {
    std::cout << "Error code = "  << ec.value() << " : " << ec.message();
    onFinish();
    return;
  }
  std::string buf_str{req_to_str(m_request)};
  std::cout << "TCP: " << buf_str;
  if(buf_str == TCPH_DISCONNECT)
  {
    std::cout << "disconnecting ...\n";
    onFinish();
  }
  else
  {
    m_response = ProcessRequest(buf_str); // m_request

    boost::asio::streambuf::const_buffers_type bufs = m_request.data();
    std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + m_request.size());
    
    //int k;
      //std::cout << "press key to send reply to client\n";
      //std::cin >> k;
      std::cout << "a moment ...\n";
      //std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << str;
      // Initiate asynchronous write operation.
      asio::async_write(*m_sock.get(),
          asio::buffer(m_response),
          [this](
          const boost::system::error_code& ec,
          std::size_t bytes_transferred)
          {
              onResponseSent(ec, bytes_transferred);
          });
    }
}

void tcp_service::onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  static int count = 0;
  count++;
  std::cout << "tcp_service::onResponseSent"  << std::endl;
  /*if(count <= 1)
  {
    //StartHandling();
    return;
  }*/
  if (ec != 0)
    std::cout << "Error code = "  << ec.value() << " : " << ec.message();
  //onFinish();
  if(count <= 2)
  {
    m_request.consume(44); // 44
    asio::async_read_until(*m_sock.get(),
        m_request,
        '\n',
        //StartHandling, this
        [this](const boost::system::error_code& ec,
            std::size_t bytes_transferred)
            {
                onRequestReceived(ec, bytes_transferred);
            }
    ); 
  }
  else
    onFinish();
}

void tcp_service::onFinish() {
  delete this;
  std::cout << "tcp_service::onFinish() " << std::endl;
}

// socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec)

//std::string tcp_service::ProcessRequest(asio::streambuf& req) {
std::string tcp_service::ProcessRequest(const std::string& buf_str) {
  //boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  //std::string buf_str(boost::asio::buffers_begin(bufs),
      //boost::asio::buffers_begin(bufs) + m_request.size());
  std::string response {  "<Responding: " + buf_str.substr(0, buf_str.size()-1) + std::string(">") + "\n" };

  tcp_request _tcp_request;
  std::string kk = buf_str.substr(0, buf_str.size());
  std::cout << "::" << kk << "::\n";
  _tcp_request.sql_statement = buf_str.substr(0, buf_str.size()); //{boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + (m_request.size() - 1)};
  _tcp_request.socket_ = m_sock.get();
  db_service_->add_request(std::move(_tcp_request));
  // NOTE: _tcp_request moved

  return response;
}
