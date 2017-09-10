#include "./headers/ftnode_mw.h"
#include "./headers/globals.h"
#include "./headers/logger.h"
#include "./headers/request.h"
#include "./headers/db_executor.h"
#include "./headers/db_info.h"
#include "./headers/xml_settings.h"
#include "./headers/db_buffer.h"
#include "./headers/sql_grain.h"
#include "./headers/db_service.h"
#include "./headers/tcp_session.h"
#include "./headers/threadsafe_queue.h"
#include "./headers/tcp_server.h"
#include "./headers/tcp_acceptor.h"
#include "./headers/tcp_msg_consts.h"


int main() {
  excep_log("main start");
  // initalize all global variables
  global_init();
  
  ftnode_mw ftn_middle_ware;
  ftn_middle_ware.start();

  return 0;
}
