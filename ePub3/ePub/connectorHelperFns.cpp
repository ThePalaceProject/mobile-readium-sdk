/*************************************************************************
 *
 * ADOBE CONFIDENTIAL
 * ___________________
 *
 *  Copyright 2015 Adobe Systems Incorporated
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Adobe Systems Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Adobe Systems Incorporated and its
 * suppliers and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Adobe Systems Incorporated.
 **************************************************************************/

#if defined(FEATURE_DRM_CONNECTOR)
#include "connectorHelperFns.h"

#include "dp_all.h"

#include <stdio.h>
#include <unistd.h>
#include <assert.h>

bool connectorutils::g_verbose = true;
FILE* connectorutils::g_msgout = stderr;

void connectorutils::b2plog_Log( b2plog_loglevel level, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    switch(level)
    {
        case B2PLOG_LOGOUT:
            vprintf(fmt, ap);
            break;
        case B2PLOG_LOGERROR:
            vfprintf(stderr, fmt, ap);
            break;
        case B2PLOG_LOGMESSAGE:
            vfprintf(connectorutils::g_msgout, fmt, ap);
            break;
    }
    va_end(ap);
}

void connectorutils::myExit( int exitCode )
{
#if defined(USES_COCOA)
	//g_exitCode = exitCode;
#else
	//::exit( exitCode );
#endif
}



// URL-encode UTF8 filename, making it absolute if needed, adding file:/// and replacing \ to /
dp::String connectorutils::urlEncodeFileName( const char * str )
{
	// filename
	char path[512] = "/";
	size_t pathSize = sizeof( path );
	if( str[0] == '/' ||
       (str[0] != '\0' && str[1] == ':' && (str[2] == '\\' || str[2] == '/')) ||
       (str[0] == '\\' || str[1] == '\\') )
	{
		// abs path already
	}
	else
	{
		// relative path; non-Unicode on Windows for now
#ifdef WINCE
		::strcpy( path, "\\My Documents" );
#else
		::getcwd( path, pathSize );
        
		/* Remove any occurances of ".." at front of string and remove a corresponding
         element from the path */
		path[pathSize -1] = '\0';
		size_t pathLength = strlen( path );
		char tempPath[sizeof(path)];
		memcpy( tempPath, path, pathLength + 1 );
		while ( str[0] == '.' && str[1] == '.' &&
               ( str[2] == '\\' || str[2] == '/' ) &&
               pathLength > 0 )
		{
			if ( path[pathLength - 1] == ':' )
			{
				pathLength = 0;
				break;
			}
			str += 3;
			while ( path[pathLength - 1] != '\\' && path[pathLength - 1] != '/' )
			{
				if ( --pathLength == 0 )
					break;
				path[pathLength] = '\0';
			}
			if ( pathLength != 0 )
			{
				if ( --pathLength == 0 )
					break;
				path[pathLength] = '\0';
			}
		}
		if ( pathLength == 0 )
        /* Not enough elements in path for the number of occurances
         of "..". Just restore path. */
			memcpy( path, tempPath, strlen( tempPath ) + 1 );
        
		if( ::strcmp( str, "." ) != 0 )
		{
			size_t remaining = pathSize - strlen( path );
			::strncat( path, "/", remaining );
			remaining -=1 ;
			::strncat( path, str, remaining );
		}
		str = path;
#endif
	}
    
	size_t len = 0;
	const char * p = str;
	while( true )
	{
		char c = *(p++);
		if( c == '\0' )
			break;
		len++;
		if( c < ' ' || (unsigned char)c > '~' || c == '%' || c == '+' )
			len += 2;
	}
    
	bool winShare = str[0] == '\\' && str[1] == '\\';
	bool unixAbs = !winShare && (str[0] == '/' || str[1] == '\\');
	char * url = new char[len+(winShare?6:(unixAbs?8:9))];
	if( winShare )
		::strcpy( url, "file:" );
	else if( unixAbs )
		::strcpy( url, "file://" );
	else
		::strcpy( url, "file:///" );
    
	char * t = url + ::strlen(url);
	p = str;
	while( true )
	{
		char c = *(p++);
		if( c == '\0' )
			break;
		if( c < ' ' || (unsigned char)c > '~' || c == '%' || c == '+' )
		{
			sprintf( t, "%%%02X", (unsigned char)c );
			t += 3;
		}
//		else if( c == ' ' )
//		{
//			*(t++) = '+';
//		}
		else if( c == '\\' )
		{
			*(t++) = '/';
		}
		else
		{
			*(t++) = c;
		}
	}
	*(t++) = '\0';
	dp::String result(url);
	delete[] url;
	return result;
}

dp::Data connectorutils::readFile( const char * file , bool bShouldEncodeURL)
{
    dp::String url = (bShouldEncodeURL)?connectorutils::urlEncodeFileName( file ): file;
	if( connectorutils::g_verbose )
		connectorutils::b2plog_Log( B2PLOG_LOGOUT,  "Reading %s\n", url.utf8() );
	dpio::Partition * partition = dpio::Partition::findPartitionForURL( url );
	if( partition )
	{
		dpio::Stream * stream = partition->readFile( url, NULL, 0 );
		if( stream )
		{
			dp::Data data = dpio::Stream::readSynchronousStream(stream);
			if( !data.isNull() )
				connectorutils::b2plog_Log( B2PLOG_LOGOUT,  "Got %d bytes\n", data.length() );
			return data;
		}
	}
	return dp::Data();
}

dp::Data connectorutils::readURL( const char * url )
{
	if( connectorutils::g_verbose )
		connectorutils::b2plog_Log( B2PLOG_LOGOUT,  "Reading %s\n", url );
	dpio::Stream * stream = dpnet::NetProvider::getProvider()->open( "GET", url, NULL, 0, NULL );
	if( stream )
	{
		dp::Data data = dpio::Stream::readSynchronousStream(stream);
		if( !data.isNull() )
			connectorutils::b2plog_Log( B2PLOG_LOGOUT,  "Got %d bytes\n", data.length() );
		return data;
	}
	return dp::Data();
}
void connectorutils::writeFile( const char * file, dp::Data &data, bool bShouldEncodeURL/* = true*/ )
{
    dp::String url = (bShouldEncodeURL)?urlEncodeFileName( file ):file;
    if( g_verbose )
        b2plog_Log( B2PLOG_LOGOUT,  "Reading %s\n", url.utf8() );
    dpio::Partition * partition = dpio::Partition::findPartitionForURL( url );
    if( partition )
    {
        dpio::Stream * stream = dpio::Stream::createDataStream("application/binary", data, NULL, NULL);
        if(!stream)
        {
            b2plog_Log( B2PLOG_LOGOUT,  "Failed to create data stream: %s\n", url.utf8() );
        }
        partition->writeFile(url, stream, NULL);
    }
}

bool connectorutils::FileHasExtention(const std::string& file, const std::string& extn)
{
    bool isFileExtnFound = true;
    
//    if(file.length() < extn.length())
//        return false;
//    
//    for (size_t i = file.length()-extn.length(), j = 0; i < file.length(); ++i, ++j)
//    {
//        if (tolower(file[i]) != extn[j])
//        {
//            isFileExtnFound = false;
//            break;
//        }
//    }
    return isFileExtnFound;
}

// ---
static
const size_t prefix_len = strlen(URL_PREFIX);

bool hasURLPrefix(const char* string)
{
	assert(string != NULL);
	if (string == NULL) {
		return false;
	}
	return strlen(string) >= prefix_len && strncmp(string,URL_PREFIX,prefix_len) == 0;
}


dp::String getFileURL(const char* nativePath)
{
	//static const size_t prefix_len = strlen(URL_PREFIX);
	if(hasURLPrefix(nativePath))
		return dp::String(nativePath);
	
	assert(nativePath != NULL);
	if (nativePath == NULL) {
		return dp::String(); //return null string
	}
    size_t path_len = strlen(nativePath);
	size_t url_len = prefix_len + path_len + 1;
	
	char* buffer = new char[url_len];
	memset(buffer, 0, url_len);
	strncpy(buffer, URL_PREFIX, prefix_len);
	strncpy(buffer+prefix_len, nativePath, path_len);
	for (size_t i = 0; i<url_len; i++) {
		if(buffer[i] == '\\') {
			buffer[i] = '/';
		}
	}
	dp::String string = dp::String::urlEncode(buffer);
	delete [] buffer;
	return string;
}

dp::String getFilePath(const char* url)
{
	if(!hasURLPrefix(url))
		return dp::String(url);
	//static const size_t prefix_len = strlen(URL_PREFIX);
	assert(url != NULL);
	if (url == NULL) {
		return dp::String();//return null string
	}
	size_t url_len = strlen(url);
	size_t path_len = url_len - prefix_len + 1;
	
	char* buffer = new char[path_len];
	///FIXME: no check for null pointer returned
	memset(buffer, 0, path_len);
	strncpy(buffer, url+prefix_len, path_len);
	dp::String path = dp::String::urlDecode(buffer);
	delete buffer;
	return path;
}

#endif //#if defined(FEATURE_DRM_CONNECTOR)