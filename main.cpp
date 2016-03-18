//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <map>
#include <vector>
#include <utility>
#include <thread>
#include "boost/asio.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include "boost/thread/thread.hpp"

#include "app_client.hpp"
#include "dev_client.hpp"

using boost::asio::ip::tcp;
using namespace std;
using boost::system::error_code;
namespace pt = boost::property_tree;

int main(int argc, char* argv[])
{

    boost::asio::io_service dev_service;
    boost::asio::io_service app_service;
    boost::asio::io_service cmd;

    AppServer appsrv(app_service,5561);
    DevServer devsrv(dev_service,5560);

    vector<boost::thread> thset;
    thset.push_back(boost::thread(boost::bind(&boost::asio::io_service::run,&dev_service)));

    thset.push_back(boost::thread(boost::bind(&boost::asio::io_service::run,&app_service)));


    for(vector<boost::thread>::const_iterator iter = thset.begin();
        iter != thset.end();++iter)
        {
            iter->join();
        }

    return 0;
}
