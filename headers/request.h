#ifndef REQUEST_H_
#define REQUEST_H_
#include <mutex>
#include <vector>
#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include "db_info.h"
#include "db_executor.h"

class request {
  private:
    int req_id{0};
    bool active {false}; // if the slot is active, then a client has a connection and has sent through a request.
                         // A tcp/ip socket would therefore have been set, and the request is currently in progress
    std::vector<db_executor> v_dg ; // vector of database grains within a request
    static std::mutex mx;
    static const int db_count;

    bool comparator_pass {false};
    bool is_single_select {false};
    unsigned short statement_cnt{0};

    void create_request(const std::string msg);
    void start_request();
    void execute_request(int);
    void verify_request();
    void commit_request();
    
    boost::asio::ip::tcp::socket* socket {nullptr};

  public:

    request(int rq_id);

    const int get_req_id () const { return req_id; };
    const int get_statement_cnt () const { return statement_cnt; };
    bool is_active() { return active ; };
    void set_socket(boost::asio::ip::tcp::socket* sk);
    void set_active(bool current_active) { active = current_active; };
    void set_connection_info(const db_info &);
    void make_connection();
    void disconnect();
    bool is_one_db_executor_complete() const;

    // friend std::ostream & operator <<(std::ostream & o, const request & rq);
    void process_request(const std::string);
};
#endif
