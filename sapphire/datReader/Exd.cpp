#include "Exd.h"

#include <boost/bind.hpp>
#include <boost/io/ios_state.hpp>

#include "bparse.h"
#include "stream.h"

#include "Exh.h"

using xiv::utils::bparse::extract;


namespace xiv 
{
   namespace exd 
   {

      struct ExdHeader
      {
         char magic[0x4];
         uint16_t unknown;
         uint16_t unknown2;
         uint32_t index_size;
      };

      struct ExdRecordIndex
      {
         uint32_t id;  
         uint32_t offset;
      };
  
   }
}   

namespace xiv
{
   namespace utils
   {
      namespace bparse
      {
         template <> inline void reorder<xiv::exd::ExdHeader>( xiv::exd::ExdHeader& i_struct ) { for( int i = 0; i < 0x4; ++i ) { xiv::utils::bparse::reorder( i_struct.magic[i] ); }  i_struct.unknown = xiv::utils::bparse::byteswap( i_struct.unknown ); xiv::utils::bparse::reorder( i_struct.unknown );  i_struct.unknown2 = xiv::utils::bparse::byteswap( i_struct.unknown2 ); xiv::utils::bparse::reorder( i_struct.unknown2 );  i_struct.index_size = xiv::utils::bparse::byteswap( i_struct.index_size ); xiv::utils::bparse::reorder( i_struct.index_size ); }
         template <> inline void reorder<xiv::exd::ExdRecordIndex>( xiv::exd::ExdRecordIndex& i_struct ) { i_struct.id = xiv::utils::bparse::byteswap( i_struct.id ); xiv::utils::bparse::reorder( i_struct.id );  i_struct.offset = xiv::utils::bparse::byteswap( i_struct.offset ); xiv::utils::bparse::reorder( i_struct.offset ); }
      }
   }
};

namespace xiv
{
   namespace exd
   {

      Exd::Exd( std::shared_ptr<Exh> i_exh, const std::vector<std::shared_ptr<dat::File>>& i_files )
      {
         _exh = i_exh;
         _files = i_files;


      }

      Exd::~Exd()
      {
      }

      const std::vector<Field>& Exd::get_row( uint32_t id )
      {

         // Iterates over all the files
         const uint32_t member_count = _exh->get_members().size();
         for( auto& file_ptr : _files )
         {
            // Get a stream
            auto stream_ptr = utils::stream::get_istream( file_ptr->get_data_sections().front() );
            auto& stream = *stream_ptr;

            // Extract the header and skip to the record indices
            auto exd_header = extract<ExdHeader>( stream );
            stream.seekg( 0x20 );

            // Preallocate and extract the record_indices
            const uint32_t record_count = exd_header.index_size / sizeof( ExdRecordIndex );
            std::vector<ExdRecordIndex> record_indices;
            record_indices.reserve( record_count );
            for( uint32_t i = 0; i < record_count; ++i )
            {
               record_indices.emplace_back( extract<ExdRecordIndex>( stream ) );
            }

            for( auto& record_index : record_indices )
            {
               if( record_index.id != id )
                  continue;
               // Get the vector fields for the given record and preallocate it
               auto& fields = _data[record_index.id];
               fields.reserve( member_count );

               for( auto& member_entry : _exh->get_exh_members() )
               {
                  // Seek to the position of the member to extract.
                  // 6 is because we have uint32_t/uint16_t at the start of each record
                  stream.seekg( record_index.offset + 6 + member_entry.offset );

                  // Switch depending on the type to extract
                  switch( member_entry.type )
                  {
                  case DataType::string:
                     // Extract the offset to the actual string
                     // Seek to it then extract the actual string
                  {
                     auto string_offset = extract<uint32_t>( stream, "string_offset", false );
                     stream.seekg( record_index.offset + 6 + _exh->get_header().data_offset + string_offset );
                     fields.emplace_back( utils::bparse::extract_cstring( stream, "string" ) );
                  }
                  break;

                  case DataType::boolean:
                     fields.emplace_back( extract<bool>( stream, "bool" ) );
                     break;

                  case DataType::int8:
                     fields.emplace_back( extract<int8_t>( stream, "int8_t" ) );
                     break;

                  case DataType::uint8:
                     fields.emplace_back( extract<uint8_t>( stream, "uint8_t" ) );
                     break;

                  case DataType::int16:
                     fields.emplace_back( extract<int16_t>( stream, "int16_t", false ) );
                     break;

                  case DataType::uint16:
                     fields.emplace_back( extract<uint16_t>( stream, "uint16_t", false ) );
                     break;

                  case DataType::int32:
                     fields.emplace_back( extract<int32_t>( stream, "int32_t", false ) );
                     break;

                  case DataType::uint32:
                     fields.emplace_back( extract<uint32_t>( stream, "uint32_t", false ) );
                     break;

                  case DataType::float32:
                     fields.emplace_back( extract<float>( stream, "float", false ) );
                     break;

                  case DataType::uint64:
                     fields.emplace_back( extract<uint64_t>( stream, "uint64_t", false ) );
                     break;

                  default:
                     //throw std::runtime_error("Unknown DataType: " + std::to_string(static_cast<uint16_t>(member_entry.second.type)));
                     fields.emplace_back( extract<bool>( stream, "bool" ) );
                     break;
                  }
               }
               return fields;
            }
         }

         auto row_it = _data.find( id );
         if( row_it == _data.end() )
         {
            throw std::runtime_error( "Id not found: " + std::to_string( id ) );
         }

         return row_it->second;
      }

      // Get all rows
      const std::map<uint32_t, std::vector<Field>>& Exd::get_rows()
      {
         // Iterates over all the files
         const uint32_t member_count = _exh->get_members().size();
         for( auto& file_ptr : _files )
         {
            // Get a stream
            auto stream_ptr = utils::stream::get_istream( file_ptr->get_data_sections().front() );
            auto& stream = *stream_ptr;

            // Extract the header and skip to the record indices
            auto exd_header = extract<ExdHeader>( stream );
            stream.seekg( 0x20 );

            // Preallocate and extract the record_indices
            const uint32_t record_count = exd_header.index_size / sizeof( ExdRecordIndex );
            std::vector<ExdRecordIndex> record_indices;
            record_indices.reserve( record_count );
            for( uint32_t i = 0; i < record_count; ++i )
            {
               record_indices.emplace_back( extract<ExdRecordIndex>( stream ) );
            }

            for( auto& record_index : record_indices )
            {
               // Get the vector fields for the given record and preallocate it
               auto& fields = _data[record_index.id];
               fields.reserve( member_count );

               for( auto& member_entry : _exh->get_exh_members() )
               //for( auto& member_entry : _exh->get_members() )
               {
                  // Seek to the position of the member to extract.
                  // 6 is because we have uint32_t/uint16_t at the start of each record
                  stream.seekg( record_index.offset + 6 + member_entry.offset );

                  // Switch depending on the type to extract
                  switch( member_entry.type )
                  {
                  case DataType::string:
                     // Extract the offset to the actual string
                     // Seek to it then extract the actual string
                  {
                     auto string_offset = extract<uint32_t>( stream, "string_offset", false );
                     stream.seekg( record_index.offset + 6 + _exh->get_header().data_offset + string_offset );
                     fields.emplace_back( utils::bparse::extract_cstring( stream, "string" ) );
                  }
                  break;

                  case DataType::boolean:
                     fields.emplace_back( extract<bool>( stream, "bool" ) );
                     break;

                  case DataType::int8:
                     fields.emplace_back( extract<int8_t>( stream, "int8_t" ) );
                     break;

                  case DataType::uint8:
                     fields.emplace_back( extract<uint8_t>( stream, "uint8_t" ) );
                     break;

                  case DataType::int16:
                     fields.emplace_back( extract<int16_t>( stream, "int16_t", false ) );
                     break;

                  case DataType::uint16:
                     fields.emplace_back( extract<uint16_t>( stream, "uint16_t", false ) );
                     break;

                  case DataType::int32:
                     fields.emplace_back( extract<int32_t>( stream, "int32_t", false ) );
                     break;

                  case DataType::uint32:
                     fields.emplace_back( extract<uint32_t>( stream, "uint32_t", false ) );
                     break;

                  case DataType::float32:
                     fields.emplace_back( extract<float>( stream, "float", false ) );
                     break;

                  case DataType::uint64:
                     fields.emplace_back( extract<uint64_t>( stream, "uint64_t", false ) );
                     break;

                  default:
                     //throw std::runtime_error("Unknown DataType: " + std::to_string(static_cast<uint16_t>(member_entry.second.type)));
                     fields.emplace_back( extract<bool>( stream, "bool" ) );
                     break;
                  }
               }
            }
         }
         return _data;
      }

      class output_field : public boost::static_visitor<>
      {
      public:
         template <typename T>
         void operator()( T operand, std::ostream& o_stream, char i_delimiter ) const
         {
            o_stream << i_delimiter << operand;
         }

         void operator()( int8_t operand, std::ostream& o_stream, char i_delimiter ) const
         {
            o_stream << i_delimiter << static_cast< int16_t >( operand );
         }

         void operator()( uint8_t operand, std::ostream& o_stream, char i_delimiter ) const
         {
            o_stream << i_delimiter << static_cast< uint16_t >( operand );
         }

         void operator()( const std::string& operand, std::ostream& o_stream, char i_delimiter ) const
         {
            o_stream << i_delimiter;
            for( char c : operand )
            {
               if( isprint( static_cast< int >( static_cast< uint8_t >( c ) ) ) )
               {
                  o_stream << c;
               }
               else
               {
                  // This saves the flags of the stream for a given scope, to be sure that manipulators are reset after the return
                  boost::io::ios_all_saver ias( o_stream );
                  o_stream << "\\x" << std::setw( 2 ) << std::setfill( '0' ) << std::hex << static_cast< uint16_t >( static_cast< uint8_t >( c ) );
               }
            }
         }
      };

      // Get as csv
      void Exd::get_as_csv( std::ostream& o_stream ) const
      {
         // tab delimited csv to avoid problems with commas in strings
         char delimiter = '\t';

         auto visitor = boost::bind( output_field(), _1, boost::ref( o_stream ), delimiter );

         for( auto& row_entry : _data )
         {
            o_stream << row_entry.first;

            for( auto& field : row_entry.second )
            {
               boost::apply_visitor( visitor, field );
            }

            o_stream << '\n';
         }
      }

   }
}

