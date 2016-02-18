#ifndef __KEYHANDLING_H__
#define __KEYHANDLING_H__

#include "types.h"

// Stores the details about a particular action
// including when it was last pressed (in system ticks)
// how long it was held for (in ticks and ms) and
// also provides space for flags which can be used
// for current state etc
typedef struct
{
    u32 lastStartTicks;
    u32 lastHeldTicks;
    u32 lastHeldMS;
    u16 flags;
    u16 padding;
} KeyActionDetails;

// Default flag values for above
#define ActionDetailFlagsIsActive 1<<0;


// The actions we are interested in
typedef enum
{
    NoAction = 0,
    MoveForward,
    MoveBackward,
    TurnLeft,
    TurnRight,
    ZoomIn,
    ZoomOut,

    __MAX_IDX__          //Slight hack to get max index in enum
} KeyActions;

// Maps key codes to action enum values
KeyActions keyMappings[256];

// Stores details of the actions
KeyActionDetails keyActionDetails[__MAX_IDX__];

#endif
