#ifndef CONSTANTS_H
#define CONSTANTS_H

const char SOCKET_MSG_END{'\n'};
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

#endif // CONSTANTS_H
