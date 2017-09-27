#ifndef GLOBALS_H
#define GLOBALS_H

#include <exception>
#include <string>

// Initiates global startup
void global_init();
struct ftnode_exception : public std::exception {
  ftnode_exception(int err_code, const std::string& msg="") : 
    message{msg}, error_code{err_code} {}
  const char* what() const throw () {
    return message.c_str();
  }
  int get_err_code() const { return error_code; }
  const std::string& get_message() const { return message; }
private:
    std::string message;
    int error_code{0};
};

#endif // GLOBALS_H
