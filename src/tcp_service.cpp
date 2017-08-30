#include "tcp_service.h"
#include "db_service.h"

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

void tcp_service::onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0) {
    std::cout << "Error code = "  << ec.value() << " : " << ec.message();
    onFinish();
    return;
  }
  m_response = ProcessRequest(m_request);

  boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + m_request.size());
}

void tcp_service::onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0)
    std::cout << "Error code = "  << ec.value() << " : " << ec.message();
  onFinish();
}

void tcp_service::onFinish() {
  delete this;
}

std::string tcp_service::ProcessRequest(asio::streambuf& req) {
  boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  std::string buf_str(boost::asio::buffers_begin(bufs),
      boost::asio::buffers_begin(bufs) + m_request.size());
  std::string response {  "<" + buf_str.substr(0, buf_str.size()-1) + std::string(">") + "\n" };

  tcp_request _tcp_request;
  _tcp_request.sql_statement = {boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + (m_request.size() - 1)};
  _tcp_request.socket_ = m_sock.get();
  db_service_->add_reguest(std::move(_tcp_request));
  // NOTE: _tcp_request moved

  return response;
}
