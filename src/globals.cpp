#include "../headers/globals.h"
#include "../headers/xml_settings.h"

void global_init() {
  settings().load_settings_xml(xmls::DEF_SETTING_FILE_NAME);
}
