#pragma once

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

#define clamp(x,lo,hi)          \
({                              \
    __typeof__ (x) _x = (x);    \
    __typeof__ (lo) _lo = (lo); \
    __typeof__ (hi) _hi = (hi); \
    min(_hi, max(_lo, _x));     \
})
