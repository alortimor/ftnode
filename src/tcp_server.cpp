#include "../headers/tcp_server.h"



tcp_server::tcp_server(db_service* _db_service) : db_service_{_db_service}
{
    m_work.reset(new asio::io_service::work(m_ios));
}

void tcp_server::Start(unsigned short port_num, unsigned int thread_pool_size)
{
    std::cout << "start";
    assert(thread_pool_size > 0);
    // Create and start Acceptor.
    acc.reset(new tcp_acceptor(m_ios, port_num, db_service_));
    acc->Start();

    // Create specified number of threads and
    // add them to the pool.
    for (unsigned int i = 0; i < thread_pool_size; i++)
    {
        std::unique_ptr<std::thread> th(
            new std::thread([this]()
            {
                m_ios.run();
            })
        );
        m_thread_pool.push_back(std::move(th));
    }
}

void tcp_server::Stop()
{
    acc->Stop();
    m_ios.stop();
    for (auto& th : m_thread_pool)
    {
        th->join();
    }
}
