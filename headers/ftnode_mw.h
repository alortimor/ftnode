#ifndef FTNODE_MW_H
#define FTNODE_MW_H
#include <memory>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include "../headers/tcp_server.h"
//#include "xml_settings.h"

const unsigned int DEFAULT_THREAD_POOL_SIZE = 2;

class ftnode_mw
{
public:
    void start();

protected:

private:
    // working objects. You change which object you want to use
    // by assigning other values to these pointers.
    //db_buffer* db_buffer_{nullptr};
    tcp_server* tcp_server_{nullptr};


    // default objects - used by default when the object is created
    //std::unique_ptr<db_buffer> def_db_buffer_;
    //std::unique_ptr<tcp_server> def_tcp_server_;
    std::unique_ptr<tcp_server> def_tcp_server_;
};


#endif // FTNODE_MW_H
