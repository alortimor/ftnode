#ifndef DB_EXECUTOR_H_
#define DB_EXECUTOR_H_

#include <vector>
#include <memory>
#include <string>
#include </home/mw/SQLAPI/include/SQLAPI.h>
#include "db_info.h"
#include "logger.h"
#include "sql_grain.h"

extern logger exception_log;

/*struct sql_grain {
  private:
    int  statement_id{0};
    bool is_result {false};
    long rows{0};
    const std::string sql;
    bool updated{false};
    std::vector<std::string> v_res; // vector of result, only used for holding sql result sets.

  public:
    sql_grain(int sid, const std::string & statement) noexcept : statement_id{sid},  sql{statement}  { }
    sql_grain() = delete;

    const std::string get_sql() const noexcept { return sql; };
    const int get_statement_id() const noexcept { return statement_id; };
    const bool is_updated() const noexcept { return updated; };
    const int get_rows() const noexcept { return rows; };
    const bool get_is_result() const noexcept { return is_result; };
    void set_update(const bool b) noexcept { updated=b ; };

    void set_db_return_values(bool is_result_set, long rows_affected) noexcept {
       is_result = is_result_set;
       rows = rows_affected;
    };
};*/

class db_executor {
  private:
    int db_id;
    std::unique_ptr<SACommand> cmd;  // SACommand object associated with db connection (SAConnection)
                                     // both con and cmd have to be wrapped in unique pointers as they cannot be copied or moved
                                     // But with unique ptr wrapping, they can at least be moved and automatically destroyed.
    std::unique_ptr<SAConnection> con;
    db_info dbi;
    std::vector<sql_grain> v_sg; // this vector should be quicker, since all member functions are defined as noexcept, which means they can not throw exceptions

  public:
    db_executor(int dbid);
    db_executor(db_executor&&) = default;
    db_executor & operator=(const db_executor &) = delete;

    int const get_db_id() const;
    
    /* Sometimes db_info is passed in by value, sometimes by rvalue i.e. db_info{"","",..}, so one can either have overloaded functions
     * or a single universal template */
    template<typename T>
    void set_connection_info(T && dbi_ref) { // db_info
      dbi = std::forward<T>(dbi_ref); // db_ref as an argument is always an lvalue, but either an lvalue or rvalue can in fact be passed in.
                                      // but it is a lvalue argument, since we know the name of the variable (dbi_ref) and therefore the address (&dbi_ref).
                                      // Within the function, it keeps its rvalue/lvalue status. So therefore a move is not required, since a move would have already occurred
                                      // due to the fact that dbi_ref is temporary.
                                      // So therefore a std::forward instead of a std::move is more appropriate.
    }

    bool make_connection();
    void disconnect();
    void commit_rollback(char);
    void add_sql_grain(int, const std::string);
    void set_statement(const std::string & sql); // used for setting one-off sql statements, i.e. "begin"
    std::string const get_begin_statement() const;
    std::string const get_connection_str() const;
    std::string const get_product () const; // returns "oracle", "postgre" .. etc
    int const db_sql_grain_rows(int) const; // number of rows specific to a statement id
    bool const db_sql_grain_is_result(int) const; // result specific to a statement id

    bool is_complete();

    void exec_begin(); // the purpose of this is to explicitly execute the begin
    void execute_sql_grains ();
};

#endif
