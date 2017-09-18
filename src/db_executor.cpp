#include <string>
#include <algorithm>
#include "db_executor.h"
#include "logger.h"
#include "db_adjudicator.h"

extern logger exception_log;

// only call this within this .cpp file.
static std::string &rtrim(std::string &s, char c) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [c](int ch) { return !(ch==c);} ).base(), s.end());
  return s;
}

db_executor::db_executor(int dbid, db_adjudicator& _req) :
             db_id{dbid}
           , cmd{std::make_unique<SACommand>()}
           , con{std::make_unique<SAConnection>()}
           , cmd_hash{std::make_unique<SACommand>()}
           , req{_req}
           { }

int const db_executor::get_db_id() const { return db_id; };

void db_executor::add_sql_grain(int statement_id, const std::string sql) {
  v_sg.emplace_back( statement_id, sql);
}

void db_executor::clear_sql_grains() {
  v_sg.clear();
  v_result.clear();
}

void db_executor::disconnect() { 
  if (con->isConnected()) con->Disconnect(); 
}

void db_executor::set_statement(const std::string & sql) {
  cmd->setCommandText(sql.c_str());
}

std::string const db_executor::get_begin_statement() const { return dbi.begin_statement; }

int const db_executor::get_rows_affected(int statement_id) const {
  return v_sg.at(statement_id).get_rows_affected();
}

std::string const db_executor::get_hash(int statement_id) const {
  return v_sg.at(statement_id).get_hash();
}

bool const db_executor::get_is_result(int statement_id) const {
  return v_sg.at(statement_id).get_is_result();
}

const std::string db_executor::get_product() const { return dbi.product; }

const std::string db_executor::get_connection_str() const { return dbi.con_str; }

void db_executor::execute_hash_select(int statement_id) {
  std::string hash_sql;
  hash_sql = dbi.properties.at("hash_prefix") + generate_concat_columns(v_sg.at(statement_id).get_sql()) 
                + dbi.properties.at("hash_mid") + v_sg.at(statement_id).get_sql() + dbi.properties.at("hash_suffix");
  cmd_hash->setCommandText(hash_sql.c_str());

  try {
    excep_log("DB ID: " + std::to_string(db_id) + " sid " + std::to_string(statement_id) +  " before HASH SELECT " );
    cmd_hash->Execute();
    std::string hash_val{""};
    while (cmd_hash->FetchNext())
      hash_val = (const char*)cmd_hash->Field(1).asString();
    v_sg.at(statement_id).set_hash_val(hash_val);

  }
  catch (SAException &x) {
    excep_log( "Get HASH exception : DB ID " + std::to_string(db_id) + ": " + hash_sql );
    excep_log( "Get HASH exception : " +  std::string((const char*)x.ErrText()) + " DB ID " + std::to_string(db_id) );
  }
}

void db_executor::execute_select (int statement_id) {
  cmd->setCommandText(v_sg.at(statement_id).get_sql().c_str());
  try {
    excep_log("DB ID: " + std::to_string(db_id) + " sid " + std::to_string(statement_id) +  " before SELECT " );
    cmd->Execute();
    v_sg.at(statement_id).set_db_return_values(true,0);
  }
  catch (SAException &x) {
    excep_log( "SELECT error:  DB ID " + std::to_string(db_id) + " :" + v_sg.at(statement_id).get_sql());
    excep_log( "SELECT error: " +  std::string((const char*)x.ErrText()) + " DB ID " + std::to_string(db_id) + " :" + v_sg.at(statement_id).get_sql());
    v_sg.at(statement_id).set_db_return_values(true, -1);
  }
}

// generates a string that results in part of a SELECT statement, which
// concatenates all columns of a client SELECT into a single column.
// For a given SELECT statement, in an db_executor, this function is only ever called once.
std::string db_executor::generate_concat_columns(const std::string & sql) {
  std::string exec_sql {""};
  exec_sql = dbi.properties.at("concat_col_prefix") + sql + dbi.properties.at("concat_col_suffix");
  cmd->setCommandText(exec_sql.c_str());

  try {
    cmd->Execute(); // executes a dummy statement that performs no fetch, but exposes all columns and data types
  }
  catch (SAException &x) {
    excep_log( "Generate Columns Error : " + std::string( (const char*)x.ErrText()) + " DB ID "+ std::to_string(db_id) + " " + exec_sql);
  }

  std::string concat_str {""};
  std::string fmt {""};
  std::string field_name;
  
  // excep_log(std::string("After Concat dummy column generate ") + std::to_string(cmd->FieldCount()) );

  // using the information related to data types, we can now generated a concatenated string
  // that can be hashed
  for (int i{1}; i <= cmd->FieldCount(); i++) {
    field_name = (const char*)cmd->Field(i).Name();
    try {
      switch (cmd->Field(i).FieldType()) {
        case SA_dtNumeric:
          // cmd->Field(field_name.c_str()).FieldScale() != 65531 -- this is due to a bug in PostgreSQL library
          if (cmd->Field(field_name.c_str()).FieldScale() >0 && cmd->Field(field_name.c_str()).FieldScale() != 65531) {
            fmt = "";
            if (dbi.properties.at("apply_number_format_mask")=="true")
              fmt += std::string(cmd->Field(field_name.c_str()).FieldPrecision(),'9') +"."+ std::string(cmd->Field(field_name.c_str()).FieldScale(), '9');

            concat_str += dbi.properties.at("number_scale_fmt_prefix") + field_name 
                         + dbi.properties.at("number_scale_fmt_mid") + fmt + dbi.properties.at("number_scale_fmt_suffix") +" ||";
          }
          else {
            concat_str += " " + field_name + " ||";
          }
          break;
        case SA_dtDateTime:
          concat_str += dbi.properties.at("date_fmt_prefix") +  field_name + dbi.properties.at("date_fmt_suffix") + " ||";
          break;
        default:
          concat_str += " " + field_name + " ||";
          break;
      }
    }
    catch (SAException &x) {
      excep_log( "DB Column Concat error : " +  std::string((const char*)x.ErrText()) );
    }
  }
  // strip off trailing concat operators prior to returning.
  return rtrim(concat_str, '|');
}

void db_executor::exec_sql() {
  try {
    cmd->Execute();
  }
  catch (SAException &x) {
    excep_log( "EXEC SQL Error: " + std::string( (const char*)x.ErrText() ) );
  }
}

const std::vector<std::pair<char, std::string>> & db_executor::get_sql_results() const {
  return v_result;
}

void db_executor::prepare_client_results() {
  std::string str;
  for (const auto & s : v_sg ) {
    if (!s.get_is_result())
      v_result.emplace_back(std::make_pair('M', std::to_string(s.get_rows_affected() )));
    else {
      excep_log("DB ID : " + std::to_string(db_id) + " sid " + std::to_string(s.get_statement_id())+ " get_is_result - before fetch");
      if (cmd->isOpened()) { // checks 
        while(cmd->FetchNext()) {
          str = "";
          for (int i{1}; i <= cmd->FieldCount(); i++) {
            str += cmd->Field(i).asString();
            str += ",";
          }
          rtrim(str, ',');
          v_result.emplace_back(std::make_pair('S', str));
        }
      }
    }
    std::cout << "Results " << v_result.back().first << " " << v_result.back().second << "\n";
  }
}

void db_executor::commit() {
  con->Commit();
}

void db_executor::rollback() {
  con->Rollback();
}

bool db_executor::make_connection() {
  std::string str;
  try {

    if (dbi.product=="sqlanywhere") {
      con->setOption(_TSA("SQLANY.LIBS")) = _TSA("/opt/sqlanywhere17/lib64/libdbcapi_r.so");
    }
      
    con->Connect(dbi.con_str.c_str(), dbi.usr.c_str(), dbi.pswd.c_str(), dbi.con_cl);
    con->setAutoCommit(SA_AutoCommitOff);
    if (dbi.set_isolation) con->setIsolationLevel(dbi.con_isolation_evel);
    
    /*  cmd:      used to execute all DML
     *  cmd_hash: used to obtain the hash of the result set generated by cmd, if the statement is a SELECT, and
     *            is run asynchronously to the cmd, but uses the same SELECT statement.
     *            Executes a controlled SELECT to generate a hash instead
     *            No ORDER BY injection is necessary.
     *            
     */ 

    cmd->setConnection(con.get());
    cmd_hash->setConnection(con.get());
  }
  catch (SAException &x) { 
    excep_log( "Connection error :" + dbi.product + " - " + std::string((const char*)x.ErrText()) );
    return false; 
  }
  return true;
}

