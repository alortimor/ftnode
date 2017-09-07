#include "./headers/ftnode_mw.h"
#include "./headers/globals.h"
#include "./headers/logger.h"
#include "./headers/request.h"
#include "./headers/db_executor.h"
#include "./headers/xml_settings.h"
#include "./headers/constants.h"
#include "./headers/db_buffer.h"
#include "./headers/db_executor.h"
#include "./headers/db_service.h"
#include "./headers/tcp_acceptor.h"
#include "./headers/tcp_request.h"
#include "./headers/tcp_server.h"
#include "./headers/db_service.h"
#include "./headers/thread_pool.h"


int main() {
  excep_log("main start");
  // initalize all global variables
  global_init();
  
  ftnode_mw ftn_middle_ware;
  ftn_middle_ware.start();

  return 0;
}
