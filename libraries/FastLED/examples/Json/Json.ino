#include "fl/sketch_macros.h"
#if SKETCH_HAS_LOTS_OF_MEMORY
#include "JsonSketch.h"
#else
void setup() {}
void loop() {}
#endif  // SKETCH_HAS_LOTS_OF_MEMORY
