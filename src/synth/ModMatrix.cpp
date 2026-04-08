#include "ModMatrix.h"

#include "synth/Types.h"

#include <cstdint>
#include <cstring>
#include <sstream>

namespace synth::mod_matrix {
// ===== Route(s) Management ========
bool addRoute(ModMatrix& matrix, ModSrc src, ModDest dest, float amount) {
  if (matrix.count >= MAX_MOD_ROUTES)
    return false;

  matrix.routes[matrix.count] = {src, dest, amount};
  matrix.count++;

  return true;
}

bool addRoute(ModMatrix& matrix, const ModRoute& route) {
  if (matrix.count >= MAX_MOD_ROUTES)
    return false;

  matrix.routes[matrix.count] = route;
  matrix.count++;

  return true;
}

bool removeRoute(ModMatrix& matrix, uint8_t index) {
  if (index >= matrix.count || matrix.count < 1)
    return false;

  // TODO(nico): does order matter?  don't think it does...
  matrix.count--;

  matrix.routes[index] = matrix.routes[matrix.count];

  matrix.routes[matrix.count].src = ModSrc::NoSrc;
  matrix.routes[matrix.count].dest = ModDest::NoDest;
  matrix.routes[matrix.count].amount = 0.0f;

  return true;
}

bool clearRoutes(ModMatrix& matrix) {
  for (auto& route : matrix.routes) {
    route.src = ModSrc::NoSrc;
    route.dest = ModDest::NoDest;
    route.amount = 0.0f;
  }

  matrix.count = 0;
  return true;
}

// ====== Steps Management =======
void clearPrevModDests(ModMatrix& matrix) {
  for (uint8_t d = 0; d < ModDest::DEST_COUNT; d++) {
    for (uint8_t v = 0; v < MAX_VOICES; v++) {
      matrix.prevDestValues[d][v] = 0;
    }
  }
}
void clearModDestSteps(ModMatrix& matrix) {
  for (uint8_t d = 0; d < ModDest::DEST_COUNT; d++) {
    for (uint8_t v = 0; v < MAX_VOICES; v++) {
      matrix.destStepValues[d][v] = 0;
    }
  }
}

void setModDestStep(ModMatrix& matrix, ModDest dest, uint32_t voiceIndex, float invNumSamples) {
  matrix.destStepValues[dest][voiceIndex] =
      (matrix.destValues[dest][voiceIndex] - matrix.prevDestValues[dest][voiceIndex]) *
      invNumSamples;
}

// ==========================
// Parsing Helpers
// ==========================

// ==== <Internal Helpers> ====
namespace {
ModSrc modSrcFromString(const char* input) {
  for (auto& mapping : modSrcMappings) {
    if (strcasecmp(mapping.name, input) == 0)
      return mapping.src;
  }

  return ModSrc::NoSrc;
}

ModDest modDestFromString(const char* input) {
  for (auto& mapping : modDestMappings) {
    if (strcasecmp(mapping.name, input) == 0)
      return mapping.dest;
  }
  return ModDest::NoDest;
};

void parseAddModCommand(std::istringstream& iss, ModMatrix& modMatrix) {
  std::string srcStr, destStr;
  float amount;

  if (!(iss >> srcStr >> destStr >> amount)) {
    printf("Usage: mod add <source> <dest> <amount>\n");
    return;
  }

  ModSrc src = modSrcFromString(srcStr.c_str());
  ModDest dest = modDestFromString(destStr.c_str());

  if (src == ModSrc::NoSrc) {
    printf("Error: Unknown mod source '%s'\n", srcStr.c_str());
    return;
  }
  if (dest == ModDest::NoDest) {
    printf("Error: Unknown mod destination '%s'\n", destStr.c_str());
    return;
  }

  if (!addRoute(modMatrix, src, dest, amount)) {
    printf("Error: Mod matrix full (max %d routes)\n", MAX_MOD_ROUTES);
    return;
  }

  printf("OK: [%d] %s → %s  x%.2f\n", modMatrix.count - 1, srcStr.c_str(), destStr.c_str(), amount);
}

void parseRemoveModCommand(std::istringstream& iss, ModMatrix& modMatrix) {
  int index;

  if (!(iss >> index) || index < 0) {
    printf("Usage: mod remove <index>\n");
    return;
  }

  if (!removeRoute(modMatrix, static_cast<uint8_t>(index))) {
    printf("Error: No route at index %d (count = %d)\n", index, modMatrix.count);
    return;
  }

  printf("OK: route %d removed\n", index);
}

void parseListModCommand(ModMatrix& modMatrix) {
  const ModMatrix& matrix = modMatrix;

  if (matrix.count == 0) {
    printf("No active mod routes.\n");
    return;
  }

  printf("Mod routes (%d/%d):\n", matrix.count, MAX_MOD_ROUTES);
  for (uint8_t i = 0; i < matrix.count; i++) {
    const ModRoute& r = matrix.routes[i];
    printf("  [%d] %-12s → %-20s  x%.2f\n",
           i,
           modSrcToString(r.src),
           modDestToString(r.dest),
           r.amount);
  }
}
void parseClearModCommand(ModMatrix& modMatrix) {
  clearRoutes(modMatrix);
  printf("OK: mod matrix cleared\n");
}

void parseHelpModCommand() {
  printf("Usage:\n");
  printf("  mod add <source> <dest> <amount>\n");
  printf("  mod remove <index>\n");
  printf("  mod list\n");
  printf("  mod clear\n");
  printf("  mod help\n");
  printf("\nSources:\n");
  for (auto& m : modSrcMappings)
    printf("  %s\n", m.name);

  printf("\nDestinations:\n");
  for (auto& m : modDestMappings)
    printf("  %s\n", m.name);
}

} // namespace
// ==== </ Internal Helpers> ====

void parseModCommand(std::istringstream& iss, ModMatrix& modMatrix) {
  std::string subcmd;
  iss >> subcmd;

  if (subcmd == "add") {
    parseAddModCommand(iss, modMatrix);

  } else if (subcmd == "remove") {
    parseRemoveModCommand(iss, modMatrix);

  } else if (subcmd == "list") {
    parseListModCommand(modMatrix);

  } else if (subcmd == "clear") {
    parseClearModCommand(modMatrix);

  } else if (subcmd == "help") {
    parseHelpModCommand();

  } else {
    printf("Error: Unknown mod subcommand '%s'. Try 'mod help'.\n", subcmd.c_str());
  }
}

} // namespace synth::mod_matrix
