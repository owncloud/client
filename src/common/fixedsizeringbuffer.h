/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QtGlobal>

#include <array>
#include <functional>
#include <vector>

namespace OCC {

/**
 * A Fixed sized ring buffer optimized on continouous insertion
 */
template <typename TYPE>
class FixedSizeRingBuffer
{
public:
    FixedSizeRingBuffer(size_t size)
        : _data(size, TYPE())
    {
    }

    constexpr size_t capacity() const
    {
        return _data.size();
    }

    constexpr bool isFull() const
    {
        return size() >= _data.size();
    }

    constexpr size_t size() const
    {
        return _end - _start;
    }

    constexpr bool empty() const
    {
        return size() == 0;
    }

    // iterators pointing to the raw data, the range might be smaller than _Size
    // if ever needed we could implement a iterator for the data window
    auto begin()
    {
        return _data.begin();
    }

    auto end()
    {
        return isFull() ? _data.end() : _data.begin() + size();
    }

    auto cbegin() const
    {
        return _data.cbegin();
    }

    auto cend() const
    {
        return isFull() ? _data.cend() : _data.cbegin() + size();
    }

    void pop_front()
    {
        // move the windows without changing the underlying data
        _start++;
        Q_ASSERT(_start < _end);
        // adjust offset to prevent overflow
        const auto s = size();
        _start %= _data.size();
        _end = _start + s;
    }

    void push_back(TYPE &&data)
    {
        Q_ASSERT(!isFull());
        _data[convertToIndex(size())] = data;
        _end++;
    }

    const TYPE &at(size_t index) const
    {
        return _data.at(convertToIndex(index));
    }

    /*
     * Remove items if f returns true
     * The filtered result is unordered
     */
    void remove_if(const std::function<bool(const TYPE &)> &f)
    {
        // filter and sort the data
        FixedSizeRingBuffer<TYPE> tmp(_data.size());
        const auto start = convertToIndex(0);
        for (auto it = begin() + start; it != end(); ++it) {
            if (!f(*it)) {
                tmp.push_back(std::move(*it));
            }
        }
        for (auto it = begin(); it != begin() + start; ++it) {
            if (!f(*it)) {
                tmp.push_back(std::move(*it));
            }
        }
        *this = std::move(tmp);
    }

    void reset(std::vector<TYPE> &&data)
    {
        Q_ASSERT(data.size() <= _data.size());
        _start = 0;
        _end = data.size();
        std::move(data.begin(), data.end(), _data.begin());
    }

private:
    std::vector<TYPE> _data;

    // the sliding window of the ring buffer
    size_t _start = 0;
    size_t _end = 0;

    // converts an array index to the underlying array index
    constexpr size_t convertToIndex(size_t i) const
    {
        return (_start + i) % _data.size();
    }
};

}
