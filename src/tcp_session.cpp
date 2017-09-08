#include "tcp_session.h"
#include "db_service.h"
#include "tcp_msg_consts.h"

//https://stackoverflow.com/questions/28478278/working-with-boostasiostreambuf

tcp_session::tcp_session(std::shared_ptr<asio::ip::tcp::socket> sock, db_service* _db_service) :
    m_sock{sock}, db_service_{_db_service} { }

void tcp_session::start() {
    asio::async_read_until(*m_sock.get(),
        m_request,
        SOCKET_MSG_END,
        [this](const boost::system::error_code& ec,
            std::size_t bytes_transferred)
            {
                onRequestReceived(ec, bytes_transferred);
            }
    );
}

void tcp_session::stop() {
  if(m_sock) {
    boost::system::error_code ec;
    m_sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if(ec != 0)
      excep_log("tcp_session::stop: Error code = " + std::to_string(ec.value()) + ": " + ec.message());
  }
  else {
    excep_log("tcp_session::stop: Warning - m_sock is null");
  }
}

void tcp_session::client_response(const std::string& msg) {
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

std::string tcp_session::req_to_str(asio::streambuf& req) {
  boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  std::string buf_str(boost::asio::buffers_begin(bufs),
      boost::asio::buffers_begin(bufs) + m_request.size() - 1);
      
  return buf_str;
}

void tcp_session::onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0) {
    std::cout << "Error code = "  << ec.value() << " : " << ec.message();
    onFinish();
    return;
  }
  
  socket_msg = req_to_str(m_request);
  
  std::cout << "TCP msg: " << socket_msg << "size: " << socket_msg.size();
  {
    m_response = ProcessRequest(socket_msg);

    std::cout << socket_msg;
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

void tcp_session::onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  static int count = 0;
  count++;
  std::cout << "tcp_session::onResponseSent"  << std::endl;

  if (ec != 0)
    std::cout << "Error code = "  << ec.value() << " : " << ec.message();

  m_request.consume(m_request.size()); // 44
  if(socket_msg != TCPH_DISCONNECT)
  {
    asio::async_read_until(*m_sock.get(),
        m_request,
        SOCKET_MSG_END,
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

void tcp_session::onFinish() {
  std::cout << "tcp_session::onFinish() " << std::endl;
  stop();
}

std::string tcp_session::ProcessRequest(const std::string& buf_str) {
  std::string response;
  
  if(buf_str == TCPH_DISCONNECT)
    response = std::string("Disconnecting ...\n");
  else
  {
    response = "<Responding: " + buf_str.substr(0, buf_str.size()-1) + std::string(">\n");

    tcp_request _tcp_request;
    std::string kk = buf_str.substr(0, buf_str.size());
    std::cout << "::" << kk << "::\n";
    _tcp_request.sql_statement = buf_str.substr(0, buf_str.size()); 
    _tcp_request.socket_ = m_sock.get();
    db_service_->add_request(std::move(_tcp_request));
    // NOTE: _tcp_request moved
  }

  return response;
}
