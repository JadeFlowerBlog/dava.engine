#if defined(__DAVAENGINE_COREV2__)

#include "Engine/Private/OsX/CoreNativeBridgeOsX.h"

#if defined(__DAVAENGINE_QT__)
// TODO: plarform defines
#elif defined(__DAVAENGINE_MACOS__)

#include "Engine/Private/EngineBackend.h"
#include "Engine/Private/OsX/PlatformCoreOsx.h"
#include "Engine/Private/OsX/Window/WindowBackendOsX.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

#include "Engine/Private/OsX/AppDelegateOsX.h"

#include "Logger/Logger.h"
#include "Platform/SystemTimer.h"

// Wrapper over NSTimer to connect Objective-C NSTimer object to
// C++ class CoreNativeBridge
@interface FrameTimer : NSObject
{
    DAVA::Private::CoreNativeBridge* bridge;
    NSTimer* timer;
}

- (id)init:(DAVA::Private::CoreNativeBridge*)nativeBridge;
- (void)set:(double)interval;
- (void)cancel;
- (void)timerFired:(NSTimer*)timer;

@end

@implementation FrameTimer

- (id)init:(DAVA::Private::CoreNativeBridge*)nativeBridge
{
    self = [super init];
    if (self != nil)
    {
        bridge = nativeBridge;
    }
    return self;
}

- (void)set:(double)interval
{
    [self cancel];
    timer = [NSTimer scheduledTimerWithTimeInterval:interval
                                             target:self
                                           selector:@selector(timerFired:)
                                           userInfo:nil
                                            repeats:NO];
}

- (void)cancel
{
    [timer invalidate];
    //[timer release];  // ?????
}

- (void)timerFired:(NSTimer*)timer
{
    bridge->OnFrameTimer();
}

@end

//////////////////////////////////////////////////////////////////

namespace DAVA
{
namespace Private
{
CoreNativeBridge::CoreNativeBridge(PlatformCore* c)
    : core(c)
{
}

CoreNativeBridge::~CoreNativeBridge()
{
    [[NSApplication sharedApplication] setDelegate:nil];
    [appDelegate release];
    [frameTimer release];
}

void CoreNativeBridge::InitNSApplication()
{
    [NSApplication sharedApplication];

    appDelegate = [[AppDelegate alloc] initWithBridge:this];
    [[NSApplication sharedApplication] setDelegate:(id<NSApplicationDelegate>)appDelegate];
}

void CoreNativeBridge::Quit()
{
    if (!quitSent)
    {
        quitSent = true;
        [[NSApplication sharedApplication] terminate:nil];
    }
}

void CoreNativeBridge::OnFrameTimer()
{
    int32 fps = core->OnFrame();
    if (fps <= 0)
    {
        // To prevent division by zero
        fps = std::numeric_limits<int32>::max();
    }

    double interval = 1.0 / fps;
    [frameTimer set:interval];
}

void CoreNativeBridge::ApplicationWillFinishLaunching()
{
}

void CoreNativeBridge::ApplicationDidFinishLaunching()
{
    core->engineBackend->OnGameLoopStarted();
    core->CreateNativeWindow(core->engineBackend->GetPrimaryWindow(), 640.0f, 480.0f);

    frameTimer = [[FrameTimer alloc] init:this];
    [frameTimer set:1.0 / 60.0];
}

void CoreNativeBridge::ApplicationDidChangeScreenParameters()
{
    Logger::Debug("****** CoreNativeBridge::ApplicationDidChangeScreenParameters");
}

void CoreNativeBridge::ApplicationDidBecomeActive()
{
}

void CoreNativeBridge::ApplicationDidResignActive()
{
}

void CoreNativeBridge::ApplicationDidHide()
{
    core->didHideUnhide.Emit(true);
}

void CoreNativeBridge::ApplicationDidUnhide()
{
    core->didHideUnhide.Emit(false);
}

bool CoreNativeBridge::ApplicationShouldTerminate()
{
    if (!quitSent)
    {
        core->engineBackend->PostAppTerminate();
        return false;
    }
    return true;
}

bool CoreNativeBridge::ApplicationShouldTerminateAfterLastWindowClosed()
{
    return false;
}

void CoreNativeBridge::ApplicationWillTerminate()
{
    [frameTimer cancel];

    core->engineBackend->OnGameLoopStopped();

    [[NSApplication sharedApplication] setDelegate:nil];
    [appDelegate release];
    [frameTimer release];

    int exitCode = core->engineBackend->GetExitCode();
    core->engineBackend->OnBeforeTerminate();
    std::exit(exitCode);
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_MACOS__
#endif // __DAVAENGINE_COREV2__
