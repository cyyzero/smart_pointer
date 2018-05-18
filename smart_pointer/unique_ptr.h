#ifndef UNIQUE_PTR_H
#define UNIQUE_PTR_H

#include <type_traits>
#include <tuple>
#include <functional>
namespace sm_ptr
{
    // Primary template of default_delete, used by unique_ptr
    template<typename Tp>
    class default_delete
    {
    public:
        constexpr default_delete() noexcept = default;

        /*
        * Allows conversion from a deleter for arrays of another type
        * only if Up* is convertible to @p Tp*.
        */
        template<typename Up, typename = typename
            std::enable_if<std::is_convertible<Up*, Tp*>::value>::type>
        default_delete(const default_delete<Up>&) noexcept { }

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
    class default_delete<Tp[]>
    {
    public:
        constexpr default_delete() noexcept = default;

        /* Since C++17
        * Constructs a std::default_delete<U[]> object from another std::default_delete<U[]> object. 
        * This constructor will only participate in overload resolution if U(*)[] is implicitly convertible to T(*)[].
        */
        template <typename Up, typename = typename
            std::enable_if<std::is_convertible<Up(*)[], Tp(*)[]>::value>::type>
        default_delete(const default_delete<Up[]> &) noexcept { }

        template <typename Up, typename = typename
            std::enable_if<std::is_convertible<Up(*)[], Tp(*)[]>::value>::type>
        void operator()(Up* ptr) const
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
            using _Del = typename std::remove_reference<Deleter>::type;

            template<typename U>
            static typename U::pointer __test(typename U::pointer*);

            template<typename U>
            static T* __test(...);

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

        constexpr unique_ptr(std::nullptr_t) noexcept
            :_M_t()
        {
            static_assert(!std::is_pointer<deleter_type>::value,
                          "constructed with null function pointer deleter");
            static_assert(!std::is_reference<deleter_type>::value,
                          "can't constructed with a deleter of reference type");
        }

        explicit
        unique_ptr(pointer p) noexcept
            :_M_t(p, deleter_type())
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
            :_M_t(p, d) { }

        unique_ptr(pointer p, typename std::remove_reference<deleter_type>::type&& d) noexcept
            :_M_t(std::move(p), std::move(d))
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
                    typename = typename std::enable_if<
                        std::is_convertible<typename unique_ptr<U, E>::pointer, pointer>::value &&
                        !std::is_array<U>::value &&
                        std::conditional<
                            std::is_reference<Deleter>::value,
                            std::is_same<E, Deleter>,
                            std::is_convertible<E, Deleter>
                        >::type::value
                    >::type
                >
        unique_ptr(unique_ptr<U, E>&& u) noexcept
            :_M_t(u.release(), std::forward<E>(u.get_deleter())) { };


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
        template<typename U, typename E,
                    typename = typename std::enable_if<
                        !std::is_array<U>::value &&
                        std::is_convertible<typename unique_ptr<U, E>::pointer, pointer>::value
                    >::type
                >
        unique_ptr& operator=(unique_ptr<U,E>&& r) noexcept
        {
            reset(r.release());
            get_deleter() = std::forward<deleter_type>(r.get_deleter());
            return *this;
        }

        unique_ptr& operator=(std::nullptr_t) noexcept
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
    };

    // Unique_ptr for array
    template <typename T, typename Deleter>
    class unique_ptr<T[], Deleter>
    {
    private:
        // Use SFINAE to determine whether std::remove_reference<Deleter>::type::pointer exits.
        class _Pointer
        {
        private:
            template<typename U>
            static typename U::Pointer __test(typename U::Pointer *);

            template<typename U>
            static T* __test(...);

            using _Del = typename std::remove_reference<Deleter>::type;
        public:
            using type = decltype(__test<_Del>(0));
        };

        using __tuple_type = std::tuple<typename _Pointer::type, Deleter>;
        __tuple_type _M_t;

    public:
        using pointer      = typename _Pointer::type;
        using element_type = T;
        using deleter_type = Deleter;

        // Constructors


        constexpr unique_ptr() noexcept
            :_M_t()
        {
            static_assert(!std::is_pointer<Deleter>::value,
                          "constructed with null function pointer deleter");
            static_assert(!std::is_reference<Deleter>::value,
                           "constructed with reference deleter");
        }

        constexpr unique_ptr(std::nullptr_t) noexcept
            :_M_t()
        {
            static_assert(!std::is_pointer<Deleter>::value,
                          "constructed with null function pointer deleter");
            static_assert(!std::is_reference<Deleter>::value,
                           "constructed with reference deleter");
        }

        explicit
        unique_ptr(pointer p) noexcept
            :_M_t(p, deleter_type())
        {
            static_assert(!std::is_pointer<Deleter>::value,
                          "constructed with null function pointer deleter");
            static_assert(!std::is_reference<Deleter>::value,
                           "constructed with reference deleter");
        }

        unique_ptr(pointer p,
            typename std::conditional<
                std::is_reference<deleter_type>::value,
                deleter_type,
                const deleter_type&>::type d) noexcept
        :_M_t(p, d) { }

        unique_ptr(pointer p,
            typename std::remove_reference<deleter_type>::type&& d) noexcept
        :_M_t(std::move(p), std::move(d)) 
        {
            static_assert(!std::is_reference<deleter_type>::value,
		        "rvalue deleter bound to reference");
        }

        unique_ptr(unique_ptr&& u) noexcept
            :_M_t(u.release(), std::forward<deleter_type>(u.get_deleter())) { }
        /*
        * In the specialization for arrays behaves the same as in the primary template, except that it will only participate in overload resolution if all of the following is true
        *   U is an array type
        *   pointer is the same type as element_type*
        *   unique_ptr<U,E>::pointer is the same type as unique_ptr<U,E>::element_type*
        *   unique_ptr<U,E>::element_type(*)[] is convertible to element_type(*)[]
        *   either Deleter is a reference type and E is the same type as Deleter, or Deleter is not a reference type and E is implicitly convertible to Deleter. 
        */
        template<typename U, typename E,
            typename std::enable_if<
                std::is_array<U>::value &&
                std::is_same<pointer, element_type*>::value &&
                std::is_same<typename unique_ptr<U, E>::pointer, typename unique_ptr<U, E>::element_type*>::value &&
                std::is_convertible<typename unique_ptr<U, E>::element_type(*)[], element_type(*)[]>::value &&
                std::conditional<
                    std::is_reference<Deleter>::value,
                    std::is_same<E, Deleter>,
                    std::is_convertible<E, Deleter>
                >::type::value
            >::type
        >
        unique_ptr(unique_ptr<U, E>&& u) noexcept
            :_M_t(u.release(), std::forward<E>(u.get_deleter())) { }

        // Destructor
        ~unique_ptr()
        {
            auto &ptr = std::get<0>(_M_t);
            if (ptr != nullptr)
            {
                get_deleter()(ptr);
            }
            ptr = pointer();
        }

        // Asignment

        unique_ptr& operator=(unique_ptr && r) noexcept
        {
            reset(r.release());
            get_deleter() = std::forward<deleter_type>(r.get_deleter());
            return *this;
        }

        unique_ptr& operator=(std::nullptr_t) noexcept
        {
            reset();
            return *this;
        }

        // Since C++17
        template<typename U, typename E,
            typename std::enable_if<
                std::is_array<U>::value &&
                std::is_same<pointer, element_type*>::value &&
                std::is_same<typename unique_ptr<U, E>::pointer, typename unique_ptr<U, E>::element_type*>::value &&
                std::is_same<typename unique_ptr<U, E>::element_type(*)[], element_type(*)[]>::value
            >::type
        >
        unique_ptr& operator=(unique_ptr<U, E>&& r) noexcept
        {
            reset(r.release());
            get_deleter() = std::forward<E>(r.get_deleter());
        }

        // Observer

        /// Return the stored pointer
        pointer get() const noexcept
        {
            return std::get<0>(_M_t);
        }

        /// return the stored deleter
        deleter_type& get_deleter() noexcept
        {
            return std::get<1>(_M_t);
        }

        const deleter_type& get_deleter() const noexcept
        {
            return std::get<1>(_M_t);
        }

        /// Return true if the stored pointer is not null
        explicit operator bool() const
        {
            return get() != pointer();
        }

        /// Access an element of own array
        typename std::add_lvalue_reference<element_type>::type
        operator[](std::size_t i) const
        {
            return get()[i];
        }

        // Modifiers

        // Release ownship of stored pointer
        pointer release() noexcept
        {
            pointer p = get();
            std::get<0>(_M_t) = pointer();
            return p;
        }

        // Replace the stpred pointer.
        void reset(pointer p = pointer()) noexcept
        {
            using std::swap;
            swap(std::get<0>(_M_t), p);
            if (p != pointer())
                get_deleter()(p);
        }

        // Disable resetting from convertible pointer types.
        template <typename U, typename = 
            typename std::enable_if<
                std::is_pointer<pointer>::value && 
                std::is_convertible<U*, pointer>::value &&
                std::is_base_of<T, U>::value &&
                (!std::is_same<typename std::remove_cv<T>::type, typename std::remove_cv<U>::type>::value)
            >::type
        >
        void reset(U*) = delete;

        /// Exchange the pointer and deleter with another object
        void swap(unique_ptr& u) noexcept
        {
            using std::swap;
            swap(u._M_t, _M_t);
        }

        // Disable copy from lvalue.
        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(unique_ptr&) = delete;

        // Disable construction from convertible pointer types.
        template <typename U, typename = 
            typename std::enable_if<
                std::is_pointer<pointer>::value && 
                std::is_convertible<U*, pointer>::value &&
                std::is_base_of<T, U>::value &&
                (!std::is_same<typename std::remove_cv<T>::type, typename std::remove_cv<U>::type>::value)
            >::type
        >
        unique_ptr(U*, 
            typename std::conditional<
                std::is_reference<deleter_type>::value, 
                deleter_type,
                const deleter_type&
            >::type) = delete;

        // Disable construction from convertible pointer types.
        template <typename U, typename = 
            typename std::enable_if<
                std::is_pointer<pointer>::value && 
                std::is_convertible<U*, pointer>::value &&
                std::is_base_of<T, U>::value &&
                (!std::is_same<typename std::remove_cv<T>::type, typename std::remove_cv<U>::type>::value)
            >::type
        >
        unique_ptr(U*,
            typename std::remove_reference<deleter_type>::type&&) = delete;
    };

    template<typename T, class Deleter>
    inline void swap(sm_ptr::unique_ptr<T, Deleter>& lhs, sm_ptr::unique_ptr<T, Deleter>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template <typename T1, typename D1, typename T2, typename D2>
    inline bool operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {
        return x.get() == y.get();
    }

    template <typename T1, typename D1, typename T2, typename D2>
    inline bool operator!=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {
        return x.get() != y.get();
    }

    template<typename T1, typename D1, typename T2, typename D2>
    inline bool operator<(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {
        using CT = typename std::common_type<typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>::type;
        return std::less<CT>()(x.get(), y.get());
    }

    template<class T1, class D1, class T2, class D2>
    inline bool operator<=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {
        return !(y < x);
    }

    template<class T1, class D1, class T2, class D2>
    inline bool operator>(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {
        return y < x;
    }

    template<class T1, class D1, class T2, class D2>
    inline bool operator>=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y)
    {
        return !(x < y);
    }

    template <class T, class D>
    inline bool operator==(const unique_ptr<T, D>& x, std::nullptr_t) noexcept
    {
        return !x;
    }

    template <class T, class D>
    inline bool operator==(std::nullptr_t, const unique_ptr<T, D>& x) noexcept
    {
        return !x;
    }

    template <class T, class D>
    inline bool operator!=(const unique_ptr<T, D>& x, std::nullptr_t) noexcept
    {
        return (bool)x;
    }

    template <class T, class D>
    inline bool operator!=( std::nullptr_t, const unique_ptr<T, D>& x) noexcept
    {
        return (bool)x;
    }

    template <class T, class D>
    inline bool operator<(const unique_ptr<T, D>& x, std::nullptr_t)
    {
        return std::less<typename unique_ptr<T, D>::pointer>()(x.get(), nullptr);
    }

    template <class T, class D>
    inline bool operator<(std::nullptr_t, const unique_ptr<T, D>& y)
    {
        return std::less<typename unique_ptr<T, D>::pointer>()(nullptr, y.get());
    }

    template <class T, class D>
    inline bool operator<=(const unique_ptr<T, D>& x, std::nullptr_t)
    {
        return !(nullptr < x);
    }

    template <class T, class D>
    inline bool operator<=(std::nullptr_t, const unique_ptr<T, D>& y)
    {
        return !(y < nullptr);
    }

    template <class T, class D>
    inline bool operator>(const unique_ptr<T, D>& x, std::nullptr_t)
    {
        return nullptr < x;
    }

    template <class T, class D>
    inline bool operator>(std::nullptr_t, const unique_ptr<T, D>& y)
    {
        return y < nullptr;
    }

    template <class T, class D>
    inline bool operator>=(const unique_ptr<T, D>& x, std::nullptr_t)
    {
        return !(x < nullptr);
    }

    template <class T, class D>
    inline bool operator>=(std::nullptr_t, const unique_ptr<T, D>& y)
    {
        return !(nullptr < y);
    }


    template <typename T>
    struct _MakeUniq
    {
        using __signle_object = unique_ptr<T>;
    };

    template<typename T>
    struct _MakeUniq<T[]>
    {
        using __array = unique_ptr<T[]>;
    };

    template<typename T, std::size_t bound>
    struct _MakeUniq<T[bound]>
    {
        struct __invalid_type{ };
    };

    // make_unique for signle object
    template <typename T, typename ... Args>
    inline typename _MakeUniq<T>::__signle_object
    make_unique(Args&& ... args)
    {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    // make_unique for array of unknown bound
    template<typename T>
    inline typename _MakeUniq<T>::__array
    make_unique(std::size_t size)
    {
        return unique_ptr<T>(new typename std::remove_extent<T>::type[size]());
    }

    // make_unique for array with known bound is deleted
    template<typename T, typename ... Args>
    inline typename _MakeUniq<T>::__invalid_type
    make_unique(Args&& ... args) = delete;

    // Hash for unique_ptr
    struct unique_ptr_hash
    {
        template<typename T, typename Deleter>
        std::size_t operator()(const unique_ptr<T, Deleter>& u) const
        {
            return std::hash<typename unique_ptr<T, Deleter>::pointer>()(u.get());
        }
    };
}

#endif // UNIQUE_PTR_H
