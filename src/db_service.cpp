#include <SQLAPI.h>
#include <boost/asio.hpp>
#include <algorithm>
#include <thread>
#include <condition_variable>
#include "db_service.h"
#include "thread_pool.h"
#include "threadsafe_queue.h"
#include "tcp_session.h"
#include "db_adjudicator.h"
#include "db_buffer.h"
#include "logger.h"
#include "xml_settings.h"
#include "globals.h"

// Manages the Interface between the Adjudicator and the TCP/IP Service

// Refer to this blog, since it effects performance regarding double moves
// http://scottmeyers.blogspot.co.uk/2014/07/should-move-only-types-ever-be-passed.html
// void db_service::add_request(tcp_session&& _tcp_session) {
void db_service::add_request(std::unique_ptr<tcp_session>&& _tcp_session) {
  tcp_sess_q.push_unique( std::move(_tcp_session) );
}

void db_service::stop() {
  stop_process = true;
}

void db_service::operator()() {
  thread_pool tp{12};
  db_buffer dbf(200);

  db_buffer * dbf_ptr = &dbf;
  db_adjudicator * rq{nullptr};

  do {
    auto tcp_sess = tcp_sess_q.pop_unique();
    try {
      log_2(std::to_string(tcp_sess->get_session_id()) + ":A");
      int rq_id = dbf.make_active (std::move(tcp_sess)); // blocks if no slot is free in the buffer. Uses a stack for managing the free list.
      
      rq = dbf.get_request(rq_id);
      tp.run_job( [rq, dbf_ptr ]() { rq->process_request(); if (rq->is_active()) dbf_ptr->make_inactive(rq->get_req_id()); });
    }
    catch ( ... ) {
      // tp.stop_service(); // destructor will stop the service
      throw;
    }    
  } while (!stop_process);

  tp.stop_service();
}
