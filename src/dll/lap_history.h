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
            m_has_prev = false; // out lap: false on first frame, no record
            m_session_started = false; // this run hasn't claimed a session number yet
        }

        float last_lap_time = LMUTelemetry::get().get_last_lap_time(); 
        long  cur_lap_number = LMUTelemetry::get().get_lap_number();

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
            }
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

    float all_time_best_lap() const { return m_best_lap; }
    float all_time_best_sector(int i) const { return (i >= 0 && i < 3) ? m_best_sector[i] : -1.0f; }

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

#endif