#pragma once
#include <xdk.h>
#include <wrl.h>
#include <atomic>
using namespace Windows::Xbox::Input;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
//--------------------------------------------------------------------------------------
// Helper class for managing the gamepads
//--------------------------------------------------------------------------------------
class GamepadManager
{
public:
	GamepadManager();
    static IGamepad^ GetMostRecentGamepad()
    {
        IGamepad^ gamepad = nullptr;

        IVectorView< IGamepad^ >^ allGamepads = Gamepad::Gamepads;

        if( allGamepads->Size > 0 )
        {
            gamepad = allGamepads->GetAt( 0 );
        }

        return gamepad;
    }

    static bool IsGamepadValid( IGamepad^ gamepad )
    {
        IVectorView<IGamepad^>^ gamepads = Gamepad::Gamepads;

        IIterator<IGamepad^>^ it = gamepads->First();

        while( it->HasCurrent )
        {
            if( gamepad == it->Current )
            {
                return true;
            }

            it->MoveNext();
        }

        return false;
    }

    void InitializeCurrentGamepad();
    void ShutdownCurrentGamepad();
	void UpdateDPadStatesAndEvents();
	void Update(float timeDelta );

    // variables associated with gamepad currently in use
    std::atomic< bool > m_currentGamepadNeedsRefresh;
    IGamepad^           m_currentGamepad;
    IGamepadReading^    m_reading;
    float               m_leftThumbstickDeadzone;
    float               m_rightThumbstickDeadzone;;
    float               m_leftMotorSpeed;
    float               m_rightMotorSpeed;
    float               m_leftTriggerLevel;
    float               m_rightTriggerLevel;
    GamepadVibration    m_vibration;
    DWORD               m_dPadPressedEvents;
    DWORD               m_dPadStates;
	float dx,dy;
};