#include "Connection.h"
#include "MySqlBase.h"
#include "Statement.h"
#include "PreparedStatement.h"
#include "ResultSet.h"

#include <string>
#include <vector>
#include <boost/scoped_array.hpp>
#include <boost/make_shared.hpp>
#include <algorithm>
#include <chrono>

Mysql::Connection::Connection( boost::shared_ptr< MySqlBase > pBase,
                               const std::string& hostName, 
                               const std::string& userName,
                               const std::string& password,
                               uint16_t port ) :
    m_pBase( pBase ),
    m_bConnected( false )
{
   m_pRawCon = mysql_init( nullptr );
   if( mysql_real_connect( m_pRawCon, hostName.c_str(), userName.c_str(), password.c_str(),
                          nullptr, port, nullptr, 0) == nullptr )
      throw std::runtime_error( mysql_error( m_pRawCon ) );
   m_bConnected = true;
   m_pingThread = std::thread([this]() { pingLoop(); });
}

Mysql::Connection::Connection( boost::shared_ptr< MySqlBase > pBase,
                               const std::string& hostName,
                               const std::string& userName,
                               const std::string& password,
                               const optionMap& options,
                               uint16_t port ) :
    m_pBase( pBase )
{
   m_pRawCon = mysql_init( nullptr );
   // Different mysql versions support different options, for now whatever was unsupporter here was commented out
   // but left there.
   for( auto entry : options )
   {
      switch( entry.first )
      {
      // integer based options
      case MYSQL_OPT_CONNECT_TIMEOUT:
      case MYSQL_OPT_PROTOCOL:
      case MYSQL_OPT_READ_TIMEOUT:
      //    case MYSQL_OPT_SSL_MODE:
      //    case MYSQL_OPT_RETRY_COUNT:
      case MYSQL_OPT_WRITE_TIMEOUT:
      //    case MYSQL_OPT_MAX_ALLOWED_PACKET:
      //    case MYSQL_OPT_NET_BUFFER_LENGTH:
      {
         uint32_t optVal = std::stoi( entry.second, nullptr, 10 );
         setOption( entry.first, optVal );
      }
      break;

      // bool based options
      //    case MYSQL_ENABLE_CLEARTEXT_PLUGIN:
      //    case MYSQL_OPT_CAN_HANDLE_EXPIRED_PASSWORDS:
      case MYSQL_OPT_COMPRESS:
      case MYSQL_OPT_GUESS_CONNECTION:
      case MYSQL_OPT_LOCAL_INFILE:
      case MYSQL_OPT_RECONNECT:
      //    case MYSQL_OPT_SSL_ENFORCE:
      //    case MYSQL_OPT_SSL_VERIFY_SERVER_CERT:
      case MYSQL_OPT_USE_EMBEDDED_CONNECTION:
      case MYSQL_OPT_USE_REMOTE_CONNECTION:
      case MYSQL_REPORT_DATA_TRUNCATION:
      case MYSQL_SECURE_AUTH:
      {
         my_bool optVal = entry.second == "0" ? 0 : 1;
         setOption( entry.first, &optVal );
      }
      break;

      // string based options
      //    case MYSQL_DEFAULT_AUTH:
      //    case MYSQL_OPT_BIND:
      //    case MYSQL_OPT_SSL_CA:
      //    case MYSQL_OPT_SSL_CAPATH:
      //    case MYSQL_OPT_SSL_CERT:
      //    case MYSQL_OPT_SSL_CIPHER:
      //    case MYSQL_OPT_SSL_CRL:
      //    case MYSQL_OPT_SSL_CRLPATH:
      //    case MYSQL_OPT_SSL_KEY:
      //    case MYSQL_OPT_TLS_VERSION:
      //    case MYSQL_PLUGIN_DIR:
      case MYSQL_READ_DEFAULT_FILE:
      case MYSQL_READ_DEFAULT_GROUP:
      //    case MYSQL_SERVER_PUBLIC_KEY:
      case MYSQL_SET_CHARSET_DIR:
      case MYSQL_SET_CHARSET_NAME:
      case MYSQL_SET_CLIENT_IP:
      case MYSQL_SHARED_MEMORY_BASE_NAME:
      {
         setOption( entry.first, entry.second.c_str() );
      }
      break;

      default:
         throw std::runtime_error( "Unknown option: " + std::to_string( entry.first ) );
    }

   }


   if( mysql_real_connect( m_pRawCon, hostName.c_str(), userName.c_str(), password.c_str(),
                           nullptr, port, nullptr, 0) == nullptr )
      throw std::runtime_error( mysql_error( m_pRawCon ) );
   m_bConnected = true;
   m_pingThread = std::thread( [this](){ pingLoop(); } );
}


Mysql::Connection::~Connection()
{
   close();
}

void Mysql::Connection::setOption( enum mysql_option option, const void *arg )
{

   if( mysql_options( m_pRawCon, option, arg ) != 0  )
      throw std::runtime_error( "Connection::setOption failed!" );

}

void Mysql::Connection::setOption( enum mysql_option option, uint32_t arg )
{
 
   if( mysql_options( m_pRawCon, option, &arg ) != 0  )
      throw std::runtime_error( "Connection::setOption failed!" );

}

void Mysql::Connection::setOption( enum mysql_option option, const std::string& arg )
{

   if( mysql_options( m_pRawCon, option, arg.c_str() ) != 0 )
      throw std::runtime_error( "Connection::setOption failed!" );

}

void Mysql::Connection::close()
{
   std::lock_guard< std::mutex > lock( m_connMutex );
   if( m_pRawCon )
      mysql_close( m_pRawCon );

   m_bConnected = false;

   if( m_pingThread.joinable() )
      m_pingThread.join();
}

bool Mysql::Connection::isClosed() const
{
   return !m_bConnected;
}

boost::shared_ptr< Mysql::MySqlBase > Mysql::Connection::getMySqlBase() const
{
   return m_pBase;
}

void Mysql::Connection::setAutoCommit( bool autoCommit )
{
   auto b = static_cast< my_bool >( autoCommit == true ? 1 : 0 );
   if( mysql_autocommit( m_pRawCon, b ) != 0 )
      throw std::runtime_error( "Connection::setAutoCommit failed!" );
}

bool Mysql::Connection::getAutoCommit()
{
   // TODO: should be replaced with wrapped sql query function once available
   std::string query("SELECT @@autocommit");
   auto res = mysql_real_query( m_pRawCon, query.c_str(), query.length() );

   if( res != 0 )
      throw std::runtime_error( "Query failed!" );

   auto pRes = mysql_store_result( m_pRawCon );
   auto row = mysql_fetch_row( pRes );

   uint32_t ac = atoi( row[0] );
   mysql_free_result( pRes );
   return ac != 0;
}

void Mysql::Connection::beginTransaction()
{
   auto stmt = createStatement();
   stmt->execute( "START TRANSACTION;" );
}

void Mysql::Connection::commitTransaction()
{
   auto stmt = createStatement();
   stmt->execute( "COMMIT" );
}

void Mysql::Connection::rollbackTransaction()
{
   auto stmt = createStatement();
   stmt->execute( "ROLLBACK" );
}

std::string Mysql::Connection::escapeString( const std::string &inData )
{
   boost::scoped_array< char > buffer( new char[inData.length() * 2 + 1] );
   if( !buffer.get() )
      return "";
   unsigned long return_len = mysql_real_escape_string( m_pRawCon, buffer.get(),
                                                        inData.c_str(), static_cast< unsigned long > ( inData.length() ) );
   return std::string( buffer.get(), return_len );
}

void Mysql::Connection::setSchema( const std::string &schema )
{
   if( mysql_select_db( m_pRawCon, schema.c_str() ) != 0 )
      throw std::runtime_error( "Current database could not be changed to " + schema );
}

boost::shared_ptr< Mysql::Statement > Mysql::Connection::createStatement()
{
   return boost::make_shared< Mysql::Statement >( shared_from_this() );
}

MYSQL* Mysql::Connection::getRawCon()
{
   return m_pRawCon;
}

void Mysql::Connection::pingLoop()
{
   // todo: this sleep is so dumb, initialise the loop somewhere outside the constructor
   std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
   std::chrono::seconds timeout;
   {
      std::lock_guard< std::mutex > lock( m_connMutex );

      if( isClosed() )
         throw std::runtime_error( "Mysql: No connection to retrieve wait_timeout!" );

      std::string query( "SHOW VARIABLES LIKE 'wait_timeout';" );
      auto res = mysql_real_query( m_pRawCon, query.c_str(), query.length() );

      if( res != 0 )
         throw std::runtime_error("Mysql::pingLoop wait_timeout query failed!");

      auto pRes = mysql_store_result( m_pRawCon );
      auto row = mysql_fetch_row( pRes );

      // todo: probably use a config value for ping timeout and use that instead of hardcoded 30sec leeway
      int seconds = atoi( row[1] );
      seconds = seconds - 30 < 1 ? 30 : seconds - 30;

      timeout = std::chrono::seconds( seconds );
      mysql_free_result( pRes );
   }

   while( true )
   {
      {
         std::lock_guard< std::mutex > lock( m_connMutex );
         if ( isClosed() )
            break;

         ping();
      }
      std::this_thread::sleep_for( timeout );
   }
}

std::string Mysql::Connection::getError()
{
   auto mysqlError = mysql_error( m_pRawCon );
   if( mysqlError )
      return std::string( mysqlError );
   return "";
}

boost::shared_ptr< Mysql::PreparedStatement > Mysql::Connection::prepareStatement( const std::string &sql )
{
   MYSQL_STMT* stmt = mysql_stmt_init( getRawCon() );

   if( !stmt )
      throw std::runtime_error( "Could not init prepared statement: " + getError() );

   if( mysql_stmt_prepare( stmt, sql.c_str(), sql.size() ) )
      throw std::runtime_error( "Could not prepare statement: " + getError() );

   return boost::make_shared< PreparedStatement >( stmt, shared_from_this() );
}

uint32_t Mysql::Connection::getErrorNo()
{
   return mysql_errno( m_pRawCon );
}

void Mysql::Connection::ping()
{
   mysql_ping( m_pRawCon );
}

