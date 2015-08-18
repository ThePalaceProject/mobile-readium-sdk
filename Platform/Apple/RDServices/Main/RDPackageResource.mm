//
//  RDPackageResource.mm
//  RDServices
//
//  Created by Shane Meyer on 2/28/13.
//  Copyright (c) 2014 Readium Foundation and/or its licensees. All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification, 
//  are permitted provided that the following conditions are met:
//  1. Redistributions of source code must retain the above copyright notice, this 
//  list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice, 
//  this list of conditions and the following disclaimer in the documentation and/or 
//  other materials provided with the distribution.
//  3. Neither the name of the organization nor the names of its contributors may be 
//  used to endorse or promote products derived from this software without specific 
//  prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
//  OF THE POSSIBILITY OF SUCH DAMAGE.

#import "RDPackageResource.h"
#import <ePub3/archive.h>
#import <ePub3/filter.h>
#import <ePub3/filter_chain.h>
#import <ePub3/filter_chain_byte_stream_range.h>
#import <ePub3/filter_chain_byte_stream.h>
#import <ePub3/package.h>
#import <ePub3/utilities/byte_stream.h>
#import "RDPackage.h"
#import <UIKit/UIKit.h>

// Same as HTTPConnection.m (to avoid unnecessary intermediary buffer iterations)
#define READ_CHUNKSIZE  (1024 * 256)

@interface RDPackageResource() <UIAlertViewDelegate> {
	@private UInt8 m_buffer[READ_CHUNKSIZE];
	@private std::unique_ptr<ePub3::ByteStream> m_byteStream;
	@private NSUInteger m_contentLength;
	@private NSUInteger m_contentLengthCheck;
	@private NSData *m_data;
	@private RDPackage *m_package;
	@private NSString *m_relativePath;
	@private BOOL m_hasProperStream;
}

@end


@implementation RDPackageResource


@synthesize contentLength = m_contentLength;
@synthesize contentLengthCheck = m_contentLengthCheck;
@synthesize package = m_package;
@synthesize relativePath = m_relativePath;

- (void *)byteStream {
	return m_byteStream.get();
}

#if defined(FEATURE_DRM_CONNECTOR)
- (NSUInteger)getUncompressedContentLength {
    [self ensureProperByteStream:NO];
    return m_contentLength;
}
#endif

- (NSData *)readDataFull {
	if (m_data == nil) {
		
		[self ensureProperByteStream:NO];

#if defined(FEATURE_DRM_CONNECTOR)
        if (m_contentLength == NSUIntegerMax)
            return nil;
#endif
		NSMutableData *md = [[NSMutableData alloc] initWithCapacity:m_contentLength == 0 ? 1 : m_contentLength];

		m_contentLengthCheck = 0;

		while (YES)
		{
			std::size_t count = m_byteStream->ReadBytes(m_buffer, sizeof(m_buffer));
			if (count == 0)
			{
				break;
			}

			m_contentLengthCheck += count;

			[md appendBytes:m_buffer length:count];
		}

		if (m_contentLength != m_contentLengthCheck)
		{
			// place breakpoint here to debug (should occur with Content Filter, greater or smaller size is possible)
			m_contentLength = m_contentLengthCheck;
		}

		m_data = md;
	}
	
	return m_data;
}

#if defined(FEATURE_DRM_CONNECTOR)
+(NSString*)getMimeTypeFor:(NSString*)relativePath package:(RDPackage*)package
{
    if(relativePath) {
        NSString* ext = [[relativePath pathExtension] lowercaseString];
        if([ext isEqualToString:@"xhtml"] || [ext isEqualToString:@"html"]) {
            return @"application/xhtml+xml"; // FORCE
        }
        else if([ext isEqualToString:@"xml"]) {
            return @"application/xml"; // FORCE
        }
        else if ([ext isEqualToString:@"svg"]) {
            return @"image/svg+xml"; // FORCE
        }
        else if([ext isEqualToString:@"js"]) {
            return @"text/javascript"; // FORCE
        }
        else if([ext isEqualToString:@"css"]) {
            return @"text/css"; // FORCE
        }
    }
    
    if(package)
    {
        ePub3::string s = ePub3::string(relativePath.UTF8String);
        ePub3::Package* sdkPackage = (ePub3::Package*)(package.sdkPackage);
        ePub3::ManifestTable manifest = sdkPackage->Manifest();
        for (auto i = manifest.begin(); i != manifest.end(); i++) {
            std::shared_ptr<ePub3::ManifestItem> item = i->second;
            if (item->Href() == s) {
                NSString * contentType = [NSString stringWithUTF8String: item->MediaType().c_str()];
                return contentType;
            }
        }
    }
    return nil;
}
#endif		//FEATURE_DRM_CONNECTOR


- (instancetype)initWithByteStream:(void *)byteStream
	package:(RDPackage *)package
	relativePath:(NSString *)relativePath
{
	if (byteStream == nil || package == nil || relativePath == nil || relativePath.length == 0) {
		return nil;
	}
	
	if (self = [super init]) {
		m_byteStream.reset((ePub3::ByteStream *)byteStream);
		m_contentLength = m_byteStream->BytesAvailable();
		m_package = package;
		m_relativePath = relativePath;
		m_hasProperStream = NO;
		
		if (m_contentLength == 0) {
			NSLog(@"The resource content length is zero! %@", m_relativePath);
		}
	}
	
	return self;
}


- (NSData *)readDataOfLength:(NSUInteger)length offset:(UInt64)offset isRangeRequest:(BOOL)isRangeRequest {

	[self ensureProperByteStream:isRangeRequest];

	ePub3::FilterChainByteStreamRange *filterStream = dynamic_cast<ePub3::FilterChainByteStreamRange *>(m_byteStream.get());
	if (filterStream != nullptr)
	{
		NSMutableData *md = [[NSMutableData alloc] initWithCapacity:length];

		ePub3::ByteRange range;
		range.Location(offset);
		NSUInteger totalRead = 0;

		while (totalRead < length)
		{
			range.Length(MIN(sizeof(m_buffer), length - totalRead));
			std::size_t count = filterStream->ReadBytes(m_buffer, sizeof(m_buffer), range);
			if (count == 0)
			{
				break;
			}

			[md appendBytes:m_buffer length:count];

			m_contentLengthCheck += count;
			totalRead += count;
			range.Location(range.Location() + count);
		}

		return md;
	}

	// Note about HTTP consecutive chunk requests (NOT byte range, just content chunks) =>
	// we can track the ByteStream position as it is being sequentially read,
	// and check that the requested offset (i.e. HTTP buffered progress) is consistent with the ByteStream's internal "cursor".

	ePub3::SeekableByteStream *seekableByteStream = dynamic_cast<ePub3::SeekableByteStream *>(m_byteStream.get());
	if (seekableByteStream != nullptr
		|| true)
	{
		//ASSERT (m_contentLength - m_byteStream->BytesAvailable()) == offset
		// (does not work because underlying raw byte stream may not map 1-1 to output ranges)
		// ... we assume that this is part of a series of contiguous subsequent buffer requests from the HTTP chunking.

		NSMutableData *md = [[NSMutableData alloc] initWithCapacity:length];

		if (seekableByteStream != nullptr)
		{
			seekableByteStream->Seek(offset, std::ios::seekdir::beg);
		}

		NSUInteger totalRead = 0;

		while (totalRead < length)
		{
#if defined(FEATURE_DRM_CONNECTOR)
            std::size_t toRead = sizeof(m_buffer);
#else
			std::size_t toRead = MIN(sizeof(m_buffer), length - totalRead);
#endif
			std::size_t count = m_byteStream->ReadBytes(m_buffer, toRead);
			if (count == 0)
			{
				if (m_contentLength != m_contentLengthCheck)
				{
					// place breakpoint here to debug (should occur with Content Filter, greater or smaller size is possible)
					m_contentLength = m_contentLengthCheck;
				}
				break;
			}

			[md appendBytes:m_buffer length:count];

			m_contentLengthCheck += count;
			totalRead += count;
		}

		return md;
	}

	NSLog(@"readDataOfLength prefetchedData should never happen! %@", m_relativePath);
	NSData *prefetchedData = [self readDataFull]; // Note: ensureProperByteStream was already called above.
	NSUInteger prefetchedDataLength = [prefetchedData length];
	NSUInteger adjustedLength = prefetchedDataLength < length ? prefetchedDataLength : length;
	NSMutableData *md = [[NSMutableData alloc] initWithCapacity:adjustedLength];
	[md appendBytes:prefetchedData.bytes length:adjustedLength];
	return md;
}

- (void)ensureProperByteStream:(BOOL)isRangeRequest {
	if (!m_hasProperStream)
	{
		ePub3::ByteStream *byteStream = m_byteStream.release();
		m_byteStream.reset((ePub3::ByteStream *)[m_package getProperByteStream:m_relativePath currentByteStream:byteStream isRangeRequest:isRangeRequest]);
#if defined(FEATURE_DRM_CONNECTOR)
		m_contentLength = m_byteStream->BytesAvailable();
        if(m_contentLength == NSUIntegerMax) {
            NSLog(@"Please use the ACS6 packaged file.");
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"DRM"
                                                            message:@"File is not packaged using ACS-6."
                                                           delegate:nil
                                                  cancelButtonTitle:@"OK"
                                                  otherButtonTitles:nil];
            [alert show];
        }
#else
		m_contentLength = m_byteStream->BytesAvailable();
#endif
		m_contentLengthCheck = 0;
		m_hasProperStream = YES;
	}
}

@end
