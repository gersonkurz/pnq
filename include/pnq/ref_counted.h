#pragma once

#include <atomic>
#include <cstdint>
#include <vector>
#include <initializer_list>

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

#define PNQ_ADDREF(p) do { if (p) (p)->retain(REFCOUNT_DEBUG_ARGS); } while(0)
#define PNQ_RELEASE(p) do { if (p) (p)->release(REFCOUNT_DEBUG_ARGS); } while(0)

namespace pnq
{
    /// Vector that automatically manages ref-counted pointers.
    /// Releases all elements on destruction, retains on copy.
    /// @tparam T pointer type to a class derived from IRefCounted
    template <typename T> class RefCountedVector final
    {
    public:
        RefCountedVector() = default;

        RefCountedVector(std::initializer_list<T> init)
            : m_items(init)
        {
            for (auto p : m_items)
                PNQ_ADDREF(p);
        }

        ~RefCountedVector()
        {
            for (auto p : m_items)
                PNQ_RELEASE(p);
        }

        RefCountedVector(const RefCountedVector& other)
            : m_items(other.m_items)
        {
            for (auto p : m_items)
                PNQ_ADDREF(p);
        }

        RefCountedVector& operator=(const RefCountedVector& other)
        {
            if (this != &other)
            {
                for (auto p : m_items)
                    PNQ_RELEASE(p);
                m_items = other.m_items;
                for (auto p : m_items)
                    PNQ_ADDREF(p);
            }
            return *this;
        }

        RefCountedVector(RefCountedVector&& other) noexcept
            : m_items(std::move(other.m_items))
        {
        }

        RefCountedVector& operator=(RefCountedVector&& other) noexcept
        {
            if (this != &other)
            {
                for (auto p : m_items)
                    PNQ_RELEASE(p);
                m_items = std::move(other.m_items);
            }
            return *this;
        }

        /// Add element (retains it).
        void push_back(T p)
        {
            PNQ_ADDREF(p);
            m_items.push_back(p);
        }

        /// Remove last element (releases it).
        void pop_back()
        {
            if (!m_items.empty())
            {
                PNQ_RELEASE(m_items.back());
                m_items.pop_back();
            }
        }

        /// Clear all elements (releases each).
        void clear()
        {
            for (auto p : m_items)
                PNQ_RELEASE(p);
            m_items.clear();
        }

        /// Access element by index (no bounds check).
        T operator[](size_t i) const { return m_items[i]; }

        /// Access element by index with bounds check.
        T at(size_t i) const { return m_items.at(i); }

        size_t size() const { return m_items.size(); }
        bool empty() const { return m_items.empty(); }

        auto begin() { return m_items.begin(); }
        auto end() { return m_items.end(); }
        auto begin() const { return m_items.begin(); }
        auto end() const { return m_items.end(); }

    private:
        std::vector<T> m_items;
    };
} // namespace pnq
