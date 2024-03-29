// File_Extractor 1.0.0. http://www.slack.net/~ant/

#include "Data_Reader.h"

#include "blargg_endian.h"
#include <stdio.h>
#include <errno.h>
#include <algorithm>

/* Copyright (C) 2005-2009 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

#ifdef BUILD_KODI_ADDON
#include <kodi/Filesystem.h>
#endif

// Data_Reader

blargg_err_t Data_Reader::read( void* p, int n )
{
	assert( n >= 0 );
	
	if ( n < 0 )
		return blargg_err_caller;
	
	if ( n <= 0 )
		return blargg_ok;
	
	if ( n > remain() )
		return blargg_err_file_eof;
	
	blargg_err_t err = read_v( p, n );
	if ( !err )
		remain_ -= n;
	
	return err;
}

blargg_err_t Data_Reader::read_avail( void* p, int* n_ )
{
	assert( *n_ >= 0 );
	
	int n = std::min( (BOOST::uint64_t)(*n_), remain() );
	*n_ = 0;
	
	if ( n < 0 )
		return blargg_err_caller;
	
	if ( n <= 0 )
		return blargg_ok;
	
	blargg_err_t err = read_v( p, n );
	if ( !err )
	{
		remain_ -= n;
		*n_ = n;
	}
	
	return err;
}

blargg_err_t Data_Reader::read_avail( void* p, long* n )
{
	int i = STATIC_CAST(int, *n);
	blargg_err_t err = read_avail( p, &i );
	*n = i;
	return err;
}

blargg_err_t Data_Reader::skip_v( int count )
{
	char buf [512];
	while ( count )
	{
		int n = std::min( count, (int) sizeof buf );
		count -= n;
		RETURN_ERR( read_v( buf, n ) );
	}
	return blargg_ok;
}

blargg_err_t Data_Reader::skip( int n )
{
	assert( n >= 0 );
	
	if ( n < 0 )
		return blargg_err_caller;
	
	if ( n <= 0 )
		return blargg_ok;
	
	if ( n > remain() )
		return blargg_err_file_eof;
	
	blargg_err_t err = skip_v( n );
	if ( !err )
		remain_ -= n;
	
	return err;
}


// File_Reader

blargg_err_t File_Reader::seek( BOOST::uint64_t n )
{
	assert( n >= 0 );
	
	if ( n < 0 )
		return blargg_err_caller;
	
	if ( n == tell() )
		return blargg_ok;
	
	if ( n > size() )
		return blargg_err_file_eof;
	
	blargg_err_t err = seek_v( n );
	if ( !err )
		set_tell( n );
	
	return err;
}

blargg_err_t File_Reader::skip_v( BOOST::uint64_t n )
{
	return seek_v( tell() + n );
}


// Subset_Reader

Subset_Reader::Subset_Reader( Data_Reader* dr, BOOST::uint64_t size ) :
	in( dr )
{
	set_remain( std::min( size, dr->remain() ) );
}

blargg_err_t Subset_Reader::read_v( void* p, int s )
{
	return in->read( p, s );
}


// Remaining_Reader

Remaining_Reader::Remaining_Reader( void const* h, int size, Data_Reader* r ) :
	in( r )
{
	header        = h;
	header_remain = size;
	
	set_remain( size + r->remain() );
}

blargg_err_t Remaining_Reader::read_v( void* out, int count )
{
	int first = std::min( count, header_remain );
	if ( first )
	{
		memcpy( out, header, first );
		header = STATIC_CAST(char const*, header) + first;
		header_remain -= first;
	}
	
	return in->read( STATIC_CAST(char*, out) + first, count - first );
}


// Mem_File_Reader

Mem_File_Reader::Mem_File_Reader( const void* p, long s ) :
	begin( STATIC_CAST(const char*, p) )
{
	set_size( s );
}

blargg_err_t Mem_File_Reader::read_v( void* p, int s )
{
	memcpy( p, begin + tell(), s );
	return blargg_ok;
}

blargg_err_t Mem_File_Reader::seek_v( int )
{
	return blargg_ok;
}


// Callback_Reader

Callback_Reader::Callback_Reader( callback_t c, BOOST::uint64_t s, void* d ) :
	callback( c ),
	user_data( d )
{
	set_remain( s );
}

blargg_err_t Callback_Reader::read_v( void* out, int count )
{
	return callback( user_data, out, count );
}


// Callback_File_Reader

Callback_File_Reader::Callback_File_Reader( callback_t c, BOOST::uint64_t s, void* d ) :
	callback( c ),
	user_data( d )
{
	set_size( s );
}

blargg_err_t Callback_File_Reader::read_v( void* out, int count )
{
	return callback( user_data, out, count, tell() );
}

blargg_err_t Callback_File_Reader::seek_v( int )
{
	return blargg_ok;
}

static const BOOST::uint8_t mask_tab[6]={0x80,0xE0,0xF0,0xF8,0xFC,0xFE};

static const BOOST::uint8_t val_tab[6]={0,0xC0,0xE0,0xF0,0xF8,0xFC};

size_t utf8_char_len_from_header( char p_c )
{
	BOOST::uint8_t c = (BOOST::uint8_t)p_c;

	size_t cnt = 0;
	for(;;)
	{
		if ( ( p_c & mask_tab[cnt] ) == val_tab[cnt] ) break;
		if ( ++cnt >= 6 ) return 0;
	}

	return cnt + 1;
}

size_t utf8_decode_char( const char *p_utf8, unsigned & wide, size_t mmax )
{
	const BOOST::uint8_t * utf8 = ( const BOOST::uint8_t* )p_utf8;
	
	if ( mmax == 0 )
	{
		wide = 0;
		return 0;
	}

	if ( utf8[0] < 0x80 )
	{
		wide = utf8[0];
		return utf8[0]>0 ? 1 : 0;
	}
	if ( mmax > 6 ) mmax = 6;
	wide = 0;

	unsigned res=0;
	unsigned n;
	unsigned cnt=0;
	for(;;)
	{
		if ( ( *utf8 & mask_tab[cnt] ) == val_tab[cnt] ) break;
		if ( ++cnt >= mmax ) return 0;
	}
	cnt++;

	if ( cnt==2 && !( *utf8 & 0x1E ) ) return 0;

	if ( cnt == 1 )
		res = *utf8;
	else
		res = ( 0xFF >> ( cnt + 1 ) ) & *utf8;

	for ( n = 1; n < cnt; n++ )
	{
		if ( ( utf8[n] & 0xC0 ) != 0x80 )
			return 0;
		if ( !res && n == 2 && !( ( utf8[n] & 0x7F ) >> ( 7 - cnt ) ) )
			return 0;

		res = ( res << 6 ) | ( utf8[n] & 0x3F );
	}

	wide = res;

	return cnt;
}

size_t utf8_encode_char( unsigned wide, char * target )
{
	size_t count;

	if ( wide < 0x80 )
		count = 1;
	else if ( wide < 0x800 )
		count = 2;
	else if ( wide < 0x10000 )
		count = 3;
	else if ( wide < 0x200000 )
		count = 4;
	else if ( wide < 0x4000000 )
		count = 5;
	else if ( wide <= 0x7FFFFFFF )
		count = 6;
	else
		return 0;

	if ( target == 0 )
		return count;

	switch ( count )
	{
    case 6:
		target[5] = 0x80 | ( wide & 0x3F );
		wide = wide >> 6;
		wide |= 0x4000000;
    case 5:
		target[4] = 0x80 | ( wide & 0x3F );
		wide = wide >> 6;
		wide |= 0x200000;
    case 4:
		target[3] = 0x80 | ( wide & 0x3F );
		wide = wide >> 6;
		wide |= 0x10000;
    case 3:
		target[2] = 0x80 | ( wide & 0x3F );
		wide = wide >> 6;
		wide |= 0x800;
    case 2:
		target[1] = 0x80 | ( wide & 0x3F );
		wide = wide >> 6;
		wide |= 0xC0;
	case 1:
		target[0] = wide;
	}

	return count;
}

size_t utf16_encode_char( unsigned cur_wchar, blargg_wchar_t * out )
{
	if ( cur_wchar < 0x10000 )
	{
        if ( out ) *out = (blargg_wchar_t) cur_wchar; return 1;
	}
	else if ( cur_wchar < ( 1 << 20 ) )
	{
		unsigned c = cur_wchar - 0x10000;
		//MSDN:
        //The first (high) surrogate is a 16-bit code value in the range U+D800 to U+DBFF. The second (low) surrogate is a 16-bit code value in the range U+DC00 to U+DFFF. Using surrogates, Unicode can support over one million characters. For more details about surrogates, refer to The Unicode Standard, version 2.0.
		if ( out )
		{
            out[0] = ( blargg_wchar_t )( 0xD800 | ( 0x3FF & ( c >> 10 ) ) );
            out[1] = ( blargg_wchar_t )( 0xDC00 | ( 0x3FF & c ) ) ;
		}
		return 2;
	}
	else
	{
		if ( out ) *out = '?'; return 1;
	}
}

size_t utf16_decode_char( const blargg_wchar_t * p_source, unsigned * p_out, size_t p_source_length )
{
	if ( p_source_length == 0 ) return 0;
	else if ( p_source_length == 1 )
	{
		*p_out = p_source[0];
		return 1;
	}
	else
	{
		size_t retval = 0;
		unsigned decoded = p_source[0];
		if ( decoded != 0 )
		{
			retval = 1;
			if ( ( decoded & 0xFC00 ) == 0xD800 )
			{
				unsigned low = p_source[1];
				if ( ( low & 0xFC00 ) == 0xDC00 )
				{
					decoded = 0x10000 + ( ( ( decoded & 0x3FF ) << 10 ) | ( low & 0x3FF ) );
					retval = 2;
				}
			}
		}
		*p_out = decoded;
		return retval;
	}
}

// Converts wide-character path to UTF-8. Free result with free(). Only supported on Windows.
char* blargg_to_utf8( const blargg_wchar_t* wpath )
{
	if ( wpath == NULL )
		return NULL;
	
	size_t needed = 0;
    size_t mmax = blargg_wcslen( wpath );
	if ( mmax <= 0 )
		return NULL;

	size_t ptr = 0;
	while ( ptr < mmax )
	{
		unsigned wide = 0;
		size_t char_len = utf16_decode_char( wpath + ptr, &wide, mmax - ptr );
		if ( !char_len ) break;
		ptr += char_len;
		needed += utf8_encode_char( wide, 0 );
	}
	if ( needed <= 0 )
		return NULL;
	
	char* path = (char*) calloc( needed + 1, 1 );
	if ( path == NULL )
		return NULL;

	ptr = 0;
	size_t actual = 0;
	while ( ptr < mmax && actual < needed )
	{
		unsigned wide = 0;
		size_t char_len = utf16_decode_char( wpath + ptr, &wide, mmax - ptr );
		if ( !char_len ) break;
		ptr += char_len;
		actual += utf8_encode_char( wide, path + actual );
	}

	if ( actual == 0 )
	{
		free( path );
		return NULL;
	}
	
	assert( actual == needed );
	return path;
}

// Converts UTF-8 path to wide-character. Free result with free() Only supported on Windows.
blargg_wchar_t* blargg_to_wide( const char* path )
{
	if ( path == NULL )
		return NULL;
	
	size_t mmax = strlen( path );
	if ( mmax <= 0 )
		return NULL;

	size_t needed = 0;
	size_t ptr = 0;
	while ( ptr < mmax )
	{
		unsigned wide = 0;
		size_t char_len = utf8_decode_char( path + ptr, wide, mmax - ptr );
		if ( !char_len ) break;
		ptr += char_len;
		needed += utf16_encode_char( wide, 0 );
	}
	if ( needed <= 0 )
		return NULL;
	
    blargg_wchar_t* wpath = (blargg_wchar_t*) calloc( needed + 1, sizeof *wpath );
	if ( wpath == NULL )
		return NULL;

	ptr = 0;
	size_t actual = 0;
	while ( ptr < mmax && actual < needed )
	{
		unsigned wide = 0;
		size_t char_len = utf8_decode_char( path + ptr, wide, mmax - ptr );
		if ( !char_len ) break;
		ptr += char_len;
		actual += utf16_encode_char( wide, wpath + actual );
	}
	if ( actual == 0 )
	{
		free( wpath );
		return NULL;
	}
	
	assert( actual == needed );
	return wpath;
}

#if defined(BUILD_KODI_ADDON)
static inline FILE* blargg_fopen(const char path[], const char mode[])
{
  if (!path)
    return nullptr;

  kodi::vfs::CFile* file = new kodi::vfs::CFile;
  if (!file->OpenFile(path, 0))
  {
    delete file;
    return nullptr;
  }

  return (FILE*)file;
}

#elif defined(_WIN32)

static FILE* blargg_fopen( const char path [], const char mode [] )
{
	FILE* file = NULL;
    blargg_wchar_t* wmode = NULL;
    blargg_wchar_t* wpath = NULL;
	
	wpath = blargg_to_wide( path );
	if ( wpath )
	{
		wmode = blargg_to_wide( mode );
		if ( wmode )
			file = _wfopen( wpath, wmode );
	}
	
	// Save and restore errno in case free() clears it
	int saved_errno = errno;
	free( wmode );
	free( wpath );
	errno = saved_errno;
	
	return file;
}

#else

static inline FILE* blargg_fopen( const char path [], const char mode [] )
{
	return fopen( path, mode );
}

#endif

#ifdef BUILD_KODI_ADDON
static inline void blargg_fclose(void* f)
{
  delete static_cast<kodi::vfs::CFile*>(f);
}

static int blargg_fread(void* p, int size, int num, void* file)
{
  if (!file)
    return -1;
  return static_cast<kodi::vfs::CFile*>(file)->Read(static_cast<uint8_t*>(p), size*num);
}

static int blargg_feof(void* f)
{
  if (!f)
    return -1;
  kodi::vfs::CFile* file = static_cast<kodi::vfs::CFile*>(f);
  return file->GetPosition() == file->GetLength();
}

#else
static inline void blargg_fclose(FILE* f)
{
  fclose(f);
}

static int blargg_fread(void* p, int size, int num, FILE* file_)
{
  return fread( p, size, num, STATIC_CAST(FILE*, file_) );
}

static int blargg_feof(FILE* f)
{
  return feof(f);
}
#endif

// Std_File_Reader

Std_File_Reader::Std_File_Reader()
{
	file_ = NULL;
}

Std_File_Reader::~Std_File_Reader()
{
	close();
}

static blargg_err_t blargg_fopen( FILE** out, const char path [] )
{
	errno = 0;
	*out = blargg_fopen( path, "rb" );
	if ( !*out )
	{
		#ifdef ENOENT
			if ( errno == ENOENT )
				return blargg_err_file_missing;
		#endif
		#ifdef ENOMEM
			if ( errno == ENOMEM )
				return blargg_err_memory;
		#endif
		return blargg_err_file_read;
	}
	
	return blargg_ok;
}

static blargg_err_t blargg_fsize( FILE* f, long* out )
{
#ifdef BUILD_KODI_ADDON
        *out = reinterpret_cast<kodi::vfs::CFile*>(f)->GetLength();
#else
	if ( fseek( f, 0, SEEK_END ) )
		return blargg_err_file_io;
	
	*out = ftell( f );
	if ( *out < 0 )
		return blargg_err_file_io;
	
	if ( fseek( f, 0, SEEK_SET ) )
		return blargg_err_file_io;
#endif
	return blargg_ok;
}

blargg_err_t Std_File_Reader::open( const char path [] )
{
	close();

	FILE* f = nullptr;
	RETURN_ERR( blargg_fopen( &f, path ) );
	
	long s;
	blargg_err_t err = blargg_fsize( f, &s );
	if ( err )
	{
		blargg_fclose( f );
		return err;
	}
	
	file_ = f;
	set_size( s );
	
	return blargg_ok;
}

void Std_File_Reader::make_unbuffered()
{
#ifndef BUILD_KODI_ADDON
	if ( setvbuf( STATIC_CAST(FILE*, file_), NULL, _IONBF, 0 ) )
		check( false ); // shouldn't fail, but OK if it does
#endif
}

blargg_err_t Std_File_Reader::read_v( void* p, int s )
{
	if ( (size_t) s != blargg_fread( p, 1, s, STATIC_CAST(FILE*, file_) ) )
	{
		// Data_Reader's wrapper should prevent EOF
		check( !blargg_feof( STATIC_CAST(FILE*, file_) ) );
		
		return blargg_err_file_io;
	}
	
	return blargg_ok;
}

blargg_err_t Std_File_Reader::seek_v( BOOST::uint64_t n )
{
#if defined(BUILD_KODI_ADDON)
        if (static_cast<kodi::vfs::CFile*>(file_)->Seek(n, SEEK_SET))
#elif defined(_WIN32)
	if ( _fseeki64( STATIC_CAST(FILE*, file_), n, SEEK_SET ) )
#else
    if ( fseeko( STATIC_CAST(FILE*, file_), n, SEEK_SET ) )
#endif
	{
		// Data_Reader's wrapper should prevent EOF
		check( !blargg_feof( STATIC_CAST(FILE*, file_) ) );
		
		return blargg_err_file_io;
	}
	
	return blargg_ok;
}

void Std_File_Reader::close()
{
	if ( file_ )
	{
		blargg_fclose( STATIC_CAST(FILE*, file_) );
		file_ = NULL;
	}
}
