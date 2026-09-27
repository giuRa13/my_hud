// handles the connection to LMU's memory-mapped files ($rF2Physics$) to pull live values
#ifndef LMU_TELEMETRY_H
#define LMU_TELEMETRY_H

#include <windows.h>
#include <cmath>
#include <algorithm>

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

    // Misc
    double mSteeringShaftTorque;   
    double mFront3rdDeflection;    
    double mRear3rdDeflection;  
    
    // Aerodynamics
    double mFrontWingHeight;       
    double mFrontRideHeight;       
    double mRearRideHeight;        
    double mDrag;                  
    double mFrontDownforce;        
    double mRearDownforce;       

    // State/damage info
    double mFuel;                  // amount of fuel (liters)
    double mEngineMaxRPM;          // <--- EXACT POSITION FOR REV LIMIT
    unsigned char mScheduledStops; 
    bool  mOverheating;            
    bool  mDetached;               
    bool  mHeadlights;             
    unsigned char mDentSeverity[8];
    double mLastImpactET;          
    double mLastImpactMagnitude;   
    TelemVect3 mLastImpactPos;  

    // Expanded fields
    double mEngineTorque;          
    long mCurrentSector;           
    unsigned char mSpeedLimiter;   
    unsigned char mMaxGears;       
    unsigned char mFrontTireCompoundIndex;   
    unsigned char mRearTireCompoundIndex;    
    double mFuelCapacity;          
    unsigned char mFrontFlapActivated;       
    unsigned char mRearFlapActivated;        
    unsigned char mRearFlapLegalStatus;      
    unsigned char mIgnitionStarter;

    char mFrontTireCompoundName[18];         
    char mRearTireCompoundName[18]; 

    unsigned char mSpeedLimiterAvailable;    
    unsigned char mAntiStallActivated;       
    unsigned char mUnused[2];                
    float mVisualSteeringWheelRange;  

    double mRearBrakeBias;                   
    double mTurboBoostPressure;              
    float mPhysicsToGraphicsOffset[3];       
    float mPhysicalSteeringWheelRange;  

    // deltabest
    double mDeltaBest;
     
    double mBatteryChargeFraction; // <--- EXACT POSITION FOR HYBRID BATTERY [0.0-1.0]
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

    // calculates speed in km/h from local velocity vector (mLocalVel.z is forward/backward in rF2 vehicle coords)
    float get_speed_kmh()
    {
        if (!p_Telemetry) return 0.0f;

        // Magnitude of velocity vector: sqrt(x^2 + y^2 + z^2) meters per second
        double vx = p_Telemetry->mLocalVel.x;
        double vy = p_Telemetry->mLocalVel.y;
        double vz = p_Telemetry->mLocalVel.z;
        double speed_ms = std::sqrt(vx * vx + vy * vy + vz * vz);
        return (float)(speed_ms * 3.6); // Convert m/s to km/h
    }

    void shutdown() 
    {
        if (p_Telemetry) { UnmapViewOfFile(p_Telemetry); p_Telemetry = nullptr; }
        if (h_MapFile) CloseHandle(h_MapFile);
    }

    float get_battery_pct() 
    {
        // mBatteryChargeFraction is natively [0.0 - 1.0]
        return p_Telemetry ? (float)p_Telemetry->mBatteryChargeFraction : 1.0f; 
    }

    float get_max_rpm() 
    {
        // mEngineMaxRPM gives the correct rev limit for the current car model
        return (p_Telemetry && p_Telemetry->mEngineMaxRPM > 0.0) ? (float)p_Telemetry->mEngineMaxRPM : 8500.0f; 
    }

private:
    HANDLE h_MapFile = nullptr;
    TelemInfoV01* p_Telemetry = nullptr;
};

#endif