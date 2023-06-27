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

#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <shared_mutex>

#define __NODISCARD                                                                                                    \
    [[nodiscard(                                                                                                       \
        "No point in creating a scoped handle if not using it. The wrapped object will be immediately unlocked.")]]

namespace capybot
{
template <typename RESOURCE_T>
class mutex
{
public:
    template <typename LOCKED_RESOURCE_T, template <typename> typename LOCKER_T>
    class ScopedHandle
    {
    public:
        LOCKED_RESOURCE_T* operator->() const noexcept { return dataPtr; }
        LOCKED_RESOURCE_T& operator*() const noexcept { return *dataPtr; }

    private:
        LOCKED_RESOURCE_T* dataPtr;
        LOCKER_T<std::shared_mutex> lock;

        explicit ScopedHandle(const mutex& m)
            : dataPtr(std::addressof(m.val))
            , lock(m.mut)
        {
        }
        explicit ScopedHandle(const mutex& m, std::adopt_lock_t)
            : dataPtr(std::addressof(m.val))
            , lock(m.mut, std::adopt_lock)
        {
        }
        explicit ScopedHandle(mutex& m)
            : dataPtr(std::addressof(m.val))
            , lock(m.mut)
        {
        }
        explicit ScopedHandle(mutex& m, std::adopt_lock_t)
            : dataPtr(std::addressof(m.val))
            , lock(m.mut, std::adopt_lock)
        {
        }

        friend mutex;
    };

    /// @brief mutable handle for exclusive ownership. Usually used for writing operations.
    using Handle = ScopedHandle<RESOURCE_T, std::unique_lock>;

    /// @brief const handle for shared ownership. Usually used for reading operations.
    using ConstHandle = ScopedHandle<const RESOURCE_T, std::shared_lock>;

    /**
     * @brief create mutex element
     *
     * @tparam Args arguments for RESOURCE_T element's constructor
     */
    template <typename... Args, std::enable_if_t<std::is_constructible_v<RESOURCE_T, Args...>, int> = 0>
    constexpr explicit mutex(Args&&... args)
        : val(std::forward<Args>(args)...)
    {
    }

    mutex(const mutex&) = delete;
    mutex(mutex&&) = delete;
    mutex& operator=(const mutex&) = delete;
    mutex& operator=(mutex&&) = delete;

    /// @brief lock mutex and create a const (read-only) object handle
    __NODISCARD ConstHandle shared_lock() const { return ConstHandle{*this}; }

    /// @brief lock mutex and create a mutable object handle with exclusive ownership
    __NODISCARD Handle lock() { return Handle{*this}; }

    /// @brief try to lock mutex and create a const (read-only) object handle
    __NODISCARD std::optional<ConstHandle> try_lock_shared() const
    {
        if (mut.try_lock_shared())
        {
            return ConstHandle{*this, std::adopt_lock};
        }
        return std::nullopt;
    }

    /// @brief try to lock mutex and create a mutable object handle with exclusive ownership
    __NODISCARD std::optional<Handle> try_lock()
    {
        if (mut.try_lock())
        {
            return Handle{*this, std::adopt_lock};
        }
        return std::nullopt;
    }

    /// @brief lock mutex (shared / read-only) and execute call back on wrapped object
    template <typename RETURN_T>
    RETURN_T locked_execution_shared(std::function<RETURN_T(RESOURCE_T const&)> const&& callback) const
    {
        std::shared_lock<std::shared_mutex> lock(mut);
        return callback(val);
    }

    /// @brief lock mutex (unique / exclusive ownership) and execute call back on wrapped object
    template <typename RETURN_T>
    RETURN_T locked_execution(std::function<RETURN_T(RESOURCE_T&)> const&& callback)
    {
        std::unique_lock<std::shared_mutex> lock(mut);
        return callback(val);
    }

private:
    mutable std::shared_mutex mut;
    RESOURCE_T val;
};
} // namespace capybot