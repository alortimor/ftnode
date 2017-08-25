#include <map>
#include </home/mw/SQLAPI/include/SQLAPI.h>
#include "../headers/xml_settings.h"
#include "../tinyxml/tinyxml.h"

/*static map<int,int> create_map()
{
  map<int,int> m;
  m[1] = 2;
  m[3] = 4;
  m[5] = 6;
  return m;
}*/


std::map<std::string, SAClient_t> db_con_id{
    {"oracle", SA_Oracle_Client},
    {"postgres", SA_PostgreSQL_Client},
    {"sqlanywhere", SA_SQLAnywhere_Client}
};
// map for isolation level constants
std::map<std::string, SAIsolationLevel_t> db_iso_level {
    {"SA_RepeatableRead", SA_RepeatableRead},
    {"SA_Serializable", SA_Serializable}
};

namespace xmls {

const std::string ftnode_mw::DBSOURCES{"dbsources"};
    const std::string ftnode_mw_dbsources::DB{"db"};
        const std::string ftnode_mw_dbsources_db::HOST{"host"};
        const std::string ftnode_mw_dbsources_db::PORT{"port"};
        const std::string ftnode_mw_dbsources_db::PWD{"pwd"};
        const std::string ftnode_mw_dbsources_db::USER{"user"};
        const std::string ftnode_mw_dbsources_db::NAME{"name"};
        const std::string ftnode_mw_dbsources_db::PRODUCT{"product"};
        const std::string ftnode_mw_dbsources_db::ID{"id"};
        const std::string ftnode_mw_dbsources_db::CONSTR{"cstr"};
        const std::string ftnode_mw_dbsources_db::BEG_TR{"beg_tr"};
        const std::string ftnode_mw_dbsources_db::ISO_LEV{"iso_lev"};
const std::string ftnode_mw::ENDPOINT{"endpoint"};
    const std::string ftnode_mw_endpoint::IP{"ip"};
    const std::string ftnode_mw_endpoint::PORT{"port"};


const std::vector<std::unique_ptr<setting>>& xml_settings::get(std::string setting_name) const
{
    // TODO: could use mapping here (std::map) instead of if-else s
    static std::vector<std::unique_ptr<setting>> null_setting;
    if(setting_name == xmls::ftnode_mw::DBSOURCES)
        return db_sources;
    else if(setting_name == xmls::ftnode_mw::ENDPOINT)
        return end_point_;

    return null_setting;
}

bool xml_settings::load_settings_xml(std::string file_name)
{
	TiXmlNode* xml = NULL;
	TiXmlDocument xmlDoc;

	if (xmlDoc.LoadFile(file_name.c_str()))
	{
		// XML root
		xml = xmlDoc.FirstChild(SETTING_XML_HOME.c_str());

		if (NULL == xml)
			return false;

		load(xml);

		return true;
	}

	return false;
}

void xml_settings::load(TiXmlNode* node)
{
	TiXmlNode* item = NULL;
	// Get first item
	item = node->FirstChild();

	// Iterate all siblings
	while (NULL != item)
	{
		load_item(item);
		item = item->NextSibling();
	}
}

TiXmlNode* xml_settings::proc_elem(TiXmlNode* node)
{
	TiXmlNode* ret = nullptr;
    TiXmlElement* elem = node->ToElement();
    const std::string elemValue = elem->Value();
    if(elemValue == xmls::ftnode_mw::DBSOURCES)
    {
		proc_db_sources(node);
		ret = nullptr;
    }
	else if (elemValue == xmls::ftnode_mw::ENDPOINT)
	{
		proc_end_point(node);
		ret = nullptr;
	}
	else
		ret = node;

    return node;
}

void xml_settings::proc_db_sources(TiXmlNode* dbsources_node)
{
	TiXmlNode* child = dbsources_node->IterateChildren(NULL);
	std::string elemValue;
	bool error{false};
	while (child && !error)
	{
        TiXmlElement* elem = child->ToElement();
        elemValue = elem->Value();
        error = (elemValue != xmls::ftnode_mw_dbsources::DB);
        if(!error)
        {
            std::unique_ptr<db_source> source = std::make_unique<db_source>();

            source->host = elem->Attribute(xmls::ftnode_mw_dbsources_db::HOST.c_str());
            elem->Attribute(xmls::ftnode_mw_dbsources_db::PORT.c_str(), &(source->port));
            source->product = elem->Attribute(xmls::ftnode_mw_dbsources_db::PRODUCT.c_str());
            source->password = elem->Attribute(xmls::ftnode_mw_dbsources_db::PWD.c_str());
            source->user = elem->Attribute(xmls::ftnode_mw_dbsources_db::USER.c_str());
            source->name = elem->Attribute(xmls::ftnode_mw_dbsources_db::NAME.c_str());
            source->conn_str = elem->Attribute(xmls::ftnode_mw_dbsources_db::CONSTR.c_str());
            source->begin_tr = elem->Attribute(xmls::ftnode_mw_dbsources_db::BEG_TR.c_str());
            source->iso_level = elem->Attribute(xmls::ftnode_mw_dbsources_db::ISO_LEV.c_str());
            elem->Attribute(xmls::ftnode_mw_dbsources_db::ID.c_str(), &(source->id));

            db_sources.push_back(std::move(source)); // note: "source" moved

            child = child->NextSibling();
		}
	}
}

void xml_settings::proc_end_point(TiXmlNode* end_point_node)
{

	TiXmlElement* elem = end_point_node->ToElement();
	std::unique_ptr<end_point> _end_point = std::make_unique<end_point>();
	elem->Attribute(xmls::ftnode_mw_endpoint::PORT.c_str(), &(_end_point->port));
	_end_point->ip = elem->Attribute( xmls::ftnode_mw_endpoint::IP.c_str());

	end_point_.push_back(std::move(_end_point)); // note: "_end_point" moved
}

void xml_settings::load_item(TiXmlNode* node)
{
	TiXmlElement* elem = node->ToElement();
	TiXmlNode* child = NULL;

	node = proc_elem(node);
	if (node)
	{
		child = node->IterateChildren(NULL);
		while (child)
		{
			load_item(child);
			child = node->IterateChildren(child);
		}
	}
}

};
