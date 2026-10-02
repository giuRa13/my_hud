#ifndef WIDGETS_H
#define WIDGETS_H

#include <shared/config.h>
#include <dll/lmu_telemetry.h>
#include <shared/texture_loader.h>
#include <dll/lap_history.h>
#include <imgui.h>
#include <string>
#include <cmath>
#include <algorithm>

#ifdef IS_CONTROL_PANEL
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#endif

#ifndef RESOURCES_PATH
#define RESOURCES_PATH ""   // safety net if a build target forgets to define it
#endif

namespace widgets
{
    // draws text 4x with tiny offsets to fake a bold weight
    inline void draw_text_maybe_bold(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text, bool bold, float offset = 0.6f)
    {
        if (bold)
        {
            dl->AddText(ImVec2(pos.x - offset, pos.y), col, text);
            dl->AddText(ImVec2(pos.x + offset, pos.y), col, text);
            dl->AddText(ImVec2(pos.x, pos.y - offset), col, text);
            dl->AddText(ImVec2(pos.x, pos.y + offset), col, text);
        }
        dl->AddText(pos, col, text);
    }

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

    // pedals widget ///////////////////////////////////////////////////////////////////////////////////////////////////////////
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
        }
        ImGui::End();
    }

    // gear widget ///////////////////////////////////////////////////////////////////////////////////////////////////////////
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
            bg_color = ImVec4(config.gear_bg_overrev_color[0], config.gear_bg_overrev_color[1], config.gear_bg_overrev_color[2], config.gear_bg_overrev_color[3]);
        }
        else if (rpm_pct >= 0.95f) 
        {
            bg_color = ImVec4(config.gear_bg_optimal_color[0], config.gear_bg_optimal_color[1], config.gear_bg_optimal_color[2], config.gear_bg_optimal_color[3]);
        }
        else if (rpm_pct >= 0.91f) 
        {
            bg_color = ImVec4(config.gear_bg_redline_color[0], config.gear_bg_redline_color[1], config.gear_bg_redline_color[2], config.gear_bg_redline_color[3]);
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

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
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
                {
                    ImVec2 spos = ImGui::GetCursorScreenPos();
                    draw_text_maybe_bold(ImGui::GetWindowDrawList(), spos, ImGui::GetColorU32(ImGuiCol_Text), speed_buf, config.gear_font_bold);
                }
                ImGui::Dummy(ImVec2(speed_width, speed_height));
                ImGui::PopFont();

                // 2 - measure and render Gear (Takes all remaining space above the speed text) ---
                // avail height for gear is everything from the top down to just above the speed text
                float available_gear_height = speed_y - 2.0f; 

                // scale gear font dynamically to fill that remaining upper space
                float gear_font_scale = base_font_scale * (5.8f * dynamic_scale);
                
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
                //ImGui::Text("%s", gear_str.c_str());
                {
                    ImVec2 gpos = ImGui::GetCursorScreenPos();
                    draw_text_maybe_bold(ImGui::GetWindowDrawList(), gpos, ImGui::GetColorU32(ImGuiCol_Text), gear_str.c_str(), config.gear_font_bold);
                }
                ImGui::Dummy(ImVec2(gear_width, gear_height)); 
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
                draw_list->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.18f, 1.0f)), 0.0f);
                
                // fill fraction (if has battery)
                if (has_battery && battery_pct > 0.0f) 
                {
                    float fill_w = std::max(0.0f, std::min(1.0f, battery_pct)) * sz.x;
                    draw_list->AddRectFilled(p, ImVec2(p.x + fill_w, p.y + sz.y), ImGui::GetColorU32(ImVec4(config.gear_battery_color[0], config.gear_battery_color[1], config.gear_battery_color[2], config.gear_battery_color[3])), 0.0f);
                }

                // centered Text Overlay
                ImVec2 text_size = ImGui::CalcTextSize(batt_buf);
                ImVec2 text_pos = ImVec2(p.x + (sz.x - text_size.x) * 0.5f, p.y + (sz.y - text_size.y) * 0.5f);
                //draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), batt_buf);
                draw_text_maybe_bold(draw_list, text_pos, ImGui::GetColorU32(ImGuiCol_Text), batt_buf, config.gear_font_bold);

                // cdvance cursor so layout stays consistent
                ImGui::Dummy(sz);

                ImGui::EndChild();
            }

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
                draw_list->AddRectFilled(p, ImVec2(p.x + sz.x, p.y + sz.y), ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.18f, 1.0f)), 0.0f);
                    
                // fill fraction
                if (rpm_pct > 0.0f) 
                {
                    float fill_w = std::max(0.0f, std::min(1.0f, rpm_pct)) * sz.x;
                    draw_list->AddRectFilled(p, ImVec2(p.x + fill_w, p.y + sz.y), ImGui::GetColorU32(rpm_color), 0.0f);
                }

                // centered Text Overlay
                ImVec2 text_size = ImGui::CalcTextSize(rpm_buf);
                ImVec2 text_pos = ImVec2(p.x + (sz.x - text_size.x) * 0.5f, p.y + (sz.y - text_size.y) * 0.5f);
                //draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_WindowBg), rpm_buf);
                draw_text_maybe_bold(draw_list, text_pos, ImGui::GetColorU32(ImGuiCol_WindowBg), rpm_buf, config.gear_font_bold);

                // advance cursor
                ImGui::Dummy(sz);

                ImGui::EndChild();
            }
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

            ImGui::Spacing();
            ImGui::Checkbox("Bold Font", &config.gear_font_bold);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Shift Light Background Colors");
            ImGui::Spacing();
            ImGui::ColorEdit4("Redline (> 91%)", config.gear_bg_redline_color);
            ImGui::ColorEdit4("Optimal Shift (> 95%)", config.gear_bg_optimal_color);
            ImGui::ColorEdit4("Over-Rev (> 98%)", config.gear_bg_overrev_color);

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Battery color");
            ImGui::Spacing();
            ImGui::ColorEdit4("##battery_color", config.gear_battery_color);

            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Reset Gear Colors"))
            {
                config.gear_bg_redline_color[0] = 1.0f;  config.gear_bg_redline_color[1] = 0.118f; config.gear_bg_redline_color[2] = 0.267f; config.gear_bg_redline_color[3] = 1.0f;
                config.gear_bg_optimal_color[0] = 0.0f;  config.gear_bg_optimal_color[1] = 0.667f; config.gear_bg_optimal_color[2] = 1.0f;   config.gear_bg_optimal_color[3] = 1.0f;
                config.gear_bg_overrev_color[0] = 1.0f;  config.gear_bg_overrev_color[1] = 0.0f;   config.gear_bg_overrev_color[2] = 1.0f;   config.gear_bg_overrev_color[3] = 1.0f;
                config.gear_battery_color[0] = 0.0f, config.gear_battery_color[1] = 0.815f, config.gear_battery_color[2] = 1.0f, config.gear_battery_color[3] = 1.0f;
                //  0.2f, 0.6f, 1.0f, 1.0f old      //  0.0f, 0.815f, 1.0f, 1.0f bright     //  0.929f, 0.4f, 0.537f, 1.0f pink
            }
        }
        ImGui::End();
    }

    // delta widget ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    void delta_widget(config::AppConfig config)
    {
        float delta_val = 0.0f;

#ifdef IS_CONTROL_PANEL
        delta_val = -0.342f; 
#else
        LMUTelemetry::get().update();
        delta_val = LMUTelemetry::get().get_delta_best(); 
        
        if (!config.delta_use_all_time_best) 
        {
            delta_val = 0.0f; 
        }
#endif

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                        ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::SetNextWindowBgAlpha(config.opacity);
        ImGui::SetNextWindowSize(ImVec2(240, 70), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(160, 50), ImVec2(600, 150));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
        if (ImGui::Begin("Delta HUD", nullptr, window_flags)) 
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // dimensions for the split center progress bar
            float bar_height = avail.y * 0.45f;
            float bar_width = avail.x;
            float center_x = p.x + bar_width * 0.5f;

            // draw Split Bar Background
            ImVec2 bg_min = p;
            ImVec2 bg_max = ImVec2(p.x + bar_width, p.y + bar_height);
            draw_list->AddRectFilled(bg_min, bg_max, ImGui::GetColorU32(ImVec4(0.15f, 0.15f, 0.18f, 1.0f)), 0.0f);

            // center dividing tick mark
            draw_list->AddLine(ImVec2(center_x, p.y), ImVec2(center_x, p.y + bar_height), IM_COL32(200, 200, 200, 200), 1.5f);

            // calculate Fill Width (cap max visual reach at +/- 2.0 seconds)
            float max_delta_range = 2.0f; 
            float clamped_delta = std::clamp(delta_val, -max_delta_range, max_delta_range);
            float fill_fraction = std::abs(clamped_delta) / max_delta_range;
            float half_bar_width = (bar_width * 0.5f);
            float fill_len = fill_fraction * half_bar_width;

            ImVec4 neg_col = ImVec4(config.delta_negative_color[0], config.delta_negative_color[1], config.delta_negative_color[2], config.delta_negative_color[3]);
            ImVec4 pos_col = ImVec4(config.delta_positive_color[0], config.delta_positive_color[1], config.delta_positive_color[2], config.delta_positive_color[3]);

            //  render Bar Color
            if (delta_val < 0.0f) 
            {
                // expands to the right from center
                ImVec2 fill_min = ImVec2(center_x, p.y);
                ImVec2 fill_max = ImVec2(center_x + fill_len, p.y + bar_height);
                draw_list->AddRectFilled(fill_min, fill_max, ImGui::ColorConvertFloat4ToU32(neg_col), 0.0f);
            } 
            else if (delta_val > 0.0f) 
            {
                // expands to the left from center
                ImVec2 fill_min = ImVec2(center_x - fill_len, p.y);
                ImVec2 fill_max = ImVec2(center_x, p.y + bar_height);
                draw_list->AddRectFilled(fill_min, fill_max, ImGui::ColorConvertFloat4ToU32(pos_col), 0.0f);
            }

            // advance cursor past the drawn bar height + spacing
            ImGui::Dummy(ImVec2(bar_width, bar_height + 4.0f));

            // render Delta Value Digits 
            char delta_buf[32];
            if (delta_val < 0.0f)
                snprintf(delta_buf, sizeof(delta_buf), "-%.3f", std::abs(delta_val));
            else if (delta_val > 0.0f)
                snprintf(delta_buf, sizeof(delta_buf), "+%.3f", delta_val);
            else
                snprintf(delta_buf, sizeof(delta_buf), "0.000");

            ImVec4 text_col = (delta_val < 0.0f) ? neg_col : 
                              (delta_val > 0.0f) ? pos_col : 
                              ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            // apply font scale safely
            float old_font_scale = ImGui::GetFont()->Scale;
            ImGui::GetFont()->Scale = std::clamp(config.delta_font_scale, 0.5f, 3.0f);
            ImGui::PushFont(ImGui::GetFont());

            float text_width = ImGui::CalcTextSize(delta_buf).x;
            ImGui::SetCursorPosX((avail.x - text_width) * 0.5f);
            {
                ImVec2 dpos = ImGui::GetCursorScreenPos();
                draw_text_maybe_bold(ImGui::GetWindowDrawList(), dpos, ImGui::ColorConvertFloat4ToU32(text_col), delta_buf, config.delta_font_bold);
            }
            ImGui::Dummy(ImGui::CalcTextSize(delta_buf));

            ImGui::PopFont();
            ImGui::GetFont()->Scale = old_font_scale;
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    inline void delta_settings_panel(bool* p_open, config::AppConfig& config)
    {
        if (ImGui::Begin("Delta Settings", p_open, 0))
        {
            ImGui::SetWindowSize(ImVec2(240, 120), ImGuiCond_FirstUseEver);

            ImGui::Text("Delta Mode");
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::RadioButton("All-Time / Best Lap", config.delta_use_all_time_best)) 
                config.delta_use_all_time_best = true;
            
            if (ImGui::RadioButton("Last Lap", !config.delta_use_all_time_best)) 
                config.delta_use_all_time_best = false;

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Appearance");
            ImGui::Spacing();

            ImGui::Checkbox("Bold Font", &config.delta_font_bold);
            ImGui::SliderFloat("Font Scale", &config.delta_font_scale, 0.5f, 2.5f, "%.1fx");
            ImGui::ColorEdit4("Delta negative", config.delta_negative_color);
            ImGui::ColorEdit4("Delta positive", config.delta_positive_color);

            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Reset Delta Colors"))
            {
                config.delta_negative_color[0] = 0.02f;  config.delta_negative_color[1] = 0.9f; config.delta_negative_color[2] = 0.0f; config.delta_negative_color[3] = 1.0f;
                config.delta_positive_color[0] = 0.898f;  config.delta_positive_color[1] = 0.133f; config.delta_positive_color[2] = 0.286f;   config.delta_positive_color[3] = 1.0f;
            }
        }
        ImGui::End();
    }

    // wheel widget ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline std::string default_wheel_image_path()
    {
        return std::string(RESOURCES_PATH) + "steering_white.png";
    }

    inline std::string effective_wheel_image_path(const config::AppConfig& config)
    {
        return config.wheel_image_path.empty() ? default_wheel_image_path() : config.wheel_image_path;
    }

    inline void wheel_size_callback(ImGuiSizeCallbackData* d)
    {
        d->DesiredSize.x = d->DesiredSize.y = std::max(d->DesiredSize.x, d->DesiredSize.y); // keep square
    }

    inline void wheel_widget(const config::AppConfig& config)
    {
        static std::string loaded_path;
        static ImTextureID tex = (ImTextureID)0;
        static bool load_failed = false;

        std::string want_path = effective_wheel_image_path(config);

        // (re)load the texture only when the resolved path changes
        if (want_path != loaded_path)
        {
            if (tex) { platform_free_texture(tex); tex = (ImTextureID)0; }
            loaded_path = want_path;
            int w = 0, h = 0;
            load_failed = !platform_load_texture(loaded_path.c_str(), tex, w, h);
        }

        float steer = 0.0f;
        float range_deg = 540.0f;
#ifdef IS_CONTROL_PANEL
        steer = 0.35f; // angle = 0.35 * (540 / 2) = 0.35 * 270 = 94.5°
        // steer is meant to range -1..1 where ±1.0 = full lock (270° each way for a 540° wheel), so 0.35 is 35% of the way to full right lock, i.e. 94.5° — a reasonable amount for, say, a slow corner.
#else
        LMUTelemetry::get().update();
        steer = LMUTelemetry::get().get_steering();
        range_deg = LMUTelemetry::get().get_wheel_range_deg();
#endif
        // +steer = right = clockwise on screen
        //float angle = steer * (range_deg * 0.5f) * 0.0174532925f;
        float angle = steer * (range_deg * 0.5f) * config.wheel_rotation_multiplier * 0.0174532925f;

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoCollapse;   // still resizable from the edges

        ImGui::SetNextWindowBgAlpha(config.opacity);
        ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(60, 60), ImVec2(800, 800), wheel_size_callback);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        if (ImGui::Begin("Wheel", nullptr, flags))
        {
            ImVec2 pos  = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            ImVec2 c(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
            float half = std::min(size.x, size.y) * 0.5f;

            if (tex && !load_failed)
            {
                float ca = std::cos(angle), sa = std::sin(angle);
                auto rot = [&](float x, float y) { return ImVec2(c.x + x * ca - y * sa, c.y + x * sa + y * ca); };

                ImGui::GetWindowDrawList()->AddImageQuad(tex,
                    rot(-half, -half), rot(half, -half), rot(half, half), rot(-half, half));
            }
            else
            {
                ImGui::TextWrapped("Wheel image failed to load.");
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

#ifdef IS_CONTROL_PANEL
    inline std::string pick_image_file()
    {
        char file[MAX_PATH] = {};
        OPENFILENAMEA ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = GetActiveWindow();
        ofn.lpstrFilter = "Images (*.png;*.jpg;*.jpeg;*.bmp;*.tga)\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All files\0*.*\0";
        ofn.lpstrFile   = file;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR; // NOCHANGEDIR: keeps "config.json" relative path valid
        return GetOpenFileNameA(&ofn) ? std::string(file) : std::string();
    }

    inline void wheel_settings_panel(bool* p_open, config::AppConfig& config)
    {
        static std::string error;

        if (ImGui::Begin("Wheel Settings", p_open, 0))
        {
            ImGui::SetWindowSize(ImVec2(340, 170), ImGuiCond_FirstUseEver);

            ImGui::Text("Wheel image (square, PNG with transparent background)");
            ImGui::Spacing();

            bool using_default = config.wheel_image_path.empty();
            ImGui::TextWrapped("%s", using_default
                ? "(using built-in default)"
                : config.wheel_image_path.c_str());
            ImGui::Spacing();

            if (ImGui::Button("Browse..."))
            {
                std::string path = pick_image_file();
                if (!path.empty())
                {
                    int w = 0, h = 0, ch = 0;
                    if (!stbi_info(path.c_str(), &w, &h, &ch))
                        error = "Could not read that image.";
                    else if (w != h)
                        error = "Image must be square (it is " + std::to_string(w) + "x" + std::to_string(h) + ").";
                    else
                    {
                        config.wheel_image_path = path;
                        error.clear();
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset to Default"))
            {
                config.wheel_image_path.clear();
                error.clear();
            }

            if (!error.empty())
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", error.c_str());

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Rotation");
            ImGui::SliderFloat("Rotation Multiplier", &config.wheel_rotation_multiplier, 0.3f, 1.5f, "%.2fx");
        }
        ImGui::End();   
    }
#endif

    // lap history widget ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline std::string format_lap_time(float seconds) // format seconds into mm:ss.fff format
    {
        if (seconds <= 0.0f) return "--:--.---";
        int mins = (int)(seconds / 60.0f);
        float secs = seconds - (mins * 60.0f);
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%06.3f", mins, secs);
        return std::string(buf);
    }

    void lap_history_widget(config::AppConfig& config)
    {
        LapHistory::get().update();   // restored — only call needed here now

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                        ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::SetNextWindowBgAlpha(config.opacity);
        ImGui::SetNextWindowSize(ImVec2(240, 220), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(180, 120), ImVec2(600, 500));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (ImGui::Begin("Lap History HUD", nullptr, window_flags))
        {
            std::vector<LapRecord> laps = LapHistory::get().recent_laps(config.lap_history_count);

            // calculate baseline
            float reference_time = 0.0f;
            if (!laps.empty()) 
            {
                if (config.lap_history_delta_session) 
                    reference_time = laps.empty() ? 0.0f : LapHistory::get().session_best_lap(laps[0].session_number);
                else 
                    reference_time = LapHistory::get().all_time_best_lap();
            }

            float old_font_scale = ImGui::GetFont()->Scale;
            ImGui::GetFont()->Scale *= std::clamp(config.lap_history_font_scale, 0.5f, 3.0f);
            ImGui::PushFont(ImGui::GetFont());

            ImGuiTableFlags table_flags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Borders |
                                        ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp; //ImGuiTableFlags_NoBordersInBody

            if (ImGui::BeginTable("LapHistoryTable", 3, table_flags))
            {
                ImGui::TableSetupColumn("Lap", ImGuiTableColumnFlags_WidthFixed, 35.0f * config.lap_history_font_scale);
                ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Delta", ImGuiTableColumnFlags_WidthFixed, 55.0f * config.lap_history_font_scale);
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < laps.size(); ++i)
                {
                    const auto& lap = laps[i];
                    ImGui::TableNextRow();

                    bool is_best = (reference_time > 0.0f && std::abs(lap.lap_time - reference_time) < 0.001f);

                    ImVec4 row_col = is_best ? ImVec4(config.time_fucsia_color[0], config.time_fucsia_color[1], config.time_fucsia_color[2], config.time_fucsia_color[3]) 
                                             : ImVec4(config.lap_history_font_color[0], config.lap_history_font_color[1], config.lap_history_font_color[2], config.lap_history_font_color[3]);
                    
                    ImGui::PushStyleColor(ImGuiCol_Text, row_col);

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", lap.lap_number);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", format_lap_time(lap.lap_time).c_str());

                    ImGui::TableSetColumnIndex(2);
                    bool in_active_session = !config.lap_history_delta_session || (lap.session_number == laps[0].session_number);
                    if (!in_active_session)
                    {
                        ImGui::Text("--");
                    }
                    else if (is_best)
                    {
                        ImGui::Text("0.000"); 
                    }
                    else if (reference_time > 0.0f)
                    {
                        float delta = lap.lap_time - reference_time;
                        char delta_buf[32];

                        ImVec4 delta_col;
                        if (delta <= 0.0f)
                            delta_col = ImVec4(config.time_green_color[0], config.time_green_color[1], config.time_green_color[2], config.time_green_color[3]);
                        else if (delta < 1.0f)
                            delta_col = ImVec4(config.time_yellow_color[0], config.time_yellow_color[1], config.time_yellow_color[2], config.time_yellow_color[3]);
                        else
                            delta_col = ImVec4(config.time_red_color[0], config.time_red_color[1], config.time_red_color[2], config.time_red_color[3]);

                        snprintf(delta_buf, sizeof(delta_buf), "%s%.3f", (delta > 0.0f ? "+" : "-"), std::abs(delta));

                        ImGui::PushStyleColor(ImGuiCol_Text, delta_col);
                        ImGui::Text("%s", delta_buf);
                        ImGui::PopStyleColor(); 
                    }
                    else
                    {
                        ImGui::Text("--");
                    }
                    
                    ImGui::PopStyleColor(); 
                }
                ImGui::EndTable();
            }
            ImGui::PopFont();
            ImGui::GetFont()->Scale = old_font_scale;
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    inline void lap_history_settings_panel(bool* p_open, config::AppConfig& config)
    {
        if (ImGui::Begin("Lap History Settings", p_open, 0))
        {
            ImGui::SetWindowSize(ImVec2(280, 220), ImGuiCond_FirstUseEver);

            ImGui::Text("Lap History Options");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Delta Reference Base:");
            if (ImGui::RadioButton("Entire File (All-Time Best)", !config.lap_history_delta_session))
                config.lap_history_delta_session = false;
            if (ImGui::RadioButton("Current Session Only", config.lap_history_delta_session))
                config.lap_history_delta_session = true;

            ImGui::Spacing();
            ImGui::SliderInt("Laps to Show", &config.lap_history_count, 1, 30);

            ImGui::Spacing();
            ImGui::Text("Text");
            ImGui::SliderFloat("Font Scale", &config.lap_history_font_scale, 0.5f, 2.5f, "%.1fx");
            ImGui::ColorEdit4("Font Color", config.lap_history_font_color);

            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Reset Defaults"))
            {
                config.lap_history_count = 10;
                config.lap_history_delta_session = false;
                config.lap_history_font_color[0] = 1.0f;
                config.lap_history_font_color[1] = 1.0f;
                config.lap_history_font_color[2] = 1.0f;
                config.lap_history_font_color[3] = 1.0f;
            }
        }
        ImGui::End();
    }

};

#endif