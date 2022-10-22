#pragma once

#include <random>
#include <utility>
#include <mutex>
#include <map>
#include <stdexcept>
#include <cassert>

namespace gnunetpp::detail
{
thread_local std::mt19937_64 g_rng(std::random_device{}());

template <typename Data>
struct UniqueData
{
    std::mutex mtx;
    std::map<size_t, Data> data;

    UniqueData()
    {

    }

    std::pair<size_t, Data&> add(Data&& d)
    {
        std::lock_guard l(mtx);
        size_t id = 0;
        do {
            id = g_rng();
        } while(data.find(id) != data.end());
        data[id] = std::move(d);
        return {id, data[id]};
    }

    Data& operator[] (size_t id)
    {
        std::lock_guard l(mtx);
        auto it = data.find(id);
        if(it == data.end())
            throw std::runtime_error("Invalid ID");
        return it->second;
    }

    void remove(size_t id)
    {
        std::lock_guard l(mtx);
        auto it = data.find(id);
        assert(it != data.end());
        data.erase(it);
    }
};

}