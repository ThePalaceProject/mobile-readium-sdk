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

#include <stdio.h>
#include <string.h>

#include <dp_res.h>
#include <dp_io.h>
#include <dp_core.h>

#include "launcherResProvider.h"

#ifdef EPUB_PLATFORM_IOS
//#include <Foundation/Foundation.h>
#endif
#ifdef ANDROID_NDK
extern int getAssetBytes(const char *assetName, unsigned char **buffer);
#endif

LauncherResProvider::LauncherResProvider( dp::String resFolder, bool verbose )
	: m_resFolder(resFolder), m_verbose(verbose)
{
}

LauncherResProvider::~LauncherResProvider()
{
}

dpio::Stream * LauncherResProvider::getResourceStream( const dp::String& urlin, unsigned int capabilities )
{
	dp::String url = urlin;
	//if( m_verbose )
	//	b2plog_Log( B2PLOG_LOGOUT, "Loading %s\n", url.utf8() );
	if( ::strncmp( url.utf8(), "data:", 5 ) == 0 )
		return dpio::Stream::createDataURLStream( url, NULL, NULL );

	// resources: user stylesheet, fonts, hyphenation dictionaries and resources they references
	if( ::strncmp( url.utf8(), "res:///", 7 ) == 0 && url.length() < 1024 && !m_resFolder.isNull() )
	{
		char tmp[2048];
		::strcpy( tmp, m_resFolder.utf8() );
		::strcat( tmp, url.utf8()+7 );
		url = dp::String( tmp );
	}
#ifdef ANDROID_NDK
	if( ::strncmp( url.utf8(), "file:///", 8 ) != 0 ) { 
		unsigned char *bytes = NULL;
		int blen = getAssetBytes( url.utf8() , &bytes);
		if( bytes != NULL && blen != -1)
			return dpio::Stream::createDataStream( "application/octet-stream", dp::Data(bytes, blen), NULL, NULL );
		else
			b2plog_Log( B2PLOG_LOGERROR, "No stream returned for asset '%s'\n", url.utf8());
	} else {
#endif
    dpio::Stream * res = NULL;
	dpio::Partition * partition = dpio::Partition::findPartitionForURL( url );
	if( partition != NULL )
    {
		res = partition->readFile( url, NULL, capabilities );
        //if (!res)
        //    b2plog_Log( B2PLOG_LOGERROR, "No file stream returned for asset '%s'\n", url.utf8() );
        return res;
    }
    else
    {
        //b2plog_Log( B2PLOG_LOGERROR, "No partition found for asset '%s'\n", url.utf8() );
    }
#ifdef ANDROID_NDK
	}
#endif
	return NULL;
}

#endif //#if defined(FEATURE_DRM_CONNECTOR)

