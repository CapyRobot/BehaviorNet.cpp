/*
 * Copyright (C) 2023 Eduardo Rocha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <catch2/catch_test_macros.hpp>

#include <utils/Mutex.hpp>

#include <thread>
#include <chrono>

// end of scope will call lock destructor and unlock handle
// This logic is used to facilitate testing, but it should be avoided in general since the dangling handle is dangerous.
#define UNLOCK(handle)                                                                                                 \
    {                                                                                                                  \
        auto unlocker = std::move(handle);                                                                             \
    }

TEST_CASE("We can create mutexes for wrapped objects", "[CapybotUtils/Mutex]")
{
    // simple int, no args needed
    capybot::mutex<int> intMtx;
    std::ignore = intMtx;

    // empty std::string, no args needed
    capybot::mutex<std::string> intEmptyStr;
    REQUIRE(intEmptyStr.lock()->empty());

    // empty std::string, no args needed
    capybot::mutex<std::string> intStr("not empty");
    REQUIRE_FALSE(intStr.lock()->empty());
}

TEST_CASE("Const correctness", "[CapybotUtils/Mutex]")
{
    {
        capybot::mutex<int> intMtx;
        {
            capybot::mutex<int>::ConstHandle reader = intMtx.shared_lock();
            // should not compile, note: we could use type_traits to create test
            // *reader = 5;
        }
        {
            capybot::mutex<int>::Handle writer = intMtx.lock();
            *writer = 5;
        }
    }

    {
        const capybot::mutex<int> intMtx;

        // should not compile
        // auto writer = intMtx.lock();
        // auto writer = intMtx.try_lock();
        // std::ignore = intMtx.locked_execution<int>([](int& val) -> int { return val; });

        // reading is ok
        std::ignore = intMtx.shared_lock();
        std::ignore = intMtx.try_lock_shared();
        std::ignore = intMtx.locked_execution_shared<int>([](const int& val) -> int { return val; });
    }
}

TEST_CASE("[lock/shared_lock] many readers can share ownership, writer gets blocked", "[CapybotUtils/Mutex]")
{
    capybot::mutex<int> intMtx(0);

    auto reader1 = intMtx.shared_lock(); // owns mutex

    auto writerThread = std::thread([&intMtx] {
        auto writerHandle = intMtx.lock(); // blocked until all readers release
        *writerHandle = 1;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // no deadlock because of shared ownership
    auto reader2 = intMtx.shared_lock();
    auto reader3 = intMtx.shared_lock();

    REQUIRE(*reader1 == 0);
    REQUIRE(*reader2 == 0);
    REQUIRE(*reader3 == 0);

    UNLOCK(reader1);
    UNLOCK(reader2);
    UNLOCK(reader3);
    // writer gets unblocked

    writerThread.join();

    auto readerFinal = intMtx.shared_lock();
    REQUIRE(*readerFinal == 1);
}

TEST_CASE("[locked_execution/locked_execution_shared] many readers can share ownership, writer gets blocked",
          "[CapybotUtils/Mutex]")
{
    capybot::mutex<int> intMtx(0);

    auto reader1 = intMtx.shared_lock(); // owns mutex

    auto writerThread = std::thread([&intMtx] {
        intMtx.locked_execution<void>([](int& val) { val = 1; }); // blocked until all readers release
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // no deadlock because of shared ownership
    REQUIRE(intMtx.locked_execution_shared<int>([](const int& val) -> int { return val; }) == 0);
    REQUIRE(*reader1 == 0);

    UNLOCK(reader1);
    // writer gets unblocked

    writerThread.join();

    REQUIRE(intMtx.locked_execution_shared<int>([](const int& val) -> int { return val; }) == 1);
}

TEST_CASE("[try_lock/try_lock_shared] many readers can share ownership, writer gets blocked", "[CapybotUtils/Mutex]")
{
    capybot::mutex<int> intMtx(0);

    auto reader1Opt = intMtx.try_lock_shared(); // owns mutex shared
    REQUIRE(reader1Opt.has_value());
    auto& reader1 = reader1Opt.value();

    auto reader2Opt = intMtx.try_lock_shared(); // owns mutex shared
    REQUIRE(reader2Opt.has_value());
    auto& reader2 = reader2Opt.value();

    auto writer1Opt = intMtx.try_lock(); // fails
    REQUIRE(!writer1Opt.has_value());

    REQUIRE(*reader1 == 0);
    REQUIRE(*reader2 == 0);

    UNLOCK(reader1);
    UNLOCK(reader2);

    auto writer2Opt = intMtx.try_lock(); // owns mutex exclusively
    REQUIRE(writer2Opt.has_value());
    auto& writer2 = writer2Opt.value();
    *writer2 = 1;

    auto reader3Opt = intMtx.try_lock_shared(); // fails
    REQUIRE(!reader3Opt.has_value());

    UNLOCK(writer2);

    auto readerFinal = intMtx.shared_lock();
    REQUIRE(*readerFinal == 1);
}

TEST_CASE("[lock] a single writer can have ownership", "[CapybotUtils/Mutex]")
{
    capybot::mutex<int> intMtx(0);

    auto writer1 = intMtx.lock(); // owns mutex

    auto writerThread = std::thread([&intMtx] {
        auto writer2 = intMtx.lock(); // blockeed until all readers release
        *writer2 = 1;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    REQUIRE(*writer1 == 0);
    UNLOCK(writer1);
    // writer2 gets unblocked

    writerThread.join();

    auto readerFinal = intMtx.shared_lock();
    REQUIRE(*readerFinal == 1);
}

TEST_CASE("[locked_execution] a single writer can have ownership", "[CapybotUtils/Mutex]")
{
    capybot::mutex<int> intMtx(0);

    auto writer1 = intMtx.lock(); // owns mutex

    auto writerThread = std::thread([&intMtx] {
        intMtx.locked_execution<void>([](int& val) { val = 1; }); // blocked until all readers release
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    REQUIRE(*writer1 == 0);
    UNLOCK(writer1);
    // writer2 gets unblocked

    writerThread.join();

    auto readerFinal = intMtx.shared_lock();
    REQUIRE(*readerFinal == 1);
}
