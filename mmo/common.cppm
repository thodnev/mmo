module;
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

//#include <iostream>
export module common;
export namespace common {

template <typename T = unsigned long, unsigned long DEF_UPDATE_EVERY = 64>
class RandGen {
public:
    uint64_t last = 0;

    std::function<const T()> random_func;
    decltype(DEF_UPDATE_EVERY) update_every, update_cnt;

    RandGen(decltype(random_func) random_func, decltype(update_every) update_every = DEF_UPDATE_EVERY)
        : random_func(random_func), update_every(update_every)
    {
        update();
    }

    T update()
    {
        update_cnt = 0;
        last ^= random_func();
        //std::cout << " (UPDATE) ";
        return last;
    }
    
    T random(T from = std::numeric_limits<T>::min(),
             T to = std::numeric_limits<T>::max())
    {
        //std::cout << " [" << this->last << " @ " << this->update_every << " @ " << this->update_cnt << "] ";
        return (randval() % (to - from + 1)) + from;
    }

private:
    T randval()
    {
        if (++update_cnt >= update_every) {
            update();
        }

        // MMIX by Donald Knuth, LCG generator
        last = last * 6364136223846793005 + 1442695040888963407;
        return last;
    }
};


class BinMask {
public:
    std::vector<uint8_t> data;
    unsigned long width, height;


};

};      // namespace