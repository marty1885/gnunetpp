#pragma once

#include <random>
#include <utility>
#include <mutex>
#include <map>
#include <stdexcept>
#include <cassert>

namespace gnunetpp::detail
{
extern thread_local std::mt19937_64 g_rng;

template <typename Data>
struct UniqueData
{
    std::pair<size_t, Data&> add(Data&& d)
    {
        std::lock_guard l(mtx);
        size_t id = 0;
        do {
            id = g_rng();
        } while(id != 0 && data.find(id) != data.end());
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

    bool contains(size_t id)
    {
        std::lock_guard l(mtx);
        return data.find(id) != data.end();
    }

    // IMPORTANT! You REALLY DON'T want to use these APIs. They are dangerous
    // under multithreaded environment. Make damn sure you know what you are
    // doing else suffer the consequences.
    auto begin()
    {
        return data.begin();
    }

    auto end()
    {
        return data.end();
    }

    auto begin() const
    {
        return data.begin();
    }

    auto end() const
    {
        return data.end();
    }

    void clear()
    {
        std::lock_guard l(mtx);
        data.clear();
    }
protected:
    std::mutex mtx;
    std::map<size_t, Data> data;
};

}