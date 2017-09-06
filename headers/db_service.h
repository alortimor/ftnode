#ifndef DB_SERVICE_H
#define DB_SERVICE_H
#include <queue>
#include <condition_variable>
#include <thread>
#include <vector>
#include <memory>
#include <SQLAPI.h> // main SQLAPI++ header
#include <boost/asio.hpp>
#include <atomic>
#include <fstream>
#include <unordered_map>
#include "MPMCQueue.h"
#include "request.h"
#include "thread_pool.h"
#include "tcp_request.h"

const std::string ELEM_DBSOURCES_PRODUCT_ORACLE{"oracle"};
const std::string ELEM_DBSOURCES_PRODUCT_POSTGRES{"postgres"};
const std::string ELEM_DBSOURCES_PRODUCT_SQLANYWHERE{"sqlanywhere"};

class db_buffer;

class db_service {
public:
    //db_service(int thread_count);
    //~db_service() {}

    void operator()();
    void add_request(tcp_request&& _tcp_request);
    void stop();

protected:

private:
    static std::condition_variable cv_queue_;
    static std::mutex mutex_;

    rigtorp::MPMCQueue<tcp_request> requests_{50};
    std::unordered_map<int, db_info> dbm;
    
    bool stop_process{false};
};


#endif // DB_SERVICE_H
