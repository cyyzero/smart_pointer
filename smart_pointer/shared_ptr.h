#ifndef SHARED_PTR_H
#define SHARED_PTR_H

#include <type_traits>

namespace sm_ptr
{


template<typename T>
class __shared_ptr;


template<typename T>
class shared_ptr: public __shared_ptr<T>
{
    template<typename Ptr>
    using _Convertible = std::enable_if_t<std::is_convertible<Ptr, T*>::value>;
public:
    using element_type = T;

    constexpr shared_ptr() noexcept
        : __shared_ptr() { }

    shared_ptr(const shared_ptr&) noexcept = default;

    template<typename Tp>
    shared_ptr(Tp* p)
        : __shared_ptr<T>(p) { }

    template<typename Tp, typename Deleter>
    shared_ptr(Tp *p, Deleter d)
        : __shared_ptr<T>(p, d) { }

    template<typename Deleter>
    shared_ptr(std::nullptr_t p, Deleter d)
        : __shared_ptr<T>(p, d) { }

    template<typename Tp, typename Deleter, typename Alloc>
    shared_ptr(Tp* p, Deleter d, Alloc a)
        : __shared_ptr<T>(p, d, std::move(a)) { }

    template<typename Deleter, typename Alloc>
    shared_ptr(std::nullptr_t p, Deleter d, Alloc a)
        : __shared_ptr(p, d, std::move(a)) { }

    template<typename Tp>
    shared_ptr(const shared_ptr<Tp>& r, T* p) noexcept
        : __shared_ptr<T>(r, p) { }

    template<typename Tp, typename = _Convertible<Tp*>>
    shared_ptr(const shared_ptr<Tp>& r) noexcept
        : __shared_ptr<T>(r) { }

    shared_ptr(shared_ptr&& r) noexcept
        : __shared_ptr<T>(std::move(r)) { }

    template<typename Tp, typename = _Convertible<Tp*>>
    shared_ptr(shared_ptr<Tp>&& r) noexcept
        : __shared_ptr<T>(std::move(r)) { }

    template<typename Tp>
    shared_ptr(const weak_ptr<Tp>& r)
        : __shared_ptr<T>(r) { }

    template<typename Tp, typename Deleter, typename
             = _Convertible<typename unique_ptr<Tp, Deleter>::pointer>>
    shared_ptr(sm_ptr::unique_ptr<Tp, Deleter>&& r)
        : __shared_ptr<T>(std::move(r)) { }

    constexpr shared_ptr(std::nullptr_t) noexcept
        : shared_ptr() { };

    

    shared_ptr& operator=(const shared_ptr&) noexcept = default;

    template<typename Tp>
    shared_ptr& operator=(const shared_ptr<Tp>& r) noexcept
    {
        this->__shared_ptr<Tp>::operator=(r);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& r) noexcept
    {
        this->__shared_ptr<Tp>::operator=(std::move(r));
        return *this;
    }

    template<typename Tp, typename Deleter>
    shared_ptr& operator=(std::unique_ptr<Tp, Deleter>&& r)
    {
        this->__shared_ptr<Tp>::operator=(std::move(r));
        return *this;
    }

private:
    T* obj_ptr;
    
};

}

#endif