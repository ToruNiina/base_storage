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
