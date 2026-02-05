#include <QtCore/qglobal.h>
#ifdef Q_OS_MAC
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#endif

#include "macWindowStyle.h"
#include <QMainWindow>

void applyMacWindowRoundedCorners(QMainWindow* w, bool rounded)
{
#ifdef Q_OS_MAC
    if (!w) return;
    NSView* nsView = (NSView*)w->winId();
    if (!nsView) return;

    nsView.wantsLayer = YES;
    if (nsView.layer) {
        nsView.layer.cornerRadius = rounded ? 10.0 : 0.0;
        nsView.layer.masksToBounds = rounded;
        nsView.layer.borderWidth = rounded ? 0.5 : 0.0;
        nsView.layer.borderColor = rounded ? [NSColor separatorColor].CGColor : nullptr;
    }
#endif
}
