#include <string>
#include <utility>
#include "../headers/db_buffer.h"
#include "../headers/logger.h"
#include "../headers/xml_settings.h"

extern logger exception_log; // 


db_buffer::db_buffer(int buffer_size) : size{buffer_size} , slots_free{buffer_size}  {

  for (int i{0}; i<buffer_size; i++) {
    request_buffer.emplace(Key{i}, request(i) );
    st.push(i); // free list in the form of a stack
  }

  const auto& xml_db_sources = settings().get(xmls::ftnode_mw::DBSOURCES); // ELEM_DBSOURCES);

  for(int i{0}; i < static_cast<int>(xml_db_sources.size()); ++i) {
      auto xml_db_source = static_cast<xmls::db_source*>(xml_db_sources[i].get());
      v_dbi.emplace_back( db_info{xml_db_source->id
                                 ,xml_db_source->product
                                 ,xml_db_source->conn_str
                                 ,xml_db_source->user
                                 ,xml_db_source->password
                                 ,xml_db_source->begin_tr
                                 ,xml_db_source->iso_level != "server_specific"
                                 ,( xml_db_source->iso_level == "server_specific" 
                                    ? SA_RepeatableRead 
                                      : db_iso_level.at(xml_db_source->iso_level) 
                                  )
                                 ,db_con_id.at(xml_db_source->product)} );
    }
    
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
