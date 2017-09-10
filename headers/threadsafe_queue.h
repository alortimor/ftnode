#ifndef THREADSAFE_QUEUE_H_
#define THREADSAFE_QUEUE_H_

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

template<typename T>
class threadsafe_queue {
  private:
    mutable std::mutex mx;
    std::queue<std::unique_ptr<T>> unique_ptr_q;
    std::condition_variable cv;
  public:
    threadsafe_queue() {}
    threadsafe_queue(const threadsafe_queue&) = delete;
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

    //void push_unique(std::unique_ptr<T> ptr) {
    void push_unique(std::unique_ptr<T>&& ptr) {
      std::lock_guard<std::mutex> lk(mx);
      unique_ptr_q.push(std::move(ptr));
      cv.notify_one();
    }

    std::unique_ptr<T> pop_unique() {
      std::unique_lock<std::mutex> lk(mx);
      cv.wait(lk,[this]{return !unique_ptr_q.empty();});
      std::unique_ptr<T> up =  std::move(unique_ptr_q.front());
      unique_ptr_q.pop();
      return up;
    }

    bool empty() const {
      std::lock_guard<std::mutex> lk(mx);
      return unique_ptr_q.empty();
    }
};

#endif
