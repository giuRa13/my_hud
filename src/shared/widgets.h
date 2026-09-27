#ifndef WIDGETS_H
#define WIDGETS_H

#include <shared/config.h>
#include <dll/lmu_telemetry.h>
#include <imgui.h>
#include <algorithm>

namespace widgets
{
    inline void draw_custom_bar(const char* label, float value, bool horizontal, float width, float height, ImVec4 bar_color)
    {
        ImU32 col32 = ImGui::ColorConvertFloat4ToU32(bar_color);

        if (horizontal)
        {
            ImGui::Text("%s", label);
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, bar_color);
            ImGui::ProgressBar(value, ImVec2(-1.0f, height > 0 ? height : 14.0f), "");
            ImGui::PopStyleColor(2);
        }
        else 
        {
            // ImGui doesn't have a native vertical progress bar, so build a sleek vertical column meter
            ImGui::BeginGroup();
            //ImGui::Text("%s", label);

            // draw vertical column container
            ImVec2 bar_size(width > 0 ? width : 14.0f, height > 0 ? height : 100.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));

            // invisible button or custom draw list / child frame for vertical filling
            ImGui::BeginChild(label, bar_size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

            // calculate fill height based on value (0.0 to 1.0)
            float fill_height = bar_size.y * value;
            ImVec2 pMin = ImGui::GetWindowPos();
            ImVec2 pMax = ImVec2(pMin.x + bar_size.x, pMin.y + bar_size.y);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(pMin, pMax, IM_COL32(30, 30, 35, 255)); // background

            ImVec2 fill_min = ImVec2(pMin.x, pMax.y - fill_height);
            draw_list->AddRectFilled(fill_min, pMax, col32); // bar fill color

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::Text("%s", label);
            ImGui::EndGroup();
        }
    }

    // pedals widget //////////////////////////////////////////////////////////////////////////
    void pedals_widget(config::AppConfig config)
    {
        float throttle = 0.0f;
        float brake = 0.0f;
        float clutch = 0.0f;
        float ffb = 0.0f;

#ifdef IS_CONTROL_PANEL
        throttle = 0.75f;
        brake = 0.25f;
        clutch = 0.5f;
        ffb = 0.60f;
#else
        LMUTelemetry::get().update();
        throttle = LMUTelemetry::get().get_throttle();
        brake = LMUTelemetry::get().get_brake();
        clutch = LMUTelemetry::get().get_clutch(); 
        ffb = LMUTelemetry::get().get_ffb();
#endif

        ImVec4 t_Col = ImVec4(config.throttle_color[0], config.throttle_color[1], config.throttle_color[2], config.throttle_color[3]);
        ImVec4 b_Col = ImVec4(config.brake_color[0], config.brake_color[1], config.brake_color[2], config.brake_color[3]);
        ImVec4 c_Col = ImVec4(config.clutch_color[0], config.clutch_color[1], config.clutch_color[2], config.clutch_color[3]);
        ImVec4 f_Col = ImVec4(config.ffb_color[0], config.ffb_color[1], config.ffb_color[2], config.ffb_color[3]);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar;
        ImGui::SetNextWindowBgAlpha(config.opacity);
        if (ImGui::Begin("Pedals", nullptr, window_flags))
        {
            ImVec2 avail_size = ImGui::GetContentRegionAvail();

            if (config.pedals_horizontal)
            {
                float row_height = 14.0f;
                if (config.show_throttle) { draw_custom_bar("Thr", throttle, true, 0.0f, row_height, t_Col); }
                if (config.show_brake)    { draw_custom_bar("Brk", brake, true, 0.0f, row_height, b_Col); }
                if (config.show_clutch)   { draw_custom_bar("Clt", clutch, true, 0.0f, row_height, c_Col); }
                if (config.show_ffb)      { draw_custom_bar("FFB", ffb, true, 0.0f, row_height, f_Col); }
            }
            else
            {
                float bar_width = 14.0f;
                float bar_height = (avail_size.y > 40.0f) ? avail_size.y - 25.0f : 100.0f; // ifll vertical space dynamically
                if (bar_height < 50.0f) bar_height = 50.0f;

                if (config.show_throttle) { draw_custom_bar("T", throttle, false, bar_width, bar_height, t_Col); ImGui::SameLine(); }
                if (config.show_brake)    { draw_custom_bar("B", brake, false, bar_width, bar_height, b_Col); ImGui::SameLine(); }
                if (config.show_clutch)   { draw_custom_bar("C", clutch, false, bar_width, bar_height, c_Col); ImGui::SameLine(); }
                if (config.show_ffb)      { draw_custom_bar("F", ffb, false, bar_width, bar_height, f_Col); }
            }
            ImGui::NewLine();
        }
        ImGui::End();
    }

    inline void pedals_settings_panel(bool* p_open, config::AppConfig& config)
    {
        if (ImGui::Begin("Pedals Settings", p_open, 0))
        {
            ImGui::SetWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);

            ImGui::Text("Layout Options");
            ImGui::Checkbox("Horizontal Layout", &config.pedals_horizontal);

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Active Bars:");
            ImGui::Checkbox("Show Throttle", &config.show_throttle);
            ImGui::Checkbox("Show Brake", &config.show_brake);
            ImGui::Checkbox("Show Clutch", &config.show_clutch);
            ImGui::Checkbox("Show Force Feedback (FFB)", &config.show_ffb);

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Pedal Bar Colors");
            ImGui::ColorEdit4("Throttle Color", config.throttle_color);
            ImGui::ColorEdit4("Brake Color", config.brake_color);
            ImGui::ColorEdit4("Clutch Color", config.clutch_color);
            ImGui::ColorEdit4("FFB Color", config.ffb_color);

            ImGui::End();
        }
    }

    // gear widget //////////////////////////////////////////////////////////////////////////
    void gear_widget(config::AppConfig& config) 
    {
        std::string gear_str;
        float rpm = 0.0f;
        float max_rpm = 8500.0f;
        float rpm_pct = 0.0f;
        float speed_kmh = 0.0f;
        float battery_pct = 0.0f;
        bool has_battery = true;

#ifdef IS_CONTROL_PANEL
        gear_str = "3";
        rpm = 6500.0f;
        max_rpm = 8500.0f;
        rpm_pct = 6500.0f / 8500.0f;
        speed_kmh = 145.0f;
        battery_pct = 0.75f;
        has_battery = true;
#else
        LMUTelemetry::get().update();
        
        int raw_gear = LMUTelemetry::get().get_gear();
        if (raw_gear == -1)      gear_str = "R";
        else if (raw_gear == 0)  gear_str = "N";
        else                     gear_str = std::to_string(raw_gear);

        rpm = LMUTelemetry::get().get_RPM();
        max_rpm = LMUTelemetry::get().get_max_rpm();
        rpm_pct = std::clamp(rpm / max_rpm, 0.0f, 1.0f); 

        speed_kmh = LMUTelemetry::get().get_speed_kmh();
        battery_pct = LMUTelemetry::get().get_battery_pct(); 
        has_battery = (battery_pct > 0.0f); 
#endif

        // bg color shift threshold
        ImVec4 bg_color;
        float alpha = std::max(config.opacity, 0.4f);

        if (rpm_pct > 0.98f) 
        {
            bg_color = ImVec4(config.gear_bg_overrev_color[0], config.gear_bg_overrev_color[1], config.gear_bg_overrev_color[2], alpha);
        }
        else if (rpm_pct >= 0.95f) 
        {
            bg_color = ImVec4(config.gear_bg_optimal_color[0], config.gear_bg_optimal_color[1], config.gear_bg_optimal_color[2], alpha);
        }
        else if (rpm_pct >= 0.91f) 
        {
            bg_color = ImVec4(config.gear_bg_redline_color[0], config.gear_bg_redline_color[1], config.gear_bg_redline_color[2], alpha);
        }
        else 
        {
            bg_color = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            bg_color.w = config.opacity;
        }

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                        ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoScrollWithMouse;
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg_color);
        ImGui::SetNextWindowSize(ImVec2(200, 250), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(130, 180), ImVec2(500, 600));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        if (ImGui::Begin("Gear & Telemetry", nullptr, window_flags)) 
        {
            ImVec2 window_size = ImGui::GetWindowSize();
            float avail_height = ImGui::GetContentRegionAvail().y - 4.0f;
            float avail_width = ImGui::GetContentRegionAvail().x;

            // define fixed/proportional heights for the bottom bars (Total bottom stack = 1.5 parts out of 5.0)
            float total_parts = 5.0f;
            float section_unit = avail_height / total_parts;
            
            float battery_height = section_unit * 0.5f;
            float rpm_height     = section_unit * 1.0f;
            // remaining top space is completely consumed by the Gear & Speed section
            float gear_section_height = avail_height - battery_height - rpm_height;

            // --- GEAR & SPEED SECTION ---
            {
                ImGui::BeginChild("GearSection", ImVec2(avail_width, gear_section_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                
                ImVec2 child_size = ImGui::GetWindowSize();

                char speed_buf[32];
                snprintf(speed_buf, sizeof(speed_buf), "%.0f km/h", speed_kmh);

                float dynamic_scale = child_size.x / 130.0f;
                dynamic_scale = std::clamp(dynamic_scale, 1.0f, 9.0f); ////////////////

                float base_font_scale = 1.0f;

                // 1 - measure and render Speed (Pinned to the bottom of the section) 
                float speed_font_scale = base_font_scale * (1.1f * dynamic_scale);
                ImGui::GetFont()->Scale = speed_font_scale;
                ImGui::PushFont(ImGui::GetFont());
                
                float speed_width = ImGui::CalcTextSize(speed_buf).x;
                float speed_height = ImGui::CalcTextSize(speed_buf).y;
                
                // position speed text at the very bottom of the upper section (leaving 2px gap above the battery)
                float speed_y = std::max(0.0f, gear_section_height - speed_height - 2.0f);
                
                ImGui::SetCursorPosX((child_size.x - speed_width) * 0.5f);
                ImGui::SetCursorPosY(speed_y);
                ImGui::Text("%s", speed_buf);
                ImGui::PopFont();

                // 2 - measure and render Gear (Takes all remaining space above the speed text) ---
                // avail height for gear is everything from the top down to just above the speed text
                float available_gear_height = speed_y - 2.0f; 

                // scale gear font dynamically to fill that remaining upper space
                float gear_font_scale = base_font_scale * (5.8f * dynamic_scale);
                
                // safety check: ensure gear font doesn't overflow the available upper height
                ImGui::GetFont()->Scale = gear_font_scale;
                ImGui::PushFont(ImGui::GetFont());
                float gear_width = ImGui::CalcTextSize(gear_str.c_str()).x;
                float gear_height = ImGui::CalcTextSize(gear_str.c_str()).y;
                ImGui::PopFont();

                // auto-shrink safeguard: if it exceeds the top section height, shrink it safely to fit
                if (gear_height > available_gear_height && available_gear_height > 10.0f) 
                {
                    gear_font_scale *= (available_gear_height / gear_height);
                    ImGui::GetFont()->Scale = gear_font_scale;
                    ImGui::PushFont(ImGui::GetFont());
                    gear_width = ImGui::CalcTextSize(gear_str.c_str()).x;
                    gear_height = ImGui::CalcTextSize(gear_str.c_str()).y;
                    ImGui::PopFont();
                }

                // center the gear vertically within the remaining top space
                float gear_y = std::max(0.0f, (available_gear_height - gear_height) * 0.5f);

                ImGui::GetFont()->Scale = gear_font_scale;
                ImGui::PushFont(ImGui::GetFont());
                ImGui::SetCursorPosX((child_size.x - gear_width) * 0.5f);
                ImGui::SetCursorPosY(gear_y);
                ImGui::Text("%s", gear_str.c_str());
                ImGui::PopFont();

                ImGui::GetFont()->Scale = base_font_scale;

                ImGui::EndChild();
            }
            
            // --- BATTERY PROGRESS BAR ---
            {
                ImGui::BeginChild("BatterySection", ImVec2(avail_width, battery_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                
                char batt_buf[32];
                if (has_battery) 
                    snprintf(batt_buf, sizeof(batt_buf), "%.0f%%", battery_pct * 100.0f);
                else 
                    snprintf(batt_buf, sizeof(batt_buf), "N/A");
                
                ImVec2 bar_size = ImVec2(-FLT_MIN, battery_height * 0.75f);
                
                // Custom centered progress bar drawing
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImVec2 sz = ImVec2(ImGui::GetContentRegionAvail().x, bar_size.y);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // background box
                draw_list->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.18f, 1.0f)), 3.0f);
                
                // fill fraction (if has battery)
                if (has_battery && battery_pct > 0.0f) 
                {
                    float fill_w = std::max(0.0f, std::min(1.0f, battery_pct)) * sz.x;
                    draw_list->AddRectFilled(p, ImVec2(p.x + fill_w, p.y + sz.y), ImGui::GetColorU32(ImVec4(0.2f, 0.6f, 1.0f, 1.0f)), 3.0f);
                }

                // centered Text Overlay
                ImVec2 text_size = ImGui::CalcTextSize(batt_buf);
                ImVec2 text_pos = ImVec2(p.x + (sz.x - text_size.x) * 0.5f, p.y + (sz.y - text_size.y) * 0.5f);
                draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), batt_buf);

                // cdvance cursor so layout stays consistent
                ImGui::Dummy(sz);

                ImGui::EndChild();
            }
            /*{
                ImGui::BeginChild("BatterySection", ImVec2(avail_width, battery_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                
                char batt_buf[32];
                if (has_battery) 
                    snprintf(batt_buf, sizeof(batt_buf), "%.0f%%", battery_pct * 100.0f);
                else 
                    snprintf(batt_buf, sizeof(batt_buf), "N/A");
                
                ImVec2 bar_size = ImVec2(-FLT_MIN, battery_height * 0.75f);
                
                 if (has_battery) 
                 {
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
                    ImGui::ProgressBar(battery_pct, bar_size, batt_buf);
                    ImGui::PopStyleColor(2);
                } 
                else 
                {
                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.188f, 0.69f, 1.0f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 0.5f));
                    ImGui::ProgressBar(0.0f, bar_size, batt_buf);
                    ImGui::PopStyleColor(2);
                }

                ImGui::EndChild();
            }*/

            // --- RPM PROGRESS BAR ---
            {
                float remaining_height = ImGui::GetContentRegionAvail().y;
                ImGui::BeginChild("RpmSection", ImVec2(avail_width, remaining_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                    
                char rpm_buf[32];
                snprintf(rpm_buf, sizeof(rpm_buf), "%.0f RPM", rpm);

                ImVec4 rpm_color = (rpm_pct > 0.9f) ? ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
                    
                ImVec2 bar_size = ImVec2(-FLT_MIN, remaining_height * 0.75f);

                // Custom centered progress bar drawing
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImVec2 sz = ImVec2(ImGui::GetContentRegionAvail().x, bar_size.y);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();

                // background box
                draw_list->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.18f, 1.0f)), 3.0f);
                    
                // fill fraction
                if (rpm_pct > 0.0f) 
                {
                    float fill_w = std::max(0.0f, std::min(1.0f, rpm_pct)) * sz.x;
                    draw_list->AddRectFilled(p, ImVec2(p.x + fill_w, p.y + sz.y), ImGui::GetColorU32(rpm_color), 3.0f);
                }

                // centered Text Overlay
                ImVec2 text_size = ImGui::CalcTextSize(rpm_buf);
                ImVec2 text_pos = ImVec2(p.x + (sz.x - text_size.x) * 0.5f, p.y + (sz.y - text_size.y) * 0.5f);
                draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_WindowBg), rpm_buf);

                // advance cursor
                ImGui::Dummy(sz);

                ImGui::EndChild();
            }
             /*{
                float remaining_height = ImGui::GetContentRegionAvail().y;
                ImGui::BeginChild("RpmSection", ImVec2(avail_width, remaining_height), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                
                char rpm_buf[16];
                snprintf(rpm_buf, sizeof(rpm_buf), "%.0f RPM", rpm);

                ImVec4 rpm_color = (rpm_pct > 0.9f) ? ImVec4(0.9f, 0.1f, 0.1f, 1.0f) : ImVec4(0.95, 0.95, 0.95, 1.0f);
                
                ImVec2 bar_size = ImVec2(-FLT_MIN, remaining_height * 0.75f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, rpm_color);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
                ImGui::ProgressBar(rpm_pct, bar_size, rpm_buf);
                ImGui::PopStyleColor(2);

                ImGui::EndChild();
            }*/
        }   

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

    }

    inline void gear_settings_panel(bool* p_open, config::AppConfig& config)
    {
        if (ImGui::Begin("Gear Settings", p_open, 0))
        {
            ImGui::SetWindowSize(ImVec2(280, 220), ImGuiCond_FirstUseEver);

            ImGui::Text("Shift Light Background Colors");
            ImGui::Spacing();

            ImGui::ColorEdit4("Redline (> 91%)", config.gear_bg_redline_color);
            ImGui::ColorEdit4("Optimal Shift (> 95%)", config.gear_bg_optimal_color);
            ImGui::ColorEdit4("Over-Rev (> 98%)", config.gear_bg_overrev_color);

            ImGui::Spacing();
            if (ImGui::Button("Reset Default Colors"))
            {
                config.gear_bg_redline_color[0] = 1.0f;  config.gear_bg_redline_color[1] = 0.118f; config.gear_bg_redline_color[2] = 0.267f; config.gear_bg_redline_color[3] = 1.0f;
                config.gear_bg_optimal_color[0] = 0.0f;  config.gear_bg_optimal_color[1] = 0.667f; config.gear_bg_optimal_color[2] = 1.0f;   config.gear_bg_optimal_color[3] = 1.0f;
                config.gear_bg_overrev_color[0] = 1.0f;  config.gear_bg_overrev_color[1] = 0.0f;   config.gear_bg_overrev_color[2] = 1.0f;   config.gear_bg_overrev_color[3] = 1.0f;
            }

            ImGui::End();
        }
    }


    // delta widget //////////////////////////////////////////////////////////////////////////
    void delta_widget(config::AppConfig config)
    {
        ImGui::SetNextWindowBgAlpha(config.opacity);
        ImGui::Begin("Delta");
        ImGui::SetWindowSize(ImVec2(120, 80), ImGuiCond_FirstUseEver);

#ifdef IS_CONTROL_PANEL
        ImGui::Text("Delta: +0.234s (Test)");
#else
        ImGui::Text("Delta: +0.234s");
#endif

        ImGui::End();
    }

    inline void delta_settings_panel(bool* p_open, config::AppConfig& config)
    {
        if (ImGui::Begin("Delta Settings", p_open, 0))
        {
            ImGui::Text("Delta settings");
        }
        ImGui::End();
    }
};

#endif