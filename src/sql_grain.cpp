#include "../headers/request.h"

const std::string sql_grain::get_sql() const { return sql; };

const int sql_grain::get_statement_id() const { return statement_id; };

const bool sql_grain::is_updated() const { return updated; };

const int sql_grain::get_rows() const { return rows; };

const bool sql_grain::get_is_result() const { return is_result; };
    
void sql_grain::set_db_return_values(bool is_result_set, long rows_affected) {
  is_result = is_result_set;
  rows = rows_affected;
  updated = true;
};
