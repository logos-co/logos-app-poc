#include "WebViewAppWidget.h"
#include "WebViewBridge.h"
#include <QVBoxLayout>
#include <QUrl>
#include <QWidget>
#include <QWindow>
#include <QDebug>
#include <QResizeEvent>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>

#import <WebKit/WebKit.h>
#import <Cocoa/Cocoa.h>

// Message handler for JavaScript bridge
@interface LogosMessageHandler : NSObject <WKScriptMessageHandler>
@property (nonatomic, assign) WebViewBridge* bridge;
- (instancetype)initWithBridge:(WebViewBridge*)bridge;
@end

@implementation LogosMessageHandler

- (instancetype)initWithBridge:(WebViewBridge*)bridge {
    self = [super init];
    if (self) {
        self.bridge = bridge;
    }
    return self;
}

- (void)userContentController:(WKUserContentController*)userContentController
      didReceiveScriptMessage:(WKScriptMessage*)message {
    if (!self.bridge) return;
    
    NSDictionary* body = message.body;
    if (![body isKindOfClass:[NSDictionary class]]) return;
    
    NSString* type = body[@"type"];
    if ([type isEqualToString:@"logos_request"]) {
        NSString* method = body[@"method"];
        NSNumber* requestId = body[@"requestId"];
        id params = body[@"params"];
        
        // Convert params to QVariantMap
        QVariantMap qParams;
        if ([params isKindOfClass:[NSDictionary class]]) {
            NSDictionary* paramsDict = (NSDictionary*)params;
            for (NSString* key in paramsDict) {
                id value = paramsDict[key];
                QString qKey = QString::fromUtf8([key UTF8String]);
                if ([value isKindOfClass:[NSString class]]) {
                    qParams[qKey] = QString::fromUtf8([(NSString*)value UTF8String]);
                } else if ([value isKindOfClass:[NSNumber class]]) {
                    NSNumber* num = (NSNumber*)value;
                    if (CFNumberIsFloatType((CFNumberRef)num)) {
                        qParams[qKey] = [num doubleValue];
                    } else {
                        qParams[qKey] = [num intValue];
                    }
                }
            }
        }
        
        // Handle the request
        QString qMethod = QString::fromUtf8([method UTF8String]);
        self.bridge->handleJavaScriptRequest(qMethod, qParams);
        
        // Send response back (simplified - in production would track request IDs properly)
        // For now, we'll send a simple success response
        dispatch_async(dispatch_get_main_queue(), ^{
            WKWebView* webView = (WKWebView*)message.webView;
            if (webView) {
                QJsonObject resultObj;
                resultObj["success"] = true;
                
                QJsonObject responseObj;
                responseObj["type"] = "logos_response";
                responseObj["requestId"] = [requestId intValue];
                responseObj["result"] = resultObj;
                
                QJsonDocument doc(responseObj);
                NSString* script = [NSString stringWithUTF8String:
                    QString("window.postMessage(%1, '*');").arg(QString::fromUtf8(doc.toJson())).toUtf8().constData()];
                [webView evaluateJavaScript:script completionHandler:nil];
            }
        });
    }
}

@end

// Objective-C class to wrap WKWebView in an NSView
@interface WKWebViewContainer : NSView
@property (nonatomic, strong) WKWebView* webView;
@property (nonatomic, strong) LogosMessageHandler* messageHandler;
- (instancetype)init;
- (instancetype)initWithBridge:(WebViewBridge*)bridge;
- (void)loadURL:(NSString*)urlString;
- (void)loadFile:(NSString*)filePath;
- (void)executeJavaScript:(NSString*)script;
- (void)injectLogosBridge:(WebViewBridge*)bridge;
@end

@implementation WKWebViewContainer

- (instancetype)init {
    return [self initWithBridge:nullptr];
}

- (instancetype)initWithBridge:(WebViewBridge*)bridge {
    self = [super init];
    if (self) {
        WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
        
        // Set up message handler for JavaScript bridge
        WKUserContentController* userContentController = [[WKUserContentController alloc] init];
        self.messageHandler = [[LogosMessageHandler alloc] initWithBridge:bridge];
        [userContentController addScriptMessageHandler:self.messageHandler name:@"logos"];
        
        // Inject logos object script
        NSString* logosScript = [NSString stringWithUTF8String:
            WebViewBridge::generateLogosObjectScript().toUtf8().constData()];
        WKUserScript* userScript = [[WKUserScript alloc] initWithSource:logosScript
                                                           injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                        forMainFrameOnly:NO];
        [userContentController addUserScript:userScript];
        
        config.userContentController = userContentController;
        
        self.webView = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:config];
        [self addSubview:self.webView];
        [self.webView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    }
    return self;
}

- (void)loadURL:(NSString*)urlString {
    NSURL* url = [NSURL URLWithString:urlString];
    if (url) {
        if ([url isFileURL]) {
            // For file URLs, use loadFileURL:allowingReadAccessToURL: for better security
            NSURL* directoryURL = [url URLByDeletingLastPathComponent];
            [self.webView loadFileURL:url allowingReadAccessToURL:directoryURL];
        } else {
            NSURLRequest* request = [NSURLRequest requestWithURL:url];
            [self.webView loadRequest:request];
        }
    }
}

- (void)loadFile:(NSString*)filePath {
    NSURL* fileURL = [NSURL fileURLWithPath:filePath];
    if (fileURL) {
        NSURL* directoryURL = [fileURL URLByDeletingLastPathComponent];
        [self.webView loadFileURL:fileURL allowingReadAccessToURL:directoryURL];
    }
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    [super resizeSubviewsWithOldSize:oldSize];
    [self.webView setFrame:self.bounds];
}

- (void)executeJavaScript:(NSString*)script {
    if (self.webView) {
        [self.webView evaluateJavaScript:script completionHandler:^(id result, NSError* error) {
            if (error) {
                NSLog(@"JavaScript execution error: %@", error);
            }
        }];
    }
}

- (void)injectLogosBridge:(WebViewBridge*)bridge {
    if (self.webView && bridge) {
        // Update message handler bridge reference
        self.messageHandler.bridge = bridge;
        
        // Re-inject the script
        NSString* logosScript = [NSString stringWithUTF8String:
            WebViewBridge::generateLogosObjectScript().toUtf8().constData()];
        [self.webView evaluateJavaScript:logosScript completionHandler:nil];
    }
}

@end

// C++ namespace for helper functions
namespace {
    class MacWebViewWidget : public QWidget {
        Q_OBJECT
    public:
        MacWebViewWidget(QWidget* parent) : QWidget(parent), m_container(nil) {
            setAttribute(Qt::WA_NativeWindow, true);
            
            // Create the WKWebView container (bridge will be set later)
            m_container = [[WKWebViewContainer alloc] init];
            
            // Get the Qt widget's native NSView
            NSView* qtView = reinterpret_cast<NSView*>(winId());
            if (qtView && m_container) {
                // Add the WebView container as a subview
                [qtView addSubview:m_container];
                m_container.frame = qtView.bounds;
                m_container.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            }
        }
        
        ~MacWebViewWidget() {
            if (m_container) {
                [m_container removeFromSuperview];
                m_container = nil;
            }
        }
        
        void loadURL(const QUrl& url) {
            if (m_container) {
                if (url.isLocalFile()) {
                    // For local files, use the file path directly
                    QString localPath = url.toLocalFile();
                    NSString* filePath = [NSString stringWithUTF8String:localPath.toUtf8().constData()];
                    [m_container loadFile:filePath];
                } else {
                    // For HTTP/HTTPS URLs, use the URL string
                    NSString* urlString = [NSString stringWithUTF8String:url.toString().toUtf8().constData()];
                    [m_container loadURL:urlString];
                }
            }
        }
        
        void executeJavaScript(const QString& script) {
            if (m_container) {
                NSString* nsScript = [NSString stringWithUTF8String:script.toUtf8().constData()];
                [m_container executeJavaScript:nsScript];
            }
        }
        
        void injectBridge(WebViewBridge* bridge) {
            if (m_container && bridge) {
                [m_container injectLogosBridge:bridge];
            }
        }
        
        void resizeEvent(QResizeEvent* event) override {
            QWidget::resizeEvent(event);
            if (m_container) {
                NSView* qtView = reinterpret_cast<NSView*>(winId());
                if (qtView) {
                    m_container.frame = qtView.bounds;
                }
            }
        }
        
    private:
        WKWebViewContainer* m_container;
    };
}

// Export functions for use in main widget
// These match the declarations in WebViewAppWidget_platform.h
QWidget* createMacWebViewWidget(QWidget* parent) {
    return new MacWebViewWidget(parent);
}

void loadURLInMacWebViewWidget(QWidget* widget, const QUrl& url) {
    MacWebViewWidget* macWidget = qobject_cast<MacWebViewWidget*>(widget);
    if (macWidget) {
        macWidget->loadURL(url);
    }
}

void injectJavaScriptBridgeInMacWebView(QWidget* widget, WebViewBridge* bridge) {
    MacWebViewWidget* macWidget = qobject_cast<MacWebViewWidget*>(widget);
    if (macWidget) {
        macWidget->injectBridge(bridge);
    }
}

void executeJavaScriptInMacWebView(QWidget* widget, const QString& script) {
    MacWebViewWidget* macWidget = qobject_cast<MacWebViewWidget*>(widget);
    if (macWidget) {
        macWidget->executeJavaScript(script);
    }
}

#include "WebViewAppWidget_mac.moc"
