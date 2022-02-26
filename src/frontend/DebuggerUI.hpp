#pragma once

#include "core/GBA.hpp"
#include "core/debug/Debugger.hpp"
#include "common/StringUtils.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>


auto get_mode_str(u8 mode_bits) -> std::string {
    switch(mode_bits) {
        case emu::MODE_USER : return "User";
        case emu::MODE_FIQ : return "FIQ";
        case emu::MODE_IRQ : return "IRQ";
        case emu::MODE_SUPERVISOR : return "Supervisor (SWI)";
        case emu::MODE_ABORT : return "Abort";
        case emu::MODE_UNDEFINED : return "Undefined";
        case emu::MODE_SYSTEM : return "System";
        default : return "Invalid";
    }
}


class DebuggerUI {
private:

    emu::dbg::Debugger &m_debugger;
    emu::GBA &m_gba;

    //Disassembly
    bool m_to_current = true;

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
                    m_to_current = true;
                }

            //}

            ImGui::Separator();

            if(ImGui::BeginChild("##DebuggerTabRegion_Child", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
                ImGui::BeginTabBar("##Debugger_TabBar");

                if(ImGui::BeginTabItem("CPU")) {
                    ImGui::BeginChild("##CPUState_Child");
                    drawCPUState();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                
                if(ImGui::BeginTabItem("Disassembly")) {
                    ImGui::BeginChild("##Disassembly_Child");
                    drawDisassembly();
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                
                if(ImGui::BeginTabItem("Memory Viewer")) {
                    ImGui::BeginChild("##MemoryViewer_Child");
                    drawMemoryViewer();
                    ImGui::EndChild();
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
        ImGui::Text("Current Execution Mode: %s", (m_debugger.getCPUCurrentStatus() >> 5) & 0x1 ? "THUMB" : "ARM");
        
        int mode = (m_debugger.getCPUCurrentStatus() & 0x1F);
        ImGui::Text("Current Privilege Mode: %s", get_mode_str(mode).c_str());
        
        ImGui::Text("CPSR: %08X", m_debugger.getCPUCurrentStatus());
        ImGui::BeginTable("##CPSR_Table", 32, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame);
        ImGui::TableNextRow();
        for(int i = 31; i >= 0; i--) {
            ImGui::TableNextColumn();
            ImGui::Text("%i", i);
        }
        ImGui::TableNextRow();
        for(int i = 31; i >= 0; i--) {
            ImGui::TableNextColumn();
            ImGui::Text("%i", (m_debugger.getCPUCurrentStatus() >> i) & 0x1);
        }
        ImGui::EndTable();

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
        ImGui::BeginTable("##Disassembly_Table", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg);
        
        //THUMB or ARM
        bool thumb = (m_debugger.getCPUCurrentStatus() >> 5) & 1;
        u8 instr_size = thumb ? 2 : 4;

        ImGuiListClipper clipper(m_region_sizes[3] / instr_size);
        
        while(clipper.Step()) {
            for(u32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                u32 address = m_region_start[3] + i * instr_size;
                ImGui::Text("%08X ", address);

                ImGui::TableNextColumn();

                if(address == m_debugger.getCPURegister(15)) { 
                    //Fetch
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF950000);
                } else if(address == m_debugger.getCPURegister(15) - instr_size) {
                    //Decode
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF008500);
                } else if(address == m_debugger.getCPURegister(15) - instr_size * 2) {
                    //Execute
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF000085);
                }

                //Instruction in hexadecimal
                u32 bytes = thumb ? m_debugger.read16(address) : m_debugger.read32(address);
                ImGui::Text(fmt::format("%0{}X ", thumb ? 4 : 8).c_str(), bytes);

                //Actual disassembly
                ImGui::TableNextColumn();
                std::string disassembled = thumb ? m_debugger.thumbDisassembleAt(address) : m_debugger.armDisassembleAt(address);
                ImGui::Text("%s", disassembled.c_str());
            }
        }

        if(m_to_current) {
            ImGui::SetScrollY(((m_debugger.getCPURegister(15) - m_region_start[3]) / instr_size - 10) * clipper.ItemsHeight);
            m_to_current = false;
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
                    ImGui::Text("%02X", m_debugger.read8(region_start + line_address + j));
                }

                ImGui::TableNextColumn();
                std::string ascii;
                for(int j = 0; j < 16; j++) {
                    if(line_address + j >= region_size) {
                        break;
                    }

                    char c = static_cast<char>(m_debugger.read8(region_start + line_address + j));
                    ascii += common::is_printable(c) ? c : '.';
                }
                ImGui::Text(" %-16s", ascii.c_str());
            }
        }

        ImGui::EndTable();
    }
};