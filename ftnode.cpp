#include "./headers/ftnode_mw.h"
#include "./headers/globals.h"
#include "./headers/logger.h"
#include "./headers/db_adjudicator.h"
#include "./headers/db_executor.h"
#include "./headers/db_info.h"
#include "./headers/constants.h"
#include "./headers/xml_settings.h"
#include "./headers/db_buffer.h"
#include "./headers/sql_grain.h"
#include "./headers/db_service.h"
#include "./headers/tcp_session.h"
#include "./headers/threadsafe_queue.h"
#include "./headers/tcp_server.h"
#include "./headers/tcp_acceptor.h"

int main() {
  try {
    // initalize all global variables
    global_init();
    
    ftnode_mw ftn_middle_ware;
    ftn_middle_ware.start();
  }
  catch (const ftnode_exception& e) {
    //std::cout << e.get_err_code();
    return e.get_err_code();
  }
  catch (const std::exception& e) {
    //std::cout << e.what();
    return 1; // general error
  }  
  catch ( ... ) {
    return 1; // general error
  }

  return 0;
}
