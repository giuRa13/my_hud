#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>

namespace config 
{
    struct AppConfig {
        bool enable_overlay = true; 
        bool design_mode = false;

        float opacity = 0.5f;
        bool enable_vsync = true;
        float accent_r = 0.2f;
        float accent_g = 0.6f;
        float accent_b = 1.0f;
        float accent_a = 1.0f;

        bool show_pedals = true;
        bool show_delta = false;
        bool show_gear = true;

        // pedals settings
        bool pedals_horizontal = false; // false = vertical (default), true = horizontal
        bool show_throttle = true;
        bool show_brake = true;
        bool show_clutch = true;
        bool show_ffb = true; 
        float throttle_color[4] = { 0.02f, 0.9f, 0.0f, 1.0f }; // Default Green
        float brake_color[4]    = { 0.898f, 0.133f, 0.286f, 1.0f }; // Default Red
        float clutch_color[4]   = { 0.266f, 0.462f, 0.682f, 1.0f };; // Default Blue
        float ffb_color[4]      = { 0.925f, 0.643f, 0.047f, 1.0f }; // Default Orange/Yellow
        
        // gear settings
        float gear_bg_redline_color[4]  = { 1.0f,  0.118f, 0.267f, 1.0f }; // #FF1E44 (0.91 threshold)
        float gear_bg_optimal_color[4]  = { 0.0f,  0.667f, 1.0f,   1.0f }; // #00AAFF (0.95 threshold)
        float gear_bg_overrev_color[4]  = { 1.0f,  0.0f,   1.0f,   1.0f }; // #FF00FF (0.98 threshold)

        // delta settings
        bool delta_use_all_time_best = true; // true = All Time Best (default), false = Last Lap
        float delta_font_scale = 1.0f;
        float delta_negative_color[4] = { 0.02f, 0.9f, 0.0f, 1.0f }; // Default Green
        float delta_positive_color[4]    = { 0.898f, 0.133f, 0.286f, 1.0f }; // Default Red
    };

    // gets the directory where the EXE or DLL is running
    inline std::string get_module_dir(HMODULE hModule = NULL) 
    {
        char path[MAX_PATH];
        // if hModule is NULL, GetModuleFileNameA gets the path of the running EXE.
        // if hModule is valid, it gets the exact path of the DLL.
        GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string dir(path);
        return dir.substr(0, dir.find_last_of("\\/"));
    }

    // writer: forces absolute path next to the calling module
    inline void save_config(const AppConfig& config, const std::string& filename = "config.json", HMODULE hModule = NULL) 
    {
        std::string full_path = get_module_dir(hModule) + "\\" + filename;
        std::ofstream file(full_path);
        if (file.is_open()) 
        {
            file << "enable_overlay=" << (config.enable_overlay ? 1 : 0) << "\n";
            file << "design_mode=" << (config.design_mode ? 1 : 0) << "\n";
            file << "opacity=" << config.opacity << "\n";
            file << "enable_vsync=" << (config.enable_vsync ? 1 : 0) << "\n";
            file << "accent_r=" << config.accent_r << "\n";
            file << "accent_g=" << config.accent_g << "\n";
            file << "accent_b=" << config.accent_b << "\n";
            file << "accent_a=" << config.accent_a << "\n";
            //
            file << "show_pedals=" << (config.show_pedals ? 1 : 0) << "\n";
            file << "show_delta=" << (config.show_delta ? 1 : 0) << "\n";
            file << "show_gear=" << (config.show_gear ? 1 : 0) << "\n";
            //
            file << "pedals_horizontal=" << (config.pedals_horizontal ? 1 : 0) << "\n";
            file << "show_throttle=" << (config.show_throttle ? 1 : 0) << "\n";
            file << "show_brake=" << (config.show_brake ? 1 : 0) << "\n";
            file << "show_clutch=" << (config.show_clutch ? 1 : 0) << "\n";
            file << "show_ffb=" << (config.show_ffb ? 1 : 0) << "\n";
            file << "throttle_r=" << config.throttle_color[0] << "\n";
            file << "throttle_g=" << config.throttle_color[1] << "\n";
            file << "throttle_b=" << config.throttle_color[2] << "\n";
            file << "throttle_a=" << config.throttle_color[3] << "\n";
            file << "brake_r=" << config.brake_color[0] << "\n";
            file << "brake_g=" << config.brake_color[1] << "\n";
            file << "brake_b=" << config.brake_color[2] << "\n";
            file << "brake_a=" << config.brake_color[3] << "\n";
            file << "clutch_r=" << config.clutch_color[0] << "\n";
            file << "clutch_g=" << config.clutch_color[1] << "\n";
            file << "clutch_b=" << config.clutch_color[2] << "\n";
            file << "clutch_a=" << config.clutch_color[3] << "\n";
            file << "ffb_r=" << config.ffb_color[0] << "\n";
            file << "ffb_g=" << config.ffb_color[1] << "\n";
            file << "ffb_b=" << config.ffb_color[2] << "\n";
            file << "ffb_a=" << config.ffb_color[3] << "\n";
            //
            file << "gear_bg_redline_r=" << config.gear_bg_redline_color[0] << "\n";
            file << "gear_bg_redline_g=" << config.gear_bg_redline_color[1] << "\n";
            file << "gear_bg_redline_b=" << config.gear_bg_redline_color[2] << "\n";
            file << "gear_bg_redline_a=" << config.gear_bg_redline_color[3] << "\n";
            file << "gear_bg_optimal_r=" << config.gear_bg_optimal_color[0] << "\n";
            file << "gear_bg_optimal_g=" << config.gear_bg_optimal_color[1] << "\n";
            file << "gear_bg_optimal_b=" << config.gear_bg_optimal_color[2] << "\n";
            file << "gear_bg_optimal_a=" << config.gear_bg_optimal_color[3] << "\n";
            file << "gear_bg_overrev_r=" << config.gear_bg_overrev_color[0] << "\n";
            file << "gear_bg_overrev_g=" << config.gear_bg_overrev_color[1] << "\n";
            file << "gear_bg_overrev_b=" << config.gear_bg_overrev_color[2] << "\n";
            file << "gear_bg_overrev_a=" << config.gear_bg_overrev_color[3] << "\n";
            //
            file << "delta_use_all_time_best=" << (config.delta_use_all_time_best ? 1 : 0) << "\n";
            file << "delta_font_scale=" << config.delta_font_scale << "\n";
            file << "delta_negative_r=" << config.delta_negative_color[0] << "\n";
            file << "delta_negative_g=" << config.delta_negative_color[1] << "\n";
            file << "delta_negative_b=" << config.delta_negative_color[2] << "\n";
            file << "delta_negative_a=" << config.delta_negative_color[3] << "\n";
            file << "delta_positive_r=" << config.delta_positive_color[0] << "\n";
            file << "delta_positive_g=" << config.delta_positive_color[1] << "\n";
            file << "delta_positive_b=" << config.delta_positive_color[2] << "\n";
            file << "delta_posituve_a=" << config.delta_positive_color[3] << "\n";

            file.close();
        }
    }

    // reader: forces absolute path next to the calling module
    inline AppConfig load_config(const std::string& filename = "config.json", HMODULE hModule = NULL) 
    {
        AppConfig config;
        std::string full_path = get_module_dir(hModule) + "\\" + filename;
        std::ifstream file(full_path);
        if (!file.is_open()) return config; // returns defaults if file doesn't exist yet

        std::string line;
        while (std::getline(file, line)) 
        {
            std::stringstream ss(line);
            std::string key;
            if (std::getline(ss, key, '=')) 
            {
                std::string val_str;
                if (std::getline(ss, val_str)) {
                    if (key == "enable_overlay") config.enable_overlay = (std::stoi(val_str) != 0);
                    else if (key == "design_mode") config.design_mode = (std::stoi(val_str) != 0);
                    else if (key == "opacity") config.opacity = std::stof(val_str);
                    else if (key == "enable_vsync") config.enable_vsync = (std::stoi(val_str) != 0);
                    else if (key == "accent_r") config.accent_r = std::stof(val_str);
                    else if (key == "accent_g") config.accent_g = std::stof(val_str);
                    else if (key == "accent_b") config.accent_b = std::stof(val_str);
                    else if (key == "accent_a") config.accent_a = std::stof(val_str);
                    
                    else if (key == "show_pedals") config.show_pedals = (std::stoi(val_str) != 0);
                    else if (key == "show_delta") config.show_delta = (std::stoi(val_str) != 0);
                    else if (key == "show_gear") config.show_gear = (std::stoi(val_str) != 0);
                
                    else if (key == "pedals_horizontal") config.pedals_horizontal = (std::stoi(val_str) != 0);
                    else if (key == "show_throttle") config.show_throttle = (std::stoi(val_str) != 0);
                    else if (key == "show_brake") config.show_brake = (std::stoi(val_str) != 0);
                    else if (key == "show_clutch") config.show_clutch = (std::stoi(val_str) != 0);
                    else if (key == "show_ffb") config.show_ffb = (std::stoi(val_str) != 0);
                    else if (key == "throttle_r") config.throttle_color[0] = std::stof(val_str);
                    else if (key == "throttle_g") config.throttle_color[1] = std::stof(val_str);
                    else if (key == "throttle_b") config.throttle_color[2] = std::stof(val_str);
                    else if (key == "throttle_a") config.throttle_color[3] = std::stof(val_str);
                    else if (key == "brake_r") config.brake_color[0] = std::stof(val_str);
                    else if (key == "brake_g") config.brake_color[1] = std::stof(val_str);
                    else if (key == "brake_b") config.brake_color[2] = std::stof(val_str);
                    else if (key == "brake_a") config.brake_color[3] = std::stof(val_str);
                    else if (key == "clutch_r") config.clutch_color[0] = std::stof(val_str);
                    else if (key == "clutch_g") config.clutch_color[1] = std::stof(val_str);
                    else if (key == "clutch_b") config.clutch_color[2] = std::stof(val_str);
                    else if (key == "clutch_a") config.clutch_color[3] = std::stof(val_str);
                    else if (key == "ffb_r") config.ffb_color[0] = std::stof(val_str);
                    else if (key == "ffb_g") config.ffb_color[1] = std::stof(val_str);
                    else if (key == "ffb_b") config.ffb_color[2] = std::stof(val_str);
                    else if (key == "ffb_a") config.ffb_color[3] = std::stof(val_str);

                    else if (key == "gear_bg_redline_r") config.gear_bg_redline_color[0] = std::stof(val_str);
                    else if (key == "gear_bg_redline_g") config.gear_bg_redline_color[1] = std::stof(val_str);
                    else if (key == "gear_bg_redline_b") config.gear_bg_redline_color[2] = std::stof(val_str);
                    else if (key == "gear_bg_redline_a") config.gear_bg_redline_color[3] = std::stof(val_str);
                    else if (key == "gear_bg_optimal_r") config.gear_bg_optimal_color[0] = std::stof(val_str);
                    else if (key == "gear_bg_optimal_g") config.gear_bg_optimal_color[1] = std::stof(val_str);
                    else if (key == "gear_bg_optimal_b") config.gear_bg_optimal_color[2] = std::stof(val_str);
                    else if (key == "gear_bg_optimal_a") config.gear_bg_optimal_color[3] = std::stof(val_str);
                    else if (key == "gear_bg_overrev_r") config.gear_bg_overrev_color[0] = std::stof(val_str);
                    else if (key == "gear_bg_overrev_g") config.gear_bg_overrev_color[1] = std::stof(val_str);
                    else if (key == "gear_bg_overrev_b") config.gear_bg_overrev_color[2] = std::stof(val_str);
                    else if (key == "gear_bg_overrev_a") config.gear_bg_overrev_color[3] = std::stof(val_str);

                    else if (key == "delta_use_all_time_best") config.delta_use_all_time_best = (std::stoi(val_str) != 0);
                    else if (key == "delta_font_scale") config.delta_font_scale = std::stof(val_str);
                    else if (key == "delta_negative_r") config.delta_negative_color[0] = std::stof(val_str);
                    else if (key == "delta_negative_g") config.delta_negative_color[1] = std::stof(val_str);
                    else if (key == "delta_negative_b") config.delta_negative_color[2] = std::stof(val_str);
                    else if (key == "delta_negative_a") config.delta_negative_color[3] = std::stof(val_str);
                    else if (key == "delta_positive_r") config.delta_positive_color[0] = std::stof(val_str);
                    else if (key == "delta_positive_g") config.delta_positive_color[1] = std::stof(val_str);
                    else if (key == "delta_positive_b") config.delta_positive_color[2] = std::stof(val_str);
                    else if (key == "delta_positive_a") config.delta_positive_color[3] = std::stof(val_str);
                }
            }
        }
        file.close();
        return config;
    }

};

#endif