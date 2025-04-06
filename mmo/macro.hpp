// set constant to allow using in regular if-blocks
#if defined(DEBUG) && ((DEBUG) == 1)
    #define USE_DEBUG (1)
#else
    #define USE_DEBUG (0)
#endif

#if defined(DEBUG) && ((DEBUG) == 1)
    #include <format>
    #include <iostream>
    #define LOG(...) do {std::cerr << "LOG: " << std::format(__VA_ARGS__) << std::endl;} while(0)
#else
    #define LOG(...)
#endif

#if defined(DEBUG) && ((DEBUG) == 1)
    #include <chrono>
    #define TIMEIT_EXPR(TIMEVAR, EXPR) ([&] {                             \
        auto __tstart = std::chrono::high_resolution_clock::now();        \
        auto __res = ( EXPR );                                            \
        auto __tstop = std::chrono::high_resolution_clock::now();         \
        std::chrono::duration<double> __took = __tstop - __tstart;        \
        (TIMEVAR) += __took;                                              \
        return __res;                                                     \
    })()
#else
    #define TIMEIT_EXPR(TIMEVAR, EXPR)  ( EXPR )
#endif