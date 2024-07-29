#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <Utils.h>
#include "Discord.h"
#include "resource.h"
#include "MemoryUtility.h"
#include "nlohmann/json.hpp"
#include <format>

using namespace Utils;
using json = nlohmann::json;


static Discord* g_discord;
static MemoryUtility* g_memoryUtility;
static HMODULE g_hModule = NULL;
static json locationRegister;
static json classesRegister;



static json LoadJsonResource(UINT resourceId) {
	HRSRC hResource = FindResourceW(g_hModule, MAKEINTRESOURCEW(resourceId), L"JSON");
	if (!hResource) {
		Log("Failed to find resource. Resource ID: %d", resourceId);
		return nullptr;
	}

	HGLOBAL hLoadedResource = LoadResource(g_hModule, hResource);
	if (!hLoadedResource) {
		Log("Failed to load resource");
		return nullptr;
	}

	LPVOID pLockedResource = LockResource(hLoadedResource);
	if (!pLockedResource) {
		Log("Failed to lock resource");
		return nullptr;
	}

	DWORD resourceSize = SizeofResource(g_hModule, hResource);
	if (resourceSize == 0) {
		Log("Resource size is 0");
		return nullptr;
	}

	std::string jsonData(static_cast<char*>(pLockedResource), resourceSize);

	try {
		json parsedJson = json::parse(jsonData);
		return parsedJson;
	}
	catch (json::parse_error& e) {
		Log("JSON parsing error: %s", e.what());
		return nullptr;
	}
}


static std::map<long, std::string> DeserializeJsonToMap(const std::string& jsonData) {
    json j = json::parse(jsonData);
    std::map<long, std::string> data;

    for (auto& element : j.items()) {
        data[std::stol(element.key())] = element.value().dump();
    }

    return data;
}


static std::map<std::string, long> ReadAllAttributes(MemoryUtility* memoryUtility) {
	std::map<std::string, long> attrs;
	attrs["Strength"] = memoryUtility->ReadStrengthAttr();
	attrs["Dexterity"] = memoryUtility->ReadDexterityAttr();
	attrs["Intelligence"] = memoryUtility->ReadIntelligenceAttr();
	attrs["Faith"] = memoryUtility->ReadFaithAttr();
	attrs["Arcane"] = memoryUtility->ReadArcaneAttr();
	return attrs;
}

static std::string GetHighestAttribute(const std::map<std::string, long>& attrs) {
	auto highest = std::max_element(attrs.begin(), attrs.end(),
		[](const std::pair<std::string, long>& a, const std::pair<std::string, long>& b) {
			return a.second < b.second;
		});

	return highest->first;
}


static void UpdateDiscordPresence() {
	while (true) {
		std::string localPlayerName = g_memoryUtility->ReadPlayerName(0);
		if (localPlayerName != "") {
			long level = g_memoryUtility->ReadLocalPlayerLevel();
			long playTimeMs = g_memoryUtility->ReadPlayTime();
			long deathCount = g_memoryUtility->ReadDeathCount();
			long runesHeld = g_memoryUtility->ReadRunesHeld();
			std::string locationId = std::to_string(g_memoryUtility->ReadLastGraceLocationId());
			double playTimeHours = playTimeMs / (1000.0 * 60 * 60);
			int netPlayers = g_memoryUtility->CountNetPlayers();
			std::string playerClassId = std::to_string(g_memoryUtility->ReadPlayerClassId());
			std::string location = "The Lands Between";
			std::string LargeImageText = "The Lands Between";
			std::string largeImageKey = "none";
			std::string className = "";
			std::string smallImageKey = "";
			std::string detailsStr = "";
			std::string characterInfoStr = "";
			std::string stateStr = "";
			if (locationRegister.contains(locationId)) {
				location = locationRegister[locationId];
			}
			else {
				Log("Location ID not found: %d", locationId);
			}
			if (classesRegister.contains(playerClassId)) {
				className = classesRegister[playerClassId];
				smallImageKey = classesRegister[playerClassId];
				std::transform(smallImageKey.begin(), smallImageKey.end(), smallImageKey.begin(), ::tolower);
			}
			else {
				Log("Class ID not found: %s", playerClassId);
			}

			std::size_t pos = location.rfind(" - ");
			if (pos != std::string::npos) {
				LargeImageText = location.substr(0, pos);
				std::string tempImageKey = LargeImageText;

				// Remove all spaces
				tempImageKey.erase(std::remove(tempImageKey.begin(), tempImageKey.end(), ' '), tempImageKey.end());

				// Remove all non-alphabet characters
				tempImageKey.erase(std::remove_if(tempImageKey.begin(), tempImageKey.end(),
					[](char c) { return !std::isalpha(c); }), tempImageKey.end());

				// Convert to lowercase
				std::transform(tempImageKey.begin(), tempImageKey.end(), tempImageKey.begin(), ::tolower);

				largeImageKey = tempImageKey;
			}

			DiscordRichPresence discordRichPresence;
			memset(&discordRichPresence, 0, sizeof(discordRichPresence));


			discordRichPresence.smallImageKey = smallImageKey.c_str();
			discordRichPresence.largeImageKey = largeImageKey.c_str();

			auto attrs = ReadAllAttributes(g_memoryUtility);
			std::string highestAttr = GetHighestAttribute(attrs);

			characterInfoStr = std::format("Level {} - {} build",
				level, highestAttr);
			discordRichPresence.smallImageText = characterInfoStr.c_str();
			detailsStr = std::format("| Playtime: {:.2f} hours\n| Deaths: {}\n| Runes Held: {}",
				 playTimeHours, deathCount, runesHeld);

			
			discordRichPresence.largeImageText = location.c_str();
			discordRichPresence.partyId = localPlayerName.c_str();
			discordRichPresence.partySize = netPlayers;
			discordRichPresence.partyMax = 6;
			discordRichPresence.startTimestamp = Discord::GetStartTime();
			discordRichPresence.endTimestamp = NULL;
			discordRichPresence.button1Url = "https://www.nexusmods.com/eldenring/mods/5483";
			discordRichPresence.button1Label = "Download Mod";

			if (netPlayers > 1) {
				stateStr = std::format("Playing with {} other(s)", netPlayers - 1);
			}
			else {
				stateStr = "Playing Solo";
			}

			discordRichPresence.state = stateStr.c_str();
			discordRichPresence.details = detailsStr.c_str();
			g_discord->update(discordRichPresence);
		}
		else {
			g_discord->initialize(false);
		}

		Sleep(10000);
	}
}

static DWORD WINAPI MainThread(LPVOID param) {
	Log("Activating DiscordRPC..");

	g_discord->initialize();
	g_memoryUtility->initialize();
	locationRegister = LoadJsonResource(IDR_JSON1);
	classesRegister = LoadJsonResource(IDR_JSON2);



	// Create a thread to update Discord presence
	HANDLE hThread = CreateThread(
		NULL,            // default security attributes
		0,               // use default stack size
		[](LPVOID) -> DWORD {
			UpdateDiscordPresence();
			return 0;
		},
		NULL,            // argument to thread function
		0,               // use default creation flags
		NULL);           // returns the thread identifier

	if (hThread == NULL) {
		DWORD dwError = GetLastError();
		Log("CreateThread failed with error: %d", dwError);
	}
	else {
		CloseHandle(hThread);
	}

	return 0;
}


BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(module);
		g_hModule = module;
		CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
	}
	return TRUE;
}
