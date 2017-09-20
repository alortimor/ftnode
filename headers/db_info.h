#ifndef DB_INFO_H_
#define DB_INFO_H_

#include <SQLAPI.h>
#include <map>
#include <string>

// Metadata for each OTSDB
struct db_info {
  int db_id{0};
  std::string product ; // i.e. oracle, postgre, sqlanywhere
  std::string con_str ; // connection string used to connect to remote database
  std::string usr;
  std::string pswd;
  std::string begin_statement;
  bool set_isolation;
  SAIsolationLevel_t con_isolation_evel; //isolation levels are either repeatable read or serializable (whichever maps to snapshot isolation behaviour in the respective product)
  SAClient_t con_cl; // type of connection (enum)
  std::map<std::string, std::string> properties;
};

#endif

