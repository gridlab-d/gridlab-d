# Fix for Windows getpid() Issue in gldcore/gldapi.cpp

## Issue
Line 416 in gldcore/gldapi.cpp calls `getpid()` without Windows compatibility.

## Current Code (Lines 41-46)
```cpp
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif
```

## Fixed Code (Add process.h include)
```cpp
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define getcwd _getcwd
#define getpid _getpid
#else
#include <unistd.h>
#endif
```

## Changes Made
1. Added `#include <process.h>` for Windows (provides `_getpid()`)
2. Added `#define getpid _getpid` for Windows (maps standard name to Windows name)

## Reference Pattern
This follows the exact pattern used in other GridLAB-D files:
- **gldcore/main.cpp** (lines 22-24, 275): Correct Windows handling
- **gldcore/load.cpp** (lines 50-58, 500): Correct Windows handling
- **gldcore/random.cpp** (lines 105): Correct Windows handling

## Verification
After applying fix, compile on Windows and verify:
```bash
# Python on Windows should be able to import gridlabd without crash
python -c "from gridlabd import gridlabd_core; g = gridlabd_core.GridLabD()"
```

## Impact
- Minimal change - just 2 lines added
- Follows existing codebase patterns
- No behavior change on Linux/Mac
- Fixes crash on Windows Python extension loading
