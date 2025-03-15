module;     // Global module fragment
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

export module types;
export namespace types {

template <typename T = uint8_t>
struct Stats
{
    T STR = 0;       // Strength
    T INT = 0;       // Intelligence
    T DEX = 0;       // Dexterity

    T CON = 0;       // Constitution
    T WIS = 0;       // Wisdom
    T AGI = 0;       // Agility

    T LUK = 0;       // Luck
    T INS = 0;       // Insanity

    std::vector<T> to_vector() const {
        return {
            this->STR, this->INT, this->DEX, this->CON, 
            this->WIS, this->AGI, this->LUK, this->INS
        };
    }

    operator std::vector<T>() const {
        return this->to_vector();
    }

    std::vector<std::pair<std::string, T>> to_pairs() const {
        const std::string names[] = { "STR", "INT", "DEX", "CON", "WIS", "AGI", "LUK", "INS" };
        auto vals = this->to_vector();

        decltype(this->to_pairs()) res;
        for (auto i = 0; i < vals.size(); i++) {
            res.emplace_back(names[i], vals[i]);
        }
        return res;
        // return {
        //     // __STR_PAIR(STR, this),
        //     // __STR_PAIR(INT, this),
        //     {"STR", this->STR},
        //     {"INT", this->INT},
        //     {"DEX", this->DEX},
        //     {"CON", this->CON},
        //     {"WIS", this->WIS},
        //     {"AGI", this->AGI},
        //     {"LUK", this->LUK},
        //     {"INS", this->INS}
        // };
    }

    std::string to_string(const std::string sep = ", ") const {
        std::string res;

        auto vec = this->to_pairs();
        for (auto it_pair = vec.begin(); it_pair != vec.end(); ) {
            res += it_pair->first + ": " + std::to_string(it_pair->second);
            if (++it_pair == vec.end()) {
                break;          // Avoid adding after the last string
            }
            res += sep;
        }

        return res;
    }

    operator std::string() const {
        return this->to_string();
    }

    friend std::ostream & operator<<(std::ostream &os, const Stats &stats) {
        return os << stats.to_string();
    }
};

}       // namespace