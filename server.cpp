#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include "boost/asio.hpp"
#include "boost/container/map.hpp"
#include "boost/unordered_map.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
//#include "Message.hpp"
#include "app_client.hpp"
#include "dev_client.hpp"
#include "console.hpp"



using boost::system::error_code;
namespace pt = boost::property_tree;
using namespace boost::posix_time;
using namespace std;
using namespace boost;


AppClient::AppClient(ip::tcp::socket socket)
    : sock_(std::move(socket))
{

}

void  AppClient::start()
{
    // started_ = true;
    //clients.push_back( shared_from_this());
    // last_ping = boost::posix_time::microsec_clock::local_time();
    // first, we wait for client to login
    do_read();/* 读取客户端*/
}

void  AppClient::stop()
{
    sock_.close();
    /*
        ptr self = shared_from_this();
        array::iterator it = std::find(clients.begin(), clients.end(), self);
        clients.erase(it);
       // update_clients_changed();
       */
}


void  AppClient::on_read(const error_code & err, size_t bytes)
{
    if ( err)
    {
        stop();
        return;
    }

//    if ( !started() ) return;
    // process the msg
    //std::string msg(read_buffer_, bytes);

    /*处理json格式的命令*/
    // cout << "app recv data" << read_buffer_ << endl;
    pt::ptree pt ;
    try
    {
        pt = GetJson(read_buffer_);

    }
    catch(std::exception &e)
    {
        stop();
        return;
    }
    std::string cmd ;
    //cout << "remote address is " << sock_.remote_endpoint().address().to_string() << endl;
    try
    {
        cmd = pt.get<std::string>(CMD);

    }
    catch(std::exception& e)
    {
        //  cout << "read unkown command " << endl;
        do_write(cmd_unkown,true);
        //CloseConnection();
        return ;

    }


    if(!cmd.compare(CONN))
    {
        /*查找相应用的设备端响应用*/
        string uuid;
        string pwd;
        try
        {
            pwd = pt.get<std::string>(PWD);
            uuid = pt.get<std::string>(UUID);
        }
        catch(std::exception& e)
        {
            do_write(cmd_unkown);
            stop();
            return ;
        }
        DevClientPtr dcp;
        try
        {
            dcp =  devs_map[uuid];
        }
        catch(std::exception &e)
        {
            do_write(err_offline);
            stop();
            return ;
        }
        if(dcp.get() == nullptr) /* 设备不在线*/
        {
            do_write(err_offline);
            stop();
            return ;
        }
        /*简易认证*/

        string aid = sock_.remote_endpoint().address().to_string();
        pt.put(AID,aid);
        pt.erase(CMD);
        pt.put(MSG,CONN);
        std::stringstream ss;
        pt::write_json(ss,pt,false);


        // std::make_shared<DevClient>(std::move(dcp))->do_write(ss.str());
        // cout << " send to data to device " << ss.str() << endl;
        std::string wmsg = ss.str();

        wmsg.erase(std::remove(wmsg.begin(),wmsg.end(),'\n'),wmsg.end());
        wmsg.append("\r\n");
        boost::shared_ptr<DevClient>(dcp)->do_write(wmsg);
        //  apps_map[aid] = boost::shared_ptr<AppClient>(this);
        apps_map[aid] = GetSelf();

    }
}

void AppClient::handle_system_error()
{
    auto self(shared_from_this());
    for(AppMap::iterator it = apps_map.begin(); it != apps_map.end() ; ++it)
    {
        if(it->second == self)
        {
            stop();
            apps_map.erase(it->first);
            break;
        }
    }
}


void  AppClient::on_write(const error_code & err, size_t bytes)
{
    do_read();
}
void  AppClient::do_read()
{
    auto self(shared_from_this());

    sock_.async_read_some(boost::asio::buffer(read_buffer_),
                          [this,self](boost::system::error_code ec,std::size_t length)
    {
        if(!ec)
        {
            on_read(ec,length);
        }
        else
        {
            if(ec.value() == 2)
            {

                handle_system_error();
            }

        }
    });
}

void  AppClient::do_write(const std::string & msg,bool wclose)
{
    auto self(shared_from_this());
    std::copy(msg.begin(), msg.end(), write_buffer_);
    sock_.async_write_some( buffer(msg),
                            //    MEM_FN2(on_write,_1,_2));
                            [this,self,wclose](boost::system::error_code ec,std::size_t length)
    {
        if(wclose)
        {
            stop();
            return;
        }
        if(!ec)
        {
            do_read();
        }

        else
        {
            if(ec.value() == 2)
            {

                handle_system_error();
            }

        }
    });

}


DevClient::DevClient(ip::tcp::socket socket)
    :sock_(std::move(socket)),
     timer_(sock_.get_io_service())
{

}



void DevClient::stop()
{

    sock_.close();
}




void DevClient::on_read(const error_code & err, size_t bytes)
{

    if ( err)
    {
        stop();
        return;
    }
    // if ( !started() ) return;

    // process the msg
    //std::string msg(read_buffer_, bytes);

    /*处理json格式的命令*/

    //  cout << "recv data "  << read_buffer_ << endl;
    std::string read_msg(read_buffer_,bytes);
    pt::ptree pt;
    try
    {
        pt =  GetJson(read_buffer_);
    }
    catch(std::exception& e)
    {
        stop();
        return ;
    }



    memset(read_buffer_,0,max_msg);
    std::string cmd ;
    std::string control;
    // cout << "remote address is " << sock_.remote_endpoint().address().to_string() << endl;
    try
    {
        control = pt.get<std::string>("INFO");

    }
    catch(std::exception &e)
    {

    }
    if(control.)

    try
    {
        cmd = pt.get<std::string>(CMD);

    }
    catch(std::exception& e)
    {
        //cout << "read unkown command " << endl;
        do_write(cmd_unkown,true);
        return ;

    }

    if(!cmd.compare(LOGIN) )
    {
        // cout << "dev login " << endl;
        // msg_.SetNewData(msg_.msg_ok);
        std::string pwd;
        std::string uuid;
        try
        {
            pwd = pt.get<std::string>(PWD);
            uuid = pt.get<std::string>(UUID);
        }
        catch (std::exception& e)
        {
            do_write(cmd_unkown,true);
            stop();
            return ;

        }

        /* 在这里要添加是否重复登录的检测*/
        DevClientPtr dcp;
        try
        {
            dcp  = devs_map[uuid];
        }
        catch(std::exception& e)
        {

        }
        if(dcp.get() != nullptr )
        {
            /* 断开之前的连接 */
            boost::shared_ptr<DevClient>(dcp)->stop();
            devs_map.erase(uuid);
        }

        devs_map[uuid] = GetSelf();

        do_write(msg_ok);

    }
    else if(!cmd.compare(KEEP))
    {
        do_write(msg_keep);
    }
    else if(!cmd.compare(CONN))
    {
        string aid;
        try
        {
            aid = pt.get<std::string>(AID);

        }
        catch (std::exception& e)
        {
            do_write(cmd_unkown,true);

            return ;
        }

        AppClientPtr acp ;
        try
        {
            acp = apps_map[aid];
        }
        catch(std::exception& e)
        {

        }
        if(acp.get() != nullptr )
        {
            //  cout << " write msg to app client " << read_msg << endl;
            boost::shared_ptr<AppClient>(acp)->do_write(read_msg,true);
            apps_map.erase(aid);
        }
    }
    else
    {
        do_write(cmd_unkown);
        stop();
        return ;
    }
}


void DevClient::start()
{
    do_read();
}


/*
void DevClient::on_check_ping()
{
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    if ( (now - last_ping).total_milliseconds() > 5000)
    {
        std::cout << "stopping " << username_ << " - no ping in time" << std::endl;
        stop();
    }
    last_ping = boost::posix_time::microsec_clock::local_time();
}
void DevClient::post_check_ping()
{
    timer_.expires_from_now(boost::posix_time::millisec(5000));
    timer_.async_wait( MEM_FN(on_check_ping));
}
*/

void DevClient::on_write(const error_code & err, size_t bytes)
{
    do_read();
}

void DevClient::handle_system_error()
{
    auto self(shared_from_this());
    for(DevMap::iterator it = devs_map.begin(); it != devs_map.end() ; ++it)
    {
        if(it->second == self)
        {
            stop();
            devs_map.erase(it->first);
            break;
        }
    }
}

void DevClient::do_read()
{
    auto self(shared_from_this());
    sock_.async_read_some(boost::asio::buffer(read_buffer_),
                          [this,self](boost::system::error_code ec,std::size_t length)
    {
        if(!ec)
        {
            on_read(ec,length);
        }
        else
        {
            if(ec.value() == 2)
            {

                handle_system_error();
            }

        }
    });

    // post_check_ping();
}



void DevClient::do_write(const std::string & msg,bool wclose)
{
    // if ( !started() ) return;
    memset(write_buffer_,0,max_msg);
    std::copy(msg.begin(), msg.end(), write_buffer_);
    auto self(shared_from_this());
    boost::asio::async_write(sock_, buffer(write_buffer_, strlen(write_buffer_)),
                             [this,self,wclose](boost::system::error_code ec,size_t)
    {

        if(wclose)
        {
            stop();
            return;
        }
        if(!ec)
        {
            do_read();
        }
        else
        {
            if(ec.value() == 2)
            {

                handle_system_error();
            }

        }
    } );
}

