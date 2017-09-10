#include <SQLAPI.h>
#include <vector>
#include <boost/asio.hpp>
#include <cassert>
#include <algorithm>
#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "db_service.h"
#include "thread_pool.h"
#include "threadsafe_queue.h"
#include "tcp_session.h"
#include "request.h"
#include "db_buffer.h"
#include "logger.h"
#include "xml_settings.h"

std::condition_variable db_service::cv_queue_;
std::mutex db_service::mutex_;

// Refer to this blog, since it effects performance regarding double moves
// http://scottmeyers.blogspot.co.uk/2014/07/should-move-only-types-ever-be-passed.html
// void db_service::add_request(tcp_session&& _tcp_session) {
void db_service::add_request(std::unique_ptr<tcp_session>&& _tcp_session) {
  tcp_sess_q.push_unique( std::move(_tcp_session) );
  cv_queue_.notify_all();
}

void db_service::stop() {
  stop_process = true;
  excep_log("db_service::stop");
  cv_queue_.notify_all(); 
}

void db_service::operator()() {
  thread_pool tp{8};
  db_buffer dbf(10); // 100

  db_buffer * dbf_ptr = &dbf;
  request * rq{nullptr};

  do {
    auto tcp_sess = tcp_sess_q.pop_unique();
    // std::unique_ptr<tcp_session> tcp_sess = std::move(tcp_sess_q.pop_unique()); // blocks using a conditional variable with a unique_lock
  
    excep_log("tcp_sess set ");
    int rq_id = dbf.make_active (std::move(tcp_sess)); // blocks if no slot is free in the buffer. Uses a stack for managing the free list.

    rq = dbf.get_request(rq_id);
    excep_log("request_id " + std::to_string(rq_id));
    tp.run_job( [rq, dbf_ptr ]() {rq->process_request();  if (!rq->is_active()) dbf_ptr->make_inactive(rq->get_req_id()); });
  } while (!stop_process);

  tp.stop_service();
  excep_log("Thread pool service shut down");
}

