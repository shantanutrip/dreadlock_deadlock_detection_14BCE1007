#ifndef DREADLOCK_H
#define DREADLOCK_H

#include <atomic>
#include <vector>
#include <unordered_map>
#include <thread>
#include <stdexcept>
#include <memory>
#include <boost/thread/locks.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread.hpp>
#include <mutex>
#include "bloom_filter.hpp"


const std::hash<std::thread::id> thread_id_hasher;

class spinlock {
private:
    boost::detail::spinlock lock_;
public:
    spinlock() : lock_(BOOST_DETAIL_SPINLOCK_INIT) { }
    spinlock(spinlock&& s) : lock_(std::move(s.lock_)) { }
    spinlock(const spinlock&) = delete;

    void lock() { lock_.lock(); }
    void unlock() { lock_.unlock(); }
};


class DreadlockManager {
private:
    std::unordered_map<std::thread::id, bloom_filter> thread_digests_; // Stores bloom filters for threads.
    std::unordered_map<std::thread::id, spinlock> digests_locks_;

    bloom_filter constructBloomFilterWithId() {
        bloom_parameters parameters;
        parameters.false_positive_probability = FALSE_POSITIVE_PROBABILITY;
        parameters.projected_element_count = APPROX_NUM_OF_THREADS;
        parameters.compute_optimal_parameters();

        bloom_filter res(parameters);
        res.insert(thread_id_hasher(std::this_thread::get_id()));
        return res;
    }
public:
    DreadlockManager() { }
    DreadlockManager(const DreadlockManager&) = delete;
    DreadlockManager& operator=(const DreadlockManager&) = delete;

    double FALSE_POSITIVE_PROBABILITY = 0.0001;
    unsigned long long int APPROX_NUM_OF_THREADS = 32;

    bloom_filter& getMyFilter() {
        std::thread::id this_thread_id = std::this_thread::get_id();
        std::lock_guard<spinlock> _(digests_locks_[this_thread_id]);
        if (thread_digests_[this_thread_id].element_count() == 0) {
            thread_digests_[this_thread_id] = constructBloomFilterWithId();
        }
        return thread_digests_[this_thread_id];
    }

    void unionMyFilter(const bloom_filter& with) {
        std::thread::id this_thread_id = std::this_thread::get_id();
        std::lock_guard<spinlock> _(digests_locks_[this_thread_id]);
        if (thread_digests_[this_thread_id].element_count() == 0) {
            thread_digests_[this_thread_id] = constructBloomFilterWithId().operator|=(with);
        } else {
            thread_digests_[this_thread_id].operator|=(with);
        }
        return;
    }

    bloom_filter getFilterById(const std::thread::id& id) {
        std::lock_guard<spinlock> _(digests_locks_[id]);
        bloom_filter res(thread_digests_[id]);
        return res;
    }

    void resetMyFilter() {
        std::thread::id this_thread_id = std::this_thread::get_id();
        std::lock_guard<spinlock> _(digests_locks_[this_thread_id]);
        thread_digests_[this_thread_id] = constructBloomFilterWithId();
    }
};


class Dreadlock {
private:
    DreadlockManager& manager_;
    std::thread::id owner_id_;

    bloom_filter state_;
    mutable spinlock state_lock_;

    void updateState_() {
        std::lock_guard<spinlock> _(state_lock_);
        state_ = manager_.getFilterById(owner_id_);
        return;
    }

    bloom_filter getState_() const {
        std::lock_guard<spinlock> _(state_lock_);
        return state_;
    }

public:
    Dreadlock(DreadlockManager& manager) : manager_(manager) { }
    Dreadlock(Dreadlock&& d) : manager_(d.manager_), state_(std::move(d.state_)), state_lock_(std::move(d.state_lock_)) { }

    void lock() {
        size_t current_thread_id_hash = thread_id_hasher(std::this_thread::get_id());
        while (true) {
            bloom_filter digest;
            while ((digest = getState_()).element_count() != 0) { // while lock is owned by somebody
                if (digest.contains(current_thread_id_hash)) {
                    // IMPROVE HERE, CUSTOM EXCEPTION MAYBE
                    throw std::logic_error("Deadlock detected!");
                } else {
                    manager_.unionMyFilter(digest);
                    updateState_();
                }
            }

            std::lock_guard<spinlock> _(state_lock_);
            if (state_.element_count() == 0) { // if lock is free
                manager_.resetMyFilter();
                state_ = manager_.getMyFilter();
                owner_id_ = std::this_thread::get_id();
                return;
            }
        }
    }

    void unlock() {
        std::lock_guard<spinlock> _(state_lock_);
        if (std::this_thread::get_id() != owner_id_) {
            throw std::logic_error("You're trying to unlock the lock which you don't own");
        }
        state_ = bloom_filter();
        owner_id_ = std::thread::id();
        return;
    }
};

#endif //DREADLOCK_H
