#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Memory.hpp"

// IMPORTANT: This is Updated for version "version-76173e47a79145c7"
// 4C 8B 25 ? ? ? ? 4C 89 65
// 4C 8B 25 ? ? ? ? 4C 89 ? ? 66 0F 6F 05
// 4C 8B 25 ? ? ? ? 4C 89 ? ? ? 66 0F 6F 05
namespace offsets {
    uint64_t v17 = 0x1000001E3;
    uint64_t Encryptions1[] = { 0x1AAAF, 0x1AE2B, 0x545DF };
    uint64_t Encryptions2[] = { 0x1546A, 0xB8DC758, 0x27FE };
    uint64_t v12 = 0x7825530;
    uint64_t si = 0x6C141F0;
}
// Thanks to Roblox for these changes but you'll NEVER be able to stop me. <3

uint64_t CoolShit(uint64_t value, int index, int n = 1) {
    uint64_t result = value;
    for (int i = 0; i < n; i++) {
        result = (offsets::Encryptions2[index] + offsets::Encryptions1[index] * result) % offsets::v17;
    }
    return result;
}

std::string DecryptRuleset(int index) {
    uint64_t RobloxBase = Memory::RobloxBase;

    uint64_t v12_ptr = Memory::read<uint64_t>(RobloxBase + offsets::v12);

    uint32_t v79 = (uint32_t)(_rotl64(0x41692277ULL, 0x37) ^ 0x6CE50D47);
    uint64_t v14 = (v79 ^ 0x64DA6611) + 0x5984AF49;
    v14 &= 0xFFFFFFFF;

    uint64_t sibase = Memory::read<uint64_t>(RobloxBase + offsets::si);
    uint64_t sibase2 = Memory::read<uint64_t>(RobloxBase + offsets::si + 8);

    uint64_t s1 = CoolShit(v14, 0, 2);
    uint64_t s2 = CoolShit(sibase, 1, 2);
    uint64_t s3 = CoolShit(sibase2, 2, 2);

    uint32_t head = Memory::read<uint32_t>(v12_ptr + 4);
    uint32_t v7 = (head ^ (uint32_t)s1 ^ (uint32_t)s2 ^ (uint32_t)s3) + (12 * index);

    s1 = CoolShit(v7, 0, 2);
    s2 = CoolShit(s2, 1, 3);
    s3 = CoolShit(s3, 2, 3);

    uint32_t v8 = Memory::read<uint32_t>(v12_ptr + v7) ^ (uint32_t)s1 ^ (uint32_t)s2 ^ (uint32_t)s3;

    s1 = CoolShit(v14, 0, 1);
    s2 = CoolShit(v8, 1, 1);
    s3 = 0x7785BD7CLL;

    uint64_t Data = v12_ptr + v8;
    uint32_t Entry = Memory::read<uint32_t>(Data) ^ (uint32_t)s1 ^ (uint32_t)s2 ^ (uint32_t)s3;
    Data += 4;

    uint16_t lenght = (uint16_t)Entry;
    if (lenght <= 2 || lenght > 0x500) {
        return "";
    }

    std::string DecryptedRuleset = "";
    DecryptedRuleset += (char)((Entry >> 16) & 0xFF);
    DecryptedRuleset += (char)((Entry >> 24) & 0xFF);

    uint32_t SigmaKey = 0;
    for (int i = 0; i < (lenght - 2); i++) {
        if (i % 4 == 0) {
            s1 = CoolShit(s1, 0, 1);
            s2 = CoolShit(s2, 1, 1);
            s3 = CoolShit(s3, 2, 1);
            uint32_t data_val = Memory::read<uint32_t>(Data);
            SigmaKey = data_val ^ (uint32_t)s1 ^ (uint32_t)s2 ^ (uint32_t)s3;
            Data += 4;
        }
        char charac = (char)((SigmaKey >> ((i % 4) * 8)) & 0xFF);
        if (charac) {
            DecryptedRuleset += charac;
        }
    }

    return DecryptedRuleset;
}

// yes i didn't write this
std::string JsonEscape(const std::string& input) {
    std::ostringstream ss;

    for (unsigned char c : input) {
        switch (c) {
        case '"':
            ss << "\\\"";
            break;
        case '\\':
            ss << "\\\\";
            break;
        case '\b':
            ss << "\\b";
            break;
        case '\f':
            ss << "\\f";
            break;
        case '\n':
            ss << "\\n";
            break;
        case '\r':
            ss << "\\r";
            break;
        case '\t':
            ss << "\\t";
            break;
        default:
            if (c < 0x20) {
                ss << "\\u"
                    << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(c);
            }
            else {
                ss << c;
            }
            break;
        }
    }

    return ss.str();
}

int main() {
    while (Memory::ProcessId == 0) {
        Memory::ProcessId = Memory::GetPID(L"RobloxPlayerBeta.exe");
        Sleep(100);
    }
    if (!Memory::AttachToProcess(Memory::ProcessId)) {
        std::cout << "failed to attach to RobloxPlayerBeta.exe!" << std::endl;
        system("pause");
        return 1;
    }
    Memory::RobloxBase = Memory::GetModuleBaseAddress(L"RobloxPlayerBeta.exe");

    std::cout << "Roblox PID -> " << std::dec << Memory::ProcessId << std::endl;
    std::cout << "Roblox Base -> 0x" << std::hex << Memory::RobloxBase << std::endl;

    std::ofstream outFile("rulesets.txt");
    std::ofstream jsonoutFile("rulesets.json");

    jsonoutFile << "{\n";
    jsonoutFile << "  \"rulesets\": [\n";

    int Successful = 0;
    bool firstJsonEntry = true;

    for (int i = 0; i < 167; i++) {
        std::string ruleset = DecryptRuleset(i);
        if (!ruleset.empty()) {
            std::cout << "Ruleset " << std::dec << i << " -> " << ruleset << std::endl;
            outFile << "Ruleset " << std::dec << i << " -> " << ruleset << '\n';
            if (!firstJsonEntry) {
                jsonoutFile << ",\n";
            }

            jsonoutFile << "    {\n";
            jsonoutFile << "      \"index\": " << std::dec << i << ",\n";
            jsonoutFile << "      \"value\": \"" << JsonEscape(ruleset) << "\"\n";
            jsonoutFile << "    }";

            firstJsonEntry = false;
            Successful++;
        }
    }

    jsonoutFile << "\n";
    jsonoutFile << "  ]\n";
    jsonoutFile << "}\n";

    outFile.close();
    jsonoutFile.close();
    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << "saved " << Successful << " rulesets to rulesets.txt and rulesets.json" << std::endl;

    system("pause");
    return 0;
}
