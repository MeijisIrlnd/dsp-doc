#include "delay.h" // Anything that includes "common.h" should do

SIGNALSMITH_DSP_VERSION_CHECK(1, 3, 0)

// Test with some compatible earlier versions
SIGNALSMITH_DSP_VERSION_CHECK(
	SIGNALSMITH_DSP_VERSION_MAJOR,
	SIGNALSMITH_DSP_VERSION_MINOR,
	SIGNALSMITH_DSP_VERSION_PATCH
)
SIGNALSMITH_DSP_VERSION_CHECK(
	SIGNALSMITH_DSP_VERSION_MAJOR,
	SIGNALSMITH_DSP_VERSION_MINOR,
	0
)
SIGNALSMITH_DSP_VERSION_CHECK(
	SIGNALSMITH_DSP_VERSION_MAJOR,
	0,
	0
)
