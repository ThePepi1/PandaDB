#pragma once
#include "IMetrics.h"

#define PANDA_METRIC_PLUGIN(CLASS)                                          \
    extern "C" __attribute__((visibility("default")))                       \
    ::metrics::IMetrics* panda_create_metric() { return new CLASS(); }