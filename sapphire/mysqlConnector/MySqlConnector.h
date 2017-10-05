#include "MySqlBase.h"
#include "Connection.h"
#include "Statement.h"
#include "PreparedStatement.h"
#include "ResultSet.h"
#include "PreparedResultSet.h"
#include <boost/scoped_ptr.hpp>

namespace Mysql
{
   typedef boost::scoped_ptr< Mysql::PreparedResultSet > PreparedResultSetScopedPtr;
}
