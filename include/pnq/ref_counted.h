#pragma once

#include <atomic>
#include <cstdint>

#undef USE_REFCOUNT_DEBUGGING
#ifdef USE_REFCOUNT_DEBUGGING
#define REFCOUNT_DEBUG_ARGS __FILE__, __LINE__
#define REFCOUNT_DEBUG_SPEC const char *file, int line
#else
#define REFCOUNT_DEBUG_ARGS
#define REFCOUNT_DEBUG_SPEC
#endif

namespace pnq
{
    /// Interface for reference-counted objects.
    /// Provides COM-style AddRef/Release semantics with thread-safe reference counting.
    struct IRefCounted
    {
        IRefCounted() = default;
        virtual ~IRefCounted() = default;

        IRefCounted(const IRefCounted &) = delete;
        IRefCounted &operator=(const IRefCounted &) = delete;
        IRefCounted(IRefCounted &&) = default;
        IRefCounted &operator=(IRefCounted &&) = default;

        /// Increment the reference count (thread-safe).
        virtual void retain(REFCOUNT_DEBUG_SPEC) const = 0;

        /// Decrement the reference count (thread-safe).
        /// Object is deleted when count reaches zero.
        virtual void release(REFCOUNT_DEBUG_SPEC) const = 0;

        /// Reset internal state without affecting reference count.
        virtual void clear() = 0;
    };

    /// Mixin template that adds reference counting to a base class.
    /// @tparam T base class that inherits from IRefCounted
    template <typename T> class RefCounted : public T
    {
    public:
        RefCounted()
            : m_refCount{1}
        {
        }

        void retain(REFCOUNT_DEBUG_SPEC) const override final
        {
            ++m_refCount;
        }

        void release(REFCOUNT_DEBUG_SPEC) const override final
        {
            if (--m_refCount == 0)
                delete this;
        }

        void clear() override
        {
        }

    private:
        mutable std::atomic<int> m_refCount;
    };

    /// Concrete base class with reference counting built-in.
    /// Derive from this when you don't need the mixin pattern.
    class RefCountImpl : public IRefCounted
    {
    public:
        RefCountImpl()
            : m_refCount{1}
        {
        }

        void retain(REFCOUNT_DEBUG_SPEC) const override final
        {
            ++m_refCount;
        }

        void release(REFCOUNT_DEBUG_SPEC) const override final
        {
            if (--m_refCount == 0)
                delete this;
        }

        void clear() override
        {
        }

    private:
        mutable std::atomic<int> m_refCount;
    };
} // namespace pnq
