#include "tcp_session.h"

//https://stackoverflow.com/questions/28478278/working-with-boostasiostreambuf

// TCP Server, manager over acceptor

std::atomic<long> tcp_session::atomic_sess_id{0};

tcp_session::tcp_session(std::shared_ptr<asio::ip::tcp::socket> sock) : m_sock{sock} {
  ++atomic_sess_id;
  session_id =  atomic_sess_id.load();
  // start time stamp for this session
  log_2(std::to_string(session_id));
}

void tcp_session::start() {
  read_handler();
}

const long  tcp_session::get_session_id() const {
  return session_id;
}

/*void tcp_session::stop() {
  if(m_sock) {
    boost::system::error_code ec;
    m_sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if(ec != 0)
      log_1("Shutdown Error = " + std::to_string(ec.value()) + ": " + ec.message());
  }
  else {
    log_1("tcp_session::stop: Warning - m_sock is null");
  }
}*/

void tcp_session::client_response(const std::string & msg) {
  // Initiate asynchronous write operation.
  std::string buf = msg + SOCKET_MSG_END_CHAR; // "\n" needs to have \n at the end - message format
  //asio::write(*m_sock.get(), asio::buffer(buf));
  asio::async_write(*m_sock.get(),
      asio::buffer(buf), [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
        if (ec != 0) {
          log_err("Socket Write Error " + std::to_string(ec.value()) + ": " + ec.message());
        }  
      });
}

std::string tcp_session::req_to_str(std::size_t bytes_transferred) {
  boost::asio::streambuf::const_buffers_type bufs = m_request.data();
  std::string buf_str(boost::asio::buffers_begin(bufs),
  boost::asio::buffers_begin(bufs) + bytes_transferred);
  return buf_str;
}

void tcp_session::action_msg_received(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  if (ec != 0) {
    msg_q.push(SOCKET_ERROR);
    cv_sess.notify_one();
    //stop_session();
    return;
  }
  
  socket_msg = req_to_str(bytes_transferred); 
  msg_q.push(socket_msg);  // push onto queue that is read from db_adjudicator
  cv_sess.notify_one();

  m_request.consume(bytes_transferred); // ensure buffer is empty prior to starting to read
  read_handler(); // continue reading
}

void tcp_session::read_handler() {
  asio::async_read_until(*m_sock.get(), m_request, SOCKET_MSG_END_CHAR, // '\n'
      [this](const boost::system::error_code& ec,
          std::size_t bytes_transferred) {
              // log_1("Before action " + std::to_string(bytes_transferred) + " " + std::to_string(ec.value()) );
              action_msg_received(ec, bytes_transferred);
          }
  );
}

/*void tcp_session::stop_session() {
  log_1("Stop Session "+ std::to_string(session_id));
  stop();
}*/

// this is called from db_adjudicator class, once the request has been activated
// and is continually read until the request has been finalised.
std::string tcp_session::get_client_msg() {
  std::string msg;
  {
    std::unique_lock<std::mutex> lk(tcp_sess_mx);
    cv_sess.wait(lk, [this]{ return ( !msg_q.empty() ); });
    msg = msg_q.front();
    msg_q.pop();
  }

  return msg;
}

