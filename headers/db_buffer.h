#ifndef DB_BUFFER_H
#define DB_BUFFER_H
#include <unordered_map>
#include <functional>
#include <exception>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>
#include <stack>
#include "db_adjudicator.h"
#include "db_info.h"

// Fixed buffer, unlike algorithm description in Fig 13 in patent document, which is a variable produced/consumer (Q) type buffer
// this is hash table instead.

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

using hash_table = std::unordered_map<Key, std::unique_ptr<db_adjudicator>, KeyHasher>;

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
    explicit db_buffer(int); // size
    db_buffer(const db_buffer &) = delete;
    db_buffer & operator=(const db_buffer &) = delete;
    ~db_buffer();

    void set_db_info();
    bool make_inactive (int);
    auto percent_free() const;
    int make_active (std::unique_ptr<tcp_session>&& );
    db_adjudicator * get_request(int);
};

#endif // DB_BUFFER_H
