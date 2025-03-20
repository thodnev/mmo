#if defined(DEBUG) && ((DEBUG) == 1)
    #include <format>
    #include <iostream>
    #define LOG(...) do {std::cerr << "LOG: " << std::format(__VA_ARGS__) << std::endl;} while(0)
#else
    #define LOG(...)
#endif