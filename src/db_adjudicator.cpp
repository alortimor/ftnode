#include <algorithm>
#include <future>
#include <string>
#include <sstream>
#include "db_adjudicator.h"
#include "logger.h"
#include "tcp_session.h"

extern logger exception_log;

// Labelled as Adjudicaor in EU patent and Stankovic's thesis

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

void db_adjudicator::create_request(const std::string & msg, int msg_cnt) {
  // count of the number of sql statements
  committed=false;
  rolled_back=false;
  first_done=false;
  verify_completed=false;
  comparator_pass=false;
  statement_cnt = std::count (msg.begin(), msg.end(), ';');

  std::string sql_part; // sql satement
  unsigned short pos{0}; // position of ";" character
  unsigned short start{0}; // start position to initiate search from

  for ( auto & d : v_dg ) d.clear_sql_grains();

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
    return true;
  }
  else
    return false;
}

void db_adjudicator::start_request() {
  // Set BEGIN transaction statement.
  //for ( auto & d : v_dg )
  //  d.set_statement(d.get_begin_statement());

  // ensure begin transaction is performed exclusively
  {
    std::lock_guard<std::mutex> lk(mx);
    try {
      for ( auto & d : v_dg ) {
        d.exec_sql(d.get_begin_statement());
//        if( !d.get_db_info().properties.at("beg_tr2").empty() )
  //      {
      //    d.set_statement(d.get_db_info().properties.at("beg_tr2")); 
          d.exec_sql(d.get_db_info().properties.at("beg_tr2"));
    //    }
      }
    }
    catch (SAException &x) {
      failure_msg = "EXEC SQL Error: " + std::string( (const char*)x.ErrText() );
      log_err(failure_msg);
      throw std::runtime_error(failure_msg);
    }
  }
}

void db_adjudicator::execute_request() {
  // execute database grains asynchronously
  std::vector<std::future<void>> futures;

  for ( auto & d : v_dg )
    futures.push_back(std::async([&d] () { d.execute_sql_grains(); } ));

  try {
    for (auto & fut : futures)
      fut.get();
  }
  catch (const std::exception& e) { 
    log_err("execute_request: " + e.what());
    throw std::runtime_error(e.what());
  }
  verify_request();
  if (!comparator_pass) 
    throw std::runtime_error("COMPARATOR_FAIL " + failure_msg);
}

void db_adjudicator::process_request() {
  std::string msg;
  unsigned short msg_cnt{0};
  db_session_completed=false;
  while (!db_session_completed) {
    msg = "";
    msg = tcp_sess->get_client_msg();
    rtrim(msg, SOCKET_MSG_END_CHAR); // '\n'
    log_1("Client msg: " + msg + "\n");

    if (msg==COMMIT && verify_completed) {
        if ( (!committed) || (!rolled_back) ) { // verify in case user has sent commit twice
          if (comparator_pass) {
            commit_request();
            committed=true;
            rolled_back=false;
            verify_completed=false;
            comparator_pass=false;
            tcp_sess->client_response(COMMITED + SOCKET_MSG_END_CHAR); // "\n"
          }
          else {
            rollback_request();
            committed=false;
            rolled_back=true;
            verify_completed=false;
            comparator_pass=false;
            tcp_sess->client_response(ROLLED_BACK + SOCKET_MSG_END_CHAR); // "\n"
          }
        }
        msg_cnt=0;
    }
    else if (msg==ROLLBACK ) {
        if ( (!committed) && (!rolled_back) ) rollback_request();
        rolled_back=true;
        verify_completed=false;
        committed=false;
        comparator_pass=false;
        tcp_sess->client_response(ROLLED_BACK + SOCKET_MSG_END_CHAR); // "\n"
        msg_cnt=0;
    }
    else if (msg==DISCONNECT ) {
        if ( (!committed) && (!rolled_back) ) rollback_request(); // Ensure a rollback occurs for a premature disconnect
        rolled_back=true;
        committed=false;
        verify_completed=false;
        comparator_pass=false;
        db_session_completed=true;
        msg_cnt=0;
        // stop time stamp for this session
        if(tcp_sess)
          log_2(std::to_string(tcp_sess->get_session_id()));
        tcp_sess->client_response(DISCONNECTED + SOCKET_MSG_END_CHAR); // "\n"
        
        return; // once process_request is complete, db_buffer.make_inactive is run, which elegantly cleans memory 
    }
    else if (msg==SOCKET_ERROR) {
        if ( (!committed) && (!rolled_back) ) rollback_request(); // ensure rollback does not re-occur, since this causes db errors!!
        db_session_completed=true;
        return; // once process_request is complete, db_buffer make_inactive is run
    }
    else {
        msg_cnt++;
        create_request(msg, msg_cnt);
        if ( (msg_cnt==1) || ( (!rolled_back) && (!committed)) )
          start_request(); // only set snapshot if neccessary

        try {
          execute_request();
          log_1("Msg: " + msg + " executed");
        }
        catch(std::exception & e ) {
          handle_failure(e.what());
          msg_cnt=0;
        }
    }
  }
}

void db_adjudicator::verify_request() {
  comparator_pass=true;

  if (db_count == 1) {
    verify_completed=true;
    return;
  }

  for ( auto & d1 : v_dg ) {
    for ( auto & d2 : v_dg ) {
      if (d1.get_db_id() != d2.get_db_id()) {
        for (int i{0}; i<statement_cnt; ++i) {
          comparator_pass = (d1.get_rows_affected(i) == d2.get_rows_affected(i)) && (d1.get_hash(i) == d2.get_hash(i));
          if (!comparator_pass) {
            failure_msg = "REQ ID " + std::to_string(req_id) + " Session ID: " + std::to_string(tcp_sess->get_session_id()) + " "
                        + d1.get_hash(i) + " " + d2.get_hash(i) + " " + std::to_string(d1.get_rows_affected(i)) 
                        + " " + std::to_string(d2.get_rows_affected(i)) + " comprator fail ";
            break;
          }
        }
      }
      if (!comparator_pass) break;
    }
    if (!comparator_pass) break;
  }
  verify_completed=true;
}

void db_adjudicator::commit_request() {
  std::lock_guard<std::mutex> lk(mx);
  for ( auto & d : v_dg )
    d.commit();
  committed=true;
}

void db_adjudicator::rollback_request() {
  std::lock_guard<std::mutex> lk(mx);
  for ( auto & d : v_dg )
    d.rollback();
  rolled_back=true;
}

void db_adjudicator::start_session(std::unique_ptr<tcp_session>&& sess) {
  tcp_sess = std::move(sess); // moved from the tcp_server, which successfully accepted a network connection and established a network session
  tcp_sess->start(); // starts reading messages from the already opened network socket
}

void db_adjudicator::stop_session() { tcp_sess = nullptr; }

void db_adjudicator::disconnect() {
  for ( auto & d : v_dg )
    d.disconnect();
}

const long db_adjudicator::get_session_id() const {
  return tcp_sess->get_session_id();
}

const int db_adjudicator::get_req_id() const { return req_id; }

void db_adjudicator::set_active(bool current_active) { active = current_active; };

const bool db_adjudicator::is_active() const  { return active; }

const int db_adjudicator::get_statement_cnt() const  { return statement_cnt; };

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

void db_adjudicator::handle_failure(const std::string & err) {
  // issue rollback first-off before anything else
  //if ( (!committed) && (!rolled_back) ) {
  //  rollback_request();
  //}

  if ( err==COMPARATOR_FAIL )  {
    log_err(std::to_string(tcp_sess->get_session_id()) + " COMPARATOR_FAIL");
    tcp_sess->client_response(COMPARATOR_FAIL + SOCKET_MSG_END_CHAR); // "\n"
  }
  else {
    log_err(std::to_string(tcp_sess->get_session_id()) + " FAILURE " + err);
    tcp_sess->client_response(FAILURE + " Transaction rolled back " + err + SOCKET_MSG_END_CHAR); // "\n"
  }

  committed=false;
  rolled_back=false;
  first_done=false;
  comparator_pass=false;
  verify_completed=false;
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
        if ( dbi.properties.at("asynch_hash")=="Y"  ) { // check in place, since SQL Anywhere library cannot execute asynchronous queries
                                                        // on the same connection object, but Oracle & PostgreSQL are able to.
          int sid = s.get_statement_id();
          std::future<void> hash_result ( std::async([this, sid]() { execute_hash_select(sid);} ));
          std::future<void> select_result ( std::async([this, sid]() { execute_select(sid);} ));

          select_result.get();
          hash_result.get();
        }
        else {
          execute_select(s.get_statement_id());
          execute_hash_select(s.get_statement_id());
        }
      }
      else {
        try {
          set_statement(s.get_sql());
          cmd->Execute();
          s.set_db_return_values(false, cmd->RowsAffected() );
        }
        catch (SAException &x) {
          failure_msg = "DB Execute error: " + std::string( (const char*)x.ErrText() ) + " DB ID: " + std::to_string(db_id);
          cmd->Cancel();
          throw std::runtime_error(failure_msg);
        }
      }
    }
    catch (std::exception & e) {
      throw std::runtime_error(e.what());
    }

  }
  if (req.reply_to_client_upon_first_done(db_id) ) {
    try {
      prepare_client_results();
      const auto & v_result = get_sql_results();
      req.send_results_to_client(v_result);
    }
    catch (std::exception & e) {
      failure_msg = "Send Results error: " +std::string(e.what())+ " DB ID: " + std::to_string(db_id);
      throw std::runtime_error(failure_msg);
      throw;
    }
  }
}

