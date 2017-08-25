#include <algorithm>
#include <future>
#include <string>
#include <iostream>
#include <thread>
#include <sstream>
#include "../headers/request.h"
#include "../headers/logger.h"

extern logger exception_log;

/*
 * Order of processing
 * 1. Create Request
 * 2. Start Request
 * 3. Execute request
 *    a) Asnchronous execution of Database command object 1, for each sql grain
 *    b) Asnchronous execution of Database command object 2, for each sql grain
 *    c) ..
 * 4. Verify Request
 * 5. Commit Request
 */
 
std::string thread_id() {
  auto id = std::this_thread::get_id();
  std::stringstream s;
  s << id;
  return s.str();;
}

std::mutex request::mx;
const int request::db_count{3};

request::request(int rq_id) : req_id{rq_id} {  // the number of requests is finite (between 100 & 1000) and set at server startup time
  for (int i{0}; i<db_count; i++)              // therefore initialising the vector of database grains should be quick enough, given that
    v_dg.emplace_back(i);                      // the database count (db_count) will realistically never be more than 3 to 5
}

void request::create_request(const std::string msg) {
  // count of the number of sql statements
  statement_cnt = std::count (msg.begin(), msg.end(), ';');
  is_single_select = (statement_cnt == 1 && msg.substr(0, msg.find(' '))=="select" );

  std::string sql_part; // sql satement
  unsigned short pos{0}; // position of ";" character
  unsigned short start{0}; // start position to initiate search from

  for (int i{0}; i<statement_cnt; ++i) {
    pos = msg.find(';', start);
    sql_part = msg.substr(start, pos-start);
    start = pos+1;
    for ( auto & d : v_dg )
      d.add_sql_grain(i, sql_part);
  }
}

void request::start_request() {
  // Set BEGIN transaction statement.
  for ( auto & d : v_dg )
    d.set_statement(d.get_begin_statement());

  // ensure begin transaction is performed exclusively
  {
    std::lock_guard<std::mutex> lk(mx);
    for ( auto & d : v_dg )
      d.exec_begin();
  }
}

void request::execute_request(int rq_id) {
  // execute database grains asynchronously
  std::vector<std::future<void>> futures;

  for ( auto & d : v_dg )
    futures.push_back(std::async([&d, rq_id] () { d.execute_sql_grains(); } ));

  for (auto & fut : futures)
    fut.get();

  verify_request();
}
    
void request::process_request(const std::string str) {
  create_request(str);
  start_request();
  execute_request(req_id);
}

void request::verify_request() {
  comparator_pass=true;

  if (db_count == 1) {
    commit_request();
    return;
  }

  for ( auto & d1 : v_dg ) {
    for ( auto & d2 : v_dg ) {
      if (d1.get_db_id() != d2.get_db_id()) {
        for (int i{0}; i<statement_cnt; ++i) {
          comparator_pass = (d1.get_rows_affected(i) == d2.get_rows_affected(i))
                         && (d1.get_is_result(i) == d2.get_is_result(i));
          if (!comparator_pass) break;
        }
      }
      if (!comparator_pass) break;
    }
    if (!comparator_pass) break;
  }
  commit_request();
}

void request::commit_request() {
  if (comparator_pass) {
    {
      std::lock_guard<std::mutex> lk(mx);
      for ( auto & d : v_dg )
        d.commit_rollback('c');
    }
  }
  else  {
    {
      std::lock_guard<std::mutex> lk(mx);
      for ( auto & d : v_dg )
        d.commit_rollback('r');
    }
  }
}

void request::set_socket(boost::asio::ip::tcp::socket* sk) {
  socket = sk;
}

void request::disconnect() {
  for ( auto & d : v_dg )
    d.disconnect();
}

void request::set_connection_info(const db_info & dbi) {
  for ( auto & d : v_dg ) {
    if (d.get_db_id() == dbi.db_id )  {
      d.set_connection_info(dbi);
    }
  }
}

void request::make_connection() {
  for ( auto & d : v_dg ) {
    if (!d.make_connection()) break;
  }
}


/*
std::ostream & operator <<(std::ostream & o, const request & rq) {
  if (rq.statement_cnt > 0) {
    for ( const auto & d : rq.v_dg  ) {
      o << " Request ID : " << rq.req_id 
        << " Database ID : " << d->db_id;
      for ( const auto & s : d->s_vg) {
        o << " Statement ID : " << s->get_statement_id()
         << " SQL : " << s->get_sql() << "\n";
      }
    }
  }
  
  return  o;
}
*/
