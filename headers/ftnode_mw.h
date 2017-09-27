#ifndef FTNODE_MW_H
#define FTNODE_MW_H
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <thread>
#include <mutex>
#include "tcp_server.h"

// Kick starts the TCP/IP server
class ftnode_mw {
public:
  void start();

protected:

private:
  std::unique_ptr<tcp_server> tcp_srv;
};


#endif // FTNODE_MW_H
