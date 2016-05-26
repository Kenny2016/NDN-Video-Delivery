/**
 * The copyright in this software is being made available under the BSD License,
 * included below. This software may be subject to other third party and contributor
 * rights, including patent rights, and no such rights are granted under this license.
 *
 * Copyright (c) 2013, Dash Industry Forum.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation and/or
 *  other materials provided with the distribution.
 *  * Neither the name of Dash Industry Forum nor the names of its
 *  contributors may be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
import {HTTPRequest} from './vo/metrics/HTTPRequest.js';
import FactoryMaker from '../core/FactoryMaker.js';
import MediaPlayerModel from './models/MediaPlayerModel.js';
import ErrorHandler from './utils/ErrorHandler.js';

const NDN_REPO_BLOCK_SIZE = 8192;
var Face = require('/home/qian/ndn-js/js/face.js').Face;
var Name = require('/home/qian/ndn-js/js/name.js').Name;

/**
 * @Module XHRLoader
 * @param {Object} cfg - dependancies from parent
 * @description Manages download of resources via HTTP.
 */
function XHRLoader(cfg) {
    const context = this.context;

    const mediaPlayerModel = MediaPlayerModel(context).getInstance();

    const errHandler = cfg.errHandler;
    const metricsModel = cfg.metricsModel;
    const requestModifier = cfg.requestModifier;

    let instance;
    let xhrs;
    let delayedXhrs;
    let retryTimers;
    let downloadErrorToRequestTypeMap;
    let face;

    function setup() {
        xhrs = [];
        delayedXhrs = [];
        retryTimers = [];
        face = new Face({host: 'ws://localhost:9696'});

        downloadErrorToRequestTypeMap = {
            [HTTPRequest.MPD_TYPE]:                         ErrorHandler.DOWNLOAD_ERROR_ID_MANIFEST,
            [HTTPRequest.XLINK_EXPANSION_TYPE]:             ErrorHandler.DOWNLOAD_ERROR_ID_XLINK,
            [HTTPRequest.INIT_SEGMENT_TYPE]:                ErrorHandler.DOWNLOAD_ERROR_ID_CONTENT,
            [HTTPRequest.MEDIA_SEGMENT_TYPE]:               ErrorHandler.DOWNLOAD_ERROR_ID_CONTENT,
            [HTTPRequest.INDEX_SEGMENT_TYPE]:               ErrorHandler.DOWNLOAD_ERROR_ID_CONTENT,
            [HTTPRequest.BITSTREAM_SWITCHING_SEGMENT_TYPE]: ErrorHandler.DOWNLOAD_ERROR_ID_CONTENT,
            [HTTPRequest.OTHER_TYPE]:                       ErrorHandler.DOWNLOAD_ERROR_ID_CONTENT
        };
    }

    function internalLoadNDN(config, remainingAttempts) {

        var request = config.request;
        //var xhr = new XMLHttpRequest();
        var xhr = config.request;
        var traces = [];
        var firstProgress = true;
        var needFailureReport = true;
        const requestStartTime = new Date();
        var lastTraceTime = requestStartTime;
        var lastTraceReceivedCount = 0;

        const handleLoaded = function (success) {
            needFailureReport = false;

            request.requestStartDate = requestStartTime;
            request.requestEndDate = new Date();
            request.firstByteDate = request.firstByteDate || requestStartTime;

            if (!request.checkExistenceOnly) {
                metricsModel.addHttpRequest(
                    request.mediaType,
                    null,
                    request.type,
                    request.url,
                    xhr.responseURL || null,
                    request.serviceLocation || null,
                    request.range || null,
                    request.requestStartDate,
                    request.firstByteDate,
                    request.requestEndDate,
                    xhr.status,
                    request.duration,
                    //xhr.getAllResponseHeaders(),
                    null,
                    success ? traces : null
                );
            }
        };

        const onloadend = function () {
            if (xhrs.indexOf(xhr) === -1) {
                return;
            } else {
                xhrs.splice(xhrs.indexOf(xhr), 1);
            }

            if (needFailureReport) {
                handleLoaded(false);

                if (remainingAttempts > 0) {
                    remainingAttempts--;
                    retryTimers.push(
                        setTimeout(function () {
                            internalLoadNDN(config, remainingAttempts);
                        }, mediaPlayerModel.getRetryIntervalForType(request.type))
                    );
                } else {
                    errHandler.downloadError(
                        downloadErrorToRequestTypeMap[request.type],
                        request.url,
                        request
                    );

                    if (config.error) {
                        config.error(request, 'error', xhr.statusText);
                    }

                    if (config.complete) {
                        config.complete(request, xhr.statusText);
                    }
                }
            }
        };

        const progress = function (data) {
            var currentTime = new Date();

            if (firstProgress) {
                firstProgress = false;
                if (!data || data.buffer.byteLength !== length) {
                    request.firstByteDate = currentTime;
                }
            }

            if (data.buffer.byteLength == length) {
                request.bytesLoaded = data.buffer.byteLength;
                request.bytesTotal = data.buffer.byteLength;
            }

            traces.push({
                s: lastTraceTime,//starttime
                d: currentTime.getTime() - lastTraceTime.getTime(),//duration
                b: [data.buffer.byteLength ? data.buffer.byteLength - lastTraceReceivedCount : 0]//bytes received
            });
            lastTraceTime = currentTime;
            lastTraceReceivedCount = data.buffer.byteLength;

            if (config.progress) {
                config.progress();
            }
        };

        const onload = function (data) {
            //console.log('http and ndn equal?: ' + equal(httpresponse, data.buffer));
            if (xhr.status >= 200 && xhr.status <= 299) {
                //buffers.push(data.buffer);
                progress(data);
                handleLoaded(true);

                if (config.success) {
                    config.success(data.buffer, xhr.statusText, xhr);
                }

                if (config.complete) {
                    config.complete(request, xhr.statusText);
                }
            }
        };

        try {
            const modifiedUrl = requestModifier.modifyRequestURL(request.url);
            var name = new Name(modifiedUrl);

            if (request.range) {
                var length = parseInt(request.range.split('-').slice(1, 2)) - parseInt(request.range.split('-').slice(0, 1)) + 1;
                var offset = 0;
                var buffer = new Uint8Array(length);
                if (parseInt(request.range.split('-').slice(0, 1)) !== 0) {
                    name.appendSegment( parseInt(parseInt(request.range.split('-').slice(0, 1)) / NDN_REPO_BLOCK_SIZE));//.appendSegmentOffset( parseInt(request.range.split('-').slice(0, 1)) % NDN_REPO_BLOCK_SIZE);
                }

                /*
                if (parseInt(request.range.split('-').slice(0, 1)) > 300000) {
                    return;
                }
                */
            }

            // Adds the ability to delay single fragment loading time to control buffer.
            let now = new Date().getTime();
            if (isNaN(request.delayLoadingTime) || now >= request.delayLoadingTime) {
                // no delay - just send xhr

                xhrs.push(xhr);
                //xhr.send();

                face.expressInterest
                (name,
                    function (interest, content) { onData(interest, content, onload); },
                    function (interest) { onTimeout(interest); });
            } else {
                // delay
                let delayedXhr = {xhr: xhr};
                delayedXhrs.push(delayedXhr);
                delayedXhr.delayTimeout = setTimeout(function () {
                    if (delayedXhrs.indexOf(delayedXhr) === -1) {
                        return;
                    } else {
                        delayedXhrs.splice(delayedXhrs.indexOf(delayedXhr), 1);
                    }
                    try {
                        xhrs.push(delayedXhr.xhr);
                        //delayedXhr.xhr.send();
                        face.expressInterest
                        (name,
                            function (interest, content) { onData(interest, content, onload); },
                            function (interest) { onTimeout(interest); });
                    } catch (e) {
                        delayedXhr.xhr.onerror();
                    }
                }, (request.delayLoadingTime - now));
            }

        } catch (e) {
            //xhr.onerror();
        }

        function onTimeout (interest)
        {
            var nameStr = interest.getName().toUri();//.split('/').slice(0,-1).join('/');
            console.log('onTimeout: ' + nameStr);
            onloadend();

            /*
            face.expressInterest
            (interest,
                function (interest, content) { onData(interest, content, onDataCallback); },
                function (interest) { onTimeout(interest, onDataCallback); });
                */

        }

        function onData (interest, content, onDataCallback) {
            //var nameStr = content.getName().toUri();
            var data = content.getContent().buf();
            var nameNoSegment = new Name(request.url);

            if (request.range) {
                if (offset === 0) {
                    data = data.slice(parseInt(request.range.split('-').slice(0, 1)) % NDN_REPO_BLOCK_SIZE);
                }
                if (offset + data.byteLength > length) {
                    data = data.slice(0, length - offset);
                }
                buffer.set(data, offset);
                offset += data.byteLength;
                if (offset < length) {
                    face.expressInterest
                    (nameNoSegment.appendSegment(parseInt((parseInt(request.range.split('-').slice(0, 1)) + offset) / NDN_REPO_BLOCK_SIZE)),
                        function (interest, content) { onData(interest, content, onDataCallback); },
                        function (interest) { onTimeout(interest); });
                }
                else {
                    xhr.status = 200;
                    onDataCallback(buffer);
                }
            }
            else {
                //console.log('request with no range!!!');
                xhr.status = 200;
                if (onDataCallback) {
                    onDataCallback(content.getContent());
                }
            }
        }

    }

    function internalLoadHTTP(config, remainingAttempts) {

        var request = config.request;
        var xhr = new XMLHttpRequest();
        var traces = [];
        var firstProgress = true;
        var needFailureReport = true;
        const requestStartTime = new Date();
        var lastTraceTime = requestStartTime;
        var lastTraceReceivedCount = 0;

        const handleLoaded = function (success) {
            console.log('XHR::HandleLoaded!!!');
            needFailureReport = false;

            request.requestStartDate = requestStartTime;
            request.requestEndDate = new Date();
            request.firstByteDate = request.firstByteDate || requestStartTime;

            if (!request.checkExistenceOnly) {
                metricsModel.addHttpRequest(
                    request.mediaType,
                    null,
                    request.type,
                    request.url,
                    xhr.responseURL || null,
                    request.serviceLocation || null,
                    request.range || null,
                    request.requestStartDate,
                    request.firstByteDate,
                    request.requestEndDate,
                    xhr.status,
                    request.duration,
                    xhr.getAllResponseHeaders(),
                    success ? traces : null
                );
            }
        };

        const onloadend = function () {
            //console.log('XHR onLoadend!!! responseLength: ' + xhr.response.length);
            if (xhrs.indexOf(xhr) === -1) {
                return;
            } else {
                xhrs.splice(xhrs.indexOf(xhr), 1);
            }

            if (needFailureReport) {
                handleLoaded(false);

                if (remainingAttempts > 0) {
                    remainingAttempts--;
                    retryTimers.push(
                        setTimeout(function () {
                            internalLoadHTTP(config, remainingAttempts);
                        }, mediaPlayerModel.getRetryIntervalForType(request.type))
                    );
                } else {
                    errHandler.downloadError(
                        downloadErrorToRequestTypeMap[request.type],
                        request.url,
                        request
                    );

                    if (config.error) {
                        config.error(request, 'error', xhr.statusText);
                    }

                    if (config.complete) {
                        config.complete(request, xhr.statusText);
                    }
                }
            }
        };


        const progress = function (event) {
            //console.log('XHR onProgress!!! event: ' + event);
            var currentTime = new Date();

            if (firstProgress) {
                //console.log('XHR::firstProgress!!!');
                firstProgress = false;
                if (!event.lengthComputable ||
                    (event.lengthComputable && event.total !== event.loaded)) {
                    request.firstByteDate = currentTime;
                    console.log('XHR::firstProgress!!! firstByteDate = ' + request.firstByteDate);
                }
            }

            if (event.lengthComputable) {
                //console.log('XHR::event.lengthComputable!!! bytesloaded = ' + event.loaded + ' ; bytestotal = ' + event.total);
                request.bytesLoaded = event.loaded;
                request.bytesTotal = event.total;
            }

            console.log('XHR::event.lengthComputable!!! lastTraceTime = ' + lastTraceTime + ' ; duration = ' + (currentTime.getTime() - lastTraceTime.getTime()) + ' ; bytesloaded = ' + (event.loaded - lastTraceReceivedCount) + ' ; bytestotal = ' + event.total);

            traces.push({
                s: lastTraceTime,
                d: currentTime.getTime() - lastTraceTime.getTime(),
                b: [event.loaded ? event.loaded - lastTraceReceivedCount : 0]
            });

            lastTraceTime = currentTime;
            lastTraceReceivedCount = event.loaded;

            if (config.progress) {
                console.log('XHR::config.progress!!!');
                config.progress();
            }
        };

        const onload = function () {
            console.log('XHR onLoad!!! responseLength: ' + (xhr.responseType === 'arraybuffer' ? xhr.response.byteLength : xhr.response.length));
            console.log('responseType: ' + xhr.responseType + ' ; response: ' + xhr.response);
            if (xhr.status >= 200 && xhr.status <= 299) {
                //progress(xhr.response);
                handleLoaded(true);

                if (config.success) {
                    console.log('XHR::config.success!!!');
                    config.success(xhr.response, xhr.statusText, xhr);
                }

                if (config.complete) {
                    config.complete(request, xhr.statusText);
                }
            }
        };

        try {
            const modifiedUrl = requestModifier.modifyRequestURL(request.url);
            const verb = request.checkExistenceOnly ? 'HEAD' : 'GET';

            xhr.open(verb, modifiedUrl, true);

            if (request.responseType) {
                xhr.responseType = request.responseType;
            }

            if (request.range) {
                console.log('XHRLoader::modifiedURL: ' + modifiedUrl + '; Range: ' + request.range);
                xhr.setRequestHeader('Range', 'bytes=' + request.range);

                /*
                 if(parseInt(request.range.split('-').slice(0, 1)) > 300000) {
                 return;
                 }
                 */

            }

            if (!request.requestStartDate) {
                request.requestStartDate = requestStartTime;
            }

            xhr = requestModifier.modifyRequestHeader(xhr);

            xhr.onload = onload;
            xhr.onloadend = onloadend;
            xhr.onerror = onloadend;
            xhr.onprogress = progress;

            // Adds the ability to delay single fragment loading time to control buffer.
            let now = new Date().getTime();
            if (isNaN(request.delayLoadingTime) || now >= request.delayLoadingTime) {
                // no delay - just send xhr

                xhrs.push(xhr);
                xhr.send();
            } else {
                // delay
                let delayedXhr = {xhr: xhr};
                delayedXhrs.push(delayedXhr);
                delayedXhr.delayTimeout = setTimeout(function () {
                    if (delayedXhrs.indexOf(delayedXhr) === -1) {
                        return;
                    } else {
                        delayedXhrs.splice(delayedXhrs.indexOf(delayedXhr), 1);
                    }
                    try {
                        xhrs.push(delayedXhr.xhr);
                        delayedXhr.xhr.send();
                    } catch (e) {
                        delayedXhr.xhr.onerror();
                    }
                }, (request.delayLoadingTime - now));
            }

        } catch (e) {
            xhr.onerror();
        }
    }

    /**
     * Initiates a download of the resource described by config.request
     * @param {Object} config - contains request (FragmentRequest or derived type), and callbacks
     * @memberof module:XHRLoader
     * @instance
     */
    function load(config) {
        if (config.request) {
            if (config.request.url.indexOf('ndn:') < 0) {
                internalLoadHTTP(
                    config,
                    mediaPlayerModel.getRetryAttemptsForType(
                        config.request.type
                    )
                );
            }
            else {
                internalLoadNDN(
                    config,
                    mediaPlayerModel.getRetryAttemptsForType(
                        config.request.type
                    )
                );
            }
        }
    }

    /**
     * Aborts any inflight downloads
     * @memberof module:XHRLoader
     * @instance
     */
    function abort() {
        retryTimers.forEach(t => clearTimeout(t));
        retryTimers = [];

        delayedXhrs.forEach(x => clearTimeout(x.delayTimeout));
        delayedXhrs = [];

        /*
        xhrs.forEach(x => {
            // abort will trigger onloadend which we don't want
            // when deliberately aborting inflight requests -
            // set them to undefined so they are not called
            x.onloadend = x.onerror = undefined;
            x.abort();
        });
        */
        xhrs = [];
        //face.close();
        //face = new Face({host: 'ws://localhost:9696'});
    }

    instance = {
        load: load,
        abort: abort
    };

    setup();

    return instance;
}

XHRLoader.__dashjs_factory_name = 'XHRLoader';

const factory = FactoryMaker.getClassFactory(XHRLoader);
export default factory;
