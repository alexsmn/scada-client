#pragma once

#include "base/promise.h"

#include <memory>

class ClientApplication;
class Executor;

namespace client {

promise<> RunE2eObjectViewValuesCheck(ClientApplication& app,
                                      std::shared_ptr<Executor> executor);
promise<> RunE2eOperatorUseCaseSmoke(ClientApplication& app);
promise<> RunE2eObjectTreeLabelsCheck(ClientApplication& app,
                                      std::shared_ptr<Executor> executor);
promise<> RunE2eHardwareTreeDevicesCheck(ClientApplication& app,
                                         std::shared_ptr<Executor> executor);

}  // namespace client
