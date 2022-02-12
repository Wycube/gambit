#pragma once

#include "core/GBA.hpp"
#include "core/debug/Debugger.hpp"
#include "common/StringUtils.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>


class DebuggerUI {
private:

    emu::dbg::Debugger &m_debugger;
    emu::GBA &m_gba;

    //Memory Viewer
    int m_memory_region = 0;
    const char *m_regions = "BIOS\0EWRAM\0IWRAM\0Cartridge ROM\0";
    u32 m_region_sizes[4] = {16_KiB, 256_KiB, 32_KiB, 16_MiB};
    u32 m_region_start[4] = {0, 0x02000000, 0x03000000, 0x08000000};

public:

    DebuggerUI(emu::dbg::Debugger &debugger, emu::GBA &gba) : m_debugger(debugger), m_gba(gba) { }

    void draw(bool *show = nullptr) {
        if(!*show) {
            return;
        }
        
        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if(ImGui::Begin("Debugger", show)) {
            //ImGui::PopStyleVar();

            ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_ChildWindow;
            //if(ImGui::BeginChild("##DebuggerControlRegion_Child", ImVec2(0, 0), false, flags)) {
                if(ImGui::Button("Step CPU")) {
                    m_gba.step();
                }

            //}

            ImGui::Separator();

            if(ImGui::BeginChild("##DebuggerTabRegion_Child", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
                ImGui::BeginTabBar("##Debugger_TabBar");

                if(ImGui::BeginTabItem("CPU")) {
                    drawCPUState();
                    ImGui::EndTabItem();
                }
                
                if(ImGui::BeginTabItem("Disassembly")) {
                    drawDisassembly();
                    ImGui::EndTabItem();
                }
                
                if(ImGui::BeginTabItem("Memory Viewer")) {
                    drawMemoryViewer();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
            ImGui::EndChild();
        } else {
            ImGui::PopStyleVar();
        }
        ImGui::End();
    }

    void drawCPUState() {
        ImGui::BeginTable("##CPURegisters_Table", 4);
        ImGui::TableNextRow();

        for(u8 i = 0; i < 16; i++) {
            ImGui::TableNextColumn();
            ImGui::Text("R%-2i: %08X", i, m_debugger.getCPURegister(i));
        }
        ImGui::EndTable();

        static const char *FLAG_NAMES[4] = {"Z", "N", "C", "V"};
        static const emu::Flag FLAGS[4] = {emu::FLAG_ZERO, emu::FLAG_NEGATIVE, emu::FLAG_CARRY, emu::FLAG_OVERFLOW};
        
        ImGui::Separator();
        ImGui::Text("CPSR: %08X", m_debugger.getCPUCurrentStatus());
        ImGui::BeginTable("##CPSR_Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);
        ImGui::TableNextRow();
        for(auto name : FLAG_NAMES) {
            ImGui::TableNextColumn();
            ImGui::Text("%s", name);
        }
        ImGui::TableNextRow();
        for(auto flag : {emu::FLAG_ZERO, emu::FLAG_NEGATIVE, emu::FLAG_CARRY, emu::FLAG_OVERFLOW}) {
            ImGui::TableNextColumn();
            ImGui::Text("%i", (m_debugger.getCPUCurrentStatus() >> flag) & 0x1);
        }
        ImGui::EndTable();
        ImGui::Text("Current Mode: %s", (m_debugger.getCPUCurrentStatus() >> 5) & 0x1 ? "THUMB" : "ARM");

        ImGui::Text("SPSR: %08X", m_debugger.getCPUSavedStatus());
        ImGui::BeginTable("##SPSR_Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit);
        ImGui::TableNextRow();
        for(auto name : FLAG_NAMES) {
            ImGui::TableNextColumn();
            ImGui::Text("%s", name);
        }
        ImGui::TableNextRow();
        for(auto flag : {emu::FLAG_ZERO, emu::FLAG_NEGATIVE, emu::FLAG_CARRY, emu::FLAG_OVERFLOW}) {
            ImGui::TableNextColumn();
            ImGui::Text("%i", (m_debugger.getCPUSavedStatus() >> flag) & 0x1);
        }
        ImGui::EndTable();
    }

    void drawDisassembly() {
        ImGui::BeginTable("##Disassembly_Table", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersH);
        
        for(int i = -11; i <= 11; i++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            ImGui::Text("%08X ", m_debugger.getCPURegister(15) + i * 4);

            ImGui::TableNextColumn();

            if(i == 0) {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.7f, 0.0f, 0.0f, 1.0f});
            } else if(i == -1) {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.7f, 0.0f, 1.0f});
            } else if(i == -2) {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.7f, 1.0f});
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, {0.0f, 0.0f, 0.0f, 0.0f});
            }

            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::Button(m_debugger.armDisassembleAt(m_debugger.getCPURegister(15) + i * 4).c_str());
            ImGui::PopItemFlag();
            ImGui::PopStyleColor();

            if(i == 0) {
                ImGui::TableNextColumn();
                ImGui::Text("(Fetch)");
            } else if(i == -1) {
                ImGui::TableNextColumn();
                ImGui::Text("(Decode)");
            } else if(i == -2) {
                ImGui::TableNextColumn();
                ImGui::Text("(Execute)");
            }
        }

        ImGui::EndTable();
    }

    void drawMemoryViewer() {
        ImGui::Text("Memory Region");
        ImGui::SameLine();
        ImGui::Combo("##Memory_Region_Combo", &m_memory_region, m_regions);

        u32 region_start = m_region_start[m_memory_region];
        u32 region_size = m_region_sizes[m_memory_region];

        ImGui::BeginTable("##MemoryViewer_Table", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH);
        ImGui::TableHeadersRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Address");
        ImGui::TableSetColumnIndex(1);
        ImGui::Text("Hex (16 Bytes)");
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("ASCII");

        ImGuiListClipper clipper(std::round((float)region_size / 16.0f));

        while(clipper.Step()) {
            for(size_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                u32 line_address = i * 16;
                ImGui::Text("%08X ", region_start + line_address);

                ImGui::TableNextColumn();

                for(int j = 0; j < 16; j++) {
                    if(line_address + j >= region_size) {
                        break;
                    }

                    ImGui::SameLine();
                    ImGui::Text("%02X", m_debugger.readByte(region_start + line_address + j));
                }

                ImGui::TableNextColumn();
                std::string ascii;
                for(int j = 0; j < 16; j++) {
                    if(line_address + j >= region_size) {
                        break;
                    }

                    char c = static_cast<char>(m_debugger.readByte(region_start + line_address + j));
                    ascii += common::is_printable(c) ? c : '.';
                }
                ImGui::Text(" %-16s", ascii.c_str());
            }
        }

        ImGui::EndTable();
    }
};