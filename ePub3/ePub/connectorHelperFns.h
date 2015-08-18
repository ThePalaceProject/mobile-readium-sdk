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

#ifndef __CONNECTORHELPERFNS_H__
#define __CONNECTORHELPERFNS_H__

#include "dp_all.h"

#define CONV_INT_TO_STR_X_(a) #a
#define CONV_INT_TO_STR(a) CONV_INT_TO_STR_X_(a)

namespace connectorutils {
    
#if 1
enum b2plog_loglevel {
    B2PLOG_LOGOUT = 1,
    B2PLOG_LOGERROR = 2,
    B2PLOG_LOGMESSAGE = 3,
};
#endif

extern bool g_verbose;
extern FILE* g_msgout;

void b2plog_Log( b2plog_loglevel level, const char* fmt, ...);
void myExit( int exitCode );

dp::String urlEncodeFileName( const char * str );
dp::Data readFile( const char * file,bool bShouldEncodeURL );
dp::Data readURL( const char * url );
void writeFile( const char * file, dp::Data &data, bool bShouldEncodeURL/* = true*/ );

bool FileHasExtention(const std::string& file, const std::string& extn);

// ---
#define URL_PREFIX "file://"
extern const size_t prefix_len;

bool hasURLPrefix(const char* string);
dp::String getFileURL(const char* nativePath);
dp::String getFilePath(const char* url);
#ifdef EPUB_PLATFORM_IOS
dp::String getResourceURL();
dp::String getDocumentsDirectory();
dp::String uniqueFileName(dp::String fileName);
#endif
}

#endif // __CONNECTORHELPERFNS_H__
#endif
