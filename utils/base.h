// Base file for utils; hooks with the dcpp lib.

#include <dcpp/stdinc.h>

#include <dcpp/PluginApiImpl.h>

#define _(x)

namespace dcpp {

// Functions for DCUI
void PluginApiImpl::addCommand(const char* name, DCCommandFunc command) { }
void PluginApiImpl::removeCommand(const char* name) { }
void PluginApiImpl::playSound(const char* path) { }

} // namespace dcpp
