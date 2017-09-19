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

class db_adjudicator {
  private:
    int req_id{0};
    int db_count{0};
    bool active {false}; // if the slot is active, then a client has a connection and has sent through a request.
                         // A tcp/ip socket would therefore have been set, and the request is currently in progress

    std::vector<db_executor> v_dg ; // vector of database executors within a request
    static std::mutex mx; // mutex used to synchronise the begins and locks
    bool first_done{false}; // boolean which is set once the first executor completes,
                            // so that communication with the client can proceed
                            // asynchronously while the other db_executors are still busy

    unsigned short statement_cnt{0};

    // logic booleans
    bool committed {false};
    bool rolled_back {false};
    bool verify_completed {false};
    bool comparator_pass {false}; // boolean set when all db_executors have completed execution of DML
                                  // and each result for each DML statement matches
    bool db_session_completed {false};  // set when a client issues disconnect
    std::string failure_msg {""};

    std::queue<std::string> msg_q; // q for seding message to session, which writes the msg to the socket
    void create_request(const std::string &, int); // client_msg, msg_cnt

    void start_request(); // executes "begin" with a mutex/lock guard
    void execute_request(); // runs database executors asynchronously
    void verify_request(); // equivalent of the comparator
    void commit_request(); // executes "commit" with a mutex/lock guard
                           // based on outcome of verify request as well as client
                           // explicit commit or rollback, once results of the execute_request have
                           // been sucessfully communicated to the client
    void rollback_request(); // can be called based on an explicit rollback from the client
                             // or if the comparator
    std::shared_ptr<tcp_session> tcp_sess;
  public:
    db_adjudicator(int rq_id, int db_cnt);

    void initialize(); // note: this needs to be called always immediately after creating this object
    const int get_req_id () const;
    const int get_statement_cnt () const;
    const bool is_active() const;
    void stop_session();
    void start_session(std::unique_ptr<tcp_session>&& ); // sets the private ptr to the tcp_session
                                                         // and starts reading from the socket
    const long get_session_id() const;
    void set_active(bool) ;  // set with a mutex in db_buffer when a slot in the buffer becomes available
    void set_connection_info(const db_info &);
    void make_connection();
    void disconnect();
    void handle_failure(const std::string &);
    bool reply_to_client_upon_first_done (int); //db_id
    void send_results_to_client(const std::vector<std::pair<char, std::string>> &); // v_results. is executed if reply_to_client_upon_first_done is true

    void process_request(); // called in a lambda (run in a boost thread) in db_service, serves as  entry point for kickstarting a client request
};
#endif
