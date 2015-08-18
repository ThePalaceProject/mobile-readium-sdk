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

#include "DRMWrapper.h"

#ifndef WIN_ENV
#include <unistd.h>
#endif
#include <zlib.h>

#include "consoledrmclient.h"
#include "curlnetprovider.h"
#include "connectorHelperFns.h"
#include "launcherResProvider.h"

#include "adept_client.h"
#include "adept_public.h"

#include <string.h>

static dpnet::NetProvider * g_netProvider = NULL;

DRMWrapper& getWrapperObj()
{
    static DRMWrapper drmWrapperObj;// (/*NULL*/);
    return drmWrapperObj;
}

DRMWrapper::DRMWrapper()
: m_pDRMClient ( nullptr )
, m_pDevice ( nullptr )
, m_error (false)
{
    initializeDP();
#ifdef NETPROVIDERIMPL
    g_netProvider = new NETPROVIDERIMPL(/*g_verbose*/false);
    dpnet::NetProvider::setProvider(g_netProvider);
#endif
#ifdef EPUB_PLATFORM_IOS
    dp::String resFolderURL = connectorutils::getResourceURL();
#else
    dp::String resFolderURL = dp::String();
#endif
    LauncherResProvider *launcherResProvider = new LauncherResProvider( resFolderURL, false /* g_verbose */ );
    dpres::ResourceProvider::setProvider( launcherResProvider );

    // --
    dpdev::DeviceProvider *deviceProvider = dpdev::DeviceProvider::getProvider(0);
    if (deviceProvider == NULL)
    {
        return;
    }
    
    m_pDevice = deviceProvider->getDevice(0);
    if( m_pDevice == NULL )
    {
        return;
    }
    
    if( m_pDevice != NULL )
    {
        m_pDRMClient = new ConsoleDRMProcessorClient (m_pDevice);
    }
}

DRMWrapper::~DRMWrapper()
{
    if(m_pDRMClient)
        delete m_pDRMClient;
}

void DRMWrapper::initializeDP()
{
    static bool g_bInitialized = false;
    if(!g_bInitialized)
    {
        dp::platformInit( dp::PI_DEFAULT );
        
        // set version info - each product should customize!
        dp::setVersionInfo( "product", "SDKLauncher" );
        dp::setVersionInfo( "clientVersion", "SDKLauncher 1.0" );
        dp::setVersionInfo( "hobbes", "11.0.1" );
        dp::setVersionInfo("connectorOnly", "true");
        
        dp::cryptRegisterOpenSSL();
        dp::deviceRegisterPrimary();
        if( !dpdev::isMobileOS() ) // no external devices for mobile OSes
            dp::deviceRegisterExternal();
        
        g_bInitialized = true;
    }
}

bool
DRMWrapper::IsDeviceAuthorized()
{
    m_error = false;
    if( NULL == m_pDRMClient )
    {
        //b2plog_Log( B2PLOG_LOGERROR, "No client initialized\n" );
        //myExit(2);
        m_error = true;
        m_errorString = dp::String("DRM : No client initialized");
        return false;
    }
    
    dp::list<dpdrm::Activation> activations = m_pDRMClient->getDRMProcessor()->getActivations();
    
    return activations.length();
}


void
DRMWrapper::Deactivate()
{
    if (m_pDevice) {
        m_pDevice->setActivationRecord( dp::Data() );
    }
}

// Authorize this device for the user to open ePub file
void
DRMWrapper::AuthorizeDevice(const std::string& authProvider, const std::string& uName, const std::string& uPasswd)
{
    m_error = false;
    if( NULL == m_pDRMClient )
    {
        m_error = true;
        m_errorString = dp::String("DRM : No client initialized");
        return;
    }
    
    dp::String theAuthProvider (authProvider.c_str());
    dp::String username (uName.c_str());
    unsigned int workflows = dpdrm::DW_AUTH_SIGN_IN | dpdrm::DW_GET_CREDENTIAL_LIST| dpdrm::DW_ACTIVATE;
    
    dp::String password (uPasswd.c_str());
    
    m_pDRMClient->getDRMProcessor()->reset();
    workflows = m_pDRMClient->getDRMProcessor()->initSignInWorkflow(workflows, theAuthProvider, username, password);
    
    m_pDRMClient->getDRMProcessor()->startWorkflows(workflows);

    m_error = false;
    m_errorString = dp::String();
    if (!(m_pDRMClient->getErrorInWorkflow().isNull()))
    {
        m_error = true;
        m_errorString = m_pDRMClient->getErrorInWorkflow();
        m_pDRMClient->resetError();
        ::printf ("DRM: Error in Authorization workflow : %s\n", m_errorString.utf8());
    }
    else
        ::printf ("DRM: Authorization successful for user: %s\n", uName.c_str());
}


void
DRMWrapper::DoFulFill(const std::string& tokenFile, std::string& outputPath)
{
    m_error = false;
    if( NULL == m_pDRMClient )
    {
        m_error = true;
        m_errorString = dp::String("DRM : No client initialized");
        return;
    }
    
	dpdrm::DRMProcessor* drmProcessor = m_pDRMClient->getDRMProcessor();
	
    {
        dp::Data activation = m_pDevice->getActivationRecord();
        
		char * buf = new char[activation.length()+1];
		::memcpy( buf, activation.data(), activation.length());
		buf[activation.length()] = '\0';
		delete[] buf;
        
        // ---------
		dp::list<dpdrm::Activation> activations = drmProcessor->getActivations();
        size_t len = activations.length();
        for(size_t i = 0; i < len; i++)
        {
            dp::ref<dpdrm::Activation> activation = activations[i];
            const char* p1 = activation->getUserID().utf8();
            p1 = activation->getDeviceID().utf8();
            dp::time_t exp = activation->getExpiration();
            if( exp == 0 )
                ;
            else
                dp::String::timeToString(exp);
            p1 = activation->getUsername().utf8();
            bool has = activation->hasCredentials();
            p1 = activation->getAuthority().utf8();
            if(has)
            {
                *p1;
            }
        }
        
        // --------
        if( drmProcessor->getActivations().length() == 0 )
        {
            m_error = true;
            m_errorString = dp::String("DRM : Device is not activated");

            return;
        }
    }
    
	dp::Data contentData;
	if( ::strncmp( tokenFile.c_str(), "http://", 7 ) == 0 || ::strncmp( tokenFile.c_str(), "https://", 8 ) == 0 )
        contentData = connectorutils::readURL( tokenFile.c_str() );
	else
        contentData = connectorutils::readFile( tokenFile.c_str(), true );
	if( contentData.isNull() )
	{
        m_error = true;
        m_errorString = dp::String("Cannot read ACSM file");
		return;
	}
    else if( std::strstr((const char*)contentData.data(0),"application/pdf") != NULL)
    {
        int len = strlen("Download failed.\nFulfillment of PDF files are not supported on your Application");
        char *errorMsg = (char*)malloc(sizeof(char)*len);
        strncpy(errorMsg, "Download failed.\nFulfillment of PDF files are not supported on your Application", len);
        //throw (const char *)errorMsg;
        m_error = true;
        m_errorString = dp::String(errorMsg);
        free(errorMsg);
        return;
    }
    
    m_pDRMClient->getDRMProcessor()->reset();
	drmProcessor->initWorkflows(dpdrm::DW_FULFILL | dpdrm::DW_DOWNLOAD | dpdrm::DW_NOTIFY, contentData);
	drmProcessor->startWorkflows(dpdrm::DW_FULFILL | dpdrm::DW_DOWNLOAD | dpdrm::DW_NOTIFY);
    
    m_error = false;
    m_errorString = dp::String();
    if (!(m_pDRMClient->getErrorInWorkflow().isNull()))
    {
        m_error = true;
        m_errorString = m_pDRMClient->getErrorInWorkflow();
        m_pDRMClient->resetError();
        ::printf ("DRM: Error in fulfillment workflow : %s\n", m_errorString.utf8());
        outputPath = "";
    }
    else
        outputPath = m_pDRMClient->getFulfilledFilePath().utf8();
}

dp::String DRMWrapper::getErrorString()
{
    return m_errorString;
}

#endif // FEATURE_DRM_CONNECTOR

