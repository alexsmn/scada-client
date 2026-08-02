#include "services/app_nap_suppressor.h"

#import <Foundation/Foundation.h>

// This translation unit is deliberately dependency-free (its own header plus
// Foundation). It is compiled into a separate `client_app_nap_mac` target
// because `client_services_*` reuses scada_base's C++ precompiled header, which
// cannot be applied to an Objective-C++ source.

AppNapSuppressor::AppNapSuppressor() {
  // NSActivityUserInitiatedAllowingIdleSystemSleep is NSActivityUserInitiated
  // minus NSActivityIdleSystemSleepDisabled: it clears the NSActivityBackground
  // bits that put the process into App Nap, without also asserting that the
  // machine must stay awake. See the header for why that split matters.
  id<NSObject> activity = [[NSProcessInfo processInfo]
      beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
                        reason:@"SCADA session: the client's asio io_context is "
                               @"polled from a Qt timer that App Nap coalesces"];
  // The returned token is autoreleased; retain it so it survives past the
  // enclosing autorelease pool, and balance the retain in the destructor.
  token_ = static_cast<void*>([activity retain]);
}

AppNapSuppressor::~AppNapSuppressor() {
  if (!token_)
    return;

  id<NSObject> activity = static_cast<id<NSObject>>(token_);
  [[NSProcessInfo processInfo] endActivity:activity];
  [activity release];
  token_ = nullptr;
}
