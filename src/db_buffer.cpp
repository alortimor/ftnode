#include <string>
#include <utility>
#include "../headers/db_buffer.h"
#include "../headers/logger.h"

extern logger exception_log;

db_buffer::db_buffer(int buffer_size) : size{buffer_size} , slots_free{buffer_size}  {
  for (int i{0}; i<buffer_size; i++) {
    request_buffer.emplace(Key{i}, request(i) );
    st.push(i); // free list in the form of a stack
  }

 /* v_dbi.emplace_back( db_info{0,"postgre","10.11.12.6,5432@ft_node","pg","pg","begin transaction read write","begin transaction read only",true,true,SA_RepeatableRead,SA_PostgreSQL_Client} );
  v_dbi.emplace_back( db_info{1,"oracle","//10.11.12.9:1521/ftnode","ordb","ordb","set transaction isolation level serializable", "set transaction isolation level serializable",false,false,SA_Serializable,SA_Oracle_Client} );
  v_dbi.emplace_back( db_info{2,"sqlanywhere","links=tcpip(host=10.11.12.10;port=2638);databasename=ftnode_sa;servername=ftnode_db","sadb","sadb","begin snapshot","begin snapshot",true,false,SA_RepeatableRead,SA_SQLAnywhere_Client} );
*/
  v_dbi.emplace_back( db_info{0,"postgre","10.11.12.6,5432@ft_node","pg","pg","select 1",true,SA_RepeatableRead,SA_PostgreSQL_Client} );
  v_dbi.emplace_back( db_info{1,"oracle","//10.11.12.9:1521/ftnode","ordb","ordb","select 1 from dual", true,SA_Serializable,SA_Oracle_Client} );
  v_dbi.emplace_back( db_info{2,"sqlanywhere","links=tcpip(host=10.11.12.10;port=2638);databasename=ftnode_sa;servername=ftnode_db","sadb","sadb","select 1",false,SA_RepeatableRead,SA_SQLAnywhere_Client} );


 // v_dbi.emplace_back( db_info{0,"postgre","10.11.12.11,5432@ft_node","pg","pg","select 1",true,SA_RepeatableRead,SA_PostgreSQL_Client} );
 // v_dbi.emplace_back( db_info{1,"oracle","//10.11.12.18:1521/FTNODE","ordb","ordb","select 1 from dual", true,SA_Serializable,SA_Oracle_Client} );
 // v_dbi.emplace_back( db_info{2,"sqlanywhere","links=tcpip(host=10.11.12.17;port=49153);databasename=ftnode_sa;servername=ftnode_sa","sadb","sadb","select 1",false,SA_RepeatableRead,SA_SQLAnywhere_Client} );
  set_db_info();
  make_connections();
}

db_buffer::~db_buffer() { }

auto db_buffer::percent_free() const {
  return (slots_free/size)*100;
}

// the return ptr is used in a lambda, to asynchronously process requests
request * db_buffer::get_request(const int rq_id) { return &request_buffer.at({rq_id});}

void db_buffer::make_connections() {
  for (auto & rq : request_buffer)
    (rq.second).make_connection();
}

void db_buffer::set_db_info() {
  for (auto & rq : request_buffer) {
    for (const auto & temp_d : v_dbi) {
       (rq.second).set_connection_info(temp_d);
    }
  }
}

bool db_buffer::make_inactive (const int req_id) {
  {
    std::lock_guard<std::mutex> lk(mx);
    st.push(req_id);
    request_buffer.at({req_id}).set_active(false);
    request_buffer.at({req_id}).set_socket(nullptr);
    slots_free++;
  }
  cv_stack.notify_one();
  return true;
}

int db_buffer::make_active (boost::asio::ip::tcp::socket *socket) {
  int rq_id;
  {
    std::unique_lock<std::mutex> lk(mx);
    const int& slot_free_ref{slots_free};
    cv_stack.wait(lk, [&slot_free_ref]{ return (slot_free_ref>0) ;});
    
    slots_free--;
    rq_id = st.top();
    st.pop();
    request_buffer.at({rq_id}).set_active(true);
    request_buffer.at({rq_id}).set_socket(socket);
  }
  return rq_id;
}
