#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <string>
#include "logger.h"

const char SOCKET_MSG_END_CHAR{'\n'};
const std::string CLIENT_MSG_END{"\n\n"};

// Should be changed toenums
const std::string COMMIT{"COMMIT"};
const std::string COMMITED{"COMMITED"};
const std::string ROLLBACK{"ROLLBACK"};
const std::string ROLLED_BACK{"ROLLED_BACK"};
const std::string DISCONNECT{"DISCONNECT"};
const std::string DISCONNECTED{"DISCONNECTED"};
const std::string SOCKET_ERROR{"SOCKET_ERROR"};
const std::string FAILURE{"FAILURE"};
const std::string COMPARATOR_FAIL{"COMPARATOR_FAIL"};

// macros
#define log_err(x) log_1(std::string("Error: ") + x)

// exception errors
/*
2 - tcp server networking failure
3 - database connectivity failure
4 - file i/o failure
5 - no settings.xml failure
6 - incorrect xml format
*/
constexpr int ERR_TCP_FAILURE{2};
constexpr int ERR_DB_CONNECTION{3};
constexpr int ERR_FILE{4};
constexpr int ERR_XML_NO_FILE{5};
constexpr int ERR_XML_FORMAT{6};




#endif // CONSTANTS_H
