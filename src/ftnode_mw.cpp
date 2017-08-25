#include <thread>
#include "../headers/ftnode_mw.h"

//#include "db_buffer.h"

#include "../headers/db_service.h"
#include "../headers/globals.h"
#include "../headers/xml_settings.h"
//#include "globals.h"


void ftnode_mw::start()
{
    db_service _db_service; //{8};

    // create/use default tcp objects
//    def_db_buffer_ = std::make_unique<db_buffer>();
    def_tcp_server_ = std::make_unique<tcp_server>(&_db_service);

//    db_buffer_ = def_db_buffer_.get();
    tcp_server_ = def_tcp_server_.get();

    if(def_tcp_server_ == nullptr)
    {
        std::cout << "Error: no tcp server.\n";
        return;
    }

//    _comparator.set_db_buffer(db_buffer_);
    //_db_service.set_db_buffer(db_buffer_);
    // ref() because we want to use _db_service object created above and not a copy of it
    //std::thread comparator_thread(std::ref(_comparator));
    std::thread db_service_thread(std::ref(_db_service));

    // endpoint has only one element at position 0
    auto _end_point = settings().get(xmls::ftnode_mw::ENDPOINT).at(0).get(); // xmls::ELEM_ENDPOINT
    const unsigned short port_num = static_cast<xmls::end_point*>(_end_point)->port;
    try
    {
        unsigned int thread_pool_size = std::thread::hardware_concurrency() * 2;
        if (thread_pool_size == 0)
            thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
        def_tcp_server_->Start(port_num, thread_pool_size);
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::string input_str;
        while(input_str != "q" && input_str != "Q")
        {
          std::cin >> input_str;
          if(input_str == "q" || input_str == "Q")
            _db_service.stop();
          excep_log(input_str);
        }

        //comparator_thread.join();
        db_service_thread.join();
        def_tcp_server_->Stop();
    }
    catch (system::system_error&e)
    {
        //comparator_thread.join();
        db_service_thread.join();
        std::cerr << "Error occured! Error code = "
            <<e.code() << ". Message: "
            <<e.what();
    }
}
