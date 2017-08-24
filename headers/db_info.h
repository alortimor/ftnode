#ifndef DB_INFO_H_
#define DB_INFO_H_

#include </home/mw/SQLAPI/include/SQLAPI.h>
#include <string>

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
};

#endif
