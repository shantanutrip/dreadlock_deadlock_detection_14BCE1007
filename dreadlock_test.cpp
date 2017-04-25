#define BOOST_TEST_MODULE Dreadlock Test

#include <iostream>
#include <thread>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <atomic>
#include <vector>
#include <boost/test/unit_test.hpp>
#include "dreadlock.hpp"

BOOST_AUTO_TEST_CASE(one_thread_no_deadlocks) {
        DreadlockManager m;
        Dreadlock d1(m);
        for (int i = 0; i < 100; ++i) {
           d1.lock();
           d1.unlock();
        }

        std::thread t1([&d1]{
          for (int i = 0; i < 1000; ++i) {
              d1.lock();
              std::this_thread::sleep_for(std::chrono::milliseconds(1));
              d1.unlock();
          }
        });
        t1.join();
}

BOOST_AUTO_TEST_CASE(two_threads_different_dreadlocks) {
        DreadlockManager m;
        Dreadlock d1(m), d2(m);
        std::thread t1([&d1, &d2]{
          for (int i = 0; i < 1000; ++i) {
              d1.lock();
              std::this_thread::sleep_for(std::chrono::milliseconds(1));
              d1.unlock();
          }
        });

        std::thread t2([&d1, &d2]{
           for (int i = 0; i < 1000; ++i) {
                d2.lock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                d2.unlock();
           }
        });
        t1.join();
        t2.join();
}

BOOST_AUTO_TEST_CASE(two_threads_one_dreadlock_without_deadlocks) {
        DreadlockManager m;
        Dreadlock d(m);
        std::thread t1([&d]{
           for (int i = 0; i < 1000; ++i) {
                d.lock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                d.unlock();
           }
        });

        std::thread t2([&d]{
           for (int i = 0; i < 1000; ++i) {
                d.lock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                d.unlock();
           }
        });
        t1.join();
        t2.join();
}

BOOST_AUTO_TEST_CASE(two_threads_one_dreadlock_with_deadlock) {
        DreadlockManager m;
        Dreadlock d(m);
        std::thread t1([&d]{
           for (int i = 0; i < 100; ++i) {
                d.lock();
             //  std::cout << "thread 1 lock\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                d.unlock();
           }
           d.lock();
        //    std::cout << "thread 1 lock PREDEADLOCK CONDITION\n";
           BOOST_CHECK_THROW(d.lock(), std::logic_error);
           d.unlock();
        });

        std::thread t2([&d]{
           for (int i = 0; i < 100; ++i) {
                d.lock();
         //      std::cout << "thread 2 lock\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                d.unlock();
           }
        });
        t1.join();
        t2.join();
}

BOOST_AUTO_TEST_CASE(more_complex_lock_chain_without_deadlock) {
        DreadlockManager m;
        Dreadlock d1(m), d2(m), d3(m), d4(m);

        std::thread t1([&]{
           d1.lock();
           d2.lock();
           std::this_thread::sleep_for(std::chrono::seconds(1));
           d1.unlock();
           d2.unlock();
           d3.lock();
           d4.lock();

        });

        std::thread t2([&]{
           d3.lock();
           d4.lock();
           std::this_thread::sleep_for(std::chrono::seconds(1));
           d3.unlock();
           d4.unlock();
           d2.lock();
           d1.lock();
        });
        t1.join();
        t2.join();
}

BOOST_AUTO_TEST_CASE(more_complex_lock_chain_with_deadlock) {
        DreadlockManager m;
        Dreadlock d1(m), d2(m);

        d2.lock();
        std::thread t1([&]{
           d1.lock();
           d2.lock();
           d2.unlock();
           d1.unlock();
        });
        std::this_thread::sleep_for(std::chrono::seconds(1));
        BOOST_CHECK_THROW(d1.lock(), std::logic_error);
        d2.unlock();
        t1.join();
}

BOOST_AUTO_TEST_CASE(eating_philosophers_test) {
        DreadlockManager m;
        const int N = 5;
        std::vector<Dreadlock> locks;
        for (int i = 0; i < N; ++i)
            locks.emplace_back(m);
        std::vector<std::thread> threads;
        for (int i = 0; i < N; ++i) {
            threads.emplace_back([i, &locks]{
                while (true) {
                    try {
                        int min = std::min(i, (i+1)%N);
                        int max = std::max(i, (i+1)%N);
                        locks[min].lock();
                        locks[max].lock();
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                        locks[max].unlock();
                        locks[min].unlock();
                    } catch (...) {
                        std::string s = "thread " + std::to_string(i) + " detected deadlock\n";
                        std::cout << s;
                    }
                }
            });
        }
        for (int i = 0; i < N; ++i) {
            threads[i].join();
        }
}
