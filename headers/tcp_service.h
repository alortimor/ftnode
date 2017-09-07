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
public:
  tcp_service(std::shared_ptr<asio::ip::tcp::socket> sock, db_service* _db_service);
  void StartHandling();

  void client_response(const std::string& msg);
  std::shared_ptr<asio::ip::tcp::socket> m_sock;

private:
  void onRequestReceived(const boost::system::error_code& ec, std::size_t bytes_transferred);

  void onResponseSent(const boost::system::error_code& ec, std::size_t bytes_transferred);

  // onFinish performs the cleanup.
  void onFinish();

  //std::string ProcessRequest(asio::streambuf& req);
  std::string ProcessRequest(const std::string& buf_str);
  std::string req_to_str(asio::streambuf& req);

private:
  
  std::string m_response;
  asio::streambuf m_request;
  db_service* db_service_{nullptr};
};

#endif // SERVICE_H
