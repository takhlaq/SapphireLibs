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

Mysql::Connection* Mysql::MySqlBase::connect( const std::string& hostName, const std::string& userName, const std::string& password )
{
   return new Connection( this, hostName, userName, password );
}

Mysql::Connection* Mysql::MySqlBase::connect( const std::string& hostName, const std::string& userName, const std::string& password, const optionMap& options )
{
   return new Connection( this, hostName, userName, password, options );
}
