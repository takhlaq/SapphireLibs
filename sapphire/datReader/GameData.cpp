#include "GameData.h"

#include <string>
#include <sstream>
#include <algorithm>

#include <boost/assign/list_of.hpp>
#include <boost/bimap.hpp>

#include <zlib/zlib.h>

#include "bparse.h"
#include "DatCat.h"
#include "File.h"
#include <common/Logging/Logger.h>

Core::Logger m_log;

namespace
{
    // Relation between category number and category name
    // These names are taken straight from the exe, it helps resolve dispatching when getting files by path
    boost::bimap<std::string, uint32_t> category_map = boost::assign::list_of<boost::bimap<std::string, uint32_t>::relation>
        ("common",          0x00)
        ("bgcommon",        0x01)
        ("bg",              0x02)
        ("cut",             0x03)
        ("chara",           0x04)
        ("shader",          0x05)
        ("ui",              0x06)
        ("sound",           0x07)
        ("vfx",             0x08)
        ("ui_script",       0x09)
        ("exd",             0x0A)
        ("game_script",     0x0B)
        ("music",           0x0C);
}

namespace xiv
{
namespace dat
{

GameData::GameData(const boost::filesystem::path& i_path) try :
    m_path(i_path)
{
   int maxExLevel = 0;
   
   // Determine which expansions are available
   while( boost::filesystem::exists( boost::filesystem::path( m_path.string() + "\\..\\ex" + std::to_string( maxExLevel + 1) + "\\ex" + std::to_string( maxExLevel + 1) + ".ver" ) ) )
   {
      maxExLevel++;
   }
   
   m_log.info( "[GameData] Detected Expansion Level: " + std::to_string( maxExLevel ) );

   // Iterate over the files in path
   for( auto it = boost::filesystem::directory_iterator( m_path ); it != boost::filesystem::directory_iterator(); ++it )
   {
      // Get the filename of the current element
      auto filename = it->path().filename().string();

      // If it contains ".win32.index" this is most likely a hit for a category
      if( filename.find( ".win32.index" ) != std::string::npos &&  filename.find( ".win32.index2" ) == std::string::npos )
      {
         // Format of indexes is XX0000.win32.index, so fetch the hex number for category number
         std::istringstream iss( filename.substr( 0, 2 ) );
         uint32_t cat_nb;
         iss >> std::hex >> cat_nb;

         m_log.info( "[GameData] Found Category: " + filename + " (" + iss.str() + ")");

         // Add to the list of category number
         // creates the empty category in the cats map
         // instantiate the creation mutex for this category
         m_catNums.push_back( cat_nb );
         m_cats[cat_nb] = std::unique_ptr<Cat>();
         m_catCreationMutexes[cat_nb] = std::unique_ptr<std::mutex>( new std::mutex() );

         // Check for expansion
         for( int exNum = 1; exNum <= maxExLevel; exNum++ )
         {
            const std::string path = m_path.string() + "\\..\\" + buildDatStr( "ex" + std::to_string( exNum ), cat_nb, exNum, 0, "win32", "index" );

            if( boost::filesystem::exists( boost::filesystem::path( path ) ) )
            {
               m_log.info( "[GameData] -> Found cat for ex" + std::to_string( exNum ) );

               int chunkCount = 0;
               
               while( boost::filesystem::exists( m_path.string() + "\\..\\" + buildDatStr( "ex" + std::to_string( exNum ), cat_nb, exNum, chunkCount, "win32", "index" ) ) )
               {
                  m_exCats[cat_nb][exNum].push_back( std::unique_ptr<Cat>() );
                  m_log.info( "[GameData}  -> This chunk: " + buildDatStr( "ex" + std::to_string( exNum ), cat_nb, exNum, chunkCount, "win32", "index" ) );
                  chunkCount++;
               }

               m_log.info( "[GameData]  -> Found chunks: " + std::to_string( chunkCount ) );

               //m_exCats[cat_nb] = 
            }
         }
      }
   }

   m_log.info( "[GameData] GameData init at " + i_path.string() );
}
catch( std::exception& e )
{
   // In case of failure here, client is supposed to catch the exception because it is not recoverable on our side
   m_log.error( "[GameData] Initialization failed: " + std::string( e.what() ) );
   throw std::runtime_error( "GameData initialization failed: " + std::string( e.what() ) );
}

GameData::~GameData()
{

}

const std::string GameData::buildDatStr( const std::string folder, const int cat, const int exNum, const int chunk, const std::string platform, const std::string type )
{
   char dat[1024];
   sprintf_s( dat, "%s/%02x%02x%02x.%s.%s", folder.c_str(), cat, exNum, chunk, platform.c_str(), type.c_str() );
   return std::string( dat );
}

const std::vector<uint32_t>& GameData::getCatNumbers() const
{
    return m_catNums;
}

std::unique_ptr<File> GameData::getFile(const std::string& path)
{
   // Get the hashes, the category from the path then call the getFile of the category
   uint32_t dir_hash;
   uint32_t filename_hash;
   getHashes( path, dir_hash, filename_hash );

   return getCategoryFromPath( path ).getFile( dir_hash, filename_hash );
}

bool GameData::doesFileExist(const std::string& path)
{
   uint32_t dir_hash;
   uint32_t filename_hash;
   getHashes( path, dir_hash, filename_hash );

   return getCategoryFromPath( path ).doesFileExist( dir_hash, filename_hash );
}

bool GameData::doesDirExist(const std::string& i_path)
{
   uint32_t dir_hash;
   uint32_t filename_hash;
   getHashes( i_path, dir_hash, filename_hash );

   return getCategoryFromPath( i_path ).doesDirExist( dir_hash );
}

const Cat& GameData::getCategory(uint32_t catNum)
{
   // Check that the category number exists
   auto cat_it = m_cats.find( catNum );
   if( cat_it == m_cats.end() )
   {
      throw std::runtime_error( "Category not found: " + std::to_string( catNum ) );
   }

   // If it exists and already instantiated return it
   if( cat_it->second )
   {
      return *( cat_it->second );
   }
   else
   {
      // Else create it and return it
      createCategory( catNum );
      return *( m_cats[catNum] );
   }
}

const Cat& GameData::getCategory(const std::string& catName)
{
   // Find the category number from the name
   auto category_map_it = ::category_map.left.find( catName );
   if( category_map_it == ::category_map.left.end() )
   {
      throw std::runtime_error( "Category not found: " + catName );
   }

   // From the category number return the category
   return getCategory( category_map_it->second );
}

const uint32_t GameData::getExChunkAmount( uint32_t catNum, uint32_t exNum)
{
   // Check that the category number exists
   auto cat_it = m_exCats.find( catNum );
   if( cat_it == m_exCats.end() )
   {
      throw std::runtime_error( "Category not found: " + std::to_string( catNum ) );
   }

   return cat_it->second[exNum].size();
}

const Cat& GameData::getExCategory( uint32_t catNum, uint32_t exNum, uint32_t chunk )
{
   // Check that the category number exists
   auto cat_it = m_exCats.find( catNum );
   if( cat_it == m_exCats.end() )
   {
      throw std::runtime_error( "Category not found: " + std::to_string( catNum ) );
   }

   // If it exists and already instantiated return it
   if( cat_it->second[exNum][chunk] )
   {
      return *( cat_it->second[exNum][chunk] );
   }
   else
   {
      // Else create it and return it
      createExCategory( catNum );
      return *( m_exCats[catNum][exNum][chunk] );
   }
}

const Cat& GameData::getExCategory( const std::string& catName, uint32_t exNum, uint32_t chunk )
{
   // Find the category number from the name
   auto category_map_it = ::category_map.left.find( catName );
   if( category_map_it == ::category_map.left.end() )
   {
      throw std::runtime_error( "Category not found: " + catName );
   }

   // From the category number return the category
   return getExCategory( category_map_it->second, exNum, chunk );
}

const Cat& GameData::getCategoryFromPath(const std::string& path)
{
   // Find the first / in the string, paths are in the format CAT_NAME/..../.../../....
   auto first_slash_pos = path.find( '/' );
   if( first_slash_pos == std::string::npos )
   {
      throw std::runtime_error( "Path do not have a / char: " + path );
   }

   if( path.substr( first_slash_pos + 1, 2) == "ex" )
   {
      return getExCategory( path.substr( 0, first_slash_pos ), std::stoi( path.substr( first_slash_pos + 3, 1 ) ), 0 );
   }
   else
   {
      // From the sub string found beforethe first / get the category
      return getCategory( path.substr( 0, first_slash_pos ) );
   }
}

void GameData::getHashes(const std::string& path, uint32_t& dirHash, uint32_t& filenameHash) const
{
   // Convert the path to lowercase before getting the hashes
   std::string path_lower;
   path_lower.resize( path.size() );
   std::transform( path.begin(), path.end(), path_lower.begin(), ::tolower );

   // Find last / to separate dir from filename
   auto last_slash_pos = path_lower.rfind( '/' );
   if( last_slash_pos == std::string::npos )
   {
      throw std::runtime_error( "Path do not have a / char: " + path );
   }

   std::string dir_part = path_lower.substr( 0, last_slash_pos );
   std::string filename_part = path_lower.substr( last_slash_pos + 1 );

   // Get the crc32 values from zlib, to compensate the final XOR 0xFFFFFFFF that isnot done in the exe we just reXOR
   dirHash = crc32( 0, reinterpret_cast<const uint8_t*>( dir_part.data() ), dir_part.size() ) ^ 0xFFFFFFFF;
   filenameHash = crc32( 0, reinterpret_cast<const uint8_t*>( filename_part.data() ), filename_part.size() ) ^ 0xFFFFFFFF;
}

void GameData::createCategory(uint32_t catNum)
{
   // Lock mutex in this scope
   std::lock_guard<std::mutex> lock( *( m_catCreationMutexes[catNum] ) );
   // Maybe after unlocking it has already been created, so check (most likely if it blocked)
   if( !m_cats[catNum] )
   {
      // Get the category name if we have it
      std::string cat_name;
      auto category_map_it = ::category_map.right.find( catNum );
      if( category_map_it != ::category_map.right.end() )
      {
         cat_name = category_map_it->second;
      }

      // Actually creates the category
      m_cats[catNum] = std::unique_ptr<Cat>( new Cat( m_path, catNum, cat_name ) );
   }
}

void GameData::createExCategory( uint32_t catNum )
{
   // Maybe after unlocking it has already been created, so check (most likely if it blocked)
   if( !m_exCats[catNum][1][0] )
   {
      // Get the category name if we have it
      std::string cat_name;
      auto category_map_it = ::category_map.right.find( catNum );
      if( category_map_it != ::category_map.right.end() )
      {
         cat_name = category_map_it->second;
      }

      for( auto const& ex : m_exCats[catNum])
      {
         m_log.info( "[GameData] Creating " + std::to_string( catNum ) + " DAT for ex" + std::to_string(ex.first) + " with " + std::to_string(getExChunkAmount( catNum, ex.first )) + " chunks" );
         for( int i = 0; i < getExChunkAmount( catNum, ex.first ); i++ )
         {
            // Actually creates the category
            m_exCats[catNum][ex.first][i] = std::unique_ptr<Cat>( new Cat( m_path, catNum, cat_name, ex.first, i ) );
         }
      }
   }
}

}
}
