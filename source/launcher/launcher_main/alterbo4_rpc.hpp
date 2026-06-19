#pragma once
#include "discord_rpc.h"
#include <ctime>
#include <cstring>
#include <string>

// ============================================================
//  RICH PRESENCE ALTERBO4
//  >>> REMPLACE LES 2 VALEURS CI-DESSOUS PAR LES TIENNES <<<
// ============================================================

// 1) Ton Application ID Discord (Portail dev -> General Information -> Application ID)
#define ALTERBO4_DISCORD_APP_ID "1517349699554639892"

// 2) Le nom de ton image (Portail dev -> Rich Presence -> Art Assets -> la cle de la grande image)
#define ALTERBO4_LARGE_IMAGE "alterbo4_logo"

namespace AlterRPC {

    inline void init() {
        DiscordEventHandlers handlers;
        memset(&handlers, 0, sizeof(handlers));
        Discord_Initialize(ALTERBO4_DISCORD_APP_ID, &handlers, 1, nullptr);
    }

    // Met a jour le statut. details = ligne 1, state = ligne 2.
    inline void update(const std::string& details, const std::string& state) {
        DiscordRichPresence presence;
        memset(&presence, 0, sizeof(presence));
        presence.details = details.c_str();          // ex: "Sur le launcher"
        presence.state = state.c_str();              // ex: "En ligne"
        // Image pas encore uploadee -> on l'active plus tard (Art Assets)
        // presence.largeImageKey = ALTERBO4_LARGE_IMAGE;
        // presence.largeImageText = "AlterBO4";
        presence.startTimestamp = time(nullptr);     // chrono "depuis X min"
        Discord_UpdatePresence(&presence);
    }

    inline void shutdown() {
        Discord_Shutdown();
    }
}
