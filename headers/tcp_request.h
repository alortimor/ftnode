#ifndef TCP_REQUEST_H
#define TCP_REQUEST_H
#include <string>
#include <boost/asio.hpp>

struct tcp_request
{
    std::string sql_statement;
    boost::asio::ip::tcp::socket* socket_{nullptr};

    tcp_request() = default;

    tcp_request& operator = (const tcp_request& req) noexcept
    {
        sql_statement = req.sql_statement ;
        socket_ = req.socket_;
        return *this;
    }

    tcp_request(const tcp_request& req)  noexcept
    {
        sql_statement = req.sql_statement ;
        socket_ = req.socket_;
    }
};

#endif // TCP_REQUEST_H
