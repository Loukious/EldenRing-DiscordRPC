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
	Log("Resource found successfully. Resource ID: %d", resourceId);

	HGLOBAL hLoadedResource = LoadResource(g_hModule, hResource);
	if (!hLoadedResource) {
		Log("Failed to load resource");
		return nullptr;
	}
	Log("Resource loaded successfully.");

	LPVOID pLockedResource = LockResource(hLoadedResource);
	if (!pLockedResource) {
		Log("Failed to lock resource");
		return nullptr;
	}
	Log("Resource locked successfully.");

	DWORD resourceSize = SizeofResource(g_hModule, hResource);
	if (resourceSize == 0) {
		Log("Resource size is 0");
		return nullptr;
	}
	Log("Resource size: %d", resourceSize);

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



static void UpdateDiscordPresence() {
	while (true) {
		Log("Trying to read name..");
		std::string localPlayerName = g_memoryUtility->ReadPlayerName(0);
		if (localPlayerName != "") {
			Log("localPlayerName: %s", localPlayerName.c_str());
			long level = g_memoryUtility->ReadLocalPlayerLevel();
			long playTimeMs = g_memoryUtility->ReadPlayTime();
			long deathCount = g_memoryUtility->ReadDeathCount();
			long runesHeld = g_memoryUtility->ReadRunesHeld();
			long locationId = g_memoryUtility->ReadLastGraceLocationId();
			double playTimeHours = playTimeMs / (1000.0 * 60 * 60);
			int netPlayers = g_memoryUtility->CountNetPlayers();
			Log("netPlayers: %d", netPlayers);
			std::string location = "The Lands Between";
			std::string LargeImageText = "The Lands Between";
			std::string imageKey = "none";

			if (!locationRegister.empty() && locationRegister.find(locationId) != locationRegister.end()) {
				location = locationRegister[locationId];
				Log("Location: %s", location.c_str());
			}
			else {
				Log("Location ID not found in the location register");
			}
			std::size_t pos = location.find(" - ");
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

				imageKey = tempImageKey;
			}

			Log("Updating Discord presence..");
			DiscordRichPresence discordRichPresence;

			discordRichPresence.state = location.c_str();

			discordRichPresence.largeImageKey = imageKey.c_str();
			discordRichPresence.largeImageText = LargeImageText.c_str();
			discordRichPresence.partyId = localPlayerName.c_str();
			discordRichPresence.partySize = netPlayers;
			discordRichPresence.partyMax = 10;
			discordRichPresence.startTimestamp = Discord::GetStartTime();
			discordRichPresence.endTimestamp = NULL;

			std::string detailsStr = std::format("| Level: {}\n| Playtime: {:.2f} hours\n| Deaths: {}\n| Runes Held: {}",
				level, playTimeHours, deathCount, runesHeld);
			discordRichPresence.details = detailsStr.c_str();
			g_discord->update(discordRichPresence);
		}
		else {
			g_discord->initialize(false);
		}

		Sleep(10000);
	}
}

DWORD WINAPI MainThread(LPVOID param) {
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
