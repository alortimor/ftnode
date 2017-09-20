#include "globals.h"
#include "xml_settings.h"

// Initiates global startup

void global_init() {
  settings().load_settings_xml(xmls::DEF_SETTING_FILE_NAME);
}
