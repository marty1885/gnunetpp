#pragma once
#include <cppcoro/generator.hpp>
#include <cppcoro/task.hpp>
#include <cppcoro/async_generator.hpp>

#include <optional>

namespace gnunetpp
{

// Fires a coroutine and doesn't force waiting nor deallocates upon promise destructs
// NOTE: AsyncTask is designed to be not awaitable. And kills the entire process
// if exception escaped.
struct AsyncTask
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    AsyncTask() = default;

    AsyncTask(handle_type h) : coro_(h)
    {
    }

    AsyncTask(const AsyncTask &) = delete;
    AsyncTask(AsyncTask &&other)
    {
        coro_ = other.coro_;
        other.coro_ = nullptr;
    }

    AsyncTask &operator=(const AsyncTask &) = delete;
    AsyncTask &operator=(AsyncTask &&other)
    {
        if (std::addressof(other) == this)
            return *this;

        coro_ = other.coro_;
        other.coro_ = nullptr;
        return *this;
    }

    struct promise_type
    {
        std::coroutine_handle<> continuation_;

        AsyncTask get_return_object() noexcept
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() const noexcept
        {
            return {};
        }

        void unhandled_exception()
        {
            std::terminate();
        }

        void return_void() noexcept
        {
        }

        void setContinuation(std::coroutine_handle<> handle)
        {
            continuation_ = handle;
        }

        auto final_suspend() const noexcept
        {
            // Can't simply use suspend_never because we need symmetric transfer
            struct awaiter final
            {
                bool await_ready() const noexcept
                {
                    return true;
                }

                auto await_suspend(
                    std::coroutine_handle<promise_type> coro) const noexcept
                {
                    return coro.promise().continuation_;
                }

                void await_resume() const noexcept
                {
                }
            };
            return awaiter{};
        }
    };
    bool await_ready() const noexcept
    {
        return coro_.done();
    }

    void await_resume() const noexcept
    {
    }

    auto await_suspend(std::coroutine_handle<> coroutine) noexcept
    {
        coro_.promise().setContinuation(coroutine);
        return coro_;
    }

    handle_type coro_;
};

// Helper class that provides the infrastructure for turning callback into coroutines
// The user is responsible to fill in `await_suspend()` and constructors.
template <typename T = void>
struct CallbackAwaiter
{
    bool await_ready() noexcept
    {
        return false;
    }

    const T &await_resume() const noexcept(false)
    {
        // await_resume() should always be called after co_await
        // (await_suspend()) is called. Therefore the value should always be set
        // (or there's an exception)
        assert(result_.has_value() == true || exception_ != nullptr);

        if (exception_)
            std::rethrow_exception(exception_);
        return result_.value();
    }

  protected:
    // HACK: Not all desired types are default constructable. But we need the
    // entire struct to be constructed for awaiting. std::optional takes care of
    // that.
    std::optional<T> result_;
    std::exception_ptr exception_{nullptr};

  protected:
    void setException(const std::exception_ptr &e)
    {
        exception_ = e;
    }
    void setValue(const T &v)
    {
        result_.emplace(v);
    }
    void setValue(T &&v)
    {
        result_.emplace(std::move(v));
    }
};

template <>
struct CallbackAwaiter<void>
{
    bool await_ready() noexcept
    {
        return false;
    }

    void await_resume() noexcept(false)
    {
        if (exception_)
            std::rethrow_exception(exception_);
    }

  protected:
    std::exception_ptr exception_{nullptr};

    void setException(const std::exception_ptr &e)
    {
        exception_ = e;
    }
};

template <typename T = void>
struct EagerAwaiter : public CallbackAwaiter<T>
{
    std::coroutine_handle<> handle_ = std::noop_coroutine();
    void await_suspend(std::coroutine_handle<> handle_) noexcept
    {
        this->handle_ = handle_;
        if(CallbackAwaiter<T>::result_.has_value() || CallbackAwaiter<T>::exception_ != nullptr)
            handle_.resume();
    }

    void setValue(const T &v)
    {
        CallbackAwaiter<T>::setValue(v);
        if(handle_ != std::noop_coroutine())
            handle_.resume();
    }

    void setValue(T &&v)
    {
        CallbackAwaiter<T>::setValue(std::move(v));
        if(handle_ != std::noop_coroutine())
            handle_.resume();
    }

    void setException(const std::exception_ptr &e)
    {
        CallbackAwaiter<T>::setException(e);
        if(handle_ != std::noop_coroutine())
            handle_.resume();
    }

    bool hasResult() const noexcept
    {
        return CallbackAwaiter<T>::result_.has_value();
    }
};

template <>
struct EagerAwaiter<void> : public CallbackAwaiter<>
{
    std::coroutine_handle<> handle_ = std::noop_coroutine();
    bool value_set = false;
    void await_suspend(std::coroutine_handle<> handle_) noexcept
    {
        this->handle_ = handle_;
        if(exception_ != nullptr || value_set)
            handle_.resume();
    }

    void setException(const std::exception_ptr &e)
    {
        CallbackAwaiter<>::setException(e);
        if(handle_ != std::noop_coroutine())
            handle_.resume();
    }

    void setValue()
    {
        value_set = true;
        if(handle_ != std::noop_coroutine())
            handle_.resume();
    }
    
    bool hasResult() const noexcept
    {
        return value_set;
    }
};

/**
 * @brief Runs a coroutine from a regular function
 * @param coro A coroutine that is awaitable
 */
template <typename Coro>
void async_run(Coro &&coro)
{
    using CoroValueType = std::decay_t<Coro>;
    auto functor = [](CoroValueType coro) -> AsyncTask {
        auto frame = coro();

        using FrameType = std::decay_t<decltype(frame)>;
        static_assert(cppcoro::is_awaitable_v<FrameType>);

        co_await frame;
        co_return;
    };
    functor(std::forward<Coro>(coro));
}

/**
 * @brief returns a function that calls a coroutine
 * @param Coro A coroutine that is awaitable
 */
template <typename Coro>
std::function<void()> async_func(Coro &&coro)
{
    return [coro = std::forward<Coro>(coro)]() mutable {
        async_run(std::move(coro));
    };
}

}