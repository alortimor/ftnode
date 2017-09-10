#include "tcp_session.h"
#include "tcp_msg_consts.h"

//https://stackoverflow.com/questions/28478278/working-with-boostasiostreambuf

tcp_session::tcp_session(std::shared_ptr<asio::ip::tcp::socket> sock) : m_sock{sock} { }

void tcp_session::start() {
    asio::async_read_until(*m_sock.get(),
        m_request,
        SOCKET_MSG_END,
        [this](const boost::system::error_code& ec,
            std::size_t bytes_transferred)  {
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

std::string tcp_session::req_to_str(std::size_t bytes_transferred) {
  boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  std::string buf_str(boost::asio::buffers_begin(bufs),
  //boost::asio::buffers_begin(bufs) + bytes_transferred);
  boost::asio::buffers_begin(bufs) + m_request.size() - 1);

  return buf_str;
}

void tcp_session::onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0) {
    excep_log("onRequestReceived Error " + std::to_string(ec.value()) + ": " + ec.message());
    onFinish();
    return;
  }
  
  socket_msg = req_to_str(bytes_transferred); 
  q.push(socket_msg);  // push onto queue that is read from request
  
  onResponseSent(ec, bytes_transferred);
}

void tcp_session::onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0) {
    excep_log("onResponseSent Error " + std::to_string(ec.value()) + ": " + ec.message());
    onFinish();
  }

  m_request.consume(m_request.size()); // 44
  if(socket_msg != TCPH_DISCONNECT) {
    asio::async_read_until(*m_sock.get(),
        m_request,
        SOCKET_MSG_END,
        [this](const boost::system::error_code& ec,
            std::size_t bytes_transferred) {
                onRequestReceived(ec, bytes_transferred);
            }
    ); 
  }
  else 
    onFinish();
}

void tcp_session::onFinish() {
  stop();
}

std::string tcp_session::get_client_msg() {
  std::unique_lock<std::mutex> lk(tcp_sess_mx);
  cv_sess.wait(lk, [this]{return ( !q.empty() ); });
  std::string q_msg;
  q_msg = q.front();
  q.pop();
  return q_msg;
}

