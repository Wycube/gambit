#pragma once

#include "device/OGLVideoDevice.hpp"
#include "emulator/core/GBA.hpp"
#include "emulator/core/debug/Debugger.hpp"
#include "emulator/core/cpu/arm/Instruction.hpp"
#include "emulator/core/cpu/thumb/Instruction.hpp"
#include "common/StringUtils.hpp"
#include "common/Log.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>


inline auto get_mode_str(u8 mode_bits) -> std::string {
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

    DebuggerUI(emu::GBA &gba) : m_debugger(gba.debugger), m_gba(gba), m_video_device(dynamic_cast<OGLVideoDevice&>(gba.video_device)) {
        m_region_sizes[7] = m_gba.getGamePak().size();

        // m_debugger.addBreakpoint(0x03003CBE);
    }

    void drawScreen() {
        //TODO: Only do this on resize
        ImVec2 available = ImGui::GetContentRegionAvail();
        float gba_aspect_ratio = 240.0f / 160.0f;
        float new_width, new_height;

        if(available.x / available.y > gba_aspect_ratio) {
            new_width = available.y * gba_aspect_ratio;
            new_height = available.y;
        } else {
            new_width = available.x;
            new_height = available.x / gba_aspect_ratio;
        }

        ImGui::SetCursorPos(ImVec2((available.x - new_width) * 0.5f, (available.y - new_height) * 0.5f));
        ImGui::Image((void*)(intptr_t)m_video_device.getTextureID(), ImVec2(new_width, new_height));
    }

    void drawPPUState() {
        ImGui::Text("Mode: %i", m_debugger.read8(0x4000000) & 0x7);
        ImGui::Text("DSPCNT: %04X", m_debugger.read16(0x4000000));
        ImGui::Text("WININ: %04X", m_debugger.read16(0x40000048));
        ImGui::Text("WINOUT: %04X", m_debugger.read16(0x400004A));
    }

    void drawBreakpoints(bool running) {
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

    void drawCPUDebugger(bool running) {
        if(running) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        }

        if(ImGui::Button("Step")) {
            m_gba.step();
        }

        if(running) {
            ImGui::PopItemFlag();
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
                ImGui::Text("r%-2i: %08X", i, m_debugger.getCPURegister(i));
                
                ImGui::TableNextColumn();
                ImGui::Text("r%-2i: %08X", 8 + i, m_debugger.getCPURegister(8 + i));
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

        ImGui::Spacing();

        if(!running) {
            // u32 sp = m_debugger.getCPURegister(13);
            // for(u32 i = 0; i < (0x3007FFF - sp + 4 ) / 4; i++) {
            //     ImGui::Text("%08X : %08X", 0x3007FFF - i * 4 & ~3, m_debugger.read32(0x3007FFF - i * 4 & ~3));
            // }
        }

        
        ImGui::EndGroup();
        //ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0xFF0000FF);
    
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetContentRegionAvail().y));
        ImGui::SameLine();
        // ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();


        // ImGui::BeginGroup();

        // ImGui::Text("Disassembly");
        // ImGui::Separator();

        // bool go_to_pc = ImGui::Button("Go to PC");

        // if(ImGui::BeginChild("##DebuggerDisassemblyList_Child", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
        //     ImGui::BeginTable("##Disassembly_Table", 4);
        //     ImGui::TableSetupColumn("col_0", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
        //     ImGui::TableSetupColumn("col_1", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
        //     ImGui::TableSetupColumn("col_2", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
        //     ImGui::TableSetupColumn("col_3");
         
        //     //THUMB or ARM
        //     bool thumb = (m_debugger.getCPUCPSR() >> 5) & 1;
        //     u8 instr_size = thumb ? 2 : 4;

        //     ImGuiListClipper clipper(101);
            
        //     while(clipper.Step()) {
        //         for(u32 i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
        //             ImGui::TableNextRow();
        //             ImGui::TableSetColumnIndex(0);

        //             u32 address = m_debugger.getCPURegister(15) + (i - 50) * instr_size;
        //             ImGui::Text("%08X: ", address);

        //             ImGui::TableNextColumn();

        //             if(address == m_debugger.getCPURegister(15)) { 
        //                 //Fetch
        //                 ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF950000);
        //             } else if(address == m_debugger.getCPURegister(15) - instr_size) {
        //                 //Decode
        //                 ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF008500);
        //             } else if(address == m_debugger.getCPURegister(15) - instr_size * 2) {
        //                 //Execute
        //                 ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF000085);
        //             }

        //             //Instruction in hexadecimal
        //             u32 bytes = thumb ? m_debugger.read16(address) : m_debugger.read32(address);
        //             ImGui::Text(fmt::format("%0{}X ", thumb ? 4 : 8).c_str(), bytes);

        //             //Actual disassembly
        //             ImGui::TableNextColumn();
        //             std::string disassembled = thumb ? m_debugger.thumbDisassembleAt(address) : m_debugger.armDisassembleAt(address);

        //             //Seperate mnemonic and registers
        //             size_t space = disassembled.find_first_of(' ');
        //             ImGui::Text("%s", disassembled.substr(0, space).c_str());
        //             ImGui::TableNextColumn();
        //             if(space < disassembled.size()) ImGui::Text("%s", disassembled.substr(space).c_str());
        //         }
        //     }

        //     ImGui::EndTable();
            
        //     if(go_to_pc) {
        //         //Scroll to 3 instructions before the current PC
        //         ImGui::SetScrollY(47.5f * clipper.ItemsHeight);
        //     }
            
        // }
        // ImGui::EndChild();

        // ImGui::EndGroup();
    }

    void drawDisassembly() {
        ImGui::BeginGroup();

        ImGui::Text("Disassembly");
        ImGui::Separator();

        // bool go_to_pc = ImGui::Button("Go to PC");
        static bool use_thumb; 
        ImGui::Checkbox("Thumb", &use_thumb);

        static bool scroll_to_center;
        static bool follow_pc;
        static u32 address_input;

        ImGui::SameLine();
        if(ImGui::InputScalar("Go to", ImGuiDataType_U32, &address_input, 0, 0, "%X", ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AlwaysInsertMode)) {
            scroll_to_center = true;
        }

        // if(ImGui::Button("Go to button")) {
        //     go_to_address = true;
        // }

        ImGui::Checkbox("Follow PC", &follow_pc);

        if(!follow_pc) {
            ImGui::SameLine();
        }

        if(follow_pc || ImGui::Button("Go to PC")) {
            address_input = m_debugger.getCPURegister(15) - (use_thumb ? 2 : 4);
        }

        emu::GamePak &pak = m_gba.getGamePak();


        if(ImGui::BeginChild("##DebuggerDisassemblyList_Child", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding)) {
            ImGui::BeginTable("##Disassembly_Table", 4);
            ImGui::TableSetupColumn("col_0", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
            ImGui::TableSetupColumn("col_1", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize(use_thumb ? "xx xx " : "xx xx xx xx ").x);
            ImGui::TableSetupColumn("col_2", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("xxxxxxxxx").x);
            ImGui::TableSetupColumn("col_3");
         
            //THUMB or ARM
            bool thumb = use_thumb; //(m_debugger.getCPUCPSR() >> 5) & 1;
            u8 instr_size = thumb ? 2 : 4;

            ImGuiListClipper clipper(101);
            
            while(clipper.Step()) {
                for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    //u32 address = m_debugger.getCPURegister(15) + (i - 50) * instr_size;
                    u32 address = address_input + (i - 50) * instr_size;
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
                    }

                    //Instruction in hexadecimal
                    u32 bytes = thumb ? m_debugger.read16(address) : m_debugger.read32(address); //m_debugger.read16(address) : m_debugger.read32(address);
                    ImGui::Text("%s", fmt::format(instr_size == 2 ? "{2:02X} {3:02X}" : "{:02X} {:02X} {:02X} {:02X}", 
                        m_debugger.read8(address + 3), m_debugger.read8(address + 2), 
                        m_debugger.read8(address + 1), m_debugger.read8(address + 0)).c_str());

                    //Actual disassembly
                    ImGui::TableNextColumn();
                    std::string disassembled = thumb ? emu::thumbDecodeInstruction(bytes, address, m_debugger.read16(address - 2)).disassembly : emu::armDecodeInstruction(bytes, address).disassembly;

                    //Seperate mnemonic and registers
                    size_t space = disassembled.find_first_of(' ');
                    ImGui::Text("%s", disassembled.substr(0, space).c_str());
                    ImGui::TableNextColumn();
                    if(space < disassembled.size()) ImGui::Text("%s", disassembled.substr(space).c_str());
                }
            }

            ImGui::EndTable();

            if(scroll_to_center) {
                ImGui::SetScrollY(45 * clipper.ItemsHeight);
                scroll_to_center = false;
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
            ImGui::Text("%i : %u -> %llu cycles", i, m_debugger.getEventHandle(i), m_debugger.getEventCycles(i));
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