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
import RequestModifier from '../streaming/utils/RequestModifier.js';
import Segment from './vo/Segment.js';
import Error from '../streaming/vo/Error.js';
import ErrorHandler from '../streaming/utils/ErrorHandler.js';
import Events from '../core/events/Events.js';
import EventBus from '../core/EventBus.js';
import BoxParser from '../streaming/utils/BoxParser.js';
import FactoryMaker from '../core/FactoryMaker.js';
import Debug from '../core/Debug.js';

var Face = require('/home/qian/ndn-js/js/face.js').Face;
var Name = require('/home/qian/ndn-js/js/name.js').Name;

function SegmentBaseLoaderNDN() {

    let context = this.context;
    let log = Debug(context).getInstance().log;
    let eventBus = EventBus(context).getInstance();
    let face = new Face({host: 'ws://localhost:9696'});

    let instance,
        errHandler,
        boxParser,
        requestModifier,
        baseURLController;

    function initialize() {
        errHandler = ErrorHandler(context).getInstance();
        boxParser = BoxParser(context).getInstance();
        requestModifier = RequestModifier(context).getInstance();
    }

    function setConfig(config) {
        if (config.baseURLController) {
            baseURLController = config.baseURLController;
        }
    }

    function loadInitialization(representation, loadingInfo) {
        if (baseURLController.resolve(representation.path).url.indexOf('ndn:') < 0) {
            loadInitializationHTTP(representation, loadingInfo);
        }
        else {
            loadInitializationNDN(representation, loadingInfo);
        }
    }

    function loadInitializationHTTP(representation, loadingInfo) {
        var needFailureReport = true;
        var initRange = null;
        var isoFile = null;
        var request = new XMLHttpRequest();
        var baseUrl = baseURLController.resolve(representation.path);
        var info = loadingInfo || {
                url: baseUrl ? baseUrl.url : undefined,
                range: {
                    start: 0,
                    end: 1500
                },
                searching: false,
                bytesLoaded: 0,
                bytesToLoad: 1500,
                request: request
            };

        log('Start searching for initialization.');

        request.onload = function () {
            if (request.status < 200 || request.status > 299) return;

            needFailureReport = false;

            info.bytesLoaded = info.range.end;
            isoFile = boxParser.parse(request.response);
            initRange = findInitRange(isoFile);

            if (initRange) {
                representation.range = initRange;
                representation.initialization = info.url;
                eventBus.trigger(Events.INITIALIZATION_LOADED, {representation: representation});
            } else {
                info.range.end = info.bytesLoaded + info.bytesToLoad;
                loadInitializationHTTP(representation, info);
            }

        };

        request.onloadend = request.onerror = function () {
            if (!needFailureReport) return;
            needFailureReport = false;
            errHandler.downloadError('initialization', info.url, request);
            eventBus.trigger(Events.INITIALIZATION_LOADED, {representation: representation});
        };

        sendRequest(request, info);
        log('loadInit::Perform init search: ' + info.url);
    }

    function loadInitializationNDN(representation, loadingInfo) {
        var needFailureReport = true;
        var initRange = null;
        var isoFile = null;

        var baseUrl = baseURLController.resolve(representation.path);
        var info = loadingInfo || {
                url: baseUrl ? baseUrl.url : undefined,
                range: {
                    start: 0,
                    end: 1500
                },
                searching: false,
                bytesLoaded: 0,
                bytesToLoad: 1500,
                //request: request
            };

        log('Start searching for initialization.');

        const initDataCallback = function (data) {
            needFailureReport = false;
            info.bytesLoaded = info.range.end;
            isoFile = boxParser.parse(data.getContent().buf().toArrayBuffer());
            initRange = findInitRange(isoFile);

            if (initRange) {
                representation.range = initRange;
                representation.initialization = info.url;
                eventBus.trigger(Events.INITIALIZATION_LOADED, {representation: representation});
            } else {
                info.range.end = info.bytesLoaded + info.bytesToLoad;
                loadInitializationNDN(representation, info);
            }

        };

        /*
         request.onloadend = request.onerror = function () {
         if (!needFailureReport) return;
         needFailureReport = false;

         errHandler.downloadError('initialization', info.url, request);
         eventBus.trigger(Events.INITIALIZATION_LOADED, {representation: representation});
         };
         */

        face.expressInterest
        (new Name(info.url),
            function (interest, content) { onData(interest, content, initDataCallback); },
            function (interest) { onTimeout(interest, initDataCallback); });
        log('loadInit::Perform init search: ' + info.url);
    }

    function loadSegments(representation, type, range, loadingInfo, callback) {
        if (baseURLController.resolve(representation.path).url.indexOf('ndn:') < 0) {
            loadSegmentsHTTP(representation, type, range, loadingInfo, callback);
        }
        else {
            loadSegmentsNDN(representation, type, range, loadingInfo, callback);
        }
    }

    function loadSegmentsHTTP(representation, type, range, loadingInfo, callback) {
        if (range && (range.start === undefined || range.end === undefined)) {
            var parts = range ? range.toString().split('-') : null;
            range = parts ? {start: parseFloat(parts[0]), end: parseFloat(parts[1])} : null;
        }

        callback = !callback ? onLoaded : callback;
        var needFailureReport = true;
        var isoFile = null;
        var sidx = null;
        var hasRange = !!range;
        var request = new XMLHttpRequest();
        var baseUrl = baseURLController.resolve(representation.path);
        var info = {
            url: baseUrl ? baseUrl.url : undefined,
            range: hasRange ? range : { start: 0, end: 1500 },
            searching: !hasRange,
            bytesLoaded: loadingInfo ? loadingInfo.bytesLoaded : 0,
            bytesToLoad: 1500,
            request: request
        };

        request.onload = function () {
            if (request.status < 200 || request.status > 299) return;

            var extraBytes = info.bytesToLoad;
            var loadedLength = request.response.byteLength;

            needFailureReport = false;
            info.bytesLoaded = info.range.end - info.range.start;
            isoFile = boxParser.parse(request.response);
            sidx = isoFile.getBox('sidx');

            if (!sidx || !sidx.isComplete) {
                if (sidx) {
                    info.range.start = sidx.offset || info.range.start;
                    info.range.end = info.range.start + (sidx.size || extraBytes);
                } else if (loadedLength < info.bytesLoaded) {
                    // if we have reached a search limit or if we have reached the end of the file we have to stop trying to find sidx
                    callback(null, representation, type);
                    return;
                } else {
                    var lastBox = isoFile.getLastBox();

                    if (lastBox && lastBox.size) {
                        info.range.start = lastBox.offset + lastBox.size;
                        info.range.end = info.range.start + extraBytes;
                    } else {
                        info.range.end += extraBytes;
                    }
                }
                loadSegmentsHTTP(representation, type, info.range, info, callback);
            } else {
                var ref = sidx.references;
                var loadMultiSidx,
                    segments;

                if (ref !== null && ref !== undefined && ref.length > 0) {
                    loadMultiSidx = (ref[0].reference_type === 1);
                }

                if (loadMultiSidx) {
                    info.range.end = info.range.start + sidx.size;

                    var j, len, ss, se, r;
                    var segs = [];
                    var count = 0;
                    var offset = (sidx.offset || info.range.start) + sidx.size;
                    var tmpCallback = function (result) {
                        if (result) {
                            segs = segs.concat(result);
                            count++;

                            if (count >= len) {
                                callback(segs, representation, type);
                            }
                        } else {
                            callback(null, representation, type);
                        }
                    };

                    for (j = 0, len = ref.length; j < len; j++) {
                        ss = offset;
                        se = offset + ref[j].referenced_size - 1;
                        offset = offset + ref[j].referenced_size;
                        r = {start: ss, end: se};
                        loadSegmentsHTTP(representation, null, r, info, tmpCallback);
                    }

                } else {
                    log('loadSegment::Parsing segments from SIDX.');
                    log('info.url: ' + info.url);
                    segments = getSegmentsForSidx(sidx, info);
                    callback(segments, representation, type);
                }
            }
        };

        request.onloadend = request.onerror = function () {
            if (!needFailureReport) return;

            needFailureReport = false;
            errHandler.downloadError('SIDX', info.url, request);
            callback(null, representation, type);
        };

        sendRequest(request, info);
        log('loadSegment::Perform SIDX load: ' + info.url);
    }

    function loadSegmentsNDN(representation, type, range, loadingInfo, callback) {
        if (range && (range.start === undefined || range.end === undefined)) {
            var parts = range ? range.toString().split('-') : null;
            range = parts ? {start: parseFloat(parts[0]), end: parseFloat(parts[1])} : null;
        }

        callback = !callback ? onLoaded : callback;
        var needFailureReport = true;
        var isoFile = null;
        var sidx = null;
        var hasRange = !!range;
        //var request = new XMLHttpRequest();
        var baseUrl = baseURLController.resolve(representation.path);
        var info = {
            url: baseUrl ? baseUrl.url : undefined,
            range: hasRange ? range : { start: 0, end: 1500 },
            searching: !hasRange,
            bytesLoaded: loadingInfo ? loadingInfo.bytesLoaded : 0,
            bytesToLoad: 1500,
            //request: request
        };

        const segmentDataCallback = function (data) {
            log('Segmentdatacallback:: init: ' + representation.initialization);
            //if (request.status < 200 || request.status > 299) return;

            var extraBytes = info.bytesToLoad;
            var loadedLength = data.getContent().size();
            var buf;

            needFailureReport = false;
            info.bytesLoaded = info.range.end - info.range.start;
            if (loadedLength > info.bytesLoaded) {
                buf = data.getContent().buf().toArrayBuffer().slice(info.range.start, info.range.end);
            }
            else {
                buf = data.getContent().buf().toArrayBuffer();
            }
            isoFile = boxParser.parse(buf);
            sidx = isoFile.getBox('sidx');

            if (!sidx || !sidx.isComplete) {
                if (sidx) {
                    info.range.start = sidx.offset || info.range.start;
                    info.range.end = info.range.start + (sidx.size || extraBytes);
                } else if (loadedLength < info.bytesLoaded) {
                    // if we have reached a search limit or if we have reached the end of the file we have to stop trying to find sidx
                    callback(null, representation, type);
                    return;
                } else {
                    var lastBox = isoFile.getLastBox();

                    if (lastBox && lastBox.size) {
                        info.range.start = lastBox.offset + lastBox.size;
                        info.range.end = info.range.start + extraBytes;
                    } else {
                        info.range.end += extraBytes;
                    }
                }
                loadSegmentsNDN(representation, type, info.range, info, callback);
            } else {
                var ref = sidx.references;
                var loadMultiSidx,
                    segments;

                if (ref !== null && ref !== undefined && ref.length > 0) {
                    loadMultiSidx = (ref[0].reference_type === 1);
                }

                if (loadMultiSidx) {
                    info.range.end = info.range.start + sidx.size;

                    var j, len, ss, se, r;
                    var segs = [];
                    var count = 0;
                    var offset = (sidx.offset || info.range.start) + sidx.size;
                    var tmpCallback = function (result) {
                        if (result) {
                            segs = segs.concat(result);
                            count++;

                            if (count >= len) {
                                callback(segs, representation, type);
                            }
                        } else {
                            callback(null, representation, type);
                        }
                    };

                    for (j = 0, len = ref.length; j < len; j++) {
                        ss = offset;
                        se = offset + ref[j].referenced_size - 1;
                        offset = offset + ref[j].referenced_size;
                        r = {start: ss, end: se};
                        loadSegmentsNDN(representation, null, r, info, tmpCallback);
                    }

                } else {
                    info.url = info.url + '/' + data.getName().toUri().split('/').slice(-2, -1);
                    log(info.url);
                    segments = getSegmentsForSidx(sidx, info);
                    callback(segments, representation, type);
                }
            }
        };

        face.expressInterest
        (new Name(info.url),
            function (interest, content) { onData(interest, content, segmentDataCallback); },
            function (interest) { onTimeout(interest, segmentDataCallback); });
        log('loadSegmentsNDN::Perform SIDX load: ' + info.url);
    }

    function reset() {
        errHandler = null;
        boxParser = null;
        requestModifier = null;
        log = null;
        //face.close();
        //face = new Face({host: 'ws://localhost:9696'});
    }

    function getSegmentsForSidx(sidx, info) {

        var refs = sidx.references;
        var len = refs.length;
        var timescale = sidx.timescale;
        var time = sidx.earliest_presentation_time;
        var start = info.range.start + sidx.first_offset + sidx.size;
        var segments = [];
        var segment,
            end,
            duration,
            size;

        for (var i = 0; i < len; i++) {
            duration = refs[i].subsegment_duration;
            size = refs[i].referenced_size;

            segment = new Segment();
            segment.duration = duration;
            segment.media = info.url;
            segment.startTime = time;
            segment.timescale = timescale;
            end = start + size - 1;
            segment.mediaRange = start + '-' + end;
            segments.push(segment);
            time += duration;
            start += size;
        }

        return segments;
    }

    function findInitRange(isoFile) {
        var ftyp = isoFile.getBox('ftyp');
        var moov = isoFile.getBox('moov');

        var initRange = null;
        var start,
            end;

        log('Searching for initialization.');


        if (moov && moov.isComplete) {
            start = ftyp ? ftyp.offset : moov.offset;
            end = moov.offset + moov.size - 1;
            initRange = start + '-' + end;

            log('Found the initialization.  Range: ' + initRange);
        }
        return initRange;
    }

    function sendRequest(request, info) {
        if (!info.url) {
            return;
        }

        request.open('GET', requestModifier.modifyRequestURL(info.url));
        request.responseType = 'arraybuffer';
        request.setRequestHeader('Range', 'bytes=' + info.range.start + '-' + info.range.end);
        request = requestModifier.modifyRequestHeader(request);
        request.send(null);
    }

    function onTimeout(interest, onDataCallback)
    {
        var nameStr = interest.getName().toUri();//.split('/').slice(0,-1).join('/');

        console.log('onTimeout: ' + nameStr);

        face.expressInterest
        (interest,
            function (interest, content) { onData(interest, content, onDataCallback); },
            function (interest) { onTimeout(interest, onDataCallback); });
    }

    function onData(interest, content, onDataCallback) {
        var nameStr = content.getName().toUri();
        log('onData: ' + nameStr + ' size: ' + content.getContent().size() + ' interest: ' + interest.getName());
        onDataCallback(content);
    }

    function onLoaded(segments, representation, type) {
        if (segments) {
            eventBus.trigger(Events.SEGMENTS_LOADED, {segments: segments, representation: representation, mediaType: type});
        } else {
            eventBus.trigger(Events.SEGMENTS_LOADED, {segments: null, representation: representation, mediaType: type, error: new Error(null, 'error loading segments', null)});
        }
    }

    instance = {
        setConfig: setConfig,
        initialize: initialize,
        loadInitialization: loadInitialization,
        loadSegments: loadSegments,
        reset: reset
    };

    return instance;
}

SegmentBaseLoaderNDN.__dashjs_factory_name = 'SegmentBaseLoaderNDN';
export default FactoryMaker.getSingletonFactory(SegmentBaseLoaderNDN);
