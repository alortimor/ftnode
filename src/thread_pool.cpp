#include "../headers/thread_pool.h"

// Constructor
thread_pool::thread_pool( int pool_size ) : work_ios{std::make_unique<boost::asio::io_service::work>(ios) } {
  if(pool_size==0) 
    pool_size = boost::thread::hardware_concurrency()*2; // default to number of cores x 2

  threads_free = pool_size;

  for ( int i = 0; i < pool_size; ++i )
    thread_grp.create_thread( boost::bind( &boost::asio::io_service::run, &ios ) );
}

// Destructor 2
thread_pool::~thread_pool () {
  work_ios.reset();
  try  { thread_grp.join_all(); } // ensure all threads complete before quitting
  catch ( const std::exception& ) {}
  ios.stop();
}

// ensure unique pointer is reset first off, so that threads are given
// a chance to complete.
void thread_pool::stop_service() {
  work_ios.reset(); // waits for all threads to complete
  try  { thread_grp.join_all(); }
  catch ( const std::exception& ) {}
  ios.stop();
}
