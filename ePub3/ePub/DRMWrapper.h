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

#ifndef __ePub3__DRMWrapper__
#define __ePub3__DRMWrapper__

#include "uft_error_handler.h"

#include <string>
#include <functional>
#include "dp_all.h"

namespace dpdrm
{
    class DRMProvider;
    class DRMProcessor;
    class DRMProcessorClient;
    class Activation;
    class Rights;
    class License;
    class Permission;
    class FulfillmentItem;
}

namespace dpdev {
    class Device;
}

class ConsoleDRMProcessorClient;

class DRMWrapper
{
public:
    typedef std::function<void(const std::string& path)>    CompletionFn;
    
public:
    DRMWrapper();
    
    ~DRMWrapper();
    
//    bool IsDeviceAuthorized();
  
    // Authorize this device for the user to open ePub file
//    void AuthorizeDevice(const std::string& authProvider, const std::string& uName, const std::string& uPasswd);
  
//    void Deactivate();
  
//    void DoFulFill(const std::string& tokenFile, std::string& outputPath);
//    void initializeDP();
//    dp::String getErrorString();

private:
    ConsoleDRMProcessorClient* m_pDRMClient;
    dpdev::Device* m_pDevice;
    
    CompletionFn m_CompletionFn;
    bool m_informCompletion;
    
    std::string m_FileToOpen;
protected:
    bool										m_error;
    dp::String									m_errorString;
};

DRMWrapper& getWrapperObj();

#endif // __ePub3__DRMWrapper__

#endif //#if defined(FEATURE_DRM_CONNECTOR)

