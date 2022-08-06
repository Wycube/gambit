#pragma once

#include "device/OGLVideoDevice.hpp"
#include "emulator/core/GBA.hpp"
#include "emulator/core/debug/Debugger.hpp"
#include "common/StringUtils.hpp"
#include "common/Log.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <glad/gl.h>


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

    DebuggerUI(emu::GBA &gba) : m_debugger(gba.getDebugger()), m_gba(gba), m_video_device(dynamic_cast<OGLVideoDevice&>(gba.getVideoDevice())) {
        m_region_sizes[7] = m_gba.getGamePak().size();
        m_debugger.addBreakpoint(0x02038000);
        m_debugger.addBreakpoint(0x08005C70);
        m_debugger.addBreakpoint(0x08000A92); //Good
        m_debugger.addBreakpoint(0x08004D20); //Bad
        m_debugger.addBreakpoint(0x08004D10); //Good
        m_debugger.addBreakpoint(0x03004210);
    }

    void drawScreen() {
        ImGui::Image((void*)(intptr_t)m_video_device.getTextureID(), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y));
    }

    void drawPPUState() {
        ImGui::Text("Mode: %i", m_debugger.read8(0x4000000) & 0x7);
        ImGui::Text("DSPCNT: %04X", m_debugger.read16(0x4000000));
        ImGui::Text("WININ: %04X", m_debugger.read16(0x40000048));
        ImGui::Text("WINOUT: %04X", m_debugger.read16(0x400004A));

        //Update texture
        ImGui::Image((void*)(intptr_t)m_video_device.getTextureID(), ImVec2(ImGui::GetContentRegionAvail().x, (160.0f / 240.0f) * ImGui::GetContentRegionAvail().x));
    }

    void drawBreakpoints() {
        bool running = m_debugger.running();

        if(running) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        }

        const std::vector<u32> &bkpts = m_debugger.getBreakpoints();

        ImGui::Text("Breakpoints");
        ImGui::Separator();

        static u32 address_input;
        ImGui::InputScalar("##NewBreakpointAddress_InputU32", ImGuiDataType_U32, &address_input, 0, 0, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
        address_input &= ~1;
        ImGui::SameLine();

        if(ImGui::Button("Add")) {
            m_debugger.addBreakpoint(address_input);
        }
        ImGui::Separator();

        for(int i = bkpts.size() - 1; i >= 0; i--) {
            if(ImGui::Button("Delete")) {
                m_debugger.removeBreakpoint(bkpts[i]);
                continue;
            }

            ImGui::SameLine();
            ImGui::Text("%i : 0x%08X", i, bkpts[i]);
        }

        if(running) {
            ImGui::PopItemFlag();
        }
    }

    void drawCPUDebugger() {
        bool running = m_debugger.running();

        if(running) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        }

        if(ImGui::Button("Step")) {
            m_gba.step();
        }

        if(running) {
            ImGui::PopItemFlag();
        }

        ImGui::SameLine();

        if((!running && ImGui::Button("Run")) || (running && ImGui::Button("Pause"))) {
            m_gba.getDebugger().setRunning(!running);
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

        if(ImGui::BeginChild("##DebuggerDisassemblyList_Child", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
            ImGui::BeginTable("##Disassembly_Table", 4);
            ImGui::TableSetupColumn("col_0", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
            ImGui::TableSetupColumn("col_1", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
            ImGui::TableSetupColumn("col_2", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
            ImGui::TableSetupColumn("col_3");
         
            //THUMB or ARM
            bool thumb = (m_debugger.getCPUCPSR() >> 5) & 1;
            u8 instr_size = thumb ? 2 : 4;

            ImGuiListClipper clipper(101);
            
            while(clipper.Step()) {
                for(u32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    u32 address = m_debugger.getCPURegister(15) + (i - 50) * instr_size;
                    ImGui::Text("%08X: ", address);

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

                    //Seperate mnemonic and registers
                    size_t space = disassembled.find_first_of(' ');
                    ImGui::Text("%s", disassembled.substr(0, space).c_str());
                    ImGui::TableNextColumn();
                    if(space < disassembled.size()) ImGui::Text("%s", disassembled.substr(space).c_str());
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
        ImGui::Text("Cycle: %llu", m_debugger.getCurrentCycle());
        for(u32 i = 0; i < m_debugger.numEvents(); i++) {
            ImGui::Text("%i : %s -> %llu cycles", i, m_debugger.getEventTag(i).c_str(), m_debugger.getEventCycles(i));
        }
    }

private:

    emu::dbg::Debugger &m_debugger;
    emu::GBA &m_gba;
    OGLVideoDevice &m_video_device;

    //CPU Registers mode
    int m_mode = 0;

    //Memory Viewer
    int m_memory_region = 0;
    const char *m_regions = "BIOS\0EWRAM\0IWRAM\0MMIO\0Palette RAM\0VRAM\0OAM\0Cartridge ROM\0";
    u32 m_region_sizes[8] = {16_KiB, 256_KiB, 32_KiB, 1023, 1_KiB, 96_KiB, 1_KiB, 32_MiB};
    u32 m_region_start[8] = {0, 0x02000000, 0x03000000, 0x04000000, 0x05000000, 0x06000000, 0x07000000, 0x08000000};
};