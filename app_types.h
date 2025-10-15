#pragma once

#include "types.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <array>
#include <type_traits>
#include "shell.h"

enum class DevType { None, Host, Wormhole, Mengir, Locket };
enum WHType { NonLethal=0, LethalWeak=1, LethalStrong=2 };
enum class Vote : int32_t { None = 0, A = 1, B = 2};
inline constexpr const int32_t kRpktLktsCnt = 10;

struct InListVoted { bool is_in_list=false, is_voted=false; };

namespace IDs { // ============= IDs =============
    inline constexpr const int32_t None = 0;
    inline constexpr const int32_t HostMin=1, HostMax=9;
    inline constexpr const int32_t LocketMin=10, LocketMax=127; // MSB shows vote accepted or not in the wormhole pkt
    inline constexpr const int32_t WormholeMin=200, WormholeMax=219, WormholeCnt = WormholeMax - WormholeMin + 1;
    inline constexpr const int32_t MengirMin=220, MengirMax=249, MengirCnt = MengirMax - MengirMin + 1;
    __attribute__((unused)) static inline int32_t Wormhole2indx(int32_t id) { return id - WormholeMin; }
    __attribute__((unused)) static inline int32_t Mengir2indx(int32_t id) { return id - MengirMin; }

     __attribute__((unused)) static InListVoted CheckID(uint8_t id, const uint8_t *ids) {
        InListVoted r;
        for(uint32_t i=0; i<kRpktLktsCnt; i++) {
            uint8_t lid = ids[i];
            if((lid & 0x7F) == id) {
                r.is_in_list = true;
                r.is_voted = (lid & 0x80) != 0;
                break;
            }
        }
        return r;
    }
} // namespace


struct Locket {
    enum Sta {Dead = 0, Level1 = 1, Level2 = 2};
    uint32_t id = 0;
    Sta state = Level1;
    bool btnA_pressed = false, btn_middle_pressed = false, btnB_pressed = false;
    Vote vote = Vote::None;
    Locket() {}
    Locket(uint32_t aid, uint8_t astate, uint8_t btnA, uint8_t btnMid, uint8_t btnB) :
        id(aid), state(static_cast<Sta>(astate)),
        btnA_pressed(btnA != 0),
        btn_middle_pressed(btnMid != 0),
        btnB_pressed(btnB != 0),
        vote(Vote::None) {}
    void Print() const { Printf("id=%u sta=%u bA=%u bM=%u bB=%u\n", id, state, btnA_pressed, btn_middle_pressed, btnB_pressed); }
};

using Lockets = std::unordered_map<uint32_t, Locket>;


class FailedGroup {
public:
    std::unordered_set<uint32_t> ids;
    int32_t time_left_s = 0;
    FailedGroup(const Lockets &lkts, int32_t blocking_time_m) {
        ids.reserve(lkts.size());
        for(const auto& [id, lkt] : lkts) ids.insert(id);
        time_left_s = blocking_time_m * 60L;
    }
    bool Contains(const Locket &lkt) const { return ids.contains(lkt.id); }
};
using FailedGroups = std::vector<FailedGroup>;

class Mengir {
public:
    static const uint32_t kBottom = 0, kTop = 7;
    uint32_t id = 0;
    uint32_t value = 0;
    uint32_t timestamp = 0;
    void Increment() { if(value < kTop) value++; }
    void Decrement() { if(value > kBottom) value--; }
    void Print() const { Printf("  id=%u val=%u t_passed_s=%u\n", id, value, TIME_I2S(chVTTimeElapsedSinceX(timestamp))); }
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

    Mengir* begin() { return arr.data(); }
    Mengir* end() { return arr.data() + arr.size(); }

    uint32_t size() const {
        uint32_t count = 0;
        for(const auto& mengir : arr) {
            if (mengir.id == 0) break;
            else count++;
        }
        return count;
    }

    void Print() {
        if(size() == 0) Printf("No mengirs\n");
        else {
            Printf("Mengirs:\n");
            for(auto& mengir : arr) {
                if(mengir.id == 0) break;
                else mengir.Print();
            }
        }
    }
};

#pragma region // =========================== Radio Packet ===============================
enum class WormholeCmd : uint8_t { None=0, KillThemAll=4 };

#pragma pack(push, 1)
struct rPkt {
    uint8_t id;  // 1 byte
    union {      // 11 bytes
        struct { // 5 bytes
            uint8_t state;
            uint8_t btnA_pressed, btn_middle_pressed, btnB_pressed;
            uint8_t speaks_with_wormhole;
        } locket;
        struct { // 11 bytes
            WormholeCmd cmd;
            uint8_t ids[kRpktLktsCnt];

        } wormhole;
        struct { // 1 byte
            uint8_t value;
        } mengir;
    };
    void PrintLocket(const char* S) const {
        Printf("%Sid=%u sta=%u; %u %u %u\r", S, id, locket.state, locket.btnA_pressed, locket.btn_middle_pressed, locket.btnB_pressed);
    }
    void PrintWormhole(const char* S) const {
        Printf("%Sid=%u cmd=%u L:", S, id, wormhole.cmd);
        for(uint32_t i=0; i<kRpktLktsCnt; i++) Printf(" %u", wormhole.ids[i]);
        PrintfEOL();
    }
    void PrintMengir(const char* S) const { Printf("%Sid=%u value=%u\r", S, id, mengir.value); }

    DevType GetType() const {
        if     (id >= IDs::HostMin and id <=IDs::HostMax) return DevType::Host;
        else if(id >= IDs::WormholeMin and id <= IDs::WormholeMax) return DevType::Wormhole;
        else if(id >= IDs::MengirMin and id <= IDs::MengirMax) return DevType::Mengir;
        else if(id >= IDs::LocketMin and id <= IDs::LocketMax) return DevType::Locket;
        else return DevType::None;
    }
};
#pragma pack(pop)
inline constexpr const uint8_t kRPktSz = sizeof(rPkt);

// Various checks of the rPkt
static_assert(kRpktLktsCnt <= 11, "wormhole.ids too large for rPkt");
static_assert(sizeof(rPkt) == 12, "rPkt size mismatch");
static_assert(std::is_trivially_copyable_v<rPkt>, "rPkt must be trivially copyable");
static_assert(std::is_standard_layout_v<rPkt>, "rPkt must be standard layout");
static_assert(std::is_trivial_v<rPkt>, "rPkt must be trivial");
#pragma endregion