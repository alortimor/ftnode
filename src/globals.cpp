#include "globals.h"
#include "xml_settings.h"
#include "logger.h"

// Initiates global startup

void global_init() {
  settings().load_settings_xml(xmls::DEF_SETTING_FILE_NAME);
  //set_log3_file_path("test_log", "test");
}
