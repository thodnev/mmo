#include <chrono>
#include <iostream>

import rnd;

int main(const int argc, char * const argv[])
{
    std::cout << "Test rnd" << std::endl;

    rnd::RandGenLinux rng(500, "/dev/random");
    auto maxop = 1e7;
    unsigned long sum = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (unsigned long long i = 0; i < maxop; i++) {
        auto result = rng.random(0, 502);
        //std::cout << result << std::endl;
        sum += result;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;

    std::cout << "TIME: " << 1000 * duration.count() << " ms\tSUM: " << sum << "\n";
    std::cout << "OPS/s: " << maxop / duration.count() << "\n";

    return 0;
}