#pragma once
#include <array>
#include "types.h"


enum class DevType : uint32_t { Player = 0, Place = 1 };
extern DevType self_type;
extern uint32_t self_id;
extern uint8_t tx_pwr;


inline constexpr uint32_t kDevCnt = 16U; // Total amount of devices in system

#pragma region // =============== Radio pkt ==============
#pragma pack(push, 1)
struct rPkt {
    uint32_t sender_id;
    DevType sender_type;
};
#pragma pack(pop)
inline constexpr uint8_t kRPktSz = sizeof(rPkt);
static_assert(kRPktSz == 8, "rPkt size mismatch");
static_assert(std::is_trivially_copyable<rPkt>::value, "rPkt is not trivially copyable");
#pragma endregion

bool GetPktToTx(rPkt &apkt); // implement in app

#pragma region // =============== Rx Table ==============
class RxTable {
public:
    // An item returned by iteration: the received packet plus its RSSI.
    struct RxItem {
        rPkt pkt{};
        int8_t rssi = 0;
    };
private:
    struct Item {
        RxItem rx{};
        int32_t counter = 0;  // <= 0 means empty/dead
    };
    static constexpr const uint32_t kCapacity = kDevCnt;
    std::array<Item, kCapacity> iarr;

    // Iterator implementation
    class iterator {
        const RxTable* table;
        uint32_t indx;
        void AdvanceToNextValid() {
            while(indx < RxTable::kCapacity and table->iarr[indx].counter <= 0) ++indx;
        }
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = RxItem;
        using difference_type = std::ptrdiff_t;
        using pointer = const RxItem*;
        using reference = const RxItem&;
        iterator(const RxTable* t, uint32_t i) : table(t), indx(i) { AdvanceToNextValid(); }
        reference operator*() const { return table->iarr[indx].rx; }
        pointer operator->()  const { return &table->iarr[indx].rx; }
        iterator& operator++() {
            ++indx;
            AdvanceToNextValid();
            return *this;
        }
        bool operator==(const iterator& other) const { return indx == other.indx; }
        bool operator!=(const iterator& other) const { return !(*this == other);  }
    };
public:
    // How many radio ticks an RxTable entry survives after its last refresh.
    static constexpr const int32_t kTimeout_tics = 4L;
    void AddPkt(const rPkt &apkt, int8_t rssi) {
        // Linear scan: update existing entry or insert into first empty slot.
        // Dedup key is the sender id: each device occupies one slot
        const int32_t timeout_tics = kTimeout_tics;
        uint32_t empty_slot = kCapacity;
        for(uint32_t i = 0; i < kCapacity; ++i) {
            Item &itm = iarr[i];
            if(itm.counter > 0) {
                if(itm.rx.pkt.sender_id == apkt.sender_id) { // existing sender -> update
                    itm.rx.pkt = apkt;
                    itm.rx.rssi = rssi;
                    itm.counter = timeout_tics;
                    return;
                }
            }
            else if(empty_slot == kCapacity) {
                empty_slot = i; // remember first empty slot
            }
        }
        if(empty_slot < kCapacity) { // insert new entry
            Item &itm = iarr[empty_slot];
            itm.rx.pkt = apkt;
            itm.rx.rssi = rssi;
            itm.counter = timeout_tics;
        }
    }
    void Tick() {
        for(auto &itm : iarr) { if(itm.counter > 0) itm.counter--; }
    }
    // Zero all counters so every slot is treated as empty/dead immediately.
    void Clear() { for(auto &itm : iarr) itm.counter = 0; }

    // Lookup by sender id. Returns nullptr if not present (or expired).
    const RxItem* Find(uint32_t from) const {
        for(uint32_t i = 0; i < kCapacity; ++i) {
            const Item &itm = iarr[i];
            if(itm.counter > 0 and itm.rx.pkt.sender_id == from) return &itm.rx; // found
        }
        return nullptr;
    }
    // Remove the entry pointed to by the given RxItem pointer. Returns true if
    // an entry was found and removed (its counter is zeroed, so it is treated
    // as empty/dead immediately). Safe to call during iteration: the iterator
    // only advances its index and skips dead slots in AdvanceToNextValid().
    bool Erase(const RxItem* it) {
        if(it == nullptr) return false;
        for(uint32_t i = 0; i < kCapacity; ++i) {
            if(&iarr[i].rx == it) {
                iarr[i].counter = 0;
                return true;
            }
        }
        return false;
    }
    // Range-based for support
    iterator begin() const { return iterator(this, 0); }
    iterator end()   const { return iterator(this, kCapacity); }
};

extern RxTable rx_table;
void ProcessRxTable();
#pragma endregion