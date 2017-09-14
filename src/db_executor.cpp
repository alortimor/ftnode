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
           , req{_req}
           , cmd_hash{std::make_unique<SACommand>()}
           , con_hash{std::make_unique<SAConnection>()}
           , cmd_sel{std::make_unique<SACommand>()}
           , con_sel{std::make_unique<SAConnection>()}
           { }

int const db_executor::get_db_id() const { return db_id; };

void db_executor::add_sql_grain(int statement_id, const std::string sql) {
  v_sg.emplace_back( statement_id, sql);
}

void db_executor::disconnect() { 
  if (con->isConnected()) con->Disconnect(); 
  if (con_hash->isConnected()) con_hash->Disconnect(); 
  if (con_sel->isConnected()) con_hash->Disconnect(); 
}

void db_executor::set_statement(const std::string & sql) {
  cmd_hash->setCommandText(sql.c_str());
  cmd_sel->setCommandText(sql.c_str());
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
  set_sql_hash_statement(v_sg.at(statement_id).get_sql());
  try {
    // excep_log( std::string(cmd_hash->CommandText()) );
    cmd_hash->Execute();

    std::string hash_val{""};
    while (cmd_hash->FetchNext())
      hash_val = (const char*)cmd_hash->Field(1).asString();

    v_sg.at(statement_id).set_hash_val(hash_val);
    // excep_log("Hash Value " + hash_val + " DB ID " + std::to_string(db_id));
  }
  catch (SAException &x) {
    // SQL Anywhere generates the following exception when attempting asynchronous DML statements on a single connection: 
    /* 
        42W22 https://help.sap.com/viewer/00ed8eaa5d9342029c4c48bf1c7fd52d/17.0/en-US/80e0e5946ce21014a971be81ae0ed92d.html
        The same applies to SYBASE and probably to MS SQL Server as well.
        42W22 http://infocenter.sybase.com/help/index.jsp?topic=/com.sybase.infocenter.dc00462.1520/reference/sqlstate19.htm

     In embedded SQL, you attempted to submit a database request while you have another request in progress. 
     You should either use a separate SQLCA and connection for each thread accessing the database, or use
     thread synchronization calls to ensure that a SQLCA is only accessed by one thread at a time. 
    */
    excep_log( "Get HASH exception : " +  std::string((const char*)x.ErrText()) + " DB ID " + std::to_string(db_id) );
  }
}

void db_executor::execute_select (int statement_id) {
  cmd_sel->setCommandText(v_sg.at(statement_id).get_sql().c_str());
  try {
    cmd_sel->Execute();
    v_sg.at(statement_id).set_db_return_values(true, 0 );
  }
  catch (SAException &x) {
    excep_log( "SELECT error: " +  std::string((const char*)x.ErrText()) + " DB ID " + std::to_string(db_id) + " :" + v_sg.at(statement_id).get_sql());
    v_sg.at(statement_id).set_db_return_values(false, -1);
  }
}

// generates a string that results in part of a SELECT statement, which
// concatenates all columns of a client SELECT into a single column.
// For a given SELECT statement, in an db_executor, this function is only ever called once.
std::string db_executor::generate_concat_columns(const std::string & sql) {
  cmd->setCommandText((dbi.properties.at("concat_col_prefix") + sql + dbi.properties.at("concat_col_suffix")).c_str());

  try {
    cmd->Execute(); // executes a dummy statement that performs no fetch, but exposes all columns and data types
  }
  catch (SAException &x) {
    excep_log( "Generate Columns Error : " +  std::string( (const char*)x.ErrText()) );
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

void db_executor::set_sql_hash_statement(const std::string & sql) {
  std::string hash_sql;
  hash_sql = dbi.properties.at("hash_prefix") + generate_concat_columns(sql) + dbi.properties.at("hash_mid") + sql + dbi.properties.at("hash_suffix");
  cmd_hash->setCommandText(hash_sql.c_str());
}

void db_executor::exec_sql() {
  try {
    cmd->Execute();
    cmd_hash->Execute();
    cmd_sel->Execute();
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
    std::cout << "Results " << v_result.back().first << " " << v_result.back().second << "\n";
  }
}

void db_executor::commit() {
  con->Commit();
  con_sel->Commit();
  con_hash->Commit();
}

void db_executor::rollback() {
  con->Rollback();
  con_sel->Rollback();
  con_hash->Rollback();
}

bool db_executor::make_connection() {
  std::string str;
  try {

    if (dbi.product=="sqlanywhere") {
      con->setOption(_TSA("SQLANY.LIBS")) = _TSA("/opt/sqlanywhere17/lib64/libdbcapi_r.so");
      con_hash->setOption(_TSA("SQLANY.LIBS")) = _TSA("/opt/sqlanywhere17/lib64/libdbcapi_r.so");
      con_sel->setOption(_TSA("SQLANY.LIBS")) = _TSA("/opt/sqlanywhere17/lib64/libdbcapi_r.so");
    }
      
    con->Connect(dbi.con_str.c_str(), dbi.usr.c_str(), dbi.pswd.c_str(), dbi.con_cl);
    con_hash->Connect(dbi.con_str.c_str(), dbi.usr.c_str(), dbi.pswd.c_str(), dbi.con_cl) ;
    con_sel->Connect(dbi.con_str.c_str(), dbi.usr.c_str(), dbi.pswd.c_str(), dbi.con_cl) ;
    
    con->setAutoCommit(SA_AutoCommitOff);
    con_hash->setAutoCommit(SA_AutoCommitOff);
    con_sel->setAutoCommit(SA_AutoCommitOff);

    if (dbi.set_isolation) con->setIsolationLevel(dbi.con_isolation_evel);
    if (dbi.set_isolation) con_hash->setIsolationLevel(dbi.con_isolation_evel);
    if (dbi.set_isolation) con_sel->setIsolationLevel(dbi.con_isolation_evel);
    
    /*  cmd:      used to inserts/updates and delets
     *  cmd_sel:  used to run select queries
     *  cmd_hash: used to obtain the hash of the result set generated by cmd_sel, and
     *            is run asynchronously to the cmd_sel, but uses the same SQL statement.
     *            No ORDER BY injection is necessary.
     *            
     */ 
    cmd->setConnection(con.get());
    cmd_sel->setConnection(con_sel.get());
    cmd_hash->setConnection(con_hash.get());
    
    if (dbi.product =="sqlanywhere") {
      cmd_sel->setCommandText("SET TEMPORARY OPTION isolation_level = 'snapshot'");
      cmd->setCommandText("SET TEMPORARY OPTION isolation_level = 'snapshot'");
      cmd_hash->setCommandText("SET TEMPORARY OPTION isolation_level = 'snapshot'");
      cmd->Execute();
      cmd_hash->Execute();
      cmd_sel->Execute();
    }

  }
  catch (SAException &x) { 
    excep_log( "Connection error :" + dbi.product + " - " + std::string((const char*)x.ErrText()) );
    return false; 
  }
  return true;
}

