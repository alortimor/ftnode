#ifndef DB_BUFFER_H
#define DB_BUFFER_H
#include <unordered_map>
#include <functional>
#include <exception>
#include <condition_variable>
#include <stack>
#include "request.h"
#include "db_info.h"

/*
struct empty_stack: std::exception {
  const char* what() const throw();
};

template<typename T>
class threadsafe_stack {
private:
  std::stack<T> data;
  mutable std::mutex m;

public:
  threadsafe_stack(){}
  threadsafe_stack(const threadsafe_stack& other) =delete;
  threadsafe_stack& operator=(const threadsafe_stack&) = delete;
   
  void push(T new_value) {
    std::lock_guard<std::mutex> lock(m);
    data.push(std::move(new_value));
  }

  void pop(T& value) {
    std::lock_guard<std::mutex> lock(m);
    if(data.empty()) throw empty_stack();
    value=std::move(data.top());
    data.pop();
  }
 
  bool empty() const {
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
  }

};
*/

struct Key {
  int request_id;
  bool operator==(const Key &other) const {  return (request_id == other.request_id);  }
};

struct KeyHasher {
  std::size_t operator()(const Key& k) const {
    using std::size_t;
    using std::hash; //include functional
    return (((hash<int>()(k.request_id) << 1)) >> 1);
  }
};

using hash_table = std::unordered_map<Key, request, KeyHasher>;

class db_buffer {
private:
  int size;
  int slots_free;
  std::mutex mx; // only a single instance of db_buffer therefore does not need to be static
  hash_table request_buffer;
  std::stack<int> st;
  std::vector<db_info> v_dbi;

  std::condition_variable cv_stack; // only a single instance of db_buffer therefore does not need to be static

public:
  explicit db_buffer(int buffer_size);
  db_buffer(const db_buffer &) = delete;
  db_buffer & operator=(const db_buffer &) = delete;
  ~db_buffer();

  void set_db_info();
  void make_connections();
  bool make_inactive (int);
  auto percent_free() const;
  int make_active (boost::asio::ip::tcp::socket *socket);
  request * get_request(int);

};

// void print_buffer_content(db_buffer& buff);

#endif // DB_BUFFER_H
