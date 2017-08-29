#include "../headers/request.h"

sql_grain::sql_grain(int sid, const std::string & statement) noexcept : statement_id{sid},  sql{statement}  { }

const std::string sql_grain::get_sql() const  noexcept { return sql; };

const int sql_grain::get_statement_id() const  noexcept  { return statement_id; };

const bool sql_grain::is_updated() const  noexcept  { return updated; };

const int sql_grain::get_rows_affected() const  noexcept  { return rows; };

const bool sql_grain::get_is_result() const  noexcept  { return is_result; };
    
void sql_grain::set_db_return_values(bool is_result_set, long rows_affected)  noexcept  {
  is_result = is_result_set;
  rows = rows_affected;
  updated = true;
};
