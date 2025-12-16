(function() {
    if (typeof window.logos !== "undefined") return;

    let requestIdCounter = 0;
    const pendingRequests = new Map();
    const eventListeners = new Map();
    const pluginCache = new Map();
    const qtOutbox = Array.isArray(window.qtOutbox) ? window.qtOutbox : [];
    window.qtOutbox = qtOutbox;
    const drainOutbox = function() {
        if (!qtOutbox.length) return [];
        const batch = qtOutbox.slice();
        qtOutbox.length = 0;
        return batch;
    };

    function sendRequest(moduleName, methodName, args) {
        return new Promise(function(resolve, reject) {
            const requestId = ++requestIdCounter;
            pendingRequests.set(requestId, { resolve: resolve, reject: reject });

            const payload = {
                type: "logos_request",
                requestId: requestId,
                module: moduleName,
                method: methodName,
                args: Array.isArray(args) ? args : []
            };

            qtOutbox.push(payload);

            setTimeout(function() {
                if (pendingRequests.has(requestId)) {
                    pendingRequests.delete(requestId);
                    reject(new Error("Request timeout"));
                }
            }, 30000);
        });
    }

    window.addEventListener("message", function(event) {
        const data = event.data;
        if (!data) return;

        if (data.type === "logos_response") {
            const promise = pendingRequests.get(data.requestId);
            if (promise) {
                pendingRequests.delete(data.requestId);
                if (data.error) {
                    promise.reject(new Error(data.error));
                } else {
                    promise.resolve(data.result);
                }
            }
        } else if (data.type === "logos_event") {
            const listeners = eventListeners.get(data.eventName);
            if (listeners) {
                listeners.forEach(function(cb) {
                    try { cb(data.data); } catch(e) { console.error(e); }
                });
            }
        }
    });

    function getPluginProxy(name) {
        if (pluginCache.has(name)) {
            return pluginCache.get(name);
        }
        const proxy = new Proxy({}, {
            get: function(_target, prop) {
                if (prop === "then") return undefined;
                if (typeof prop !== "string") return undefined;
                return function() {
                    return sendRequest(name, prop, Array.from(arguments));
                };
            }
        });
        pluginCache.set(name, proxy);
        return proxy;
    }

    const logosRoot = {
        on: function(eventName, callback) {
            if (!eventListeners.has(eventName)) {
                eventListeners.set(eventName, []);
            }
            eventListeners.get(eventName).push(callback);
        },

        removeListener: function(eventName, callback) {
            const listeners = eventListeners.get(eventName);
            if (listeners) {
                const idx = listeners.indexOf(callback);
                if (idx > -1) listeners.splice(idx, 1);
            }
        }
    };

    window.logos = new Proxy(logosRoot, {
        get: function(target, prop) {
            if (prop in target) {
                return target[prop];
            }
            if (typeof prop !== "string") return undefined;
            return getPluginProxy(prop);
        }
    });

    window.__logosBridge = {
        drain: drainOutbox
    };
    window._qtDrain = drainOutbox;

    window.dispatchEvent(new Event("logos#initialized"));
})();
