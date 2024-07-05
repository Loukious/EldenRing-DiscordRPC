#include <Windows.h>
#include <string>

class MemoryUtility {
public:
    static DWORD_PTR gameManAddress;
    static DWORD_PTR gameDataManAddress;
    static DWORD_PTR worldChrManAddress;

    static DWORD_PTR GetModuleBaseAddress();
    static DWORD_PTR FindPattern(DWORD_PTR base, DWORD size, const char* pattern, const char* mask);
    static DWORD_PTR CalculateAddress(HANDLE hProcess, DWORD_PTR base, const char* pattern, const char* mask, int patternOffset, int addressAdjustment);
    static int ReadInt32(DWORD_PTR address);
    static long long ReadInt64(DWORD_PTR address);
    static std::string ReadString(DWORD_PTR address, int length);
    static std::string ReadPlayerName(int playerIndex);
    static bool IsValidAddress(DWORD_PTR address);
    static long ReadLocalPlayerLevel();
    static long ReadPlayTime();
    static long ReadDeathCount();
    static long ReadRunesHeld();
    static long ReadLastGraceLocationId();
    static long ReadStrengthAttr();
    static long ReadDexterityAttr();
    static long ReadIntelligenceAttr();
    static long ReadFaithAttr();
    static long ReadArcaneAttr();
    static int ReadPlayerClassId();
    static BYTE ReadByte(DWORD_PTR address);
    static int CountNetPlayers();
    static void initialize();
};