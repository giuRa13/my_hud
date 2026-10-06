#ifndef LAP_HISTORY_H
#define LAP_HISTORY_H

#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <shared/config.h>
#include <dll/lmu_telemetry.h>

struct LapRecord
{
    int   session_number = 0;
    int   lap_number = 0;
    float lap_time = 0.0f;
    float sector1 = 0.0f;
    float sector2 = 0.0f;
    float sector3 = 0.0f;
};

struct TraceSample { 
    float dist; 
    float time; 
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// Laps
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LapHistory
{
public:
    static LapHistory& get()
    {
        static LapHistory instance;
        return instance;
    }

    void set_module(HMODULE h) { m_hModule = h; }

    void update()
    {
        std::string track = LMUTelemetry::get().get_track_name();
        std::string car   = LMUTelemetry::get().get_vehicle_class();
        if (track.empty() || car.empty()) 
        {
            load_design_mode();
            return;
        }

        std::string key = sanitize(track) + "__" + sanitize(car);
        if (key != m_current_key)
        {
            m_current_key = key;
            load();
            load_best_trace();
            m_has_prev = false; // out lap: false on first frame, no record
            m_session_started = false; // this run hasn't claimed a session number yet
            m_cur_trace.clear();
        }

        float last_lap_time = LMUTelemetry::get().get_last_lap_time(); 
        long  cur_lap_number = LMUTelemetry::get().get_lap_number();

        // record a distance/time sample for the lap in progress
        float dist = LMUTelemetry::get().get_lap_dist();
        float track_len = LMUTelemetry::get().get_track_len();
        double elapsed = LMUTelemetry::get().get_elapsed_time();
        double lap_start = LMUTelemetry::get().get_lap_start_et();
        if (dist >= 0.0f && track_len > 0.0f)
        {
            float t = (float)(elapsed - lap_start);
            // reject a stale carryover reading right after a lap rollover: early in the lap,
            // a large dist almost certainly belongs to the previous lap, not this one
            bool plausible = m_cur_trace.empty()
                ? (t > 2.0f || dist < 100.0f)   // first sample of the lap: only trust it if early-time+small-dist, or we're already well into the lap
                : (dist > m_cur_trace.back().dist + 0.5f);

            if (plausible)
                m_cur_trace.push_back({ dist, t });
        }

        if (m_has_prev && last_lap_time > 0.0f &&
            std::abs(last_lap_time - m_prev_last_lap_time) > 0.001f)
        {
            float s1    = LMUTelemetry::get().get_last_sector1();
            float s2raw = LMUTelemetry::get().get_last_sector2(); // cumulative s1+s2

            if (s1 > 0.0f && s2raw > 0.0f)
            {
                if (!m_session_started)
                {
                    m_current_session = m_highest_session_on_disk + 1;
                    m_session_started = true;
                }

                LapRecord r;
                r.session_number = m_current_session;
                r.lap_number = (int)cur_lap_number; //-1; // computed fresh this frame, not carried over
                r.lap_time   = last_lap_time;
                r.sector1    = s1;
                r.sector2    = s2raw - s1;
                r.sector3    = last_lap_time - s2raw;

                m_laps.push_back(r);
                update_bests(r);
                append_to_file(r);

                // if this lap just became the new all-time best, persist its trace —
                // but only if the recorded trace is internally well-formed
                if (last_lap_time <= m_best_lap + 0.0001f && !m_cur_trace.empty() && is_trace_sane(m_cur_trace))
                {
                    m_best_trace = m_cur_trace;
                    save_best_trace();
                }
            }
            m_cur_trace.clear(); // start fresh for the next lap regardless of whether this one was saved
        }

        m_prev_last_lap_time = last_lap_time;
        m_has_prev = true;
    }

    // most recent first
    std::vector<LapRecord> recent_laps(int n = 10) const
    {
        int count = (std::min)(n, (int)m_laps.size());
        return std::vector<LapRecord>(m_laps.rbegin(), m_laps.rbegin() + count);
    }

    // scan the full stored history, not the display slice
    float session_best_lap(int session_number) const
    {
        float best = -1.0f;
        for (const auto& l : m_laps)
            if (l.session_number == session_number && (best < 0.0f || l.lap_time < best))
                best = l.lap_time;

        return best;
    }

    // scan the full stored history, not the display slice
    float session_best_sector(int session_number, int i) const
    {
        if (i < 0 || i > 2) return -1.0f;
        float best = -1.0f;
        for (const auto& l : m_laps)
        {
            float v = (i == 0) ? l.sector1 : (i == 1 ? l.sector2 : l.sector3);
            if (l.session_number == session_number && (best < 0.0f || v < best))
                best = v;
        }
        return best;
    }

    float all_time_best_lap() const { return m_best_lap; }
    float all_time_best_sector(int i) const { return (i >= 0 && i < 3) ? m_best_sector[i] : -1.0f; }
    int current_session_number() const { return m_current_session; }
    const std::vector<TraceSample>& best_lap_trace() const { return m_best_trace; }

    std::string trace_file_path() const
    {
        return config::get_module_dir(m_hModule) + "\\laptimes\\" + m_current_key + "__besttrace.csv";
    }

    void save_best_trace()
    {
        std::string dir = config::get_module_dir(m_hModule) + "\\laptimes";
        CreateDirectoryA(dir.c_str(), nullptr);

        std::ofstream file(trace_file_path()); // overwrite, not append — always the single current best
        if (!file.is_open()) return;
        file << "dist,time\n";
        for (const auto& s : m_best_trace)
            file << s.dist << "," << s.time << "\n";
    }

    void load_best_trace()
    {
        m_best_trace.clear();
        std::ifstream file(trace_file_path());
        if (!file.is_open()) return;
        std::string line;
        std::getline(file, line); // header
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string tok;
            TraceSample s;
            if (!std::getline(ss, tok, ',')) continue; s.dist = std::stof(tok);
            if (!std::getline(ss, tok, ',')) continue; s.time = std::stof(tok);
            m_best_trace.push_back(s);
        }
    }

private:
    HMODULE m_hModule = nullptr;
    std::string m_current_key;
    std::vector<LapRecord> m_laps;

    bool  m_has_prev = false;
    float m_prev_last_lap_time = -1.0f;

    bool m_session_started = false;
    int  m_current_session = 0;
    int  m_highest_session_on_disk = 0;

    float m_best_lap = -1.0f;
    float m_best_sector[3] = { -1.0f, -1.0f, -1.0f };

    std::vector<TraceSample> m_cur_trace;
    std::vector<TraceSample> m_best_trace;

    static std::string sanitize(std::string s)
    {
        for (char& c : s)
            if (!std::isalnum((unsigned char)c)) c = '_';
        return s;
    }

    std::string file_path() const
    {
        return config::get_module_dir(m_hModule) + "\\laptimes\\" + m_current_key + ".csv";
    }

    void load()
    {
        m_laps.clear();
        m_best_lap = -1.0f;
        m_best_sector[0] = m_best_sector[1] = m_best_sector[2] = -1.0f;
        m_highest_session_on_disk = 0;

        std::ifstream file(file_path());
        if (file.is_open())
        {
            std::string line;
            std::getline(file, line); // skip header
            while (std::getline(file, line))
            {
                std::stringstream ss(line);
                std::string tok;
                LapRecord r;
                if (!std::getline(ss, tok, ',')) continue; r.session_number = std::stoi(tok);
                if (!std::getline(ss, tok, ',')) continue; r.lap_number = std::stoi(tok);
                if (!std::getline(ss, tok, ',')) continue; r.lap_time = std::stof(tok);
                if (!std::getline(ss, tok, ',')) continue; r.sector1 = std::stof(tok);
                if (!std::getline(ss, tok, ',')) continue; r.sector2 = std::stof(tok);
                if (!std::getline(ss, tok, ',')) continue; r.sector3 = std::stof(tok);
                m_laps.push_back(r);
                update_bests(r);
                if (r.session_number > m_highest_session_on_disk)
                    m_highest_session_on_disk = r.session_number;
            }
        }
    }

    void update_bests(const LapRecord& r)
    {
        if (m_best_lap < 0.0f || r.lap_time < m_best_lap) m_best_lap = r.lap_time;
        if (m_best_sector[0] < 0.0f || r.sector1 < m_best_sector[0]) m_best_sector[0] = r.sector1;
        if (m_best_sector[1] < 0.0f || r.sector2 < m_best_sector[1]) m_best_sector[1] = r.sector2;
        if (m_best_sector[2] < 0.0f || r.sector3 < m_best_sector[2]) m_best_sector[2] = r.sector3;
    }

    // never save a trace that's internally broken, even if it happens to be the fastest lap_time
    static bool is_trace_sane(const std::vector<TraceSample>& t)
    {
        if (t.size() < 10) return false; // too few samples to be a real full-lap trace
        for (size_t i = 1; i < t.size(); ++i)
        {
            if (t[i].dist <= t[i-1].dist) return false;
            if (t[i].time <= t[i-1].time) return false;
            if (t[i].dist - t[i-1].dist > 50.0f) return false; // implausible single-sample gap
        }
        return true;
    }

    void append_to_file(const LapRecord& r)
    {
        std::string dir = config::get_module_dir(m_hModule) + "\\laptimes";
        CreateDirectoryA(dir.c_str(), nullptr);

        bool need_header = !std::ifstream(file_path()).good();
        std::ofstream file(file_path(), std::ios::app);
        if (!file.is_open()) return;

        if (need_header)
            file << "session_number,lap_number,lap_time,sector1,sector2,sector3\n";

        file << r.session_number << "," << r.lap_number << "," << r.lap_time << ","
             << r.sector1 << "," << r.sector2 << "," << r.sector3 << "\n";
    }

    // fallback load for design mode when telemetry data is empty/offline
    void load_design_mode()
    {
        if (!m_laps.empty()) return; // already loaded

        std::string dir = config::get_module_dir(m_hModule) + "\\laptimes";
        // search for any csv file in the laptimes directory
        std::string search_path = dir + "\\*.csv";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(search_path.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            std::string found_file = dir + "\\" + findData.cFileName;
            FindClose(hFind);

            std::ifstream file(found_file);
            if (file.is_open())
            {
                std::string line;
                std::getline(file, line); // skip header
                while (std::getline(file, line))
                {
                    std::stringstream ss(line);
                    std::string tok;
                    LapRecord r;
                    if (!std::getline(ss, tok, ',')) continue; r.session_number = std::stoi(tok);
                    if (!std::getline(ss, tok, ',')) continue; r.lap_number = std::stoi(tok);
                    if (!std::getline(ss, tok, ',')) continue; r.lap_time = std::stof(tok);
                    if (!std::getline(ss, tok, ',')) continue; r.sector1 = std::stof(tok);
                    if (!std::getline(ss, tok, ',')) continue; r.sector2 = std::stof(tok);
                    if (!std::getline(ss, tok, ',')) continue; r.sector3 = std::stof(tok);
                    m_laps.push_back(r);
                    update_bests(r);
                }
            }
        }

        // if no CSV files exist anywhere on disk yet, populate fake dummy records for layout preview
        if (m_laps.empty())
        {
            for (int i = 1; i <= 5; ++i)
            {
                LapRecord r;
                r.session_number = 1;
                r.lap_number = i;
                r.lap_time = 92.5f + (float)(i * 0.2f);
                r.sector1 = 30.1f; r.sector2 = 31.2f; r.sector3 = 31.2f;
                m_laps.push_back(r);
                update_bests(r);
            }
        }
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////// Sectors
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class SectorTracker
{
public:
    static SectorTracker& get() { static SectorTracker i; return i; }

    void update()
    {
        long   lap_number = LMUTelemetry::get().get_lap_number();
        long   sector_raw = LMUTelemetry::get().get_current_sector_raw();
        double elapsed     = LMUTelemetry::get().get_elapsed_time();
        double lap_start   = LMUTelemetry::get().get_lap_start_et();

        if (lap_number < 0) return;

        if (lap_number != m_lap_number)
        {
            // the lap that just ended gets its sector 3 completed here, using THIS frame's
            // elapsed time as the lap-end boundary (best approximation available — the true
            // crossing happened sometime between last frame and this one)
            if (m_has_prev && m_progress_idx == 2)
            {
                m_sector_time[2] = (float)(elapsed - m_last_split_et);
                m_completed[2] = true;
            }

            m_lap_number = lap_number;
            m_progress_idx = 0;
            m_last_split_et = lap_start;
            m_has_prev = true;
            m_prev_sector_raw = sector_raw;
            return;
        }

        if (!m_has_prev) 
        { 
            m_prev_sector_raw = sector_raw; 
            m_has_prev = true; 
            m_last_split_et = lap_start; 
            return; 
        }

        long masked_prev = m_prev_sector_raw & 0x7FFFFFFF;
        long masked_cur  = sector_raw & 0x7FFFFFFF;
        if (masked_cur != masked_prev && m_progress_idx < 2)
        {
            int idx = m_progress_idx;

            if (idx == 0)
            {
                m_completed[1] = false;
                m_completed[2] = false;
            }

            m_sector_time[idx] = (float)(elapsed - m_last_split_et);
            m_completed[idx] = true;
            m_last_split_et = elapsed;
            m_progress_idx++;
        }
        m_prev_sector_raw = sector_raw;
    }

    bool  is_complete(int i) const { return (i >= 0 && i < 3) ? m_completed[i] : false; }
    float get_time(int i) const    { return (i >= 0 && i < 3) ? m_sector_time[i] : 0.0f; }

private:
    long   m_lap_number = -1;
    bool   m_has_prev = false;
    long   m_prev_sector_raw = 0;
    double m_last_split_et = 0.0;
    int    m_progress_idx = 0;
    bool   m_completed[3] = { false, false, false };
    float  m_sector_time[3] = { 0.0f, 0.0f, 0.0f };
};

#endif