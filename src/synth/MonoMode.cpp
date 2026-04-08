#include "MonoMode.h"

namespace synth::mono {

void pushNoteToStack(MonoState& mono, uint8_t midiNote) {
  if (mono.stackDepth >= 16) {
    return;
  }

  // Check for duplicate
  for (uint8_t i = 0; i < mono.stackDepth; i++)
    if (mono.noteStack[i] == midiNote) {
      return;
    }

  mono.noteStack[mono.stackDepth] = midiNote;
  mono.stackDepth++;
}

void removeNoteFromStack(MonoState& mono, uint8_t midiNote) {
  for (uint8_t i = 0; i < mono.stackDepth; i++) {
    if (mono.noteStack[i] == midiNote) {
      for (uint8_t j = i; j < mono.stackDepth - 1; j++)
        mono.noteStack[j] = mono.noteStack[j + 1];
      mono.stackDepth--;
      return;
    }
  }
}

} // namespace synth::mono
