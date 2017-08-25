#include "./headers/ftnode_mw.h"
#include "./headers/globals.h"
#include "./headers/logger.h"


int main() {
  excep_log("main start");
  // initalize all global variables
  global_init();
  
  ftnode_mw ftn_middle_ware;
  ftn_middle_ware.start();

  return 0;
}
