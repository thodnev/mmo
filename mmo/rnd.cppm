module;
#include <cstdint>
#include <limits>
#include <filesystem>

export module rnd;
namespace rnd {

template <typename T = unsigned long>
class RandGenBase {
public:
    uint64_t last = 0;

    unsigned long update_every, update_cnt;

    virtual ~RandGenBase() {};

    RandGenBase(decltype(update_every) update_every = 64)
        : update_every(update_every)
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

protected:
    virtual const T random_func();

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
                 const std::filesystem::path &randfile = "/dev/urandom")
        : RandGenBase<T>(update_every)
    {
        // @TODO: ...
    }

protected:
    const T random_func() override
    {
        // @TODO: ...
    };
};

}   // namespace