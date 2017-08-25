#ifndef SQL_GRAIN_H_
#define SQL_GRAIN_H_


class sql_grain {
  private:
    int  statement_id{0};
    bool is_result {false};
    long rows{0};
    const std::string sql;
    bool updated{false};
    std::vector<std::string> v_res; // vector of result, only used for holding sql result sets

  public:
    sql_grain(int sid, const std::string & statement) noexcept : statement_id{sid},  sql{statement}  { }
    sql_grain() = delete;

    const std::string get_sql() const noexcept ;
    const int get_statement_id() const noexcept ;
    const bool is_updated() const noexcept ;
    const int get_rows_affected() const noexcept ;
    const bool get_is_result() const noexcept;
    void set_db_return_values(bool, long ) noexcept;

};

#endif
