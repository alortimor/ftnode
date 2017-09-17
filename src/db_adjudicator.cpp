#include <algorithm>
#include <future>
#include <string>
#include <sstream>
#include "db_adjudicator.h"
#include "logger.h"
#include "tcp_session.h"

extern logger exception_log;

static std::string & ltrim(std::string &s, char c) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [c](int ch) { return !(ch==c);} ));
  return s;
}
  
static std::string & rtrim(std::string &s, char c) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [c](int ch) { return !(ch==c);} ).base(), s.end());
  return s;
}

static std::string & trim(std::string &s, char c) {
  ltrim(s, c);
  rtrim(s, c);
  return s;
}

/*
 * Order of processing
 * 1. Create Request - parse string buffer received from tcp/ip server into multiple sql statements
 * 2. Start Request - performs a synchronised explicit "begin"
 * 3. Execute request
 *    a) Asnchronous execution of Database Executor 1, for each sql grain
 *    b) Asnchronous execution of Database Executor 2, for each sql grain
 *    c) .. etc
 * 4. Client communication of the results is initiated when at least one of the db_executors completes steps 3
 *    This will occur asynchronously whilst step 5 is perfomed, but must be completed at some point in step 6.
 * 5. Verify Request. Performs comparison of results from db executors
 * 6. Commit Request. 
 *      Conditions for the commit to be performed are:
 *        a) All comparisons in step 5 were successfull
 *        b) All communication to the client, based on results of the first db_executor to complete, has been done
 *        c) The client, upon receiving all results, issues an explicit commit
*       Conditions for Rollback
*         a) If any single comparison in step 5 fails, rollback immediately and inform client
*         b) If all comparisons in step 5 were successfull and 
*            all communication to the client, based on results of the first db_executor to complete, has been done,
*            the client may however issue an explicit rollback.
*/

/*
std::string thread_id() {
  auto id = std::this_thread::get_id();
  std::stringstream s;
  s << id;
  return s.str();;
}
*/

std::mutex db_adjudicator::mx;

db_adjudicator::db_adjudicator(int rq_id, int db_cnt) : req_id{rq_id}, db_count{db_cnt} { }

void db_adjudicator::initialize() {      
  v_dg.clear();                   // the number of requests is finite (between 100 & 1000) and set at server startup time
  for (int i{0}; i<db_count; i++) // therefore initialising the vector of database grains should be quick enough, given that
    v_dg.emplace_back(i, *this);  // the database count (db_count) will realistically never be more than 3 to 5    
}

void db_adjudicator::create_request(const std::string & msg) {
  // count of the number of sql statements
  committed=false;
  rolled_back=false;
  statement_cnt = std::count (msg.begin(), msg.end(), ';');

  std::string sql_part; // sql satement
  unsigned short pos{0}; // position of ";" character
  unsigned short start{0}; // start position to initiate search from

  for (int i{0}; i<statement_cnt; ++i) {
    pos = msg.find(';', start);
    sql_part = msg.substr(start, pos-start);
    trim(sql_part, ' ');
    start = pos+1;
    for ( auto & d : v_dg )
      d.add_sql_grain(i, sql_part);
  }
  
}

// this member communicates back to the client when at least one of
// the db executors has completed.
// It is invoked based on a callback from the first db executor that completes
bool db_adjudicator::reply_to_client_upon_first_done (int db_id) {
  std::lock_guard<std::mutex> lk(mx);
  if (!first_done)  {
    first_done = true;
   // excep_log("Database ID " + std::to_string(db_id) + " completed in request " + std::to_string(req_id));
    return true;
  }
  else
    return false;
}

void db_adjudicator::set_active(bool current_active) { active = current_active; };

void db_adjudicator::start_request() {
  // Set BEGIN transaction statement.
  for ( auto & d : v_dg )
    d.set_statement(d.get_begin_statement());

  // ensure begin transaction is performed exclusively
  {
    std::lock_guard<std::mutex> lk(mx);
    for ( auto & d : v_dg )
      d.exec_sql();
  }
}

void db_adjudicator::execute_request(int rq_id) {
  // execute database grains asynchronously
  std::vector<std::future<void>> futures;

  for ( auto & d : v_dg )
    futures.push_back(std::async([&d, rq_id] () { d.execute_sql_grains(); } ));

  for (auto & fut : futures)
    fut.get();
    
}

void db_adjudicator::process_request() {
  std::string msg;
  unsigned short msg_cnt{0};
  while (!db_session_completed) {
    excep_log("Req ID " + std::to_string(req_id) + " before get_client_msg " );
    msg = tcp_sess->get_client_msg();
    rtrim(msg, '\n');
    excep_log("Req ID " + std::to_string(req_id) + " after get_client_msg " + msg );

    if (msg==COMMIT && verify_completed) {
        excep_log("Req ID COMMIT- " + std::to_string(req_id) + " ROLLBACK " + std::to_string(rolled_back) + " COMMITTED " + std::to_string(rolled_back));
        if ( (!committed) || (!rolled_back) ) { // verify in case user has sent commit twice
          excep_log("Req ID COMMIT- " + std::to_string(req_id) + " In COMMIT - comparator " + std::to_string(comparator_pass));
          if (comparator_pass) {
            commit_request();
            excep_log("Req ID COMMIT- " + std::to_string(req_id) + " ROLLBACK " + std::to_string(rolled_back) + " COMMITTED " + std::to_string(rolled_back) + " after commit ");
            committed=true;
            rolled_back=false;
            verify_completed=false;
            comparator_pass=false;
            tcp_sess->client_response(COMMITED+"\n");
          }
          else {
            rollback_request();
            excep_log("Req ID COMMIT- " + std::to_string(req_id) + " ROLLBACK " + std::to_string(rolled_back) + " COMMITTED " + std::to_string(rolled_back) + " after rollback ");
            committed=false;
            rolled_back=true;
            verify_completed=false;
            comparator_pass=false;
            tcp_sess->client_response(ROLLED_BACK + "\n");
          }
        }
        msg_cnt=0;
    }
    else if (msg==ROLLBACK ) {
        excep_log("Req ID ROLLBACK - " + std::to_string(req_id) + " ROLLBACK " + std::to_string(rolled_back) + " COMMITTED " + std::to_string(rolled_back));
        if ( (!committed) && (!rolled_back) ) rollback_request();
        tcp_sess->client_response(ROLLED_BACK + "\n");
        msg_cnt=0;
    }
    else if (msg==DISCONNECT ) {
        excep_log("Req ID DISCONNECT- " + std::to_string(req_id) + " ROLLBACK " + std::to_string(rolled_back) + " COMMITTED " + std::to_string(rolled_back));
        if ( (!committed) && (!rolled_back) ) rollback_request(); // Ensure a rollback occurs for a premature disconnect
        db_session_completed=true;
        tcp_sess->client_response(DISCONNECTED + "\n");
        return; // once process_request is complete, db_buffer.make_inactive is run, which elegantly cleans memory 
    }
    else if (msg==SOCKET_ERROR) {
        excep_log("Req ID SOCKET_ERROR" + std::to_string(req_id) + " ROLLBACK " + std::to_string(rolled_back) + " COMMITTED " + std::to_string(rolled_back));
        if ( (!committed) && (!rolled_back) ) rollback_request(); // ensure rollback does not re-occur, since this causes db errors!!
        db_session_completed=true;
        return; // once process_request is complete, db_buffer make_inactive is run
    }
    else {
        msg_cnt++;
        create_request(msg);
        if ( (msg_cnt==1) || (rolled_back) || (committed) )
          start_request(); // only set snapshot if neccessary
        execute_request(req_id);
        verify_request();
        verify_completed=true;
    }
  }
}

void db_adjudicator::verify_request() {
  comparator_pass=true;

  if (db_count == 1) return;

  for ( auto & d1 : v_dg ) {
    for ( auto & d2 : v_dg ) {
      if (d1.get_db_id() != d2.get_db_id()) {
        for (int i{0}; i<statement_cnt; ++i) {
          comparator_pass = (d1.get_rows_affected(i) == d2.get_rows_affected(i)) && (d1.get_hash(i) == d2.get_hash(i));
          if (!comparator_pass) {
            excep_log("Request ID: verify, hash " + d1.get_hash(i) + " " + d2.get_hash(i) + " row cnt " + std::to_string(d1.get_rows_affected(i)) + " " + std::to_string(d2.get_rows_affected(i)));
            break;
          }
        }
      }
      if (!comparator_pass) break;
    }
    if (!comparator_pass) break;
  }
}

void db_adjudicator::commit_request() {
  std::lock_guard<std::mutex> lk(mx);
  for ( auto & d : v_dg )
    d.commit();
  committed=true;
  excep_log("Req ID: commit_request " + std::to_string(req_id) + " COMMIT " + std::to_string(committed));
}

void db_adjudicator::rollback_request() {
  std::lock_guard<std::mutex> lk(mx);
  for ( auto & d : v_dg )
    d.rollback();
  rolled_back=true;
  excep_log("Req ID: rollback_request " + std::to_string(req_id) + " ROLLBACK " + std::to_string(rolled_back));
}

void db_adjudicator::set_session(std::unique_ptr<tcp_session>&& sess) {
  //excep_log("Req ID: before session started " );
  tcp_sess = std::move(sess); // moved from the tcp_server, which successfully accepted a network connection and established a network session
  tcp_sess->start(); // starts reading messages from the already opened network socket
  excep_log("Req ID: " + std::to_string(req_id) + " tcp session started");

}

void db_adjudicator::disconnect() {
  for ( auto & d : v_dg )
    d.disconnect();
}

void db_adjudicator::set_connection_info(const db_info & dbi) {
  for ( auto & d : v_dg ) {
    if (d.get_db_id() == dbi.db_id )  {
      d.set_connection_info(dbi);
    }
  }
}

void db_adjudicator::make_connection() {
  for ( auto & d : v_dg ) {
    if (!d.make_connection()) break;
  }
}

void db_adjudicator::send_results_to_client(const std::vector<std::pair<char, std::string>> & v_result) {
  for (const auto & r : v_result) {
    tcp_sess->client_response(r.second);
  }
  tcp_sess->client_response(CLIENT_MSG_END);
}

// member function of db_executor is defined in request due to the callback (reply_to_client_upon_first_don)
void db_executor::execute_sql_grains () {
  for ( auto & s : v_sg ) {
    try {      
      if (s.is_select()) {
        int sid = s.get_statement_id();
        std::future<void> hash_result ( std::async([this, sid]() { execute_hash_select(sid);} ));
        std::future<void> select_result ( std::async([this, sid]() { execute_select(sid);} ));

        hash_result.get();
        select_result.get();
      }
      else {
        set_statement(s.get_sql());
        cmd->Execute();
        s.set_db_return_values(false, cmd->RowsAffected() );
      }
    }
    catch (SAException &x) {
      excep_log( (const char*)x.ErrText() );
      s.set_db_return_values(false, -1);
      break;
    }

  }
  if (req.reply_to_client_upon_first_done(db_id) ) {
    prepare_client_results();
    const auto & v_result = get_sql_results();
    req.send_results_to_client(v_result);
    excep_log("REQ ID " + std::to_string(req.get_req_id()) + " after sending results");
  }
  
}

