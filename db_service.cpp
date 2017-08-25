#include </home/mw/SQLAPI/include/SQLAPI.h>
#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include "./headers/request.h"
#include "./headers/thread_pool.h"
#include "./headers/db_buffer.h"
#include "./headers/logger.h"
#include "./headers/xml_settings.h"
      
int main() {
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
