#pragma once

#include "api.h"

#include <stdbool.h>

void runRingTasks(ApiClientContext* apiClientContext);
void runHeartbeatTask(
    ApiClientContext* apiClientContext, bool applyFirmwareUpdate);
void handleOnDemandHeartbeatSequence(ApiClientContext* apiClientContext);
