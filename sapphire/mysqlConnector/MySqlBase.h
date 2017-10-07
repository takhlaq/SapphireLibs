#ifndef SAPPHIRE_MYSQLBASE_H
#define SAPPHIRE_MYSQLBASE_H

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <mysql.h>
#include <string>
#include <map>

namespace Mysql
{

typedef std::map< enum mysql_option, std::string > optionMap;

class Connection;

class MySqlBase : public boost::enable_shared_from_this< MySqlBase >
{
public:
   MySqlBase();

   ~MySqlBase();

   boost::shared_ptr< Connection > connect( const std::string& hostName, const std::string& userName,
                                            const std::string& password, uint16_t port );
   boost::shared_ptr< Connection > connect( const std::string& hostName, const std::string& userName,
                                            const std::string& password, const optionMap& map, uint16_t port );

   std::string getVersionInfo();

private:
   MySqlBase( const MySqlBase& );
   void operator=( MySqlBase& );
};

}

#endif //SAPPHIRE_MYSQLBASE_H
