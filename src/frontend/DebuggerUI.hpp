#pragma once

#include "core/GBA.hpp"
#include "core/debug/Debugger.hpp"
#include "common/StringUtils.hpp"
#include "common/Bits.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <glad/gl.h>
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
public:

    DebuggerUI(emu::GBA &gba) : m_debugger(gba.getDebugger()), m_gba(gba) {
        m_region_sizes[7] = m_gba.getGamePak().size();
        m_debugger.setBreakPoint(0x08003E20);

        //Create OpenGL Texture
        glGenTextures(1, &m_vram_tex);
        glBindTexture(GL_TEXTURE_2D, m_vram_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 240, 160, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, m_debugger.getFramebuffer());
        
        #if _DEBUG
        //Label the texture
        glObjectLabel(GL_TEXTURE, m_vram_tex, -1, "GBA Screen Texture");
        #endif

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~DebuggerUI() {
        if(m_vram_tex != 0) {
            glDeleteTextures(1, &m_vram_tex);
        }
    }

    auto running() -> bool {
        if(m_running && m_debugger.atBreakPoint()) {
            LOG_DEBUG("Break Point at 0x{:08X} hit!", m_debugger.getBreakPoint());
            m_running = false;
        }

        return m_running;
    }

    void drawPPUState() {
        ImGui::Text("Mode: %i", m_debugger.read8(0x4000000) & 0x7);
        ImGui::Text("DSPCNT: %04X", m_debugger.read16(0x4000000));

        //Update texture
        glBindTexture(GL_TEXTURE_2D, m_vram_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 240, 160, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, m_debugger.getFramebuffer());
        glBindTexture(GL_TEXTURE_2D, 0);

        ImGui::Image((void*)(intptr_t)m_vram_tex, ImVec2(ImGui::GetContentRegionAvail().x, (160.0f / 240.0f) * ImGui::GetContentRegionAvail().x));
    }

    void drawCPUDebugger() {
        if(m_running) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        }

        if(ImGui::Button("Step")) {
            m_gba.step();
        }

        if(m_running) {
            ImGui::PopItemFlag();
        }

        ImGui::SameLine();

        if((!m_running && ImGui::Button("Run")) || (m_running && ImGui::Button("Pause"))) {
            m_running = !m_running;
        }


        ImGui::Separator();
        ImGui::BeginGroup();

        //Mode select
        ImGui::SetNextItemWidth(200.0f);
        ImGui::Combo("Mode", &m_mode, "Current\0User\0System\0Supervisor\0FIQ\0IRQ\0Abort\0Undefined\0");

        static constexpr u8 modes[8] = {0, emu::MODE_USER, emu::MODE_SYSTEM, emu::MODE_SUPERVISOR, emu::MODE_FIQ, emu::MODE_IRQ, emu::MODE_ABORT, emu::MODE_UNDEFINED};
        u8 mode = modes[m_mode];

        //CPU Registers
        if(ImGui::BeginTable("##CPURegisters_Table", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableNextRow();

            for(u8 i = 0; i < 8; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("r%-2i: %08X", i, m_debugger.getCPURegister(i, mode));
                
                ImGui::TableNextColumn();
                ImGui::Text("r%-2i: %08X", 8 + i, m_debugger.getCPURegister(8 + i, mode));
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();

        u32 cpsr = m_debugger.getCPUCPSR();
        u32 spsr = m_debugger.getCPUSPSR(mode);
        static constexpr char flag_name[7] = {'N', 'Z', 'C', 'V', 'I', 'F', 'T'};
        static constexpr u8 flag_bit[7] = {31, 30, 29, 28, 7, 6, 5};

        ImGui::Text("CPSR");
        ImGui::Text("%08X", cpsr);
        ImGui::Text("Mode: %s", get_mode_str(bits::get<0, 5>(cpsr)).c_str());
        for(int i = 0; i < sizeof(flag_name); i++) {
            ImGui::Text("%c:", flag_name[i]);
            ImGui::SameLine();

            bool set = (cpsr >> flag_bit[i]) & 0x1;
            ImGui::PushStyleColor(ImGuiCol_Text, set ? ImVec4(0.0f, 0.7f, 0.0f, 1.0f) : ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("%1i", set);
            ImGui::PopStyleColor();

            if(i % 4 != 3 && i != sizeof(flag_name) - 1) ImGui::SameLine();
        }

        ImGui::Text("SPSR");
        ImGui::Text("%08X", spsr);
        ImGui::Text("Mode: %s", get_mode_str(bits::get<0, 5>(spsr)).c_str());
        for(int i = 0; i < sizeof(flag_name); i++) {
            ImGui::Text("%c:", flag_name[i]);
            ImGui::SameLine();

            bool set = (spsr >> flag_bit[i]) & 0x1;
            ImGui::PushStyleColor(ImGuiCol_Text, set ? ImVec4(0.0f, 0.7f, 0.0f, 1.0f) : ImVec4(0.7f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("%1i", set);
            ImGui::PopStyleColor();

            if(i % 4 != 3 && i != sizeof(flag_name) - 1) ImGui::SameLine();
        }

        
        ImGui::EndGroup();
        //ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF0000FF);
    
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();


        ImGui::BeginGroup();

        ImGui::Text("Disassembly");
        ImGui::Separator();

        bool go_to_pc = ImGui::Button("Go to PC");

        if(ImGui::BeginChild("##DebuggerDisassemblyList_Child", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar)) {
            ImGui::BeginTable("##Disassembly_Table", 3, ImGuiTableFlags_SizingFixedFit);
         
            //THUMB or ARM
            bool thumb = (m_debugger.getCPUCPSR() >> 5) & 1;
            u8 instr_size = thumb ? 2 : 4;

            ImGuiListClipper clipper(101);
            
            while(clipper.Step()) {
                for(u32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    u32 address = m_debugger.getCPURegister(15) + (i - 50) * instr_size;
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
                        if(go_to_pc) ImGui::SetScrollHereY();
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

            ImGui::EndTable();
            
            if(go_to_pc) {
                //Scroll to 3 instructions before the current PC
                ImGui::SetScrollY(47.5f * clipper.ItemsHeight);
            }
            
        }
        ImGui::EndChild();

        ImGui::EndGroup();
        //ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF000000);
    }

    void drawMemoryViewer() {
        ImGui::Text("Memory Region");
        ImGui::SameLine();
        ImGui::Combo("##Memory_Region_Combo", &m_memory_region, m_regions);

        if(ImGui::BeginChild("##MemoryTable_Child")) {
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
        ImGui::EndChild();
    }

    void drawSchedulerViewer() {
        ImGui::Text("Cycle: %i", m_debugger.getCurrentCycle());
        for(u32 i = 0; i < m_debugger.numEvents(); i++) {
            ImGui::Text("%i : %s -> %i cycles", i, m_debugger.getEventTag(i).c_str(), m_debugger.getEventCycles(i));
        }
    }

private:

    emu::dbg::Debugger &m_debugger;
    emu::GBA &m_gba;

    //CPU Registers mode
    int m_mode = 0;

    //VRAM Texture
    u32 m_vram_tex = 0;

    //Memory Viewer
    int m_memory_region = 0;
    const char *m_regions = "BIOS\0EWRAM\0IWRAM\0MMIO\0Palette RAM\0VRAM\0OAM\0Cartridge ROM\0";
    u32 m_region_sizes[8] = {16_KiB, 256_KiB, 32_KiB, 1023, 1_KiB, 96_KiB, 1_KiB, 32_MiB};
    u32 m_region_start[8] = {0, 0x02000000, 0x03000000, 0x04000000, 0x05000000, 0x06000000, 0x07000000, 0x08000000};

    bool m_running = false;
};