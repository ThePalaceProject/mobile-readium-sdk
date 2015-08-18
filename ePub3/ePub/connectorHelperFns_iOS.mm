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
#include <string>

#ifdef EPUB_PLATFORM_IOS
dp::String connectorutils::getResourceURL()
{
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    std::string resPath(bundlePath.UTF8String);
    resPath.append("/",1);
    return connectorutils::urlEncodeFileName(resPath.c_str());
}

dp::String connectorutils::getDocumentsDirectory()
{
    NSString *docsDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                          NSUserDomainMask, YES) objectAtIndex:0];
    std::string docsPath(docsDir.UTF8String);
    docsPath.append("/",1);
    return connectorutils::urlEncodeFileName(docsPath.c_str());
}

dp::String connectorutils::uniqueFileName(dp::String file)
{
    NSUInteger existingCount = 0;
    NSString *result;
    NSFileManager *manager = [NSFileManager defaultManager];
    NSString *name = [NSString stringWithUTF8String:file.utf8()];
    NSString *fileExt = [name pathExtension];
    NSString *fileName = [name stringByDeletingPathExtension];
    NSString *newFileName;
    
    NSString *docsDir = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                             NSUserDomainMask, YES) objectAtIndex:0];

    do {
        NSString *format = existingCount > 0 ? @"%@-%lu" : @"%@";
        
        newFileName = [NSString stringWithFormat:format, fileName, existingCount++];
        result = [newFileName stringByAppendingFormat:@".%@", fileExt];
        
        result = [docsDir stringByAppendingPathComponent:result];
    } while ([manager fileExistsAtPath:result]);
    
    return connectorutils::urlEncodeFileName(result.UTF8String);
}

#endif
#endif // FEATURE_DRM_CONNECTOR