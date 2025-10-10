#pragma once

#include"types.h"
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <array>

enum WHType { NonLethal=0, LethalWeak=1, LethalStrong=2 };
inline constexpr const int32_t kChoiceA = 1, kChoiceB = 2;

namespace IDs { // ============= IDs =============
    inline constexpr const uint32_t None = 0;
    inline constexpr const uint32_t HostMin=1, HostMax=9;
    inline constexpr const uint32_t WormholeMin=10, WormholeMax=29;
    inline constexpr const uint32_t MengirMin=30, MengirMax=49;
    inline constexpr const uint32_t LocketMin=50, LocketMax=249;
} // namespace

enum class DevType { None, Host, Wormhole, Mengir, Locket };


struct Locket {
    uint32_t id = 0, level = 1;
    bool is_alive = true;
    bool btnA_pressed = false, btn_middle_pressed = false, btnB_pressed = false;
    bool vote_accepted = false;
    Locket() {}
    Locket(uint32_t aid, uint32_t alvl, bool alive, bool btnA, bool btnMid, bool btnB) :
        id(aid), level(alvl), is_alive(alive), btnA_pressed(btnA),
        btn_middle_pressed(btnMid), btnB_pressed(btnB), vote_accepted(false) {}
    bool operator<(const Locket& other) const { return id < other.id; }
    void Print() const { Printf("id=%u lvl=%u a=%u bA=%u bM=%u bB=%u\n", id, level, is_alive, btnA_pressed, btn_middle_pressed, btnB_pressed); }
};
using Lockets = std::set<Locket>;


class FailedGroup {
public:
    std::unordered_set<uint32_t> ids;
    int32_t time_left_s = 0;
    FailedGroup(const Lockets &lkts, int32_t blocking_time_m) {
        ids.reserve(lkts.size());
        std::transform(lkts.begin(), lkts.end(),
                   std::inserter(ids, ids.end()),
                   [](const Locket& l) { return l.id; });
        time_left_s = blocking_time_m * 60L;
    }
    bool Contains(const Locket &lkt) const { return ids.contains(lkt.id); }
};
using FailedGroups = std::vector<FailedGroup>;

using Votes = std::unordered_map<uint32_t, uint32_t>;

class Mengir {
public:
    static const uint32_t kBottom = 0, kTop = 7;
    uint32_t id = 0;
    uint32_t value = 0;
    uint32_t timestamp = 0;
    void Increment() { if(value < kTop) value++; }
    void Decrement() { if(value > kBottom) value--; }
};

class MengirContainer {
public:
    static const uint32_t kMaxCnt = 11;
private:
    std::array<Mengir, kMaxCnt> arr;
public:
    MengirContainer() { for(auto& mengir : arr) mengir.id = 0; }
    Mengir& operator[](uint32_t id) {
        // Try to find existing Mengir with this ID
        for(auto& mengir : arr) {
            if (mengir.id == id) return mengir;
        }
        // If not found, find an empty slot (ID=0) and assign the new ID
        for(auto& mengir : arr) {
            if(mengir.id == 0) {
                mengir.id = id;
                mengir.value = 0;
                mengir.timestamp = 0;
                return mengir;
            }
        }
        return arr[kMaxCnt-1]; // Must not happen
    }

    uint32_t size() const {
        uint32_t count = 0;
        for(const auto& mengir : arr) {
            if (mengir.id == 0) break;
            else count++;
        }
        return count;
    }
};
