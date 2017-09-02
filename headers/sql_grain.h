#ifndef SQL_GRAIN_H_
#define SQL_GRAIN_H_
#include <string>
#include <vector>

class sql_grain {
  private:
    int  statement_id{0};
    bool is_result {false};
    long rows{0}; // rows affected
    const std::string sql;
    bool updated{false};
    std::vector<std::string> v_res; // vector of result, only used for holding sql result sets
    std::string sql_hash{""};           // sql statement that returns  a hash md5 value for an entire result set
    std::string format_columns;     // formatted columns passed to the hash sql statement

  public:
    sql_grain(int, const std::string & ) noexcept;
    sql_grain() = delete;

    const std::string get_sql() const noexcept ;
    const int get_statement_id() const noexcept ;
    const bool is_updated() const noexcept ;
    const int get_rows_affected() const noexcept ;
    const std::string get_hash() const noexcept;
    const bool get_is_result() const noexcept;
    void set_hash_val(const std::string &) noexcept;
    void set_db_return_values(bool, long ) noexcept;

};

#endif
