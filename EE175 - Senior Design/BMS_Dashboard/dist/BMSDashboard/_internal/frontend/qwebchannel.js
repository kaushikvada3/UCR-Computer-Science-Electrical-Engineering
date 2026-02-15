"use strict";

var QWebChannelMessageTypes = {
    signal: 1,
    propertyUpdate: 2,
    init: 3,
    idle: 4,
    debug: 5,
    invokeMethod: 6,
    connectToSignal: 7,
    disconnectFromSignal: 8,
    setProperty: 9,
    response: 10,
};

var QWebChannel = function (transport, initCallback) {
    if (typeof transport !== "object" || typeof transport.send !== "function") {
        console.error("The QWebChannel expects a transport object with a send function and onmessage callback signal.");
    }

    var channel = this;
    this.transport = transport;

    this.send = function (data) {
        if (typeof (data) !== "string") {
            data = JSON.stringify(data);
        }
        channel.transport.send(data);
    }

    this.transport.onmessage = function (message) {
        var data = message.data;
        if (typeof data === "string") {
            data = JSON.parse(data);
        }
        switch (data.type) {
            case QWebChannelMessageTypes.signal:
                channel.handleSignal(data);
                break;
            case QWebChannelMessageTypes.response:
                channel.handleResponse(data);
                break;
            case QWebChannelMessageTypes.propertyUpdate:
                channel.handlePropertyUpdate(data);
                break;
            default:
                console.error("invalid message received:", message.data);
                break;
        }
    }

    this.execCallbacks = {};
    this.execId = 0;
    this.objects = {};

    this.handleSignal = function (message) {
        var object = channel.objects[message.object];
        if (object) {
            object.signalEmitted(message.signal, message.args);
        } else {
            console.warn("Unhandled signal: " + message.object + "::" + message.signal);
        }
    }

    this.handleResponse = function (message) {
        if (!message.hasOwnProperty("id")) {
            console.error("Invalid response message received: ", message);
            return;
        }
        channel.execCallbacks[message.id](message.data);
        delete channel.execCallbacks[message.id];
    }

    this.handlePropertyUpdate = function (message) {
        for (var i in message.data) {
            var data = message.data[i];
            var object = channel.objects[data.object];
            if (object) {
                object.propertyUpdate(data.signals, data.properties);
            } else {
                console.warn("Unhandled property update: " + data.object + "::" + data.signal);
            }
        }
        channel.execCallbacks[message.id](data);
        delete channel.execCallbacks[message.id];
    }

    this.debug = function (message) {
        channel.send({ type: QWebChannelMessageTypes.debug, data: message });
    };

    channel.exec({ type: QWebChannelMessageTypes.init }, function (data) {
        for (var objectName in data) {
            var object = new QObject(objectName, data[objectName], channel);
        }
        for (var objectName in data) {
            var object = channel.objects[objectName];
            object.unwrapProperties();
        }
        if (initCallback) {
            initCallback(channel);
        }
        channel.exec({ type: QWebChannelMessageTypes.idle });
    });
};

QWebChannel.prototype.exec = function (data, callback) {
    if (callback) {
        this.execCallbacks[this.execId] = callback;
        data.id = this.execId;
        this.execId++;
    }
    this.send(data);
};

var QObject = function (name, data, webChannel) {
    this.__id__ = name;
    webChannel.objects[name] = this;

    this.__objectSignals__ = {};
    this.__propertyCache__ = {};

    var object = this;

    // ----------------------------------------------------------------------
    // connection to the web channel

    this.unwrapQObject = function (response) {
        if (response instanceof Array) {
            // support list of objects
            var ret = new Array(response.length);
            for (var i = 0; i < response.length; ++i) {
                ret[i] = object.unwrapQObject(response[i]);
            }
            return ret;
        }
        if (!response
            || !response["__QObject*__"]
            || response.id === undefined) {
            return response;
        }

        var objectId = response.id;
        if (webChannel.objects[objectId])
            return webChannel.objects[objectId];

        if (!response.data) {
            console.error("Cannot unwrap unknown QObject " + objectId + " without data.");
            return;
        }

        var qObject = new QObject(objectId, response.data, webChannel);
        qObject.destroyed.connect(function () {
            if (webChannel.objects[objectId] === qObject) {
                delete webChannel.objects[objectId];
                // reset the now deleted QObject to an empty {} object
                // just loosely mimicking the C++ behavior of a QPointer
                var propertyNames = ["__id__", "__objectSignals__", "__propertyCache__", "unwrapQObject", "unwrapProperties", "propertyUpdate", "signalEmitted"];
                for (var prop in qObject) {
                    if (propertyNames.indexOf(prop) !== -1) {
                        continue;
                    }
                    delete qObject[prop];
                }
            }
        });
        // here we are already initialized, and thus must call unwrapProperties ourselves
        qObject.unwrapProperties();
        return qObject;
    }

    this.unwrapProperties = function () {
        for (var propertyIdx in data.properties) {
            // remove the property from the cache on value assignment
            // to connect the notify signal on the next read of the property
            Object.defineProperty(object, data.properties[propertyIdx][0], {
                configurable: true,
                get: function (propertyIdx) {
                    var property = data.properties[propertyIdx];
                    var name = property[0];
                    if (!object.__propertyCache__[name]) {
                        var signalName = property[1];
                        // If the signal is not valid, the property is read only
                        if (signalName) {
                            object[signalName].connect(function () {
                                delete object.__propertyCache__[name];
                            });
                        }
                    }
                    return object.__propertyCache__[name];
                }.bind(undefined, propertyIdx),
                set: function (propertyIdx, value) {
                    var property = data.properties[propertyIdx];
                    var name = property[0];
                    var signalName = property[1];
                    if (object.__propertyCache__[name] === value) {
                        return;
                    }
                    if (!signalName) {
                        console.error("Property " + name + " is read only.");
                        return;
                    }
                    delete object.__propertyCache__[name];
                    webChannel.exec({ type: QWebChannelMessageTypes.setProperty, object: object.__id__, property: propertyIdx, value: value });
                }.bind(undefined, propertyIdx)
            });
        }
        data.properties.forEach(function (property) {
            object.__propertyCache__[property[0]] = property[2];
        });
    }

    this.propertyUpdate = function (signals, properties) {
        for (var signalName in signals) {
            var signal = object[signalName];
            var args = signals[signalName];
            signal.startEmit(args);
        }
        for (var propertyName in properties) {
            object.__propertyCache__[propertyName] = properties[propertyName];
        }
    }

    this.signalEmitted = function (signalName, signalArgs) {
        var signal = object[signalName];
        signal.startEmit(signalArgs);
    }

    // ----------------------------------------------------------------------
    // methods

    this._addMethod = function (methodName) {
        object[methodName] = function () {
            var args = [];
            var callback;
            for (var i = 0; i < arguments.length; ++i) {
                if (typeof arguments[i] === "function")
                    callback = arguments[i];
                else
                    args.push(arguments[i]);
            }

            webChannel.exec({
                "type": QWebChannelMessageTypes.invokeMethod,
                "object": object.__id__,
                "method": methodName,
                "args": args
            }, function (response) {
                if (response !== undefined) {
                    var result = object.unwrapQObject(response);
                    if (callback) {
                        (callback)(result);
                    }
                }
            });
        };
    }

    data.methods.forEach(function (methodName) {
        object._addMethod(methodName);
    });

    // ----------------------------------------------------------------------
    // signals

    this._addSignal = function (signalName, isPropertyNotifySignal) {
        var signal = new QSignal(signalName, isPropertyNotifySignal);
        object[signalName] = signal;
        object.__objectSignals__[signalName] = signal;
    }

    data.signals.forEach(function (signalName) {
        object._addSignal(signalName, false);
    });

    for (var propertyIdx in data.properties) {
        var signalName = data.properties[propertyIdx][1];
        if (signalName) {
            object._addSignal(signalName, true);
        }
    }
};

var QSignal = function (signalName, isPropertyNotifySignal) {
    this._listeners = [];
    this._signalName = signalName;
    this._isPropertyNotifySignal = isPropertyNotifySignal;
};

QSignal.prototype.connect = function (callback) {
    if (typeof callback !== "function") {
        console.error("QSignal.connect() requires a function callback.");
        return;
    }
    if (this._listeners.indexOf(callback) !== -1) {
        return;
    }
    this._listeners.push(callback);
}

QSignal.prototype.disconnect = function (callback) {
    var idx = this._listeners.indexOf(callback);
    if (idx !== -1) {
        this._listeners.splice(idx, 1);
    }
}

QSignal.prototype.startEmit = function (args) {
    var listenersCopy = this._listeners.slice();
    for (var i = 0; i < listenersCopy.length; ++i) {
        listenersCopy[i].apply(this, args);
    }
}

if (typeof module === 'object') {
    module.exports = {
        QWebChannel: QWebChannel
    };
}
