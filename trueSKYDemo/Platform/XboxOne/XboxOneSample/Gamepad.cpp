#include "Gamepad.h"

const float THUMBSTICK_DEADZONE  = 0.24f; 
inline static float ConvertThumbstickValue( float thumbstickValue, float deadZone )
{
    if( thumbstickValue > +deadZone )
        return ( thumbstickValue - deadZone ) / ( 1.0f - deadZone );
    if( thumbstickValue < -deadZone )
        return ( thumbstickValue + deadZone ) / ( 1.0f - deadZone );
    return 0.0f;
}

GamepadManager::GamepadManager()
	:dx(0),dy(0)
{
}

void GamepadManager::InitializeCurrentGamepad()
{
    m_leftMotorSpeed  = 0.0f;
    m_rightMotorSpeed = 0.0f;
    m_leftTriggerLevel  = 0.0f;
    m_rightTriggerLevel = 0.0f;


    m_currentGamepad = GamepadManager::GetMostRecentGamepad();
    m_currentGamepadNeedsRefresh = false;

    if( m_currentGamepad )
    {
        m_reading = m_currentGamepad->GetCurrentReading();

        m_leftThumbstickDeadzone  = THUMBSTICK_DEADZONE;
        m_rightThumbstickDeadzone = THUMBSTICK_DEADZONE;

        ZeroMemory(&m_vibration, sizeof(GamepadVibration));

        m_dPadPressedEvents = 0;
        m_dPadStates = 0;
    }

    Gamepad::GamepadAdded += ref new EventHandler<GamepadAddedEventArgs^ >( [=]( Platform::Object^ , GamepadAddedEventArgs^ args )
    {
        m_currentGamepadNeedsRefresh = true;
    } );

    Gamepad::GamepadRemoved += ref new EventHandler<GamepadRemovedEventArgs^ >( [=]( Platform::Object^ , GamepadRemovedEventArgs^ args )
    {
        m_currentGamepadNeedsRefresh = true;
    } );
}

void GamepadManager::ShutdownCurrentGamepad()
{
    if( m_currentGamepad && GamepadManager::IsGamepadValid( m_currentGamepad ) )
    {
        GamepadVibration vibration = { 0 };
        m_currentGamepad->SetVibration( vibration );
    }
}

void GamepadManager::UpdateDPadStatesAndEvents()
{
    DWORD dPadStates = 0;

    if( m_reading->IsDPadDownPressed )
    {
        dPadStates |= (DWORD)GamepadButtons::DPadDown;
    }

    if( m_reading->IsDPadUpPressed )
    {
        dPadStates |= (DWORD)GamepadButtons::DPadUp;
    }

    if( m_reading->IsDPadLeftPressed )
    {
        dPadStates |= (DWORD)GamepadButtons::DPadLeft;
    }

    if( m_reading->IsDPadRightPressed )
    {
        dPadStates |= (DWORD)GamepadButtons::DPadRight;
    }

    m_dPadPressedEvents = ( m_dPadStates ^ dPadStates ) & dPadStates;
    m_dPadStates = dPadStates;
}

void GamepadManager::Update(  float timeDelta )
{
    if( m_currentGamepadNeedsRefresh )
    {
        auto mostRecentGamepad = GamepadManager::GetMostRecentGamepad();
        if( m_currentGamepad != mostRecentGamepad )
        {
            ShutdownCurrentGamepad();
            m_currentGamepad = mostRecentGamepad;
            InitializeCurrentGamepad();
        }
        m_currentGamepadNeedsRefresh = false;
    }

    // Bail out if no controller present
    if( m_currentGamepad == nullptr )
    {
        return;
    }

    m_reading = m_currentGamepad->GetCurrentReading();
    UpdateDPadStatesAndEvents();

    m_leftMotorSpeed = 0;
    m_leftTriggerLevel = 0;
    m_rightMotorSpeed = 0;
    m_rightTriggerLevel = 0;
	
    dx = ConvertThumbstickValue( m_reading->RightThumbstickX, 8689 / 32768.0f ) * timeDelta / -2.0f;
    dy = ConvertThumbstickValue( m_reading->RightThumbstickY, 8689 / 32768.0f ) * timeDelta / -2.0f;

    m_vibration.LeftMotorLevel  = m_leftMotorSpeed;
    m_vibration.RightMotorLevel = m_rightMotorSpeed;
    m_vibration.LeftTriggerLevel  = m_leftTriggerLevel;
    m_vibration.RightTriggerLevel = m_rightTriggerLevel;

}