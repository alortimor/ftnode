#include <string>
#include <algorithm>
#include "db_executor.h"
#include "logger.h"
#include "db_adjudicator.h"

extern logger exception_log;

// Three of these exist in a vector in the Adjudicator
// only call this within this .cpp file.
static std::string &rtrim(std::string &s, char c) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [c](int ch) { return !(ch==c);} ).base(), s.end());
  return s;
}

db_executor::db_executor(int dbid, db_adjudicator& _req) :
             db_id{dbid}
           , con{std::make_unique<SAConnection>()}
           , cmd{std::make_unique<SACommand>()}
           , cmd_hash{std::make_unique<SACommand>()}
           , cmd_sel{std::make_unique<SACommand>()}
           , req{_req}
           { }

int const db_executor::get_db_id() const { return db_id; };

const db_info& db_executor::get_db_info() const { return dbi; }

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

std::string const db_executor::get_begin_statement() const { return dbi.properties.at("beg_tr1"); } // return dbi.begin_statement;

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
  try {
    hash_sql = dbi.properties.at("hash_prefix") + generate_concat_columns(v_sg.at(statement_id).get_sql()) 
                  + dbi.properties.at("hash_mid") + v_sg.at(statement_id).get_sql() + dbi.properties.at("hash_suffix");
    cmd_hash->setCommandText(hash_sql.c_str());
  }
  catch (SAException &x) {
    failure_msg =  "FAILURE HASH cols error: " +  std::string((const char*)x.ErrText()) + " DB ID " + std::to_string(db_id)+ ": " + hash_sql ;
    cmd_hash->Cancel();
    log_1(failure_msg);
    throw std::runtime_error(failure_msg);
  }

  try {
    cmd_hash->Execute();
    std::string hash_val{""};
    while (cmd_hash->FetchNext())
      hash_val = (const char*)cmd_hash->Field(1).asString();
    v_sg.at(statement_id).set_hash_val(hash_val);

  }
  catch (SAException &x) {
    cmd_hash->Cancel();
    failure_msg =  "FAILURE HASH select error: " +  std::string((const char*)x.ErrText()) + " DB ID " + std::to_string(db_id)+ ": " + hash_sql ;
    throw std::runtime_error(failure_msg);
  }
}

void db_executor::execute_select (int statement_id) {
  cmd_sel->setCommandText(v_sg.at(statement_id).get_sql().c_str());
  try {
    cmd_sel->setOption("UseCursor") = "1";
    cmd_sel->Execute();
    v_sg.at(statement_id).set_db_return_values(true,0);
  }
  catch (SAException &x) {
    cmd_sel->Cancel();
    failure_msg = "FAILURE SELECT error: " +  std::string((const char*)x.ErrText()) + " DB ID " + std::to_string(db_id) + " :" + v_sg.at(statement_id).get_sql();
    throw std::runtime_error(failure_msg);
  }
}

// generates a string that results in part of a SELECT statement, which
// concatenates all columns of a client SELECT into a single column.
// For a given SELECT statement, in an db_executor, this function is only ever called once.
std::string db_executor::generate_concat_columns(const std::string & sql) {
  std::string exec_sql {""};
  try {
    exec_sql = dbi.properties.at("concat_col_prefix") + sql + dbi.properties.at("concat_col_suffix");
    cmd_hash->setCommandText(exec_sql.c_str());
  }
  catch (SAException &x) {
    failure_msg = "FAILURE Generate Cols generate : " + std::string( (const char*)x.ErrText()) + " DB ID "+ std::to_string(db_id) + " " + exec_sql;
    cmd_hash->Cancel();
    throw std::runtime_error(failure_msg);
  }

  try {
    cmd_hash->Execute(); // executes a dummy statement that performs no fetch, but exposes all columns and data types
  }
  catch (SAException &x) {
    failure_msg = "FAILURE Generate Cols execute : " + std::string( (const char*)x.ErrText()) + " DB ID "+ std::to_string(db_id) + " " + exec_sql;
    cmd_hash->Cancel();
    throw std::runtime_error(failure_msg);
  }

  std::string concat_str {"''||"};
  std::string fmt {""};
  std::string field_name;
  
  // using the information related to data types, we can now generated a concatenated string
  // that can be hashed
  for (int i{1}; i <= cmd_hash->FieldCount(); i++) {
    field_name = (const char*)cmd_hash->Field(i).Name();
    try {
      switch (cmd_hash->Field(i).FieldType()) {
        case SA_dtNumeric:
          // cmd_hash->Field(field_name.c_str()).FieldScale() != 65531 -- this is due to a bug in PostgreSQL library
          if (cmd_hash->Field(field_name.c_str()).FieldScale() >0 && cmd_hash->Field(field_name.c_str()).FieldScale() != 65531) {
            fmt = "";
            if (dbi.properties.at("apply_number_format_mask")=="true")
              fmt += std::string(cmd_hash->Field(field_name.c_str()).FieldPrecision(),'9') +"."+ std::string(cmd_hash->Field(field_name.c_str()).FieldScale(), '9');

            concat_str += dbi.properties.at("number_scale_fmt_prefix") + field_name
                         + dbi.properties.at("number_scale_fmt_mid") + fmt + dbi.properties.at("number_scale_fmt_suffix") +"||";
          }
          else {
            concat_str += " " + field_name + "||";
          }
          break;
        case SA_dtDateTime:
          concat_str += dbi.properties.at("date_fmt_prefix") +  field_name + dbi.properties.at("date_fmt_suffix") + "||";
          break;
        default:
          concat_str += " " + field_name + "||";
          break;
      }
    }
    catch (SAException &x) {
      cmd_hash->Cancel();
      failure_msg = "FAILURE Format Cols Error: " + std::string( (const char*)x.ErrText() )  + " DB ID "+ std::to_string(db_id) + " " + exec_sql;
      throw std::runtime_error(failure_msg);
    }
  }
  // strip off trailing concat operators prior to returning.
  concat_str += "''";
  return concat_str;
}

void db_executor::exec_sql() {
  try {
    cmd->Execute();
  }
  catch (SAException &x) {
    failure_msg = "FAILURE EXEC SQL Error: " + std::string( (const char*)x.ErrText() );
    cmd->Cancel();
    throw std::runtime_error(failure_msg);
  }
}

const std::vector<std::pair<char, std::string>> & db_executor::get_sql_results() const {
  return v_result;
}

void db_executor::prepare_client_results() {
  std::string str;
  for (const auto & s : v_sg ) {
    if (!s.is_select()) {
      v_result.emplace_back(std::make_pair('M', std::to_string(s.get_rows_affected() )));
    }
    else {
      try {
        while(cmd_sel->FetchNext()) {
          str = "";
          for (int i{1}; i <= cmd_sel->FieldCount(); i++) {
            str += cmd_sel->Field(i).asString();
            str += ",";
          }
          rtrim(str, ',');
          v_result.emplace_back(std::make_pair('S', str));
        }
      }
      catch (SAException &x) {
        failure_msg = "FAILURE Prepare Client Results Error: " + std::string( (const char*)x.ErrText() ) + " DB ID: " + std::to_string(db_id);
        cmd_sel->Cancel();
        throw std::runtime_error(failure_msg);
      }
    }
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
    // con->setIsolationLevel(dbi.con_isolation_evel);
    
    /*  cmd:      used to execute all DML
     *  cmd_sel:  used to execute a SELECT to generate client result set
     *  cmd_hash: used to obtain the hash of the result set generated by cmd_sel, if the statement is a SELECT, and
     *            is run asynchronously to the cmd_sel, but uses the same SELECT statement.
     *            Executes a controlled SELECT to generate a hash instead
     *            No ORDER BY injection is necessary.
     */ 

    try {
      cmd_hash->setConnection(con.get());
      cmd_sel->setConnection(con.get());
      cmd->setConnection(con.get());
    }
    catch (SAException &x) {
      failure_msg = "FAILURE Cannot make connection: " + std::string( (const char*)x.ErrText() ) + " DB ID: " + std::to_string(db_id);
      cmd->Cancel();
      cmd_hash->Cancel();
      cmd_sel->Cancel();
      throw std::runtime_error(failure_msg);
    }
  }
  catch (SAException &x) {
    log_1( "FAILURE Connection error :" + dbi.product + " - " + std::string((const char*)x.ErrText()) );
    return false;
  }
  return true;
}

