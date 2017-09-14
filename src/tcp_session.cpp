#include "tcp_session.h"

//https://stackoverflow.com/questions/28478278/working-with-boostasiostreambuf

tcp_session::tcp_session(std::shared_ptr<asio::ip::tcp::socket> sock) : m_sock{sock} { }

void tcp_session::start() {
  read_handler();
  /*
  asio::async_read_until(*m_sock.get(), m_request, SOCKET_MSG_END,
      [this](const boost::system::error_code& ec,
          std::size_t bytes_transferred)  {
              action_msg_received(ec, bytes_transferred);
          }
  );
  */
}

void tcp_session::stop() {
  if(m_sock) {
    boost::system::error_code ec;
    m_sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if(ec != 0)
      excep_log("Shutdown Error = " + std::to_string(ec.value()) + ": " + ec.message());
  }
  else {
    excep_log("tcp_session::stop: Warning - m_sock is null");
  }
}

void tcp_session::client_response(const std::string & msg) {
  // Initiate asynchronous write operation.
  std::string buf = msg + "\n"; // needs to have \n at the end - message format
  asio::async_write(*m_sock.get(),
      asio::buffer(buf), [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (ec != 0) {
          excep_log("Socket Write Error " + std::to_string(ec.value()) + ": " + ec.message());
        }  
      });
}

std::string tcp_session::req_to_str(std::size_t bytes_transferred) {
  
  boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  std::string buf_str(boost::asio::buffers_begin(bufs),
  boost::asio::buffers_begin(bufs) + bytes_transferred);
  // boost::asio::buffers_begin(bufs) + m_request.size() - 1);

  return buf_str;
}

void tcp_session::action_msg_received(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0) {
    excep_log("Socket Read Error " + std::to_string(ec.value()) + ": " + ec.message());
    msg_q.push(SOCKET_ERROR);
   // std::this_thread::sleep_for(std::chrono::seconds(2));
    stop_session();
    return;
  }
  
  socket_msg = req_to_str(bytes_transferred); 
  msg_q.push(socket_msg);  // push onto queue that is read from db_adjudicator
  
  //read_handler(ec, bytes_transferred); // continue reading
  read_handler(); // continue reading
}

//void tcp_session::read_handler(const boost::system::error_code& ec, std::size_t bytes_transferred) {
void tcp_session::read_handler() {
  m_request.consume(m_request.size()); // ensure buffer is empty prior to starting to read

  // if(socket_msg != TCPH_DISCONNECT) {
    asio::async_read_until(*m_sock.get(), m_request, SOCKET_MSG_END,
        [this](const boost::system::error_code& ec,
            std::size_t bytes_transferred) {
                excep_log("Before action message " + std::to_string(bytes_transferred) + " " + std::to_string(ec.value()) );
                action_msg_received(ec, bytes_transferred);
            }
    ); 
  //}
  //else 
  //  stop_session();
}

void tcp_session::stop_session() {
  stop();
}

// this is called from db_adjudicator class, once the request has been activated
// and is continually read until the request has been finalised.
std::string tcp_session::get_client_msg() {
  std::unique_lock<std::mutex> lk(tcp_sess_mx);
  cv_sess.wait(lk, [this]{ return ( !msg_q.empty() ); });
  std::string msg;
  msg = msg_q.front();
  msg_q.pop();
  excep_log("TCP_SESSION Queue message read " + msg);
  return msg;
}

