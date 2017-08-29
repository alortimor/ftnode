#include <string>
#include "../headers/db_executor.h"
#include "../headers/logger.h"

extern logger exception_log;

int const db_executor::get_db_id() const { return db_id; };

db_executor::db_executor(int dbid, request& _req) : db_id{dbid}, cmd{std::make_unique<SACommand>()}, con{std::make_unique<SAConnection>()}, req{_req} { }

void db_executor::add_sql_grain(int statement_id, const std::string sql) {
  v_sg.emplace_back( statement_id, sql);
}

void db_executor::disconnect() { if(con->isConnected()) con->Disconnect(); }

void db_executor::set_statement(const std::string & sql) {
  cmd->setCommandText(sql.c_str());
}

std::string const db_executor::get_begin_statement() const {
  return dbi.begin_statement;
}

int const db_executor::get_rows_affected(int statement_id) const {
  return v_sg.at(statement_id).get_rows_affected();
}

bool const db_executor::get_is_result(int statement_id) const {
  return v_sg.at(statement_id).get_is_result();
}

const std::string db_executor::get_product() const { return dbi.product; }

const std::string db_executor::get_connection_str() const { return dbi.con_str; }

void db_executor::exec_begin() {
  try {
    cmd->Execute();
  }
  catch (SAException &x) {
    excep_log(  (const char*)x.ErrText() );
  }
}


void db_executor::commit_rollback(char c) {
  if (c=='c') {
    con->Commit();
  }
  else if (c=='r') {
    con->Rollback();
  }
}

bool db_executor::make_connection() {
  std::string str;
  try {

    if (dbi.product=="sqlanywhere")
      con->setOption(_TSA("SQLANY.LIBS")) = _TSA("/opt/sqlanywhere17/lib64/libdbcapi_r.so");
      
    con->Connect(dbi.con_str.c_str(), dbi.usr.c_str(), dbi.pswd.c_str(), dbi.con_cl) ;
    
    con->setAutoCommit(SA_AutoCommitOff);

    if (dbi.set_isolation) con->setIsolationLevel(dbi.con_isolation_evel);
    
    cmd->setConnection(con.get());
  }
  catch (SAException &x) { 
    std::string str;
    str = "Connection Exception : " + dbi.product + " - " + (const char*)x.ErrText();
    excep_log( str );
    return false; 
  }
  return true;
}


void db_executor::execute_sql_grains () {
  for ( auto & s : v_sg ) {
    set_statement(s.get_sql());
    try {
      cmd->Execute();
      s.set_db_return_values(cmd->isResultSet(), cmd->RowsAffected() );
    }
    catch (SAException &x) {
      excep_log( (const char*)x.ErrText() );
      s.set_db_return_values(false, -1);
      break;
    }

  }
  req.reply_to_client_upon_first_done(db_id);
}

