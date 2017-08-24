// xml_settings class. See intructions how to use and add new settings
// at the end of this file.
#ifndef XML_SETTINGS_H_INCLUDED
#define XML_SETTINGS_H_INCLUDED

#include <vector>
#include <string>
#include <memory>

class TiXmlNode;

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
    static const std::string BEG_SEL_TR;
    static const std::string BEG_UDI_TR;
    static const std::string ISO_LEV;
};
// endpoint
struct ftnode_mw_endpoint : public ftnode_mw {
    static const std::string IP;
    static const std::string PORT;
};

// end of: xml file content

struct setting
{
public:
    virtual ~setting(){}
};

struct db_source : public setting
{
    int id;
    int port;
    std::string product;
    std::string name;
    std::string user;
	std::string password;
    std::string host;
    std::string conn_str; // connection string
    std::string begin_select_tr; // begin select transaction
    std::string begin_udi_tr; // begin update/delete/insert transaction
    std::string iso_level; // Isolation Level
};

struct end_point : public setting
{
    std::string ip;
    int port;
};

class xml_settings
{
public:
    static xml_settings& get_instance()
    {
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

inline xmls::xml_settings& settings()
{
   return xmls::xml_settings::get_instance();
}

#endif // XML_SETTINGS_H_INCLUDED

/*
How to use:
1) Adding a new setting
- Create setting/element constant string:
const std::string ELEM_ENDPOINT{ "endpoint" };

- Create attribute const names:
const std::string ELEM_ENDPOINT_PORT{ "port" };
const std::string ELEM_ENDPOINT_IP{ "ip" };

- Create a struct for that setting, for example:
struct end_point
{
	int port;
	std::string ip;
};

- Create a function to process this setting:
a) If the setting values are child nodes under ELEM_ENDPOINT then loop them:
void xml_settings::proc_end_point(TiXmlNode* end_point_node)
{
	TiXmlNode* child = dbsources_node->IterateChildren(NULL);
	std::string elemValue;
	do
	{
		end_point _end_point;
		TiXmlElement* elem = child->ToElement();

		elem->Attribute(ELEM_ENDPOINT_PORT.c_str(), &_end_point.port);
		_end_point.ip = elem->Attribute(ELEM_ENDPOINT_IP.c_str());
		// move the element to a member object:
		end_points_.push_back(std::move(_end_point));
		// note: "_end_point" moved
		elemValue = elem->Value();
		child = child->NextSibling();
	} while (child && elemValue == "xml element name here");
}
b) if no child nodes and the setting are inside the element then do:
void xml_settings::proc_end_point(TiXmlNode* end_point_node)
{
	TiXmlElement* elem = end_point_node->ToElement();
	end_point _end_point;
	elem->Attribute(ELEM_ENDPOINT_PORT.c_str(), &_end_point.port);
	_end_point.ip = elem->Attribute(ELEM_ENDPOINT_IP.c_str());

	end_point_ = std::move(_end_point);
}
*/
