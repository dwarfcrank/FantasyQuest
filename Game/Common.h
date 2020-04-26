#pragma once

constexpr bool IsDebug =
#ifdef _DEBUG
    true
#else
    false
#endif
;
