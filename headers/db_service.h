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

#include "concurrentqueue.h"
#include "MPMCQueue.h"
//#include "constants.h"
//#include "globals.h"
#include "request.h"
#include "thread_pool.h"
#include "tcp_request.h"

const std::string ELEM_DBSOURCES_PRODUCT_ORACLE{"oracle"};
const std::string ELEM_DBSOURCES_PRODUCT_POSTGRES{"postgres"};

class db_buffer;

class db_service
{
public:
    //db_service(int thread_count);
    //~db_service() {}

    void operator()();
    void add_reguest(tcp_request&& _tcp_request);
    void stop();

protected:

private:
    static std::condition_variable cv_queue_;
    static std::mutex mutex_;

    static std::atomic<int> atomic_req_id;

    moodycamel::ConcurrentQueue<tcp_request> requests_old;
    rigtorp::MPMCQueue<tcp_request> requests_{50};
    std::unordered_map<int, db_info> dbm;
    
    bool stop_process{false};
    
    
  //void set_db_buffer(db_buffer* _db_buffer);    
  //db_buffer* db_buffer_{nullptr};
  //thread_pool tp{8};
  //int tc{0}; // thread count. Used for the thread pool.
};


#endif // DB_SERVICE_H
