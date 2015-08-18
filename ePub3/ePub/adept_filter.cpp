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

#include <ePub3/base.h>

// OpenSSL APIs are deprecated on OS X and iOS
#if EPUB_OS(DARWIN)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#elif EPUB_PLATFORM(WIN)
#include <windows.h>
#include <Wincrypt.h>
#elif EPUB_PLATFORM(WINRT)
using namespace ::Platform;
using namespace ::Windows::Security::Cryptography;
using namespace ::Windows::Security::Cryptography::Core;
//using namespace ::Windows::Storage::Streams;
#else
#include <openssl/sha.h>
#endif

#include "adept_filter.h"
#include "container.h"
#include "package.h"
#include "filter_manager.h"

//DRMConnector
#include "dp_utils_crypt.h"
#include "adept_client.h"

EPUB3_BEGIN_NAMESPACE

#if !EPUB_COMPILER_SUPPORTS(CXX_NONSTATIC_MEMBER_INIT) || EPUB_COMPILER(MSVC)
const char * const AdeptFilter::AdeptAlgorithmID = "http://www.w3.org/2001/04/xmlenc#aes128-cbc";
const char * const AdeptFilter::AdeptAlgorithmIDUncompressed = "http://ns.adobe.com/adept/xmlenc#aes128-cbc-uncompressed";
#endif

void * AdeptFilter::FilterData(FilterContext* context, void *data, size_t len, size_t *outputLen)
{
    AdeptFilterContext* p = dynamic_cast<AdeptFilterContext*>(context);
    dp::ref<dputils::EPubManifestItemDecryptor> pDecryptor = p->getDecryptor();
    if(!pDecryptor)
        return nullptr;
    size_t bytesFiltered = p->ProcessedCount();
    int blockType = (bytesFiltered==0)?dputils::EPubManifestItemDecryptor::FIRST_BLOCK:dputils::EPubManifestItemDecryptor::INTERMEDIATE_BLOCK;
    if(len%pDecryptor->getMinimumBlockSize() != 0)
        blockType = blockType | dputils::EPubManifestItemDecryptor::FINAL_BLOCK;
    dp::String error = pDecryptor->decryptBlock( blockType, (uint8_t*)data, len, NULL, _outData);
    if(!error.isNull())
        return nullptr;
    unsigned char *buf = new unsigned char[_outData->length()];
    memcpy(buf, _outData->data(), _outData->length());
    bytesFiltered += _outData->length();
    p->SetProcessedCount(bytesFiltered);
    *outputLen = _outData->length();

    return (void*)buf;
}

ContentFilterPtr AdeptFilter::AdeptFilterFactory(ConstPackagePtr package)
{
    ConstContainerPtr container = package->GetContainer();
    for ( auto& encInfo : container->EncryptionData() )
    {
        if ( encInfo->Algorithm() == AdeptAlgorithmID || encInfo->Algorithm() == AdeptAlgorithmIDUncompressed)
        {
            return New(container);
        }
    }
    
    // opted out, nothing for us to do here
    return nullptr;
}

void AdeptFilter::Register()
{
    FilterManager::Instance()->RegisterFilter("AdeptFilter", MustAccessRawBytes, AdeptFilterFactory);
}

EPUB3_END_NAMESPACE

#endif