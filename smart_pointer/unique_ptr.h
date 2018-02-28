#ifndef UNIQUE_PTR_H
#define UNIQUE_PTR_H
#include <type_traits>
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

}
#endif // UNIQUE_PTR_H
