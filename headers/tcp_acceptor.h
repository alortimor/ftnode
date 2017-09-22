#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include "tcp_session.h"

// Accepts network connections and moves socket to TCP Service, which forms part of the request
// that lives in the buffer

using namespace boost;

class db_service;

class tcp_acceptor {
  private:
    void initialise();
    void add_session_to_buffer(const boost::system::error_code&, std::shared_ptr<asio::ip::tcp::socket>);
    asio::io_service& m_ios;
    asio::ip::tcp::acceptor acc;
    std::atomic<bool> accept_cons; // not_accepting
    db_service* db_service_{nullptr};
    
    std::unique_ptr<tcp_session> session;

  public:
    tcp_acceptor(asio::io_service& , unsigned short, db_service*);
    // Start accepting incoming connection requests.
    void start();
    // Stop accepting incoming connection requests.
    void stop();
};

#endif // ACCEPTOR_H
