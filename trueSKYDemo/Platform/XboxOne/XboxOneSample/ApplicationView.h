//
// ApplicationView.h
//

#pragma once

#include "pch.h"
#include "Game.h"


// Application - implements the required functionality for an application
ref class ApplicationView sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
public:

    ApplicationView();

    // IFrameworkView Methods
    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
    virtual void Load(Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

protected:

    // Event Handlers
    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
    void OnResuming(Platform::Object^ sender, Platform::Object^ args);
    void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

private:

    Game^ m_game;
    bool m_windowClosed;
};

// ApplicationSource - responsible for creating the Application instance
// and passing it back to the system
ref class ApplicationViewSource : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
