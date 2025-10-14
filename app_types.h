#pragma once

#include "types.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <array>
#include "shell.h"

enum class DevType { None, Host, Wormhole, Mengir, Locket };
enum WHType { NonLethal=0, LethalWeak=1, LethalStrong=2 };
enum class Vote : int32_t { None = 0, A = 1, B = 2};
inline constexpr const int32_t kRpktLktsCnt = 10;

namespace IDs { // ============= IDs =============
    inline constexpr const int32_t None = 0;
    inline constexpr const int32_t HostMin=1, HostMax=9;
    inline constexpr const int32_t LocketMin=10, LocketMax=127; // MSB shows vote accepted or not in the wormhole pkt
    inline constexpr const int32_t WormholeMin=200, WormholeMax=219, WormholeCnt = WormholeMax - WormholeMin + 1;
    inline constexpr const int32_t MengirMin=220, MengirMax=249, MengirCnt = MengirMax - MengirMin + 1;
    __attribute__((unused)) static inline int32_t Wormhole2indx(int32_t id) { return id - WormholeMin; }
    __attribute__((unused)) static inline int32_t Mengir2indx(int32_t id) { return id - MengirMin; }
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

#pragma region // =========================== Radio Packet ===============================
enum class WormholeCmd : uint8_t { None=0, KillThemAll=4 };

struct InListVoted { bool is_in_list=false, is_voted=false; };

#pragma pack(push, 1)
union rPkt {
    uint32_t dw32[3];
    struct {
        uint8_t id;  // 1 byte
        union {
            struct { // 5 bytes
                uint8_t state;
                uint8_t btnA_pressed, btn_middle_pressed, btnB_pressed;
                uint8_t speaks_with_wormhole;
            } locket;
            struct { // 11 bytes
                WormholeCmd cmd;
                uint8_t ids[kRpktLktsCnt];
                InListVoted CheckID(uint8_t id) {
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
            } wormhole;
            struct { // 1 byte
                uint8_t value;
            } mengir;
        };
    };
    rPkt& operator = (const rPkt &right) {
        dw32[0] = right.dw32[0];
        dw32[1] = right.dw32[1];
        dw32[2] = right.dw32[2];
        return *this;
    }
    void PrintLocket(const char* S) {
        Printf("%Sid=%u sta=%u; %u %u %u\r", S, id, locket.state, locket.btnA_pressed, locket.btn_middle_pressed, locket.btnB_pressed);
    }
    void PrintWormhole(const char* S) {
        Printf("%Sid=%u cmd=%u L:", S, id, wormhole.cmd);
        for(uint32_t i=0; i<kRpktLktsCnt; i++) Printf(" %u", wormhole.ids[i]);
        PrintfEOL();
    }
    void PrintMengir(const char* S) {
        Printf("%Sid=%u value=%u\r", S, id, mengir.value);
    }

    DevType GetType() {
        if     (id >= IDs::HostMin and id <=IDs::HostMax) return DevType::Host;
        else if(id >= IDs::WormholeMin and id <= IDs::WormholeMax) return DevType::Wormhole;
        else if(id >= IDs::MengirMin and id <= IDs::MengirMax) return DevType::Mengir;
        else if(id >= IDs::LocketMin and id <= IDs::LocketMax) return DevType::Locket;
        else return DevType::None;
    }

};
#pragma pack(pop)
inline constexpr const uint8_t kRPktSz = sizeof(rPkt);
#pragma endregion