#ifndef WIDGETS_H
#define WIDGETS_H

#include <shared/config.h>
#include <dll/lmu_telemetry.h>
#include <imgui.h>

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
        if (ImGui::Begin("Pedals Widget Settings", p_open, 0))
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
        if (ImGui::Begin("Delta Widget Settings", p_open, 0))
        {
            ImGui::Text("Delta settings");
        }
        ImGui::End();
    }
};

#endif