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

        unique_ptr(unique_ptr &&u) noexcept
            :_M_t(u.release(), std::forward<deleter_type>(u.get_deleter())) { }

        template <typename U, typename E>
        unique_ptr(unique_ptr<U, E>&& u) noexcept
        

    }
}
#endif // UNIQUE_PTR_H
