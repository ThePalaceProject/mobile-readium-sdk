//
//  RDPackageResourceConnection.m
//  RDServices
//
//  Created by Shane Meyer on 11/23/13.
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

#import "RDPackageResourceConnection.h"
#import "RDPackage.h"
#import "RDPackageResource.h"
#import "RDPackageResourceDataResponse.h"
#import "RDPackageResourceResponse.h"
#import "RDPackageResourceServer.h"

static RDPackage *m_package = nil;

// The incremental numeral below is needed to prevent
// iOS UIWebView / NSURLCache from auto-caching requests to "readium_epubReadingSystem_inject.js"
// See implementation of:
// -(NSCachedURLResponse *)cachedResponseForRequest:(NSURLRequest *)request
// ... in NSURLCacheInterceptor
// Note that OSX implementation does not require this hack,
// as the following request interceptor method is provided natively by WebView (no caching):
/*
- (NSURLRequest*) webView:(WebView*)sender
        resource:(id)identifier
        willSendRequest:(NSURLRequest*)request
        redirectResponse:(NSURLResponse*)redirectResponse
        fromDataSource:(WebDataSource*)dataSource
{
...
}
*/
static long m_injectCounter = 0;

@implementation RDPackageResourceConnection

//
// Converts the given HTML data to a string.  The character set and encoding are assumed to be
// UTF-8, UTF-16BE, or UTF-16LE.
//
- (NSString *)htmlFromData:(NSData *)data {
    if (data == nil || data.length == 0) {
        return nil;
    }

    NSString *html = nil;
    UInt8 *bytes = (UInt8 *)data.bytes;

    if (data.length >= 3) {
        if (bytes[0] == 0xFE && bytes[1] == 0xFF) {
            html = [[NSString alloc] initWithData:data
                                         encoding:NSUTF16BigEndianStringEncoding];
        }
        else if (bytes[0] == 0xFF && bytes[1] == 0xFE) {
            html = [[NSString alloc] initWithData:data
                                         encoding:NSUTF16LittleEndianStringEncoding];
        }
        else if (bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
            html = [[NSString alloc] initWithData:data
                                         encoding:NSUTF8StringEncoding];
        }
        else if (bytes[0] == 0x00) {
            // There's a very high liklihood of this being UTF-16BE, just without the BOM.
            html = [[NSString alloc] initWithData:data
                                         encoding:NSUTF16BigEndianStringEncoding];
        }
        else if (bytes[1] == 0x00) {
            // There's a very high liklihood of this being UTF-16LE, just without the BOM.
            html = [[NSString alloc] initWithData:data
                                         encoding:NSUTF16LittleEndianStringEncoding];
        }
        else {
            html = [[NSString alloc] initWithData:data
                                         encoding:NSUTF8StringEncoding];

            if (html == nil) {
                html = [[NSString alloc] initWithData:data
                                             encoding:NSUTF16BigEndianStringEncoding];

                if (html == nil) {
                    html = [[NSString alloc] initWithData:data
                                                 encoding:NSUTF16LittleEndianStringEncoding];
                }
            }
        }
    }

    return html;
}

- (NSObject <HTTPResponse> *)httpResponseForMethod:(NSString *)method URI:(NSString *)path {
	if (m_package == nil ||
		method == nil ||
		![method isEqualToString:@"GET"] ||
		path == nil ||
		path.length == 0)
	{
		return nil;
	}

	if (path != nil) {
        if ([path hasPrefix:@"/"]) {
            path = [path substringFromIndex:1];
        }
        path = [path stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
	}

	NSObject <HTTPResponse> *response = nil;

    NSString * math = @"readium_MathJax.js";
    if ([path hasPrefix:math]) {
        NSString *filePath = [[NSBundle mainBundle] pathForResource:@"MathJax" ofType:@"js" inDirectory:@"mathjax"];
        if (filePath != nil) {
            NSString *code = [NSString stringWithContentsOfFile:filePath encoding:NSUTF8StringEncoding error:nil];
            if (code != nil) {
                NSData *data = [code dataUsingEncoding:NSUTF8StringEncoding];
                if (data != nil) {
                    RDPackageResourceDataResponse *dataResponse = [[RDPackageResourceDataResponse alloc]
                            initWithData:data];
                    dataResponse.contentType = @"text/javascript";

                    response = dataResponse;
                    return response;
                }
            }
        }
    }

	// Synchronize using a process-level lock to guard against multiple threads accessing a
	// resource byte stream, which may lead to instability.

	@synchronized ([RDPackageResourceServer resourceLock]) {
		RDPackageResource *resource = [m_package resourceAtRelativePath:path];

		if (resource == nil) {
			NSLog(@"No resource found! (%@)", path);
		}
		else
        {
            bool isHTML = [path hasSuffix:@".html"] || [path hasSuffix:@".xhtml"] || [resource.mimeType isEqualToString:@"application/xhtml+xml"];

            if (isHTML) {
                NSData *data = resource.data;
                if (data != nil) {
                    NSString* source = [self htmlFromData:data];
                    if (source != nil) {
                        NSString *pattern = @"(<head.*>)";
                        NSError *error = nil;
                        NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:pattern options:NSRegularExpressionCaseInsensitive error:&error];
                        if(error != nil) {
                            NSLog(@"RegEx error: %@", error);
                        } else {
                            //NSString *filePath = [[NSBundle mainBundle] pathForResource:@"epubReadingSystem_inject" ofType:@"js" inDirectory:@"Scripts"];
                            //NSString *code = [NSString stringWithContentsOfFile:filePath encoding:NSUTF8StringEncoding error:nil];
                            //NSString *inject_epubReadingSystem = [NSString stringWithFormat:@"<script type=\"text/javascript\"></script>", code];

                            //NSString *inject_epubReadingSystem1 = [NSString stringWithFormat:@"<script type=\"text/javascript\" src=\"%@/../epubReadingSystem_inject.js\"></script>", m_baseUrlPath];

                            // Installs "hook" function so that top-level window (application) can later inject the window.navigator.epubReadingSystem into this HTML document's iframe
                            NSString *inject_epubReadingSystem1 = [NSString stringWithFormat:@"<script id=\"readium_epubReadingSystem_inject1\" type=\"text/javascript\">\n//<![CDATA[\n%@\n//]]>\n</script>",
                                                                                             @"window.readium_set_epubReadingSystem = function (obj) {\
                                \nwindow.navigator.epubReadingSystem = obj;\
                                \nwindow.readium_set_epubReadingSystem = undefined;\
                                \nvar el1 = document.getElementById(\"readium_epubReadingSystem_inject1\");\
                                \nif (el1 && el1.parentNode) { el1.parentNode.removeChild(el1); }\
                                \nvar el2 = document.getElementById(\"readium_epubReadingSystem_inject2\");\
                                \nif (el2 && el2.parentNode) { el2.parentNode.removeChild(el2); }\
                                \n};"];

                            // Fake script, generates HTTP request => triggers the push of window.navigator.epubReadingSystem into this HTML document's iframe (see LOXWebViewController.mm where the "readium_epubReadingSystem_inject" UIWebView URI query is handled)
                            NSString *inject_epubReadingSystem2 =
                                    [NSString stringWithFormat:
                                    @"<script id=\"readium_epubReadingSystem_inject2\" type=\"text/javascript\" src=\"/%ld/readium_epubReadingSystem_inject.js\"> </script>",
                                                    m_injectCounter++];

                            NSString *inject_mathJax = @"";
                            if ([source rangeOfString:@"<math"].location != NSNotFound) {

                                //inject_mathJax = [NSString stringWithFormat:@"<script type=\"text/javascript\" src=\"%@/../mathjax/MathJax.js\"> </script>", m_baseUrlPath];
                                inject_mathJax = @"<script type=\"text/javascript\" src=\"/readium_MathJax.js\"> </script>";

                                //NSString *filePath = [[NSBundle mainBundle] pathForResource:@"MathJax" ofType:@"js" inDirectory:@"Scripts/mathjax"];
                                //NSString *code = [NSString stringWithContentsOfFile:filePath encoding:NSUTF8StringEncoding error:nil];
                                //inject_mathJax = [NSString stringWithFormat:@"<script type=\"text/javascript\">\n//<![CDATA[\n%@\n//]]>\n</script>", code];
                                //inject_mathJax = [NSString stringWithFormat:@"<script type=\"text/javascript\">\n\n%@\n\n</script>", code];
                                //inject_mathJax = [NSString stringWithFormat:@"<script type=\"text/javascript\">\n<![CDATA[\n%@\n]]>\n</script>", code];
                            }

                            NSString *newSource = [regex stringByReplacingMatchesInString:source options:0 range:NSMakeRange(0, [source length]) withTemplate:
                                    [NSString stringWithFormat:@"%@\n%@\n%@\n%@", @"$1", inject_epubReadingSystem1, inject_epubReadingSystem2, inject_mathJax]];
                            if (newSource != nil && newSource.length > 0) {
                                NSData * newData = [newSource dataUsingEncoding:NSUTF8StringEncoding];
                                if (newData != nil) {
                                    RDPackageResourceDataResponse *dataResponse = [[RDPackageResourceDataResponse alloc]
                                            initWithData:newData];

                                    if (resource.mimeType) {
                                        dataResponse.contentType = resource.mimeType;
                                    }

                                    response = dataResponse;
                                    return response;
                                }
                            }
                        }
                    }
                }
            }

            if (resource.contentLength < 1000000) {

                // This resource is small enough that we can just fetch the entire thing in memory,
                // which simplifies access into the byte stream.  Adjust the threshold to taste.

                NSData *data = resource.data;

                if (data != nil) {
                    RDPackageResourceDataResponse *dataResponse = [[RDPackageResourceDataResponse alloc]
                        initWithData:data];

                    if (resource.mimeType) {
                        dataResponse.contentType = resource.mimeType;
                    }

                    response = dataResponse;
                }
            }
            else {
                RDPackageResourceResponse *resourceResponse = [[RDPackageResourceResponse alloc]
                    initWithResource:resource];
                response = resourceResponse;
            }
        }
	}

	return response;
}


+ (void)setPackage:(RDPackage *)package {
	m_package = package;
    //m_injectCounter = 0;
}


@end
