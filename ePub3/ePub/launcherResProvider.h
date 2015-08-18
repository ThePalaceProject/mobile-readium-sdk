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

#ifndef _LAUNCHERRESPROVIDER_H
#define _LAUNCHERRESPROVIDER_H

#include <dp_res.h>
#include <dp_io.h>
#include <dp_core.h>

class LauncherResProvider : public dpres::ResourceProvider 
{
public:
	LauncherResProvider( dp::String resFolder, bool verbose );
    //LauncherResProvider();

	virtual ~LauncherResProvider();

	/**
	 *  Request a global resource download from a given url with a Stream with at least
	 *  given capabilities. Security considerations are responsibilities of the host.
	 *  If NULL is returned, request is considered to be failed.
	 */
	virtual dpio::Stream * getResourceStream( const dp::String& urlin, unsigned int capabilities );

private:
	dp::String m_resFolder;
	bool m_verbose;
};

#endif // _LAUNCHERRESPROVIDER_H

#endif //#if defined(FEATURE_DRM_CONNECTOR)

