module;
#include "macro.hpp"
#include <cstdint>
#include <fstream>
#include <limits>
#include <filesystem>

export module rnd;
export namespace rnd {

template <typename T = unsigned long>
class RandGenBase {
public:
    uint64_t last = 0;

    unsigned long update_every, update_cnt;

    virtual ~RandGenBase() {};

    RandGenBase(decltype(update_every) update_every = 64)
        : update_every(update_every), update_cnt(update_every) {}

    T update()
    {
        update_cnt = 0;
        last ^= random_func();
        // LOG("UPDATE");
        return last;
    }
    
    T random(T from = std::numeric_limits<T>::min(),
             T to = std::numeric_limits<T>::max())
    {
        //std::cout << " [" << this->last << " @ " << this->update_every << " @ " << this->update_cnt << "] ";
        return (randval() % (to - from + 1)) + from;
    }

protected:
    virtual const T random_func() =0;       // abstract method

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


template <typename T = unsigned long>
class RandGenLinux : public RandGenBase<T>
{
public:
    RandGenLinux(decltype(RandGenBase<T>::update_every) update_every = 64,
                 const std::filesystem::path &rand_file = "/dev/urandom")
        : RandGenBase<T>(update_every), rand_stream(rand_file, std::ios::binary) {}

protected:
    virtual const T random_func() override
    {
        T ret = 0;
        rand_stream.read(reinterpret_cast<char *>(&ret), sizeof(ret));
        return ret;
    };

private:
    std::ifstream rand_stream;
};

}   // namespace