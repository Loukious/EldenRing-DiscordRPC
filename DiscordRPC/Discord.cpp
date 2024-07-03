#include "Discord.h"
#include <time.h>
#include <Utils.h>
#include <ctime>

using namespace Utils;
static time_t g_startTime;

void Discord::initialize(bool setTimestamp)
{
    Log("Initializing Discord RPC...");
    
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    
    // Initialize Discord RPC
    Discord_Initialize("1243218524554530998", &handlers, 1, NULL);

    
    // Prepare rich presence struct
    DiscordRichPresence discordRichPresence;
    memset(&discordRichPresence, 0, sizeof(discordRichPresence));
    
    if (setTimestamp) {
        g_startTime = std::time(0);
    }
    discordRichPresence.startTimestamp = g_startTime;
    discordRichPresence.state = "In Menu";
    discordRichPresence.details = "Playing Elden Ring";
    discordRichPresence.largeImageKey = "none";
    discordRichPresence.largeImageText = "Elden Ring";

    // Update Discord presence
    update(discordRichPresence);
}

void Discord::update(DiscordRichPresence discordRichPresence)
{
	Discord_UpdatePresence(&discordRichPresence);
}

time_t Discord::GetStartTime()
{
    return g_startTime;
}
