# base_storage

A storage class for a polymorphic object without heap allocation.

* __stricter__ than `any`
* __looser__ than `variant`
* __faster__ than `Base* ptr = new Derived();`

`base_storage<Base, Size>` is a hybrid class that has `variant`'s speed and `Base*`'s flexibility.
It contains a polymorphic objects in statically-allocated storage. It is capable of having any class if it is derived from the `Base` class and its size is equal to or lesser than the `Size`.

* It is faster than `any` and `Base*` because there is no dynamic memory allcoation.
* It is cache-friendly because of the same reason described above.
* It is more flexible than `variant` because you don't need to specify all the possible types (except the maximum size)

The main idea is inspired by [static_any](https://github.com/david-grs/static_any) by david-grs and [Boost.PolyCollection](http://www.boost.org/doc/libs/1_65_1/doc/html/poly_collection.html) by Joaquín M López Muñoz.

## Usage

```cpp
#include <path/to/base_storage.hpp>
#include <iostream>
#include <string>

struct Base
{
    virtual ~Base() noexcept = default;
    virtual void dosomething()
    {std::cout << "Base::dosomething()" << std::endl;}
};

struct Derived1 : public Base
{
    Derived1(int i_): i(i_){}
    virtual ~Derived1() noexcept override = default;
    virtual void dosomething() override
    {std::cout << "Derived1::dosomething(): i=" << i << std::endl;}

    int i;
};

struct Derived2 : public Base
{
    Derived2(std::string str): s(str){}
    virtual ~Derived2() noexcept override = default;
    virtual void dosomething() override
    {std::cout << "Derived2::dosomething(): s=" << s << std::endl;}

    std::string s;
};

struct BigDerived : public Base
{
    BigDerived(std::string str): s1(str), s2(str), s3(str){}
    virtual ~BigDerived() noexcept override = default;
    virtual void dosomething() override
    {std::cout << "BigDerived::dosomething(): s1=" << s1 << std::endl;}

    std::string s1, s2, s3;
};

int main()
{
    constexpr static std::size_t max_size =
        psd::max_size<Base, Derived1, Derived2>::value;

    typedef psd::base_storage<Base, max_size> storage_type;

    {
        storage_type storage(Derived1(42));// it can contain Derived object
        storage.base_ptr()->dosomething(); // Derived1::dosomething(): i=42
        storage = Derived2("foo");         // you can substitute different type
        storage.base_ptr()->dosomething(); // Derived2::dosomething(): s=foo
    }

    {
        storage_type storage;
        storage.emplace<Derived1>(42);     // you can emplace Derived object
        storage.base_ptr()->dosomething(); // Derived1::dosomething(): i=42
        std::cout << std::boolalpha;
        std::cout << storage.has_value() << std::endl; // true
        storage.reset(); // you can explicitly delete the stuff contained
        std::cout << storage.has_value() << std::endl; // false
    }

//    compilation error! the size exceeds the capacity of storage_type.
//    storage_type storage(BigDerived("bar"));

    {
        storage_type storage(Derived1(42));
        Derived1& drv1 = psd::base_storage_cast<Derived1>(storage);
        drv1.i = 96;
        storage.base_ptr()->dosomething(); // Derived1::dosomething(): i=96

        try
        {
            Derived2& drv2 = psd::base_storage_cast<Derived2>(storage);
            storage.base_ptr->dosomething();
        }
        catch(psd::bad_base_storage_cast const& bbsc)
        {
            // bad_base_storage_cast: 8Derived1 -> 8Derived2
            std::cerr << bbsc.what() << std::endl;
        }
    }

    return 0;
}
```

## Synopsis

All the stuffs are in the namespace `psd`.

```cpp
namespace psd
{

template<typename Base, std::size_t Size>
class base_storage
{
  public:
    using base_type            = Base;
    using base_pointer         = base_type*;
    using const_base_pointer   = base_type const*;
    using base_reference       = base_type&;
    using const_base_reference = base_type const&;

    constexpr static std::size_t capacity = Size;
    using storage_type = typename std::aligned_storage<capacity>::type;

  public:

    base_storage()  noexcept;
    ~base_storage() noexcept;

    base_storage(base_storage const&);
    base_storage(base_storage&&);
    base_storage& operator=(base_storage const&);
    base_storage& operator=(base_storage&&);

    template<typename T, /* enabled if is_base_of<base_type, T>::value */>
    base_storage(T&&);
    template<typename T, /* enabled if is_base_of<base_type, T>::value */>
    base_storage& operator=(T&&);

    void reset()             noexcept;
    void swap(base_storage&) noexcept;
    bool has_value()   const noexcept;
    std::type_info const& type() const noexcept;

    template<typename T, typename ... Ts,
             /* enabled if is_base_of<base_type, T>::value */>
    typename std::decay<T>::type& emplace(Ts&& ... vs);

    base_pointer       base_ptr()       noexcept;
    const_base_pointer base_ptr() const noexcept;

    constexpr std::size_t max_size() const noexcept {return capacity;}
};

// exception class

struct bad_base_storage_cast : public std::bad_cast
{
    // ctor/dtor/operator= ...
    virtual const char* what() const override;
};

// non-member functions

template<typename Base, std::size_t Size>
void swap(base_storage<Base, Size>&, base_storage<Base, Size>&) noexcept;

template<typename ValueType, typename Base, std::size_t Size>
ValueType const& base_storage_cast(const base_storage<Base, Size>&);
template<typename ValueType, typename Base, std::size_t Size>
ValueType& base_storage_cast(base_storage<Base, Size>&);
template<typename ValueType, typename Base, std::size_t Size>
ValueType&& base_storage_cast(base_storage<Base, Size>&&);

template<typename ValueType, typename Base, std::size_t Size>
const ValueType* base_storage_cast(const base_storage<Base, Size>*) noexcept;
template<typename ValueType, typename Base, std::size_t Size>
ValueType* base_storage_cast(base_storage<Base, Size>*) noexcept;

template<typename Base, std::size_t Size, typename ... Ts>
base_storage<Base, Size> make_base_storage(Ts&& ... args);

// helper struct : max_size

template<typename ... Ts>
struct max_size:
std::integral_constant<std::size_t, /*maximum value in {sizeof(Ts) ...}*/>{};

} // psd
```

## Licensing Terms

This product is licensed under the terms of the [MIT License](LICENSE).

- Copyright (c) 2017 Toru Niina

All rights reserved.
