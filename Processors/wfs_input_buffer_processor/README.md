# WFS Input Buffer Processor

A GPU Audio processor for Wave Field Synthesis (WFS) input buffer processing. This processor applies per-input delays and gains to route audio from multiple inputs to multiple outputs in real-time.

## Overview

The WFS Input Buffer processor implements delay-and-sum processing for WFS systems. It accepts routing messages containing delay times and gain values for each input-output pair, applies the delays, and routes the audio accordingly.

## Specification Structure

The processor uses the following configuration structures (defined in `WfsInputSpecification.h`):

- **Specification**: Construction parameters including number of inputs/outputs and maximum delay capacity
- **RoutingMessage**: Runtime routing updates containing delay samples and gain values per input-output pair

## Building

This processor can be built as part of the GPU Audio SDK build system by adding it to the `PROCESSOR_LIST` in the SDK's main `CMakeLists.txt`, or built standalone using CMake.

See the main project documentation for build instructions.

