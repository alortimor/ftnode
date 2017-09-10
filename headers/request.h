#ifndef REQUEST_H_
#define REQUEST_H_
#include <mutex>
#include <vector>
#include <memory>
#include <boost/asio.hpp>
#include <condition_variable>
#include "tcp_session.h"
#include "db_info.h"
#include "db_executor.h"

class request {
  private:
    int req_id{0};
    int db_count{0};
    bool active {false}; // if the slot is active, then a client has a connection and has sent through a request.
                         // A tcp/ip socket would therefore have been set, and the request is currently in progress

    std::vector<db_executor> v_dg ; // vector of database executors within a request
    static std::mutex mx; // mutex used to synchronise the begins and locks
    bool first_done{false};

    bool comparator_pass {false};
    bool is_single_select {false};
    unsigned short statement_cnt{0};
    bool request_completed {true};

    void create_request(const std::string &); // sql_received
    void start_request(); // executes "begin" synchronously with a mutex/lock guard
    void execute_request(int); // executes database executors asynchronously
    void verify_request(); // equivalent of the comprator
    void commit_request(); // executes "commit" or "rollback" synchronously with a mutex/lock guard
                           // based on outcome of verify request as well as client
                           // explicit commit or rollback, once results of the execute_request have
                           // been sucessfully communicated to the client
    
    //boost::asio::ip::tcp::socket* socket {nullptr};
    std::shared_ptr<tcp_session> tcp_sess;
    std::string sql_received;
    std::mutex sess_mx;
    std::condition_variable cv_sess;

  public:

    request(int rq_id, int db_cnt);

    void initialize(); // note: this needs to be called always immediately after creating this object 
    const int get_req_id () const { return req_id; };
    const int get_statement_cnt () const { return statement_cnt; };
    bool is_active() { return active ; };
    bool is_one_select() { return is_single_select; };
    void set_session(std::unique_ptr<tcp_session>&& );
    void set_active(bool) ;
    void set_connection_info(const db_info &);
    void make_connection();
    void disconnect();
    bool is_one_db_executor_complete() const;
    void reply_to_client_upon_first_done (int);

    // friend std::ostream & operator <<(std::ostream & o, const request & rq);
    void process_request();
   // void process_request(const std::string);
};
#endif
