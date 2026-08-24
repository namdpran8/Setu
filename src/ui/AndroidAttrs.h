#pragma once

#include <cstdint>
#include <string>

namespace setu {

class ResourceManager;

// Runtime lookup for framework attribute IDs (android:attr/*).
//
// The alternative is hex literals sprinkled through the inflaters - 0x010100d4
// for background, 0x0101014f for text, and so on. Since framework-res.apk is
// already loaded, the real IDs can be asked for by name, which is both readable
// and immune to typos in a 32-bit constant.
//
// Results are memoised: an attribute ID cannot change while the process runs.
// Returns 0 when the name cannot be resolved (no framework loaded, or a typo),
// and 0 never matches a real attribute, so callers can compare safely.
uint32_t androidAttr(ResourceManager* resManager, const std::string& name);

} // namespace setu
