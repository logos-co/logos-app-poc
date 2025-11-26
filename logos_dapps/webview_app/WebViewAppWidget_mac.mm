#include "WebViewAppWidget.h"
#include <QVBoxLayout>
#include <QUrl>
#include <QWidget>
#include <QWindow>
#include <QDebug>
#include <QResizeEvent>

#import <WebKit/WebKit.h>
#import <Cocoa/Cocoa.h>

// Objective-C class to wrap WKWebView in an NSView
@interface WKWebViewContainer : NSView
@property (nonatomic, strong) WKWebView* webView;
- (instancetype)init;
- (void)loadURL:(NSString*)urlString;
- (void)loadFile:(NSString*)filePath;
@end

@implementation WKWebViewContainer

- (instancetype)init {
    self = [super init];
    if (self) {
        WKWebViewConfiguration* config = [[WKWebViewConfiguration alloc] init];
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

@end

// C++ namespace for helper functions
namespace {
    class MacWebViewWidget : public QWidget {
        Q_OBJECT
    public:
        MacWebViewWidget(QWidget* parent) : QWidget(parent), m_container(nil) {
            setAttribute(Qt::WA_NativeWindow, true);
            
            // Create the WKWebView container
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

#include "WebViewAppWidget_mac.moc"
