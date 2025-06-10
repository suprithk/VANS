#ifndef VANS_CXL_H
#define VANS_CXL_H

#include "controller.h"
#include "static_memory.h"
#include "request_queue.h"

#include <deque>

namespace vans::cxl {

struct link_entry {
    base_request req;
    clk_t ready_clk;
};

class link_controller : public memory_controller<base_request, static_media> {
  public:
    size_t bandwidth;
    clk_t latency;
    size_t queue_size;
    std::deque<link_entry> queue;
    clk_t curr_clk = 0;

    link_controller() = delete;
    explicit link_controller(const config &cfg)
        : memory_controller(cfg),
          bandwidth(cfg.get_ulong("bandwidth")),
          latency(cfg.get_ulong("latency")),
          queue_size(cfg.get_ulong("queue_size")) {}

    base_response issue_request(base_request &request) override {
        if (queue.size() >= queue_size)
            return {false, false, clk_invalid};
        queue.push_back({request, curr_clk + latency});
        return {true, false, clk_invalid};
    }

    void drain_current() override {}

    bool pending_current() override { return !queue.empty(); }

    bool full() override { return queue.size() >= queue_size; }

    void tick(clk_t clk) override {
        curr_clk = clk;
        size_t issued = 0;
        while (issued < bandwidth && !queue.empty()) {
            auto &entry = queue.front();
            if (entry.ready_clk > curr_clk)
                break;
            auto [next_addr, next] = this->get_next_level(entry.req.addr);
            entry.req.addr = next_addr;
            entry.req.arrive = curr_clk;
            if (next->full())
                break;
            next->issue_request(entry.req);
            queue.pop_front();
            issued++;
        }
    }
};

class rc : public component<link_controller, static_media> {
  public:
    rc() = delete;
    explicit rc(const config &cfg) {
        this->ctrl = std::make_shared<link_controller>(cfg);
    }
    base_response issue_request(base_request &req) override {
        return this->ctrl->issue_request(req);
    }
};

class sw : public component<link_controller, static_media> {
  public:
    sw() = delete;
    explicit sw(const config &cfg) {
        this->ctrl = std::make_shared<link_controller>(cfg);
    }
    base_response issue_request(base_request &req) override {
        return this->ctrl->issue_request(req);
    }
};

class memory_device : public static_memory {
  public:
    memory_device() = delete;
    explicit memory_device(const config &cfg) : static_memory(cfg) {}
};

} // namespace vans::cxl

#endif // VANS_CXL_H
