// timings.h

#include <GraphicsDriver.h>


// Using charptrs (instead of 64-byte character arrays like a named_timing)
// allows the compiler to pack the strings as close together as possible.

typedef struct nameptrd_display_timing {
  const char *name;
  display_timing timing;
} nameptrd_display_timing;


// The first timing in the array (timings[0]) is the default timing.
extern const nameptrd_display_timing timings[];

