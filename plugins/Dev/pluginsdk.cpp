/* This file compiles the plugin SDK. Normally the relevant files would be included directly in the
project; but it's impractical in the DC++ repository (would need some decent symlink support). */

#include "version.h"

#include <pluginsdk/Config.cpp>
#include <pluginsdk/Core.cpp>
#include <pluginsdk/Hooks.cpp>
#include <pluginsdk/Hubs.cpp>
#include <pluginsdk/Logger.cpp>
#include <pluginsdk/UI.cpp>
#include <pluginsdk/Util.cpp>
