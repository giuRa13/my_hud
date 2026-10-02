#ifndef LMU_TELEMETRY_H
#define LMU_TELEMETRY_H

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <utility>
#include <cmath>
#include <algorithm>
#include <optional>
#include <shared/SharedmemoryInterface/InternalsPlugin.hpp>
#include <shared/SharedmemoryInterface/SharedMemoryInterface.hpp>   

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
        if (!p_Layout)
        {
            h_MapFile = OpenFileMappingA(FILE_MAP_READ, FALSE, LMU_SHARED_MEMORY_FILE);
            if (!h_MapFile) return;

            p_Layout = (SharedMemoryLayout*)MapViewOfFile(h_MapFile, FILE_MAP_READ, 0, 0, sizeof(SharedMemoryLayout));
            if (!p_Layout) { CloseHandle(h_MapFile); h_MapFile = nullptr; return; }
        }

        if (!lock)
        {
            lock = SharedMemoryLock::MakeSharedMemoryLock();
            if (!lock) return;
        }

        // never block the render thread: try briefly, keep last good copy on failure
        if (!lock->TryLockSpin(1000)) return;

        const SharedMemoryTelemetryData& t = p_Layout->data.telemetry;
        if (t.playerHasVehicle && t.playerVehicleIdx < 104)
        {
            m_Telem = t.telemInfo[t.playerVehicleIdx];
            m_Valid = true;
        }
        else
        {
            m_Valid = false;
        }

        // scoring section: per-vehicle lap/sector info, needed for lap history + last-lap delta
        const SharedMemoryScoringData& s = p_Layout->data.scoring;
        m_ScoreValid = false;
        long n = (std::min)((long)s.scoringInfo.mNumVehicles, (long)104);
        for (long i = 0; i < n; ++i)
        {
            if (s.vehScoringInfo[i].mIsPlayer)
            {
                m_LapDist    = (float)s.vehScoringInfo[i].mLapDist;
                m_TrackLen   = (float)s.scoringInfo.mLapDist;
                m_ScoreVeh   = s.vehScoringInfo[i];
                m_ScoreValid = true;
                break;
            }
        }

        lock->Unlock();
    }

    void shutdown()
    {
        if (p_Layout) { UnmapViewOfFile(p_Layout); p_Layout = nullptr; }
        if (h_MapFile) { CloseHandle(h_MapFile); h_MapFile = nullptr; }
        lock.reset();       // closes the lock handles; update() recreates it when needed
        m_Valid = false;    // never serve stale data after a shutdown
        m_ScoreValid = false;
    }

    float get_throttle() { return m_Valid ? (float)m_Telem.mUnfilteredThrottle : 0.0f; }
    float get_brake()    { return m_Valid ? (float)m_Telem.mUnfilteredBrake    : 0.0f; }
    float get_clutch()   { return m_Valid ? (float)m_Telem.mUnfilteredClutch   : 0.0f; }
    float get_ffb()      { return m_Valid ? (float)m_Telem.mUnfilteredSteering : 0.0f; }
    int   get_gear()     { return m_Valid ? (int)m_Telem.mGear : 0; }
    float get_RPM()      { return m_Valid ? (float)m_Telem.mEngineRPM : 0.0f; }
    float get_delta_best()  { return m_Valid ? (float)m_Telem.mDeltaBest : 0.0f; }
    float get_battery_pct() { return m_Valid ? (float)m_Telem.mBatteryChargeFraction : 0.0f; }
    float get_max_rpm()
    {
        return (m_Valid && m_Telem.mEngineMaxRPM > 0.0) ? (float)m_Telem.mEngineMaxRPM : 8500.0f;
    }

    float get_speed_kmh()
    {
        if (!m_Valid) return 0.0f;
        double vx = m_Telem.mLocalVel.x, vy = m_Telem.mLocalVel.y, vz = m_Telem.mLocalVel.z;
        return (float)(std::sqrt(vx*vx + vy*vy + vz*vz) * 3.6);
    }

    float get_steering() { return m_Valid ? (float)m_Telem.mUnfilteredSteering : 0.0f; }   // -1..1
    float get_wheel_range_deg()   // total lock-to-lock rotation of the wheel
    {
        if (!m_Valid) return 540.0f;
        float r = m_Telem.mVisualSteeringWheelRange;
        if (r <= 0.0f) return 540.0f;
        if (r < 20.0f) r *= 57.2957795f;   // handles the value being in radians
        return r;
    }

    long   get_lap_number()      { return m_Valid ? m_Telem.mLapNumber : -1; }
    double get_lap_start_et()    { return m_Valid ? m_Telem.mLapStartET : 0.0; }
    bool   get_lap_invalidated() { return m_Valid ? m_Telem.mLapInvalidated : true; }
    float  get_cur_sector1()     { return m_ScoreValid ? (float)m_ScoreVeh.mCurSector1 : -1.0f; } // not used now
    float  get_cur_sector2()     { return m_ScoreValid ? (float)m_ScoreVeh.mCurSector2 : -1.0f; } // not used now
    long   get_current_sector_raw() { return m_Valid ? m_Telem.mCurrentSector : -1; }
    double get_elapsed_time()       { return m_Valid ? m_Telem.mElapsedTime : 0.0; }

    std::string get_track_name()     { return m_Valid ? std::string(m_Telem.mTrackName) : ""; }
    std::string get_vehicle_class()  { return m_ScoreValid ? std::string(m_ScoreVeh.mVehicleClass) : ""; }
    float get_last_sector1()         { return m_ScoreValid ? (float)m_ScoreVeh.mLastSector1 : -1.0f; }
    float get_last_sector2()         { return m_ScoreValid ? (float)m_ScoreVeh.mLastSector2 : -1.0f; }
    float get_last_lap_time()        { return m_ScoreValid ? (float)m_ScoreVeh.mLastLapTime : -1.0f; }
    bool  get_in_pits()               { return m_ScoreValid ? m_ScoreVeh.mInPits : true; }
    unsigned char get_count_lap_flag(){ return m_ScoreValid ? m_ScoreVeh.mCountLapFlag : 0; }

    float get_lap_dist()   { return m_ScoreValid ? m_LapDist  : 0.0f; }
    float get_track_len()  { return m_ScoreValid ? m_TrackLen : 0.0f; }

private:
    HANDLE h_MapFile = nullptr;
    SharedMemoryLayout* p_Layout = nullptr;
    std::optional<SharedMemoryLock> lock;
    TelemInfoV01 m_Telem{};
    bool m_Valid = false;

    VehicleScoringInfoV01 m_ScoreVeh{};
    bool  m_ScoreValid = false;
    float m_LapDist = 0.0f;
    float m_TrackLen = 0.0f;
};

#endif