#pragma once

#include <optional>

// A node's configured analog limit bands (AnalogItemType_LimitLoLo/Lo/Hi/HiHi).
// An absent entry is a band the node does not configure, which is never
// reported as breached.
struct LimitValues {
  std::optional<double> lolo;
  std::optional<double> lo;
  std::optional<double> hi;
  std::optional<double> hihi;

  bool empty() const { return !lolo && !lo && !hi && !hihi; }
};

// The band a value currently sits in.
enum class LimitBand { kNormal, kLo, kLoLo, kHi, kHiHi };

// Classifies `value` against `limits`. A value exactly on a limit counts as
// breaching it (the limit is the threshold the process must stay inside), and
// the more severe band wins where two overlap, so a misconfigured node whose
// Hi sits above its HiHi still reports the worse breach rather than the
// narrower one.
LimitBand LimitBandFor(double value, const LimitValues& limits);
