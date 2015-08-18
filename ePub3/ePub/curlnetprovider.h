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
 
#ifndef _CURLNETPROVIDER_H
#define _CURLNETPROVIDER_H

#include <dp_net.h>
#include <dp_io.h>

class CurlNetProvider : public dpnet::NetProvider 
{
public:
	CurlNetProvider(bool verbose);

	virtual ~CurlNetProvider();

	/**
	 *  Initiate a raw network request (HTTP or HTTPS, GET or POST). When request is processed,
	 *  StreamClient methods should be called with the result obtained from the server. Note that for best
	 *  performance this call should not block waiting for result from the server. Instead the communication
	 *  to the server should happen on a separate thread or through event-based system.
	 */
	virtual dpio::Stream * open( const dp::String& method, const dp::String& url, dpio::StreamClient * client,
		unsigned int cap, dpio::Stream * dataToPost = 0 );

private:
	bool m_verbose;
};

#define NETPROVIDERIMPL CurlNetProvider

#endif

#endif //#if defined(FEATURE_DRM_CONNECTOR)


