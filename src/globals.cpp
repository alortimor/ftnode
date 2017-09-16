#include "globals.h"
#include "xml_settings.h"
#include "logger.h"

void global_init() {
  settings().load_settings_xml(xmls::DEF_SETTING_FILE_NAME);
  set_log2_file_path("benchmark", "benchmark");
}
