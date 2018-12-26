/**
 * ============================================================================
 *
 *  Zombie Plague
 *
 *  File:          jumpboost.cpp
 *  Type:          Game 
 *  Description:   Modified jump vector magnitudes.
 *
 *  Copyright (C) 2015-2016 Nikita Ushakov (Ireland, Dublin)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================================
 **/
 
/**
 * Jumpboost module init function.
 **/
void JumpBoostInit(/*void*/)
{
    // If jump boost disabled, then unhook
    if(!gCvarList[CVAR_JUMPBOOST_ENABLE].BoolValue)
    {
        // Unhook player events
        UnhookEvent2("player_jump", JumpBoostOnClientJump, EventHookMode_Post);
        return;
    }
    
    // Hook player events
    HookEvent("player_jump", JumpBoostOnClientJump, EventHookMode_Post);
}

/**
 * Hook jump boost cvar changes.
 **/
void JumpBoostOnCvarInit(/*void*/)
{
    // Create boost cvars
    gCvarList[CVAR_JUMPBOOST_ENABLE]            = FindConVar("zp_jumpboost_enable");
    gCvarList[CVAR_JUMPBOOST_MULTIPLIER]        = FindConVar("zp_jumpboost_multiplier");
    gCvarList[CVAR_JUMPBOOST_MAX]               = FindConVar("zp_jumpboost_max"); 
    
    // Create leap cvars
    gCvarList[CVAR_LEAP_ZOMBIE]                 = FindConVar("zp_leap_zombies");
    gCvarList[CVAR_LEAP_ZOMBIE_FORCE]           = FindConVar("zp_leap_zombies_force");
    gCvarList[CVAR_LEAP_ZOMBIE_COUNTDOWN]       = FindConVar("zp_leap_zombies_cooldown");
    gCvarList[CVAR_LEAP_NEMESIS]                = FindConVar("zp_leap_nemesis");
    gCvarList[CVAR_LEAP_NEMESIS_FORCE]          = FindConVar("zp_leap_nemesis_force");
    gCvarList[CVAR_LEAP_NEMESIS_COUNTDOWN]      = FindConVar("zp_leap_nemesis_cooldown");
    gCvarList[CVAR_LEAP_SURVIVOR]               = FindConVar("zp_leap_survivor");
    gCvarList[CVAR_LEAP_SURVIVOR_FORCE]         = FindConVar("zp_leap_survivor_force");
    gCvarList[CVAR_LEAP_SURVIVOR_COUNTDOWN]     = FindConVar("zp_leap_survivor_cooldown");

    // Hook cvars
    HookConVarChange(gCvarList[CVAR_JUMPBOOST_ENABLE],            JumpBoostCvarsHookEnable);
}

/**
 * Cvar hook callback (zp_jumpboost_enable)
 * Jumpboost module initialization.
 * 
 * @param hConVar           The cvar handle.
 * @param oldValue          The value before the attempted change.
 * @param newValue          The new value.
 **/
public void JumpBoostCvarsHookEnable(ConVar hConVar, const char[] oldValue, const char[] newValue)
{
    // Validate new value
    if(oldValue[0] == newValue[0])
    {
        return;
    }
    
    // Forward event to modules
    JumpBoostInit();
}

/**
 * Event callback (player_jump)
 * Client has been jumped.
 * 
 * @param gEventHook        The event handle.
 * @param gEventName        The name of the event.
 * @param dontBroadcast     If true, event is broadcasted to all clients, false if not.
 **/
public Action JumpBoostOnClientJump(Event hEvent, const char[] sName, bool dontBroadcast) 
{
    // Gets all required event info
    int clientIndex = GetClientOfUserId(hEvent.GetInt("userid"));

    // Creates a single use next frame hook
    RequestFrame(view_as<RequestFrameCallback>(JumpBoostOnClientJumpPost), GetClientUserId(clientIndex));
}
 
/**
 * Client is joining the server.
 * 
 * @param clientIndex       The client index.
 **/
void JumpBoostClientInit(const int clientIndex)
{
    // Hook entity callbacks
    SDKHook(clientIndex, SDKHook_GroundEntChangedPost, JumpBoostGroundEntChangedPost);
}

/**
 * Hook: GroundEntChangedPost
 * Called right before the entities touching ground.
 * 
 * @param clientIndex       The client index.
 **/
public void JumpBoostGroundEntChangedPost(const int clientIndex)
{
    // Verify that the client is exist
    if(!IsPlayerExist(clientIndex))
    {
        return;
    }

    // If not on the ground, then stop
    if(!(GetEntityFlags(clientIndex) & FL_ONGROUND))
    {
        return;
    }
    
    // Validate movetype
    if(GetEntityMoveType(clientIndex) != MOVETYPE_LADDER)
    {
        // Is zombie ?
        if(gClientData[clientIndex][Client_Zombie])
        {
            // Reset gravity
            ToolsSetClientGravity(clientIndex, gClientData[clientIndex][Client_Nemesis] ? gCvarList[CVAR_NEMESIS_GRAVITY].FloatValue : ZombieGetGravity(gClientData[clientIndex][Client_ZombieClass]) + (gCvarList[CVAR_LEVEL_SYSTEM].BoolValue ? (gCvarList[CVAR_LEVEL_GRAVITY_RATIO].FloatValue * float(gClientData[clientIndex][Client_Level])) : 0.0));
        }   
        else
        {
            // Reset gravity
            ToolsSetClientGravity(clientIndex, gClientData[clientIndex][Client_Survivor] ? gCvarList[CVAR_SURVIVOR_GRAVITY].FloatValue : HumanGetGravity(gClientData[clientIndex][Client_HumanClass]) + (gCvarList[CVAR_LEVEL_SYSTEM].BoolValue ? (gCvarList[CVAR_LEVEL_GRAVITY_RATIO].FloatValue * float(gClientData[clientIndex][Client_Level])) : 0.0));
        }
    }
}

/**
 * Client is jumping. *(Post)
 *
 * @param userID            The user id.
 **/
public void JumpBoostOnClientJumpPost(const int userID)
{
    // Gets the client index from the user ID
    int clientIndex = GetClientOfUserId(userID);

    // Validate client
    if(clientIndex)
    {
        // Initialize velocity vector
        static float vVelocity[3];
        
        // Gets the client velocity
        ToolsGetClientVelocity(clientIndex, vVelocity);
        
        // Only apply horizontal multiplier if it not a bhop
        if(SquareRoot(Pow(vVelocity[0], 2.0) + Pow(vVelocity[1], 2.0)) < gCvarList[CVAR_JUMPBOOST_MAX].FloatValue)
        {
            // Apply horizontal multipliers to jump vector
            vVelocity[0] *= gCvarList[CVAR_JUMPBOOST_MULTIPLIER].FloatValue;
            vVelocity[1] *= gCvarList[CVAR_JUMPBOOST_MULTIPLIER].FloatValue;
        }

        // Apply height multiplier to jump vector
        vVelocity[2] *= gCvarList[CVAR_JUMPBOOST_MULTIPLIER].FloatValue;

        // Set new velocity
        ToolsClientVelocity(clientIndex, vVelocity, true, false);
    }
}

/**
 * Called when player want do the leap jump.
 *
 * @param clientIndex       The client index.
 **/
void JumpBoostOnClientLeapJump(const int clientIndex)
{
    // If not on the ground, then stop
    if(!(GetEntityFlags(clientIndex) & FL_ONGROUND))
    {
        return;
    }

    //*********************************************************************
    //*                    INITIALIZE LEAP JUMP PROPERTIES                *
    //*********************************************************************
    
    // Initialize variable
    static float flCountDown;
    
    // Verify that the client is zombie
    if(gClientData[clientIndex][Client_Zombie])
    {
        // Verify that the client is nemesis
        if(gClientData[clientIndex][Client_Nemesis])
        {
            // If nemesis leap disabled, then stop
            if(!gCvarList[CVAR_LEAP_NEMESIS].BoolValue) 
            {
                return;
            }
            
            // Gets countdown time
            flCountDown = gCvarList[CVAR_LEAP_NEMESIS_COUNTDOWN].FloatValue;
        }
        
        // If not
        else
        {
            // Switch type of leap jump
            switch(gCvarList[CVAR_LEAP_ZOMBIE].IntValue)
            {
                // If zombie leap disabled, then stop
                case 0 :
                {
                    return;
                }
                // If zombie leap just for single zombie
                case 2 :
                {
                    if(fnGetZombies() > 1) 
                    {
                        return;
                    }
                }
            }
            
            // Gets countdown time
            flCountDown = gCvarList[CVAR_LEAP_ZOMBIE_COUNTDOWN].FloatValue;
        }
    }
    
    // If not
    else
    {
        // Verify that the client is survivor
        if(gClientData[clientIndex][Client_Survivor])
        {
            // If survivor leap disabled, then stop
            if(!gCvarList[CVAR_LEAP_SURVIVOR].BoolValue)
            {
                return;
            }
            
            // Gets countdown time
            flCountDown = gCvarList[CVAR_LEAP_SURVIVOR_COUNTDOWN].FloatValue;
        }
        
        // If player is human, stop
        else return;
    }
    
    //*********************************************************************
    //*                     CHECK DELAY OF THE LEAP JUMP                  *
    //*********************************************************************
    
    // Initialize variable
    static float flDelay[MAXPLAYERS+1];
    
    // Gets the simulated game time
    float flCurrentTime = GetTickedTime();
    
    // Cooldown don't over yet, then stop
    if(flCurrentTime - flDelay[clientIndex] < flCountDown)
    {
        return;
    }
    
    // Update the leap jump delay
    flDelay[clientIndex] = flCurrentTime;
    
    //*********************************************************************
    //*                        DO THE LEAP JUMP                           *
    //*********************************************************************
    
    // Initialize some floats
    static float vAngle[3]; static float vOrigin[3]; static float vVelocity[3];
    
    // Gets client location and view direction
    GetClientAbsOrigin(clientIndex, vOrigin);
    GetClientEyeAngles(clientIndex, vAngle);
    
    // Store zero angle
    float flAngleZero = vAngle[0];    
    
    // Gets location angles
    vAngle[0] = -30.0;
    GetAngleVectors(vAngle, vVelocity, NULL_VECTOR, NULL_VECTOR);
    
    // Scale vector for the boost
    ScaleVector(vVelocity, gClientData[clientIndex][Client_Survivor] ? gCvarList[CVAR_LEAP_SURVIVOR_FORCE].FloatValue : (gClientData[clientIndex][Client_Nemesis] ? gCvarList[CVAR_LEAP_NEMESIS_FORCE].FloatValue : gCvarList[CVAR_LEAP_ZOMBIE_FORCE].FloatValue));
    
    // Restore eye angle
    vAngle[0] = flAngleZero;
    
    // Push the player
    TeleportEntity(clientIndex, vOrigin, vAngle, vVelocity);
    
    // Forward event to modules
    VEffectsOnClientJump(clientIndex);
}