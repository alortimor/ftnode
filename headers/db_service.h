#ifndef DB_SERVICE_H
#define DB_SERVICE_H
#include <unordered_map>
#include <memory>
#include <SQLAPI.h> // main SQLAPI++ header
#include <boost/asio.hpp>
#include "threadsafe_queue.h"
#include "tcp_session.h"
#include "db_info.h"
#include "thread_pool.h"

// Manages the Interface between the Adjudicator and the TCP/IP Service
const std::string ELEM_DBSOURCES_PRODUCT_ORACLE{"oracle"};
const std::string ELEM_DBSOURCES_PRODUCT_POSTGRES{"postgres"};
const std::string ELEM_DBSOURCES_PRODUCT_SQLANYWHERE{"sqlanywhere"};

class db_buffer;

class db_service {
public:
    db_service() { }

    void operator()();
    void add_request(std::unique_ptr<tcp_session>&& );
    void stop();

private:
    threadsafe_queue<tcp_session> tcp_sess_q;
    std::unordered_map<int, db_info> dbm;
    bool stop_process{false};
};

#endif // DB_SERVICE_H
