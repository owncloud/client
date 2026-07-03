/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "asserts.h"

namespace OCC {

/**
 * A Result of type T, or an Error
 */
template <typename T, typename Error>
class Result
{
    union {
        T _result;
        Error _error;
    };
    bool _isError;

public:
    Result(T value)
        : _result(std::move(value))
        , _isError(false)
    {
        static_assert(!std::is_same<T, bool>::value, "Bool is not supported as this class overrides the bool operator");
    }
    // TODO: This doesn't work if T and Error are too similar
    Result(Error error)
        : _error(std::move(error))
        , _isError(true)
    {
    }

    Result(Result &&other)
        : _isError(other._isError)
    {
        if (_isError) {
            new (&_error) Error(std::move(other._error));
        } else {
            new (&_result) T(std::move(other._result));
        }
    }

    Result(const Result &other)
        : _isError(other._isError)
    {
        if (_isError) {
            new (&_error) Error(other._error);
        } else {
            new (&_result) T(other._result);
        }
    }

    Result &operator=(Result &&other)
    {
        if (&other != this) {
            _isError = other._isError;
            if (_isError) {
                new (&_error) Error(std::move(other._error));
            } else {
                new (&_result) T(std::move(other._result));
            }
        }
        return *this;
    }

    Result &operator=(const Result &other)
    {
        if (&other != this) {
            _isError = other._isError;
            if (_isError) {
                new (&_error) Error(other._error);
            } else {
                new (&_result) T(other._result);
            }
        }
        return *this;
    }

    ~Result()
    {
        if (_isError)
            _error.~Error();
        else
            _result.~T();
    }

    explicit operator bool() const { return !_isError; }

    const T &operator*() const &
    {
        OC_ASSERT(!_isError);
        return _result;
    }

    T operator*() &&
    {
        OC_ASSERT(!_isError);
        return std::move(_result);
    }

    const T *operator->() const
    {
        OC_ASSERT(!_isError);
        return &_result;
    }

    const T &get() const
    {
        OC_ASSERT(!_isError);
        return _result;
    }

    const Error &error() const &
    {
        OC_ASSERT(_isError);
        return _error;
    }
    Error error() &&
    {
        OC_ASSERT(_isError);
        return std::move(_error);
    }
};

namespace detail {
    struct NoResultData{};
}

template <typename Error>
class Result<void, Error> : public Result<detail::NoResultData, Error>
{
public:
    using Result<detail::NoResultData, Error>::Result;
    Result() : Result(detail::NoResultData{}) {}
};

namespace detail {
struct OptionalNoErrorData{};
}

template <typename T>
class Optional : public Result<T, detail::OptionalNoErrorData>
{
public:
    using Result<T, detail::OptionalNoErrorData>::Result;

    Optional()
        : Optional(detail::OptionalNoErrorData{})
    {
    }
};

} // namespace OCC
