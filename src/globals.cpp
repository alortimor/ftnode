#include "globals.h"
#include "xml_settings.h"
#include "logger.h"
#include "constants.h"

// Initiates global startup

void global_init() {
  if( !settings().load_settings_xml(xmls::DEF_SETTING_FILE_NAME) )
    throw ftnode_exception(ERR_XML_NO_FILE);
  set_log2_file_path("benchmark", "benchmark");
}
