#include "MySqlBase.h"
#include "Connection.h"
#include <boost/make_shared.hpp>

Mysql::MySqlBase::MySqlBase() 
{
}

Mysql::MySqlBase::~MySqlBase()
{

}

std::string Mysql::MySqlBase::getVersionInfo()
{
   return std::string( mysql_get_client_info() );
}

boost::shared_ptr< Mysql::Connection > Mysql::MySqlBase::connect( const std::string& hostName, const std::string& userName,
                                                                  const std::string& password, uint16_t port = 3306 )
{
   return boost::make_shared< Mysql::Connection >( shared_from_this(), hostName, userName, password, port );
}

boost::shared_ptr< Mysql::Connection > Mysql::MySqlBase::connect( const std::string& hostName, const std::string& userName,
                                                                  const std::string& password, const optionMap& options,
                                                                  uint16_t port = 3306 )
{
   return boost::shared_ptr< Mysql::Connection >( new Mysql::Connection( shared_from_this(), hostName, userName, password, options, port ) );
}
