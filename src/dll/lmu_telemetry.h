// handles the connection to LMU's memory-mapped files ($rF2Physics$) to pull live values
#ifndef LMU_TELEMETRY_H
#define LMU_TELEMETRY_H

#include <windows.h>

// rFactor 2 and LMU expect structures to be packed on 4-byte boundaries (#pragma pack( push, 4 ))
#pragma pack( push, 4 )

// simplified structure layout matching rF2PhysicsV01 for essential pedals
struct TelemVect3 {
    double x, y, z;
};

// exact official mapping matching TelemInfoV01 from InternalPlugin.hpp
struct TelemInfoV01 {
    long mID;                      
    double mDeltaTime;             
    double mElapsedTime;           
    long mLapNumber;               
    double mLapStartET;            
    char mVehicleName[64];         
    char mTrackName[64];           

    TelemVect3 mPos;               
    TelemVect3 mLocalVel;          
    TelemVect3 mLocalAccel;        

    TelemVect3 mOri[3];            
    TelemVect3 mLocalRot;          
    TelemVect3 mLocalRotAccel;     

    long mGear;                    // -1=reverse, 0=neutral, 1+=forward gears
    double mEngineRPM;             
    double mEngineWaterTemp;       
    double mEngineOilTemp;         
    double mClutchRPM;             

    // Driver input
    double mUnfilteredThrottle;    // ranges  0.0-1.0
    double mUnfilteredBrake;       // ranges  0.0-1.0
    double mUnfilteredSteering;    
    double mUnfilteredClutch;      

    double mFilteredThrottle;      
    double mFilteredBrake;         
    double mFilteredSteering;      
    double mFilteredClutch;        
    
    // ... don't need to read past here for basic pedals/gear, 
    // but the struct size must match or map safely up to this point
};

#pragma pack( pop )

class LMUTelemetry 
{
public:
    static LMUTelemetry& get()
    {
        static LMUTelemetry instance;
        return instance;
    }

    void update()
    {
        if (!h_MapFile) 
        {
            // LMU official plugin uses this exact buffer name
            h_MapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, "$rFactor2SMMP_Telemetry$");
            if (h_MapFile) 
            {
                p_Telemetry = (TelemInfoV01*)MapViewOfFile(h_MapFile, FILE_MAP_READ, 0, 0, sizeof(TelemInfoV01));
            }
        }
    }

    float get_throttle() { return p_Telemetry ? (float)p_Telemetry->mUnfilteredThrottle : 0.0f; }
    float get_brake()    { return p_Telemetry ? (float)p_Telemetry->mUnfilteredBrake : 0.0f; }
    float get_clutch()    { return p_Telemetry ? (float)p_Telemetry->mUnfilteredClutch : 0.0f; }
    float get_ffb()    { return p_Telemetry ? (float)p_Telemetry->mUnfilteredSteering : 0.0f; }
    int   get_gear()     { return p_Telemetry ? (int)p_Telemetry->mGear : 0; }
    float get_RPM()      { return p_Telemetry ? (float)p_Telemetry->mEngineRPM : 0.0f; }

    void shutdown() 
    {
        if (p_Telemetry) { UnmapViewOfFile(p_Telemetry); p_Telemetry = nullptr; }
        if (h_MapFile) CloseHandle(h_MapFile);
    }

private:
    HANDLE h_MapFile = nullptr;
    TelemInfoV01* p_Telemetry = nullptr;
};

#endif