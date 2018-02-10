#include "DatCat.h"

#include "Index.h"
#include "Dat.h"
#include "File.h"
#include "GameData.h"

namespace xiv
{
namespace dat
{

Cat::Cat(const boost::filesystem::path& i_base_path, uint32_t i_cat_nb, const std::string& i_name) :
    m_name(i_name),
    m_catNum(i_cat_nb),
    m_chunk( -1 )
{
    // From the category number, compute back the real filename for.index .datXs
    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::hex << i_cat_nb;
    std::string prefix = ss.str() + "0000.win32";

    // Creates the index: XX0000.win32.index
    m_index = std::unique_ptr<Index>(new Index(i_base_path / (prefix + ".index")));

    // For all dat files linked to this index, create it: XX0000.win32.datX
    for (uint32_t i = 0; i < getIndex().get_dat_count(); ++i)
    {
        m_dats.emplace_back(std::unique_ptr<Dat>(new Dat(i_base_path / (prefix + ".dat" + std::to_string(i)), i)));
    }
}

Cat::Cat( const boost::filesystem::path& i_base_path, uint32_t i_cat_nb, const std::string& i_name, uint32_t exNum, uint32_t chunk ) :
   m_name( i_name ),
   m_catNum( i_cat_nb ),
   m_chunk( chunk )
{

   // TODO: Don't use the .. crap, check what crashes the watchdog
   // Creates the index: XX0000.win32.index
   m_index = std::unique_ptr<Index>( new Index( i_base_path / GameData::buildDatStr( "../ex" + std::to_string( exNum ), i_cat_nb, exNum, chunk, "win32", "index" ) ) );

   // For all dat files linked to this index, create it: XX0000.win32.datX
   for( uint32_t i = 0; i < getIndex().get_dat_count(); ++i )
   {
      m_dats.emplace_back( std::unique_ptr<Dat>( new Dat( i_base_path / GameData::buildDatStr( "../ex" + std::to_string( exNum ), i_cat_nb, exNum, chunk, "win32", "dat" + std::to_string( i ) ), i ) ) );
   }
}

Cat::~Cat()
{

}

const Index& Cat::getIndex() const
{
    return *m_index;
}

std::unique_ptr<File> Cat::getFile(uint32_t dir_hash, uint32_t filename_hash) const
{
    // Fetch the correct hash_table_entry for these hashes, from that request the file from the right dat file
    auto& hash_table_entry = getIndex().get_hash_table_entry(dir_hash, filename_hash);
    return m_dats[hash_table_entry.dat_nb]->get_file(hash_table_entry.dat_offset);
}

bool Cat::doesFileExist(uint32_t dir_hash, uint32_t filename_hash) const
{
    return getIndex().check_file_existence(dir_hash, filename_hash);
}
bool Cat::doesDirExist(uint32_t dir_hash) const
{
    return getIndex().check_dir_existence(dir_hash);
}

const std::string& Cat::getName() const
{
    return m_name;
}

uint32_t Cat::getCatNum() const
{
    return m_catNum;
}

}
}
