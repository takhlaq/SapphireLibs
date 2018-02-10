#include "Index.h"

#include "bparse.h"

namespace xiv 
{
   namespace dat 
   {
 
      struct IndexBlockRecord
      {
         uint32_t offset; 
         uint32_t size; 
         SqPackBlockHash block_hash;
      };

      struct IndexHashTableEntry
      {
         uint32_t filename_hash;
         uint32_t dir_hash;
         uint32_t dat_offset;
         uint32_t padding;
      };

   }
}

namespace xiv 
{
   namespace utils
   {
      namespace bparse 
      {
         template <> inline void reorder<xiv::dat::IndexBlockRecord>( xiv::dat::IndexBlockRecord& i_struct ) 
         { 
            xiv::utils::bparse::reorder( i_struct.offset );
            xiv::utils::bparse::reorder( i_struct.size );
            xiv::utils::bparse::reorder( i_struct.block_hash );
         }

         template <> inline void reorder<xiv::dat::IndexHashTableEntry>( xiv::dat::IndexHashTableEntry& i_struct ) 
         { 
            xiv::utils::bparse::reorder( i_struct.filename_hash );
            xiv::utils::bparse::reorder( i_struct.dir_hash ); 
            xiv::utils::bparse::reorder( i_struct.dat_offset ); 
            xiv::utils::bparse::reorder( i_struct.padding ); 
         }
      }
   }
};

using xiv::utils::bparse::extract;

namespace xiv
{
   namespace dat
   {

      Index::Index( const boost::filesystem::path& i_path ) :
         SqPack( i_path )
      {
         if( !_handle )
            throw new std::runtime_error( "Failed to load Index at " + i_path.string() );

         // Hash Table record
         auto hash_table_block_record = extract<IndexBlockRecord>( _handle );
         is_index_block_valid( hash_table_block_record );

         // Save the posin the stream to go back to it later on
         auto pos = _handle.tellg();

         // Seek to the pos of the hash table in the file
         _handle.seekg( hash_table_block_record.offset );

         // Preallocate and extract the index_hash_table_entries
         std::vector<IndexHashTableEntry> index_hash_table_entries;
         extract<IndexHashTableEntry>( _handle, hash_table_block_record.size / sizeof( IndexHashTableEntry ), index_hash_table_entries );

         // Feed the correct entry in the HashTable for each index_hash_table_entry
         for( auto& index_hash_table_entry : index_hash_table_entries )
         {
            auto& hash_table_entry = _hash_table[index_hash_table_entry.dir_hash][index_hash_table_entry.filename_hash];
            // The dat number is found in the offset, last four bits
            hash_table_entry.dat_nb = ( index_hash_table_entry.dat_offset & 0xF ) / 0x2;
            // The offset in the dat file, needs to strip the dat number indicator
            hash_table_entry.dat_offset = ( index_hash_table_entry.dat_offset & 0xFFFFFFF0 ) * 0x08;
            hash_table_entry.dir_hash = index_hash_table_entry.dir_hash;
            hash_table_entry.filename_hash = index_hash_table_entry.filename_hash;
         }

         // Come back to where we were before reading the HashTable
         _handle.seekg( pos );

         // Dat Count
         _dat_count = extract<uint32_t>( _handle, "dat_count" );

         // Free List
         is_index_block_valid( extract<IndexBlockRecord>( _handle ) );

         // Dir Hash Table
         is_index_block_valid( extract<IndexBlockRecord>( _handle ) );
      }

      Index::~Index()
      {
      }

      uint32_t Index::get_dat_count() const
      {
         return _dat_count;
      }

      const Index::HashTable& Index::get_hash_table() const
      {
         return _hash_table;
      }

      bool Index::check_file_existence( uint32_t dir_hash, uint32_t filename_hash ) const
      {
         auto dir_it = get_hash_table().find( dir_hash );
         if( dir_it != get_hash_table().end() )
         {
            return ( dir_it->second.find( filename_hash ) != dir_it->second.end() );
         }
         return false;
      }
      bool Index::check_dir_existence( uint32_t dir_hash ) const
      {
         return ( get_hash_table().find( dir_hash ) != get_hash_table().end() );
      }

      const Index::DirHashTable& Index::get_dir_hash_table( uint32_t dir_hash ) const
      {
         auto dir_it = get_hash_table().find( dir_hash );
         if( dir_it == get_hash_table().end() )
         {
            throw std::runtime_error( "dir_hash not found" );
         }
         else
         {
            return dir_it->second;
         }
      }
      const Index::HashTableEntry& Index::get_hash_table_entry( uint32_t dir_hash, uint32_t filename_hash ) const
      {
         auto& dir_hash_table = get_dir_hash_table( dir_hash );
         auto file_it = dir_hash_table.find( filename_hash );
         if( file_it == dir_hash_table.end() )
         {
            throw std::runtime_error( "filename_hash not found" );
         }
         else
         {
            return file_it->second;
         }
      }

      void Index::is_index_block_valid( const IndexBlockRecord& i_index_block_record )
      {
         is_block_valid( i_index_block_record.offset, i_index_block_record.size, i_index_block_record.block_hash );
      }

   }
}
