mergeInto(LibraryManager.library, {
    Init : function() {
        this.webSocketUrl = 'ws://127.0.0.1:23456';
        this.webSocketConnection = null;
        this.rtcPeerConnection = null;
        this.dataChannel = null;
        this.pingTimes = {};
        this.pingLatency = {};
        this.pingCount = 0;
        this.PINGS_PER_SECOND = 2;
        this.SECONDS_TO_PING = 10;
        this.pingInterval=null;
        this.startTime=null;
        this.onDataChannelMessage = function(event) {
            console.log("onDataChannelMessage this:",this);
            const key = this.lib.utf8ArrayToString(new Uint8Array(event.data));
            console.log("onDataChannelMessage:",key);
            this.lib.pingLatency[key] = performance.now() - this.lib.pingTimes[key];
        };
        this.onDataChannelOpen = function() {
            console.log('Data channel opened!', this);
        };
        this.onIceCandidate = function(event) {
            console.log("onicecandidate",this);            
            if (event && event.candidate) {
                this.lib.webSocketConnection.send(JSON.stringify({type: 'candidate', payload: event.candidate}));
            }
        };
        this.onOfferCreated = function(description) {
            console.log("onOfferCreated",this);
            this.rtcPeerConnection.setLocalDescription(description);
            this.webSocketConnection.send(JSON.stringify({type: 'offer', payload: description}));
        };

        this.onWebSocketOpen = function() {

            var isChrome = /Chrome/.test(navigator.userAgent) && /Google Inc/.test(navigator.vendor);
            var isSafari = /Safari/.test(navigator.userAgent) && /Apple Computer/.test(navigator.vendor);

            console.log("isChrome:",isChrome, "isSafari:",isSafari);

            
            if(isChrome) {
                const config = { iceServers: [{ url: 'stun:stun.l.google.com:19302' }] };
                this.lib.rtcPeerConnection = new RTCPeerConnection(config);
            }
            if(isSafari) {
                const config = { iceServers: [{ urls: 'stun:stun.l.google.com:19302' }] };
                this.lib.rtcPeerConnection = new RTCPeerConnection(config);
            }
            console.log("rtcpeerconnection:",this.lib.rtcPeerConnection);
            this.lib.rtcPeerConnection.lib = this.lib;
            const dataChannelConfig = { ordered: false, maxRetransmits: 0 };

            this.lib.dataChannel = this.lib.rtcPeerConnection.createDataChannel('dc', dataChannelConfig);
            this.lib.dataChannel.lib = this.lib;
            this.lib.dataChannel.onmessage = this.lib.onDataChannelMessage;            
            this.lib.dataChannel.onopen = this.lib.onDataChannelOpen;

            const sdpConstraints = {
                mandatory: {
                    OfferToReceiveAudio: false,
                    OfferToReceiveVideo: false
                }
            };
            this.lib.rtcPeerConnection.onicecandidate = this.lib.onIceCandidate;
            if(isChrome) {
                this.lib.rtcPeerConnection.createOffer(this.lib.onOfferCreated, function(e) { console.log("createoffer error callback:",e); }, sdpConstraints);
            } else if(isSafari){
                // http://man.hubwiz.com/docset/JavaScript.docset/Contents/Resources/Documents/developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/createOffer.html
                // it says you should use promise based createOffer 
                this.lib.rtcPeerConnection.createOffer().then(function(offer) {
                    console.log("createoffer ok:",this);
                    return this.rtcPeerConnection.setLocalDescription(offer);
                }).then(function() {
                    console.log("setlocaldescription done",this, this.rtcPeerConnection.localDescription);
                    this.webSocketConnection.send(JSON.stringify({type: 'offer', payload: this.rtcPeerConnection.localDescription}));
                }).catch(function(reason) {
                    console.log("createoffer error",reason,this);
                });
            }
            console.log("onwebsocketopen: rtcPeerConnection:",this.lib.rtcPeerConnection, this.lib.dataChannel);
        };

        this.utf8ArrayToString = function(aBytes) {
            var sView = "";
            for (var nPart, nLen = aBytes.length, nIdx = 0; nIdx < nLen; nIdx++) {
                nPart = aBytes[nIdx];
                sView += String.fromCharCode(
                    nPart > 251 && nPart < 254 && nIdx + 5 < nLen ?
                    (nPart - 252) * 1073741824 + (aBytes[++nIdx] - 128 << 24) + (aBytes[++nIdx] - 128 << 18) + (aBytes[++nIdx] - 128 << 12) + (aBytes[++nIdx] - 128 << 6) + aBytes[++nIdx] - 128
                        : nPart > 247 && nPart < 252 && nIdx + 4 < nLen ?
                    (nPart - 248 << 24) + (aBytes[++nIdx] - 128 << 18) + (aBytes[++nIdx] - 128 << 12) + (aBytes[++nIdx] - 128 << 6) + aBytes[++nIdx] - 128
                        : nPart > 239 && nPart < 248 && nIdx + 3 < nLen ?
                    (nPart - 240 << 18) + (aBytes[++nIdx] - 128 << 12) + (aBytes[++nIdx] - 128 << 6) + aBytes[++nIdx] - 128
                        : nPart > 223 && nPart < 240 && nIdx + 2 < nLen ?
                    (nPart - 224 << 12) + (aBytes[++nIdx] - 128 << 6) + aBytes[++nIdx] - 128
                        : nPart > 191 && nPart < 224 && nIdx + 1 < nLen ?
                    (nPart - 192 << 6) + aBytes[++nIdx] - 128
                        : 
                    nPart
                );
            }
            return sView;
        };
        this.onWebSocketMessage = function(event) {
            var s=this.lib.utf8ArrayToString(new Uint8Array(event.data));
            console.log("onwebsocketmessage: event:",event,event.data,s);

            const messageObject = JSON.parse(s);
            if (messageObject.type === 'ping') {
                const key = messageObject.payload;
                this.pingLatency[key] = performance.now() - this.lib.pingTimes[key];
            } else if (messageObject.type === 'answer') {
                console.log("ANSWER:",messageObject);
                this.lib.rtcPeerConnection.setRemoteDescription(new RTCSessionDescription(messageObject.payload));
            } else if (messageObject.type === 'candidate') {
                this.lib.rtcPeerConnection.addIceCandidate(new RTCIceCandidate(messageObject.payload));
            } else {
                console.log('Unrecognized WebSocket message type.');
            }
        };

        this.connect = function() {
            this.webSocketConnection = new WebSocket(webSocketUrl);
            this.webSocketConnection.lib=this; 
            this.webSocketConnection.binaryType = "arraybuffer";
            this.webSocketConnection.onopen = this.onWebSocketOpen;
            this.webSocketConnection.onmessage = this.onWebSocketMessage;
        };
        this.printLatency = function() {
            for (var i = 0; i < this.PINGS_PER_SECOND * this.SECONDS_TO_PING; i++) {
                console.log(i + ': ' + this.pingLatency[i + '']);
            }
        };
        this.sendDataChannelPing = function() {
            console.log("sendDataChannelPing",this.dataChannel);    
            const key = this.pingCount + '';
            this.pingTimes[key] = performance.now();
            this.dataChannel.send(key);
            this.pingCount++;
            if (pingCount === this.PINGS_PER_SECOND * this.SECONDS_TO_PING) {
                clearInterval(this.pingInterval);
                console.log('total time: ' + (performance.now() - this.startTime));
                setTimeout(this.printLatency, 10000);
            }
        };
        this.sendWebSocketPing = function() {
            console.log("sendWebSocketPing");
            const key = this.pingCount + '';
            this.pingTimes[key] = performance.now();
            this.webSocketConnection.send(JSON.stringify({type: 'ping', payload: key}));
            this.pingCount++;
            if (this.pingCount === this.PINGS_PER_SECOND * this.SECONDS_TO_PING) {
                clearInterval(pingInterval);
                console.log('total time: ' + (performance.now() - this.startTime));
                setTimeout(this.printLatency, 10000);
            }
        };
        this.ping = function() {
            this.startTime = performance.now();
            this.pingInterval = setInterval(this.sendDataChannelPing, 1000.0 / this.PINGS_PER_SECOND);
            //    pingInterval = setInterval(sendWebSocketPing, 1000.0 / PINGS_PER_SECOND);
        }
    },
    Connect : function() {
        this.connect();
    },
    Ping : function() {
        this.ping();
    },
    Hello: function () {
        window.alert("Hello, world!");
    },

    HelloString: function (str) {
        window.alert(Pointer_stringify(str));
    },

    PrintFloatArray: function (array, size) {
        for(var i = 0; i < size; i++)
            console.log(HEAPF32[(array >> 2) + i]);
    },

    AddNumbers: function (x, y) {
        return x + y;
    },

    StringReturnValueFunction: function () {
        var returnStr = "bla";
        var bufferSize = lengthBytesUTF8(returnStr) + 1;
        var buffer = _malloc(bufferSize);
        stringToUTF8(returnStr, buffer, bufferSize);
        return buffer;
    },

    BindWebGLTexture: function (texture) {
        GLctx.bindTexture(GLctx.TEXTURE_2D, GL.textures[texture]);
    },

});


