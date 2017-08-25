#include </home/mw/SQLAPI/include/SQLAPI.h>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include "../headers/request.h"
#include "../headers/thread_pool.h"
#include "../headers/db_buffer.h"
#include "../headers/logger.h"
#include "../headers/xml_settings.h"
/////////////////////////
#include <thread>
#include <iostream>
#include <strstream>
#include <map>
#include <cassert>
#include <algorithm>
#include <string>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
//#include "globals.h"
#include "../headers/db_service.h"
//#include "xml_settings.h"
//#include "request.h"
#include "../headers/thread_pool.h"
//#include "db_grain.h"
//#include "db_buffer.h"

std::condition_variable db_service::cv_queue_;
std::mutex db_service::mutex_;

//db_service::db_service(int thread_count)
//{
//}

void db_service::add_reguest(tcp_request&& _tcp_request)
{
    requests_.push(std::forward<tcp_request>(_tcp_request));
    cv_queue_.notify_all();
}

void db_service::stop(){ 
  stop_process = true; 
  excep_log("db_service::stop");
  cv_queue_.notify_all(); 
}

void db_service::operator()()
{
  thread_pool tp{8};
  db_buffer dbf(8); // 100
  
  db_buffer * dbf_ptr = &dbf;
  request * rq{nullptr};
  
  while(!stop_process)
  {
    tcp_request _tcp_request;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_queue_.wait(lock, [this,&_tcp_request]{return (stop_process || requests_.try_pop(_tcp_request));});
    }
    if(!stop_process)
    {
      do
      {
        int rq_id = dbf.make_active (_tcp_request.socket_);
        rq = dbf.get_request(rq_id);
        const auto & s = _tcp_request.sql_statement;
        tp.run_job( [rq, s, dbf_ptr ]() {  
          rq->process_request(s);  
          if (!rq->is_active()) 
            dbf_ptr->make_inactive(rq->get_req_id());  
        });
        
        //ERROR   DO THIS!!!
          /*int req_id = get_req_id();
          request * req = &(db_buffer_->get_request(req_id));
          std::string sql =_tcp_request.sql_statement;
          tp.run_job( [req, sql ](){ req->process_request(sql); } );*/
      } while(requests_.try_pop(_tcp_request));
    }
  }
  tp.stop_service();
  excep_log("After stop_service");
}

int main2() {
  settings().load_settings_xml(xmls::DEF_SETTING_FILE_NAME);
  
  std::vector<std::string> v_sql_req;
  thread_pool tp{8};
  db_buffer dbf(8); //n

  boost::asio::ip::tcp::socket *sk{nullptr};

  v_sql_req.emplace_back("insert into test1 (x,y) values (1,'Red'); insert into test1 (x,y) values (2,'Blue'); delete from test1 where x=1;"); 
  v_sql_req.emplace_back("insert into test1 (x,y) values (3,'Green'); insert into test1 (x,y) values (4,'Orange'); delete from test1 where x=3;");
  v_sql_req.emplace_back("insert into test1 (x,y) values (5,'Brown');");
  v_sql_req.emplace_back("insert into test1 (x,y) values (6,'White');");

  db_buffer * dbf_ptr = &dbf;
  request * rq;

  for (const auto & s : v_sql_req) {
    int rq_id = dbf.make_active (sk);
    rq = dbf.get_request(rq_id);
    tp.run_job( [rq, s, dbf_ptr ]() {  rq->process_request(s);  if (!rq->is_active()) dbf_ptr->make_inactive(rq->get_req_id());  } );
  }

  tp.stop_service();
  excep_log("After stop_service");

  return 0;
}

/*
void db_service::set_db_buffer(db_buffer* _db_buffer)
{
    db_buffer_ = _db_buffer;
}
*/
