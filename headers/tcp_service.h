#ifndef SERVICE_H
#define SERVICE_H

#include <boost/asio.hpp>
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>


using namespace boost;

class db_service;

class tcp_service {

  private:
    std::shared_ptr<asio::ip::tcp::socket> m_sock;
    std::string m_response;
    asio::streambuf m_request;
    db_service* db_service_{nullptr};

    void on_request_received(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void on_response_sent(const boost::system::error_code& ec, std::size_t bytes_transferred);
    // onFinish performs the cleanup.
    void on_finish();

    std::string process_request(asio::streambuf& req);

  public:
    tcp_service(std::shared_ptr<asio::ip::tcp::socket> sock, db_service* _db_service);
    void start_handling();
    void client_response(const std::string& msg);

};

#endif // SERVICE_H
