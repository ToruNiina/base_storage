#ifndef PSEUDO_STATIC_BASE_STORAGE_HPP
#define PSEUDO_STATIC_BASE_STORAGE_HPP
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <memory>
#include <stdexcept>
#include <array>
#include <cassert>
#include <iostream>

namespace psd
{
namespace detail
{
enum class replication_scheme : bool
{
    copy,
    move,
};
template<typename T>
static void replicator(const replication_scheme rpl, void* ptr1, void* ptr2)
{
    // copy/move ptr1 <- ptr2
    assert(ptr1);
    assert(ptr2);
    switch(rpl)
    {
        case replication_scheme::copy:
        {
            new(ptr1) T(*reinterpret_cast<T*>(ptr2));
            return;
        }
        case replication_scheme::move:
        {
            new(ptr1) T(std::move(*reinterpret_cast<T*>(ptr2)));
            return;
        }
        default:
        {
            throw std::logic_error("psd::base_storage::detail::replicator: "
                    "never reach here");
        }
    }
}

template<typename T>
using unwrapped_t =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
} // detail

template<typename Base, std::size_t Size>
class base_storage
{
  public:
    using base_type            = Base;
    using base_pointer         = base_type*;
    using const_base_pointer   = base_type const*;
    using base_reference       = base_type&;
    using const_base_reference = base_type const&;
    using replicator_ptr       =
        void(*)(const detail::replication_scheme, void*, void*);

    constexpr static std::size_t capacity = Size;
    using storage_type = typename std::aligned_storage<capacity>::type;

  public:
    base_storage(): replicator_(nullptr){}
    ~base_storage() noexcept {this->reset();}

    base_storage(base_storage const&);
    base_storage(base_storage&&);
    base_storage& operator=(base_storage const&);
    base_storage& operator=(base_storage&&);

    template<typename T, typename std::enable_if<std::is_base_of<base_type,
        detail::unwrapped_t<T>>::value, std::nullptr_t>::type = nullptr>
    base_storage(T&&);
    template<typename T, typename std::enable_if<std::is_base_of<base_type,
        detail::unwrapped_t<T>>::value, std::nullptr_t>::type = nullptr>
    base_storage& operator=(T&&);

    void reset() noexcept;
    void swap(base_storage&) noexcept;
    bool has_value() const noexcept {return this->replicator_ != nullptr;}
    std::type_info const& type() const noexcept;

    template<typename T, typename ...Ts, typename std::enable_if<
        std::is_base_of<base_type, T>::value, std::nullptr_t>::type = nullptr>
    typename std::decay<T>::type& emplace(Ts&& ... vs);

    base_pointer       base_ptr()       noexcept
    {return reinterpret_cast<base_pointer>(std::addressof(this->storage_));}
    const_base_pointer base_ptr() const noexcept
    {return reinterpret_cast<const_base_pointer>(std::addressof(this->storage_));}

    constexpr std::size_t max_size() const noexcept {return capacity;}

  private:
    replicator_ptr replicator_;
    storage_type   storage_;
};

template<typename Base, std::size_t Size>
base_storage<Base, Size>::base_storage(base_storage const& rhs)
{
    if(rhs.has_value())
    {
        this->replicator_ = rhs.replicator_;
        this->replicator_(detail::replication_scheme::copy,
            std::addressof(this->storage_), std::addressof(rhs.storage_));
    }
    else
    {
        this->replicator_ = nullptr;
    }
}

template<typename Base, std::size_t Size>
base_storage<Base, Size>::base_storage(base_storage&& rhs)
{
    if(rhs.has_value())
    {
        this->replicator_ = rhs.replicator_;
        this->replicator_(detail::replication_scheme::move,
            std::addressof(this->storage_), std::addressof(rhs.storage_));
    }
    else
    {
        this->replicator_ = nullptr;
    }
}

template<typename Base, std::size_t Size>
base_storage<Base, Size>&
base_storage<Base, Size>::operator=(base_storage const& rhs)
{
    this->reset();
    if(rhs.has_value())
    {
        this->replicator_ = rhs.replicator_;
        this->replicator_(detail::replication_scheme::copy,
            std::addressof(this->storage_), std::addressof(rhs.storage_));
    }
    else
    {
        this->replicator_ = nullptr;
    }
    return *this;
}

template<typename Base, std::size_t Size>
base_storage<Base, Size>&
base_storage<Base, Size>::operator=(base_storage&& rhs)
{
    this->reset();
    if(rhs.has_value())
    {
        this->replicator_ = rhs.replicator_;
        this->replicator_(detail::replication_scheme::move,
            std::addressof(this->storage_), std::addressof(rhs.storage_));
    }
    else
    {
        this->replicator_ = nullptr;
    }
    return *this;
}

template<typename Base, std::size_t Size>
template<typename T, typename std::enable_if<
    std::is_base_of<Base, detail::unwrapped_t<T>>::value, std::nullptr_t>::type>
inline base_storage<Base, Size>::base_storage(T&& rhs)
{
    using rhs_t = detail::unwrapped_t<T>;
    static_assert(sizeof(rhs_t) <= capacity, "base_storage: size exceed");

    this->replicator_ = &detail::replicator<rhs_t>;
    new(std::addressof(this->storage_)) rhs_t(std::forward<T>(rhs));
}

template<typename Base, std::size_t Size>
template<typename T, typename std::enable_if<
    std::is_base_of<Base, detail::unwrapped_t<T>>::value, std::nullptr_t>::type>
inline base_storage<Base, Size>& base_storage<Base, Size>::operator=(T&& rhs)
{
    using rhs_t = detail::unwrapped_t<T>;
    static_assert(sizeof(rhs_t) <= capacity, "base_storage: size exceed");

    this->reset();
    this->replicator_ = &detail::replicator<rhs_t>;
    new(std::addressof(this->storage_)) rhs_t(std::forward<T>(rhs));
    return *this;
}

template<typename Base, std::size_t Size>
void base_storage<Base, Size>::reset() noexcept
{
    if(this->has_value())
    {
        this->base_ptr()->~base_type();
        this->replicator_ = nullptr;
    }
    return;
}

template<typename Base, std::size_t Size>
void base_storage<Base, Size>::swap(base_storage& rhs) noexcept
{
    using std::swap;
    swap(this->replicator_, rhs.replicator_);
    swap(this->storage_,    rhs.storage_);
    return;
}

template<typename Base, std::size_t Size>
inline std::type_info const& base_storage<Base, Size>::type() const noexcept
{
    return typeid(*(this->base_ptr()));
}

template<typename Base, std::size_t Size>
template<typename T, typename ... Ts, typename std::enable_if<
    std::is_base_of<Base, T>::value, std::nullptr_t>::type>
typename std::decay<T>::type& base_storage<Base, Size>::emplace(Ts&& ... vs)
{
    using rhs_t = detail::unwrapped_t<T>;
    static_assert(sizeof(rhs_t) <= capacity, "base_storage: size exceed");
    this->reset();
    this->replicator_ = &detail::replicator<T>;
    new(std::addressof(this->storage_)) T(std::forward<Ts>(vs)...);
    return *reinterpret_cast<rhs_t*>(std::addressof(this->storage_));
}


} // psd
#endif// STATIC_BASE_STORAGE_HPP
