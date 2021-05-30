#include <cstdint>
#include <cstdlib>
#include <array>

#ifndef MEMORY
#define MEMORY

struct Mem{
    public:
    static const std::size_t MEM_LEN = 0xFFFF;
    std::array<uint8_t, MEM_LEN> data;

    auto get(std::size_t index) const -> uint8_t{
        return data.at(index);
    }

    auto set(std::size_t index, uint8_t value) -> void{
        data.at(index) = value;
    }
    /// Sets everything in memory to 0.
    auto reset() {
        for (auto& byte: data){
            byte = 0;
        }
    }
};

#endif