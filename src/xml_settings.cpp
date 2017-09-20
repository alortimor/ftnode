#include "xml_settings.h"
#include "tinyxml.h"

std::map<std::string, SAClient_t> create_con_map() {
  std::map<std::string, SAClient_t> m;
  m["oracle"]       = SA_Oracle_Client;
  m["postgres"]     = SA_PostgreSQL_Client;
  m["sqlanywhere"]  = SA_SQLAnywhere_Client;
  return m;
}

std::map<std::string, SAIsolationLevel_t> create_iso_level_map() {
  std::map<std::string, SAIsolationLevel_t> m;
  m["SA_RepeatableRead"]  = SA_RepeatableRead;
  m["SA_Serializable"]    = SA_Serializable;
  return m;
}

const std::map<std::string, SAClient_t> db_con_id = create_con_map();
// map for isolation level constants
const std::map<std::string, SAIsolationLevel_t> db_iso_level = create_iso_level_map();

namespace xmls {
  
  const std::string ftnode::ELEM_NAME{"ftnode_mw"};
    
  const std::string ftnode::dbsources::ELEM_NAME{"dbsources"};
  
  const std::string ftnode::dbsources::db::ELEM_NAME{"db"};
  
  const std::string ftnode::dbsources::db::HOST{"host"};
  const std::string ftnode::dbsources::db::PORT{"port"};
  const std::string ftnode::dbsources::db::PWD{"pwd"};
  const std::string ftnode::dbsources::db::USER{"user"};
  const std::string ftnode::dbsources::db::NAME{"name"};
  const std::string ftnode::dbsources::db::PRODUCT{"product"};
  const std::string ftnode::dbsources::db::ID{"id"};
  const std::string ftnode::dbsources::db::CONSTR{"cstr"};
  //const std::string ftnode::dbsources::db::BEG_TR{"beg_tr"};
  const std::string ftnode::dbsources::db::ISO_LEV{"iso_lev"};
  
  const std::string ftnode::dbsources::db::property::ELEM_NAME{"property"};
  
  const std::string ftnode::dbsources::db::property::NAME{"name"};
  const std::string ftnode::dbsources::db::property::VALUE{"value"};
  
  const std::string ftnode::endpoint::ELEM_NAME{"endpoint"};
  const std::string ftnode::endpoint::IP{"ip"};
  const std::string ftnode::endpoint::PORT{"port"};


  const std::vector<std::unique_ptr<setting>>& xml_settings::get(std::string setting_name) const {
    // TODO: could use mapping here (std::map) instead of if-else s
    static std::vector<std::unique_ptr<setting>> null_setting;
    if(setting_name == ftnode::dbsources::ELEM_NAME) // DBSOURCES
      return db_sources;
    else if(setting_name == ftnode::endpoint::ELEM_NAME) // ENDPOINT
      return end_point_;

    return null_setting;
  }

  bool xml_settings::load_settings_xml(std::string file_name) {
    TiXmlNode* xml = NULL;
    TiXmlDocument xmlDoc;

    if (xmlDoc.LoadFile(file_name.c_str()))	{
      // XML root
      xml = xmlDoc.FirstChild(ftnode::ELEM_NAME.c_str()); // SETTING_XML_HOME

      if (NULL == xml)
        return false;

      load(xml);

      return true;
    }

    return false;
  }

  void xml_settings::load(TiXmlNode* node) {
    TiXmlNode* item = NULL;
    // Get first item
    item = node->FirstChild();

    // Iterate all siblings
    while (NULL != item) {
      load_item(item);
      item = item->NextSibling();
    }
  }

  TiXmlNode* xml_settings::proc_elem(TiXmlNode* node) {
    
    TiXmlElement* elem = node->ToElement();
    const std::string elemValue = elem->Value();

    TiXmlNode* ret = nullptr;
    if(elemValue == ftnode::dbsources::ELEM_NAME) { //  DBSOURCES
      proc_db_sources(node);
      ret = nullptr;
    }
    else if (elemValue == ftnode::endpoint::ELEM_NAME) { // ENDPOINT
      proc_end_point(node);
      ret = nullptr;
    }
    else
      ret = node;

    return ret;
  }
  
  void xml_settings::proc_db_sources_properties(TiXmlNode* dbsources_node, 
      std::map<std::string, std::string>& properties) {
    properties.clear();
    TiXmlNode* child = dbsources_node->IterateChildren(NULL);
    std::string elemValue;
    bool error{false};
    while (child && !error)	{
      TiXmlElement* elem = child->ToElement();
      elemValue = elem->Value();
      error = (elemValue != ftnode::dbsources::db::property::ELEM_NAME); // xmls::db::PROPERTY
      if(!error) {
//        std::string name = elem->Attribute(xmls::ftnode_mw_dbsources_db_property::NAME.c_str());
//        std::string value = elem->Attribute(xmls::ftnode_mw_dbsources_db_property::VALUE.c_str());
        std::string name = elem->Attribute(ftnode::dbsources::db::property::NAME.c_str());
        std::string value = elem->Attribute(ftnode::dbsources::db::property::VALUE.c_str());
        
        properties.emplace(name, value);

        child = child->NextSibling();
      }
    }
  }  

  void xml_settings::proc_db_sources(TiXmlNode* dbsources_node) {
    TiXmlNode* child = dbsources_node->IterateChildren(NULL);
    std::string elemValue;
    bool error{false};
    while (child && !error)	{
      TiXmlElement* elem = child->ToElement();
      elemValue = elem->Value();
      error = (elemValue != ftnode::dbsources::db::ELEM_NAME); // xmls::dbsources::DB
      if(!error) {
        std::unique_ptr<db_source> source = std::make_unique<db_source>();

        source->host = elem->Attribute(ftnode::dbsources::db::HOST.c_str());
        elem->Attribute(ftnode::dbsources::db::PORT.c_str(), &(source->port));
        source->product = elem->Attribute(ftnode::dbsources::db::PRODUCT.c_str());
        source->password = elem->Attribute(ftnode::dbsources::db::PWD.c_str());
        source->user = elem->Attribute(ftnode::dbsources::db::USER.c_str());
        source->name = elem->Attribute(ftnode::dbsources::db::NAME.c_str());
        source->conn_str = elem->Attribute(ftnode::dbsources::db::CONSTR.c_str());
        //source->begin_tr = elem->Attribute(ftnode::dbsources::db::BEG_TR.c_str());
        source->iso_level = elem->Attribute(ftnode::dbsources::db::ISO_LEV.c_str());
        elem->Attribute(ftnode::dbsources::db::ID.c_str(), &(source->id));
        
        proc_db_sources_properties(child, source->properties);

        db_sources.push_back(std::move(source)); // note: "source" moved

        child = child->NextSibling();
      }
    }
  }

  void xml_settings::proc_end_point(TiXmlNode* end_point_node) {
    TiXmlElement* elem = end_point_node->ToElement();
    std::unique_ptr<end_point> _end_point = std::make_unique<end_point>();
//    elem->Attribute(xmls::ftnode_mw_endpoint::PORT.c_str(), &(_end_point->port));
//    _end_point->ip = elem->Attribute( xmls::ftnode_mw_endpoint::IP.c_str());
    elem->Attribute(ftnode::endpoint::PORT.c_str(), &(_end_point->port));
    _end_point->ip = elem->Attribute( ftnode::endpoint::IP.c_str());

    end_point_.push_back(std::move(_end_point)); // note: "_end_point" moved
  }

  void xml_settings::load_item(TiXmlNode* node) {
    TiXmlNode* child = NULL;

    node = proc_elem(node);
    if (node) {
      child = node->IterateChildren(NULL);
      while (child) {
        load_item(child);
        child = node->IterateChildren(child);
      }
    }
  }

};
