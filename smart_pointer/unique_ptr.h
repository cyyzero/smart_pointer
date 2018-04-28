#ifndef UNIQUE_PTR_H
#define UNIQUE_PTR_H

#include <type_traits>
#include <tuple>
namespace sm_ptr
{
    // Primary template of default_delete, used by unique_ptr
    template<typename Tp>
    struct default_delete
    {
        constexpr default_delete() noexcept = default;

        //Allows conversion from a deleter for arrays of another type
        template<typename Up, typename = typename enable_if<std::is_convertible<Up*, Tp*>::value>::type>
            default_delete(const default_delete<Up>&) noexcept {}

        void operator()(Tp* ptr) const
        {
            static_assert(!std::is_void<Tp>::value,
                          "can't delete pointer to incomplete type");
            static_assert(sizeof(Tp) > 0,
                          "can't delete pointer to incomplete type");
            delete ptr;
        }
    };

    // Specialization for arrays, default_delete.
    template<typename Tp>
    struct default_delete<Tp[]>
    {
    public:
        constexpr default_delete() noexcept = default;

        operator()(Tp* ptr) const
        {
            static_assert(sizeof(Tp) > 0,
                          "can't delete pointer to incomplete type");
            delete [] ptr;
        }
    };

    // Unique_ptr for single object
    template<typename T, typename Deleter = default_delete<T>>
    class unique_ptr
    {
    private:
        // Use SINFAE to determine whether std::remove_reference<Deleter>::type::pointer exits
        class _Pointer
        {
        private:
            using _Del = std::remove_reference<Deleter>::type;

            template<U>
            U::pointer __test(typename U::pointer*);

            template<U>
            T* __test(...);

        public:
            using type = decltype(__test<_Del>(0));
        };

        using tuple_type = std::tuple<typename _Pointer::type, Deleter>;
        tuple_type _M_t;

    public:
        using pointer      = typename _Pointer::type;
        using element_type = T;
        using deleter_type = Deleter;

        //  Constructors

        /// Default ctr
        constexpr unique_ptr() noexcept
            :_M_t()
        {
            static_assert(!std::is_pointer<deleter_type>::value,
                          "constructed with null function pointer deleter");
            static_assert(!std::is_reference<deleter_type>::value,
                          "can't constructed with a deleter of reference type");
        }

        constexpr unique_ptr(nullptr_t) noexcept
            :_M_t()
        {
            static_assert(!std::is_pointer<deleter_type>::value,
                          "constructed with null function pointer deleter");
            static_assert(!std::is_reference<deleter_type>::value,
                          "can't constructed with a deleter of reference type");
        }

        explicit
        unique_ptr(pointer p) noexcept
            :M_t(p, deleter_type())
        {
            static_assert(!std::is_pointer<deleter_type>::value,
                          "constructed with null function pointer deleter");
            static_assert(!std::is_reference<deleter_type>::value,
                          "can't constructed with a deleter of reference type");
        }

        unique_ptr(pointer p,
                   typename std::conditional<std::is_reference<deleter_type>::value,
                                             deleter_type,
                                             const deleter_type&>::type d) noexcept
            : M_t(p, d) { }

        unique_ptr(pointer p, typename std::remove_reference<delete_type>::type&& d) noexcept
            : M_t(std::move(p), std::move(d))
        {
            static_assert(!std::is_reference<deleter_type>::value,
                          "rvalue deleter bound to reference");
        }

        // move ctor
        unique_ptr(unique_ptr &&u) noexcept
            :_M_t(u.release(), std::forward<deleter_type>(u.get_deleter())) { }


        /*
        *This constructor only participates in overload resolution if all of the following is true:
        *  a) unique_ptr<U, E>::pointer is implicitly convertible to pointer
        *  b) U is not an array type
        *  c) Either Deleter is a reference type and E is the same type as D, or Deleter is not a reference type and E is implicitly convertible to D
        */
        template<typename U, typename E,
                    typename = std::enable_if<
                        std::is_convertible<typename unique_ptr<U, E>::pointer, pointer>::value &&
                        !std::is_array<U>::value &&
                        typename std::conditional<
                            std::is_reference<Deleter>::value,
                            std::is_same<E, D>,
                            std::is_convertible<E, D>::type
                        >::type::value
                    >::type
                >
        unique_ptr(unique_ptr<U, E>&& u) noexcept
            :_M_t(u.release(), std::forward<E>(u.get_deleter()));


        // Destructor
        ~unique_ptr() noexcept
        {
            auto &ptr = std::get<0>(_M_t);
            if (ptr != nullptr)
                get_deleter()(ptr);
        }

        // Assignment

        unique_ptr& operator=(unique_ptr&& r) noexcept
        {
            reset(r.release());
            get_deleter() = std::forward<deleter_type>(r.get_deleter());
            return *this;
        }

        // It requires that U is not an array type and unique_ptr<U,E>::pointer is implicitly convertible to pointer
        template<typename U, typename E
                typename = std::enable_if<
                    !std::is_array<U>::value &&
                    std::is_convertible<unique_ptr<U, E>::pointer, pointer>
                    >::type
                >
        unique_ptr& operator=( unique_ptr<U,E>&& r ) noexcept
        {
            reset(r.release());
            get_deleter() = std::forward<deleter_type>(r.get_deleter());
            return *this;
        }

        unique_ptr& operator=(nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        // Observers

        /// Dereference the stored pointer
        typename std::add_lvalue_reference<T>::type operator*() const
        {
            return *get();
        }

        /// Return the stored pointer
        pointer operator->() const noexcept
        {
            return get();
        }

        /// Return the stored pointer
        pointer get() const noexcept
        {
            return std::get<0>(_M_t);
        }

        /// Return a reference to the stored deleter
        Deleter& get_deleter() noexcept
        {
            return std::get<1>(_M_t);
        }

        const Deleter& get_deleter() const noexcept
        {
            return std::get<1>(_M_t);
        }

        /// Return true if the stored pointer is not null
        explicit
        operator bool() const noexcept
        {
            return get() != pointer();
        }

        // Modifiors

        /// Release ownership of any stored pointer
        pointer release() noexcept
        {
            pointer ret = get();
            std::get<0>(_M_t) = pointer();
            return ret;
        }

        /// Release the ownership of an old pointer and own a new one
        void reset(pointer ptr = pointer()) noexcept
        {
            using std::swap;
            swap(std::get<0>(_M_t), ptr);
            if (ptr != pointer())
                get_deleter()(ptr);
        }

        /// Exchange the pointer and deleter with another object
        void swap(unique_ptr& other) noexcept
        {
            using std::swap;
            swap(_M_t, other._M_t);
        }

        /// Disable copy from lvalue
        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;
    }
}
#endif // UNIQUE_PTR_H
