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

#ifndef __ePub3__adept_filter__
#define __ePub3__adept_filter__

#include <ePub3/filter.h>
#include <ePub3/encryption.h>
#include REGEX_INCLUDE
#include <cstring>

#if defined(FEATURE_DRM_CONNECTOR)
#include <ePub3/container.h>
#include "dp_utils_crypt.h"
#include "uft_string.h"
#include <sys/time.h>
#endif

EPUB3_BEGIN_NAMESPACE

/**
 The AdeptFilter class implements adept decryption algorithm.
 */
class AdeptFilter : public ContentFilter, public PointerType<AdeptFilter>
{
protected:
    CONSTEXPR static EPUB3_EXPORT const char * const	AdeptAlgorithmID
#if EPUB_COMPILER_SUPPORTS(CXX_NONSTATIC_MEMBER_INIT) && !EPUB_COMPILER(MSVC)
            = "http://www.w3.org/2001/04/xmlenc#aes128-cbc"
#endif
              ;
    CONSTEXPR static EPUB3_EXPORT const char * const	AdeptAlgorithmIDUncompressed
#if EPUB_COMPILER_SUPPORTS(CXX_NONSTATIC_MEMBER_INIT) && !EPUB_COMPILER(MSVC)
    = "http://ns.adobe.com/adept/xmlenc#aes128-cbc-uncompressed"
#endif
    ;
    
    static bool AdeptTypeSniffer(ConstManifestItemPtr item) {
        EncryptionInfoPtr encInfo = item->GetEncryptionInfo();
        if ( encInfo == nullptr || (encInfo->Algorithm() != AdeptAlgorithmID && encInfo->Algorithm() != AdeptAlgorithmIDUncompressed))
            return false;

        return true;
    }
    
    static ContentFilterPtr AdeptFilterFactory(ConstPackagePtr item);
    
private:
    ///
    /// There is no default constructor.
    AdeptFilter() _DELETED_;
        
private:
    class AdeptFilterContext : public FilterContext
    {
    private:
        size_t                                      _count;
        size_t                                      _resLength;
        dp::ref<dputils::EPubManifestItemDecryptor> _manifestItemDecryptor;
        
    public:
        AdeptFilterContext(ConstManifestItemPtr pManifestItem) : FilterContext(), _count(0), _resLength(0) {

            if(pManifestItem)
            {
                PackagePtr pPackage = pManifestItem->GetPackage();
                ContainerPtr pContainer = pPackage->GetContainer();
                if(pContainer)
                {
                    const dp::Data& rightsXMLData = pContainer->getRightsXMLData();
                    dp::ref<dputils::EncryptionMetadata> pEncMetadata = pContainer->getEncryptionMetadata();
                    uft::String itemPath(pManifestItem->GetEncryptionInfo()->Path().c_str());
                    dp::ref<dputils::EncryptionItemInfo> pItemInfo = pEncMetadata->getItemForURI(itemPath);
                    if(pItemInfo)
                    {
                        dp::String error;
                        _resLength = pItemInfo->getResourceLength();
                        dpdev::Device *pDevice = pContainer->getAdeptDevice();
                        _manifestItemDecryptor = dpdrm::DRMProcessor::createEPubManifestItemDecryptor(pItemInfo, rightsXMLData, pDevice, error);
                    }
                }
            }
        }
        virtual ~AdeptFilterContext() {}
        
        size_t  ProcessedCount() const      { return _count; }
        void SetProcessedCount(size_t val)  { _count = val; }
        dp::ref<dputils::EPubManifestItemDecryptor> getDecryptor(){ return _manifestItemDecryptor; }
        size_t getResourceLength(){ return _resLength; }
        
    };
    
    dp::ref<dp::Buffer> _outData;
    //dp::Data    _outData;

public:
    /**
     Create a adept filter.
     
     The obfuscation key is built using data from every manifestation within an EPUB
     container, so the Container instance is passed in for that purpose. This is
     only used during construction.
     @see BuildKey(const Container*)
     */
    AdeptFilter(ConstContainerPtr container) : ContentFilter(AdeptTypeSniffer) {
    }
    ///
    /// Copy constructor.
    AdeptFilter(const AdeptFilter& o) : ContentFilter(o) {
    }
    ///
    /// Move constructor.
    AdeptFilter(AdeptFilter&& o) : ContentFilter(std::move(o)) {
    }
    
    /**
     Applies the adept decryption algorithm to the resource data.
     @param data The data to process.
     @param len The number of bytes in `data`.
     @param outputLen Storage for the count of bytes being returned.
     @result The obfuscated or de-obfuscated bytes.
     */
    virtual void * FilterData(FilterContext* context, void * data, size_t len, size_t *outputLen) OVERRIDE;
    
    static void Register();
    
protected:
    
    virtual FilterContext *InnerMakeFilterContext(ConstManifestItemPtr pManifestItem) const OVERRIDE { return new AdeptFilterContext(pManifestItem); }
};

EPUB3_END_NAMESPACE

#endif /* defined(__ePub3__adept_filter__) */
#endif
