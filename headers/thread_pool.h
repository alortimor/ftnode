#ifndef THREADPOOL_H_
#define THREADPOOL_H_
//based on Boost Proactor model
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/condition_variable.hpp> // one cannot use stl condition variables with boost unique locks
                                               // and one cannot use stl locks with the boost asynchronous io_service
                                               // so hence when using io_service, all subsequent thread related objects
                                               // must be boost compatible.
                                               // The exception to this is unique_ptr, which use to be a boost feature
                                               // and was included in C++11 based on the underlying boost implementation.
class thread_pool {
private:
  boost::asio::io_service ios; // asynch io process, has internal enque/dequeue processes based on proactor design pattern
  std::unique_ptr<boost::asio::io_service::work> work_ios; // prevents ios from shutting down
  boost::thread_group thread_grp;
  int threads_free;
  boost::mutex mx;
  boost::condition_variable cv;
public:

   explicit thread_pool( int pool_size = 0 );
   thread_pool (const thread_pool &) = delete;
   thread_pool & operator =(thread_pool &) = delete;

  ~thread_pool();

  void stop_service();

  template < typename Job >
  void run_job( Job job ) {
    boost::unique_lock< boost::mutex > lock( mx );

    //if ( 0 == threads_free ) return; // jobs were being lost prior to the introduction of the condition variable
    while (threads_free==0) cv.wait(lock); // never blocks unless there are no available threads
    --threads_free;
    ios.dispatch( boost::bind( &thread_pool::wrap_job, this, boost::function< void() >(job) ));
  }

private:
  void wrap_job( boost::function< void() > job ) {
    try { job(); }
    catch ( const std::exception& ) {}

    {
      boost::unique_lock< boost::mutex > lock( mx );
      ++threads_free;
    }
    
    cv.notify_one(); // only notify one at time to reduce broadcasting interference related to the other io_service objects in use
  }
};

#endif
