#pragma once
#include <vector>
#include <cstring>
namespace {
struct Fake {
    std::vector<uint8_t> bytes = std::vector<uint8_t>(NOR_CAPACITY, 0xff);
    uint32_t id = 0xef4018, address = 0;
    uint8_t op = 0, status = 0;
    bool selected = false, command = false, refuseEnable = false;
    int calls = 0, failAt = 0, mutations = 0, failOp = 0, readCalls = 0, corruptReadAt = 0;
    static void select(void *ctx, int on)
    {
        auto &f = *static_cast<Fake *>(ctx);
        if (on) { f.selected = true; f.command = false; f.op = 0; }
        else {
            if (f.op == 0x20 && (f.status & 2)) {
                std::fill(f.bytes.begin()+f.address, f.bytes.begin()+f.address+NOR_SECTOR, 0xff);
                ++f.mutations; f.status &= ~2u;
            }
            f.selected = false;
        }
    }
    static int transfer(void *ctx, const uint8_t *tx, uint8_t *rx, size_t n)
    {
        auto &f = *static_cast<Fake *>(ctx);
        if (!f.selected) qFatal("transfer with CS released");
        if (++f.calls == f.failAt || (!f.command && f.failOp && tx[0]==f.failOp)) {
            if(f.command && f.op==0x02 && (f.status&2)) {
                for(size_t i=0;i<std::min<size_t>(3,n);++i) f.bytes.at(f.address+i)&=tx[i];
                ++f.mutations;f.status&=~2u;
            }
            return -1;
        }
        if (!f.command) {
            f.op = tx[0]; f.command = true;
            if (n == 4) f.address = uint32_t(tx[1])<<16 | uint32_t(tx[2])<<8 | tx[3];
            if (f.op == 0x06 && !f.refuseEnable) f.status |= 2;
            std::memset(rx, 0, n); return 0;
        }
        for (size_t i = 0; i < n; ++i) {
            const uint8_t outgoing = tx[i];
            if (f.op == 0x9f) rx[i] = uint8_t(f.id >> (16-8*i));
            else if (f.op == 0x05) rx[i] = f.status;
            else if (f.op == 0x03) rx[i] = f.bytes.at(f.address+i);
            else if (f.op == 0x02 && (f.status & 2)) { f.bytes.at(f.address+i) &= outgoing; rx[i] = 0; }
            else qFatal("unexpected NOR operation");
        }
        if (f.op == 0x03 && ++f.readCalls==f.corruptReadAt) rx[0]^=1;
        if (f.op == 0x02) { ++f.mutations; f.status &= ~2u; }
        return 0;
    }
    nor_bus bus() { return {this, select, transfer}; }
};
}
