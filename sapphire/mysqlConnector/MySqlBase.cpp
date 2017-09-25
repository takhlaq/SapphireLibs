#include "MySqlBase.h"
#include "Connection.h"

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

Mysql::Connection* Mysql::MySqlBase::connect( const std::string& hostName, const std::string& userName,
                                              const std::string& password, uint16_t port = 3306 )
{
   return new Connection( this, hostName, userName, password, port );
}

Mysql::Connection* Mysql::MySqlBase::connect( const std::string& hostName, const std::string& userName,
                                              const std::string& password, const optionMap& options, uint16_t port = 3306 )
{
   return new Connection( this, hostName, userName, password, options, port );
}
