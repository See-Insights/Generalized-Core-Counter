#pragma once
#include "Particle.h"

struct ThrashGuard {
  void markProgress(const char *tag) { sim::emit("PROGRESS %s", tag); }
};
inline ThrashGuard thrashGuard;
