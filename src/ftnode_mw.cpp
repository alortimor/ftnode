#include <thread>
#include <future>
#include "ftnode_mw.h"
#include "db_service.h"
#include "globals.h"
#include "xml_settings.h"
#include "logger.h"
#include "constants.h"
#include "globals.h"

void ftnode_mw::start() {
  db_service _db_service; 

  tcp_srv = std::make_unique<tcp_server>(&_db_service);

  if(tcp_srv == nullptr) {
    log_err("No tcp server");
    throw ftnode_exception(ERR_TCP_FAILURE);
  }

  auto fut1 = std::async (std::ref(_db_service));
  //std::thread db_service_thread(std::ref(_db_service));

  // endpoint has only one element at position 0
  auto _end_point = settings().get(xmls::ftnode::endpoint::ELEM_NAME).at(0).get();
  const unsigned short port_num = static_cast<xmls::end_point*>(_end_point)->port;
  try {
    unsigned int thread_pool_size = std::thread::hardware_concurrency()*2;
    tcp_srv->start(port_num, thread_pool_size);
    
    // TODO: code for user to close the program by pressing q or Q
    /*
    std::string input_str;
    while(input_str != "q" && input_str != "Q") {
      std::cin >> input_str;
      if(input_str == "q" || input_str == "Q")
        _db_service.stop();
    }*/
    // end of TODO

    //db_service_thread.join();
    fut1.get();
    tcp_srv->stop();
  }
  catch ( ... ) {
    tcp_srv->stop();
    throw;
  }
  /*catch (system::system_error&e) {
    log_err(e.what());
    tcp_srv->stop();
    throw;
  }
  catch (const ftnode_exception& e) {
    tcp_srv->stop();
    throw;
  }*/
}

