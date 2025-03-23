#pragma once  //mac.h

#if defined(_MSC_VER)
  #define Prefetch(ptr) _mm_prefetch(reinterpret_cast<const char*>(ptr), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
  #define Prefetch(ptr) __builtin_prefetch((ptr), 0, 3);
#else
  #define Prefetch(ptr) (void)
#endif
