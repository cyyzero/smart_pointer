#ifndef UNIQUE_PTR_H
#define UNIQUE_PTR_H

#include <type_traits>
#include <tuple>
namespace cyy
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

    template<typename T, typename Deleter = default_delete<T>>
    class unique_ptr
    {
    public:
        using pointer = T*;
        using element_type = T;
        using deleter_type = Deleter;

        constexpr unique_ptr() noexcept
            :M_t()
        {
            static_assert(!std::is_pointer<deleter_type>::value,
                          "constructed with null function pointer deleter");
        }

        explicit
        unique_ptr(pointer p) noexcept
            :M_t(p, deleter_type())
        {
            static_assert(!std::is_pointer<deleter_type>::value,
                          "constructed with null function pointer deleter");
        }

        unique_ptr(pointer p,
                   typename conditional<std::is_reference<deleter_type>::value,
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
        {

        }

     private:
        std::tuple<pointer, Deleter> M_t;

    }
}
#endif // UNIQUE_PTR_H
