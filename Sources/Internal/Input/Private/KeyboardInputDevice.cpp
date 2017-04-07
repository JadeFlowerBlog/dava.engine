#include "Input/KeyboardInputDevice.h"

#if defined(__DAVAENGINE_WIN32__)
#include "Input/Private/Win32/KeyboardDeviceImplWin32.h"
#elif defined(__DAVAENGINE_WIN_UAP__)
#include "Input/Private/Win10/KeyboardDeviceImplWin10.h"
#elif defined(__DAVAENGINE_MACOS__)
#include "Input/Private/Mac/KeyboardDeviceImplMac.h"
#elif defined(__DAVAENGINE_ANDROID__)
#include "Input/Private/Android/KeyboardDeviceImplAndroid.h"
#elif defined(__DAVAENGINE_IPHONE__)
#include "Input/Private/Ios/KeyboardDeviceImplIos.h"
#else
#error "KeyboardDevice: unknown platform"
#endif

#include "Engine/Engine.h"
#include "Engine/Private/Dispatcher/MainDispatcherEvent.h"
#include "Input/InputSystem.h"
#include "Time/SystemTimer.h"

namespace DAVA
{
KeyboardInputDevice::KeyboardInputDevice(uint32 id)
    : InputDevice(id)
    , inputSystem(GetEngineContext()->inputSystem)
    , impl(new Private::KeyboardDeviceImpl())
{
    Engine* engine = Engine::Instance();
    engine->endFrame.Connect(this, &KeyboardInputDevice::OnEndFrame);
    engine->PrimaryWindow()->focusChanged.Connect(this, &KeyboardInputDevice::OnWindowFocusChanged); // TODO: handle all the windows

    Private::EngineBackend::Instance()->InstallEventFilter(this, MakeFunction(this, &KeyboardInputDevice::HandleMainDispatcherEvent));
}

KeyboardInputDevice::~KeyboardInputDevice()
{
    Engine* engine = Engine::Instance();
    engine->endFrame.Disconnect(this);
    engine->PrimaryWindow()->focusChanged.Disconnect(this);

    Private::EngineBackend::Instance()->UninstallEventFilter(this);

    if (impl != nullptr)
    {
        delete impl;
        impl = nullptr;
    }
}

bool KeyboardInputDevice::SupportsElement(eInputElements elementId) const
{
    return IsKeyboardInputElement(elementId);
}

eDigitalElementStates KeyboardInputDevice::GetDigitalElementState(eInputElements elementId) const
{
    DVASSERT(SupportsElement(elementId));
    return keys[elementId - eInputElements::KB_FIRST].GetState();
}

AnalogElementState KeyboardInputDevice::GetAnalogElementState(eInputElements elementId) const
{
    DVASSERT(false, "KeyboardInputDevice does not support analog elements");
    return {};
}

WideString KeyboardInputDevice::TranslateElementToWideString(eInputElements elementId) const
{
    DVASSERT(SupportsElement(elementId));
    return impl->TranslateElementToWideString(elementId);
}

void KeyboardInputDevice::OnEndFrame()
{
    // Promote JustPressed & JustReleased states to Pressed/Released accordingly
    for (size_t i = 0; i < INPUT_ELEMENTS_KB_COUNT; ++i)
    {
        keys[i].OnEndFrame();
    }
}

void KeyboardInputDevice::OnWindowFocusChanged(DAVA::Window* window, bool focused)
{
    // Reset keyboard state when window is unfocused
    if (!focused)
    {
        for (size_t i = 0; i < INPUT_ELEMENTS_KB_COUNT; ++i)
        {
            if (keys[i].IsPressed())
            {
                keys[i].Release();

                // Generate release event
                eInputElements elementId = static_cast<eInputElements>(eInputElements::KB_FIRST + i);
                CreateAndSendInputEvent(elementId, keys[i], window, SystemTimer::GetMs());
            }
        }
    }
}

bool KeyboardInputDevice::HandleMainDispatcherEvent(const Private::MainDispatcherEvent& e)
{
    using Private::MainDispatcherEvent;

    if (e.type == MainDispatcherEvent::KEY_DOWN || e.type == MainDispatcherEvent::KEY_UP)
    {
        eInputElements elementId = impl->ConvertNativeScancodeToDavaScancode(e.keyEvent.key);
        if (elementId == eInputElements::NONE)
        {
            DVASSERT(false, "Couldn't map native scancode to dava scancode");
            return false;
        }

        // Update element state

        Private::DigitalElement& element = keys[elementId - eInputElements::KB_FIRST];
        if (e.type == MainDispatcherEvent::KEY_DOWN)
        {
            element.Press();
        }
        else
        {
            element.Release();
        }

        // Send event

        CreateAndSendInputEvent(elementId, element, e.window, e.timestamp);

        return true;
    }

    return false;
}

void KeyboardInputDevice::CreateAndSendInputEvent(eInputElements elementId, const Private::DigitalElement& element, Window* window, int64 timestamp) const
{
    InputEvent inputEvent;
    inputEvent.window = window;
    inputEvent.timestamp = static_cast<float64>(timestamp / 1000.0f);
    inputEvent.deviceType = eInputDeviceTypes::KEYBOARD;
    inputEvent.deviceId = GetId();
    inputEvent.digitalState = element.GetState();
    inputEvent.elementId = elementId;

    inputSystem->DispatchInputEvent(inputEvent);
}

} // namespace DAVA
