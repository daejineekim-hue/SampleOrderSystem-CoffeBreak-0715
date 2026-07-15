#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace sos::repository {
class OrderRepository;
class SampleRepository;
}  // namespace sos::repository

namespace sos::production {

// Single production line with a FIFO queue (docs/FEATURES/production-line.md).
// Completion is judged lazily: advanceIfDue() is called internally by every
// query method, so callers never need to poll on a timer/thread.
class ProductionLine {
public:
    using Clock = std::function<std::chrono::system_clock::time_point()>;

    struct Status {
        bool active = false;
        std::string orderId;
        std::string sampleId;
        int quantity = 0;
        int producedTotal = 0;
        double progressPercent = 0.0;
        double remainingMin = 0.0;
    };

    ProductionLine(repository::OrderRepository& orderRepository,
                    repository::SampleRepository& sampleRepository,
                    Clock clock = &std::chrono::system_clock::now);

    // Registers an order (already transitioned to PRODUCING by
    // service::OrderLifecycleService::approve) at the tail of the queue. If it becomes the
    // new head (queue was empty), its shortage/producedTotal/productionTimeMin
    // are computed immediately against the sample's current stock.
    void enqueue(const std::string& orderId);

    // Judges whether the head order's production time has elapsed and, if
    // so, completes it (and cascades through any further already-due heads).
    void advanceIfDue();

    // Current head's production status. Calls advanceIfDue() first.
    Status currentStatus();

    // Order ids waiting behind the head, in FIFO order. Calls
    // advanceIfDue() first.
    std::vector<std::string> waitingOrderIds();

private:
    struct QueueEntry {
        std::string orderId;
        bool started = false;
        std::chrono::system_clock::time_point productionStartedAt{};
        int producedTotal = 0;
        double productionTimeMin = 0.0;
    };

    repository::OrderRepository& orderRepository_;
    repository::SampleRepository& sampleRepository_;
    Clock clock_;
    std::deque<QueueEntry> queue_;

    void startEntry(QueueEntry& entry);
    // Applies Formula #3 (stock +producedTotal/-quantity, status ->
    // CONFIRMED) directly via OrderRepository/SampleRepository; no callback
    // into a service is needed (docs/design_refact.md item 1+2).
    void completeHead(const QueueEntry& head);
};

}  // namespace sos::production
