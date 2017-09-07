// xml_settings class. See intructions how to use and add new settings
// at the end of this file.
#ifndef XML_SETTINGS_H_INCLUDED
#define XML_SETTINGS_H_INCLUDED

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <map>
#include <SQLAPI.h>

class TiXmlNode;

extern const std::map<std::string, SAClient_t> db_con_id;
extern const std::map<std::string, SAIsolationLevel_t> db_iso_level;

namespace xmls {
  // constants
  const std::string DEF_SETTING_FILE_NAME{"settings.xml"};
  // xml file content:
  const std::string SETTING_XML_HOME = "ftnode_mw";
  
  

  struct ftnode_mw {
    static const std::string DBSOURCES;
    static const std::string ENDPOINT;
  };

  struct ftnode_mw_dbsources : public ftnode_mw {
    static const std::string DB;
  };
  
  struct ftnode_mw_dbsources_db : public ftnode_mw_dbsources {
    static const std::string HOST;
    static const std::string PORT;
    static const std::string PWD;
    static const std::string USER;
    static const std::string NAME;
    static const std::string PRODUCT;
    static const std::string ID;
    static const std::string CONSTR;
    static const std::string BEG_TR;
    static const std::string ISO_LEV;
    static const std::string PROPERTY;
  };
  
  // db_property
  
 // ftnode::db_src::property::NAME
  
  struct ftnode_mw_dbsources_db_property : public ftnode_mw_dbsources_db {
    static const std::string NAME;
    static const std::string VALUE;
  };
  
  // endpoint
  struct ftnode_mw_endpoint : public ftnode_mw {
    static const std::string IP;
    static const std::string PORT;
  };

  // end of: xml file content
  struct setting {
    public:
      virtual ~setting(){}
  };

  struct db_source : public setting {
    int id;
    int port;
    std::string product;
    std::string name;
    std::string user;
    std::string password;
    std::string host;
    std::string conn_str; // connection string
    std::string begin_tr; // begin transaction
    std::string iso_level; // Isolation Level
    std::map<std::string, std::string> properties;
  };

  struct end_point : public setting {
    std::string ip;
    int port;
  };

  class xml_settings {
    public:
      static xml_settings& get_instance() {
        static xml_settings instance;
        return instance;
      }

      bool load_settings_xml(std::string file_name);
      const std::vector<std::unique_ptr<setting>>& get(std::string setting_name) const;

    private:
      xml_settings() {}

      void load(TiXmlNode* node);
      void load_item(TiXmlNode* node);
      TiXmlNode* proc_elem(TiXmlNode* node);
      void proc_db_sources(TiXmlNode* dbsources);
      void proc_end_point(TiXmlNode* end_point_node);
      void proc_db_sources_properties(TiXmlNode* dbsources_node, 
        std::map<std::string, std::string>& properties);

      std::vector<std::unique_ptr<setting>> db_sources;
      std::vector<std::unique_ptr<setting>> end_point_;

    public:
      xml_settings(xml_settings const&) = delete;
      void operator=(xml_settings const&)  = delete;

      // Note: Scott Meyers mentions in his Effective Modern
      //       C++ book, that deleted functions should generally
      //       be public as it results in better error messages
      //       due to the compilers behavior to check accessibility
      //       before deleted status
  };

};

inline xmls::xml_settings& settings() {
  return xmls::xml_settings::get_instance();
}

#endif // XML_SETTINGS_H_INCLUDED
