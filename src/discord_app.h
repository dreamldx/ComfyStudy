#pragma once

#include "options.h"

namespace bot {

// `comfystudy --discord`: runs the bot until Ctrl+C, SIGTERM or (on Windows) the console window is closed.
// Reads the settings (discord_token and the usual ones for the model), builds everything in one place and gives
// the exit code: 0 after a clean stop, 1 with a message on stderr when it could not start.
int RunDiscord(const options::Options& options);

}  // namespace bot
