#include "modules/inspector/limit_band.h"

LimitBand LimitBandFor(double value, const LimitValues& limits) {
  // Most severe first, so overlapping bands report the worse breach.
  if (limits.hihi && value >= *limits.hihi)
    return LimitBand::kHiHi;
  if (limits.lolo && value <= *limits.lolo)
    return LimitBand::kLoLo;
  if (limits.hi && value >= *limits.hi)
    return LimitBand::kHi;
  if (limits.lo && value <= *limits.lo)
    return LimitBand::kLo;
  return LimitBand::kNormal;
}
