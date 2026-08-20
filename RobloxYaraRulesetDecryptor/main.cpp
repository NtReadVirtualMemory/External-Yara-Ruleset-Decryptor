#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "Memory.hpp"

// IMPORTANT: This is Updated for version "version-ce0bcd0fbd484804"
// 48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC 50 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 ?? 49 8B F8 4C 89 45
namespace offsets {
    uint64_t v17 = 0x1000001E3;
    uint64_t Encryptions1[] = { 0x1AAAF, 0x1AE2B, 0x545DF };
    uint64_t Encryptions2[] = { 0x1546A, 0xB8DC758, 0x27FE };
    uint64_t v12 = 0x64A5790;
    uint64_t si = 0x6F990A0;
}
// Thanks to Roblox for these changes but you'll NEVER be able to stop me. <3

bool IsValidRulesetName(const std::string& value) {
    if (value.size() < 3 || value.size() > 128) {
        return false;
    }

    for (unsigned char c : value) {
        const bool valid =
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '_';

        if (!valid) {
            return false;
        }
    }

    return true;
}

uint64_t CoolShit(uint64_t value, int index, int n = 1) {
    uint64_t result = value;
    for (int i = 0; i < n; i++) {
        result = (offsets::Encryptions2[index] + offsets::Encryptions1[index] * result) % offsets::v17;
    }
    return result;
}

std::string DecryptRuleset(int index) {
    if (index < 0) {
        return {};
    }

    constexpr uint32_t seed = 0x61A48F10u;
    constexpr uint32_t payloadSeed3 = 0x091864E3u;

    const uint64_t base = Memory::RobloxBase;
    const uint64_t table = base + offsets::v12;

    const uint64_t si1 = Memory::read<uint64_t>(base + offsets::si);
    const uint64_t si2 = Memory::read<uint64_t>(base + offsets::si + 8);

    uint64_t s1 = CoolShit(seed, 0, 2);
    uint64_t s2 = CoolShit(si1, 1, 2);
    uint64_t s3 = CoolShit(si2, 2, 2);

    const uint32_t head = Memory::read<uint32_t>(table + 4);

    const uint32_t recordOffset =
        (head ^
            static_cast<uint32_t>(s1) ^
            static_cast<uint32_t>(s2) ^
            static_cast<uint32_t>(s3))
        + 12 * static_cast<uint32_t>(index);

    s1 = CoolShit(recordOffset, 0, 2);
    s2 = CoolShit(s2, 1, 3);
    s3 = CoolShit(s3, 2, 3);

    const uint32_t payloadOffset =
        Memory::read<uint32_t>(table + recordOffset) ^
        static_cast<uint32_t>(s1) ^
        static_cast<uint32_t>(s2) ^
        static_cast<uint32_t>(s3);

    s1 = CoolShit(seed, 0, 1);
    s2 = CoolShit(payloadOffset, 1, 1);
    s3 = CoolShit(payloadSeed3, 2, 1);

    uint64_t data = table + payloadOffset;

    const uint32_t entry =
        Memory::read<uint32_t>(data) ^
        static_cast<uint32_t>(s1) ^
        static_cast<uint32_t>(s2) ^
        static_cast<uint32_t>(s3);

    data += 4;

    const uint16_t length =
        static_cast<uint16_t>(entry);

    if (length == 0 || length > 0x500) {
        return {};
    }

    std::string result;
    result.reserve(length);

    if (length >= 1) {
        result.push_back(
            static_cast<char>((entry >> 16) & 0xFFu)
        );
    }

    if (length >= 2) {
        result.push_back(
            static_cast<char>((entry >> 24) & 0xFFu)
        );
    }

    uint32_t decryptedWord = 0;

    for (uint32_t position = 0;
        position < static_cast<uint32_t>(length - 2);
        ++position) {

        if ((position & 3u) == 0) {
            s1 = CoolShit(s1, 0, 1);
            s2 = CoolShit(s2, 1, 1);
            s3 = CoolShit(s3, 2, 1);

            decryptedWord =
                Memory::read<uint32_t>(data) ^
                static_cast<uint32_t>(s1) ^
                static_cast<uint32_t>(s2) ^
                static_cast<uint32_t>(s3);

            data += 4;
        }

        result.push_back(static_cast<char>(
            (decryptedWord >> ((position & 3u) * 8u)) & 0xFFu
        ));
    }

    return result;
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

        if (ruleset.empty() || !IsValidRulesetName(ruleset)) {
            continue;
        }

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
