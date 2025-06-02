#include <algorithm>
#include <memory>
#include <memory_resource>
#include <print>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

// I'm a fool that i can't impl conditional-explicit

// I HATE YOU, ALLOCATOR

//  爱你就等于爱自己~

// todo a variant without args(get the metadata of FirstT) : ok!
template <class FirstT, class... Tail>
// expect that ignored_tailt,ignored_ft do not participate the evaluation
// ignored_tailt and ignored_ft are virtual
constexpr decltype(auto) at_variadic_template_metainfo(unsigned int indx, auto call_back_if_get, FirstT *ignored_ft,
                                                       Tail *...ignored_tailt)
{

    if (indx > sizeof...(Tail))
    {
        if consteval
        {
            static_assert(false,"idx out of range!");
        }
        throw std::range_error("indx out of range");
    }
    if (indx == 0)
    {
        // I'm the genius! deducing out FirstT with nullptr!
        return call_back_if_get(ignored_ft);
    }
    return at_variadic_template_metainfo<Tail...>(indx - 1, call_back_if_get, ignored_tailt...);
}

template <class FirstT>
constexpr decltype(auto) at_variadic_template_metainfo(unsigned int indx, auto call_back_if_get, FirstT *ignored_ft)
{

    if (indx > 0)
    {
        if consteval
        {
            static_assert(false,"idx out of range!");
        }
        throw std::range_error("indx out of range");
    }
    return call_back_if_get(ignored_ft);
}

template <class First, class... Tail>
constexpr decltype(auto) at_variadic_template(unsigned int idx, auto call_back_if_get, First &&first, Tail &&...tail)
{
    // todo : a reload resolution using C++26 pack index
    if (idx > sizeof...(Tail))
    {
        throw std::range_error("idx out of range");
    }
    if (idx == 0)
    {
        return call_back_if_get(std::forward<First>(first));
    }
    return at_variadic_template(idx - 1, call_back_if_get, std::forward<Tail>(tail)...);
}

template <class First>
constexpr decltype(auto) at_variadic_template(unsigned int idx, auto call_back_if_get, First &&first)
{
    if (idx > 0)
    {
        throw std::range_error("idx out of range");
    }
    return call_back_if_get(std::forward<First>(first));
}

/**
 * Description:
 *
 *
 *     __ElemT(in elements_manager) == FirstT(in recursion) == ElemT(in my_tuple)
 *
 *
 *     std-lib compatible alias:
 *
 *         ElemT : Ti, aka(ElemT)
 *
 *         T : a sequence of Ti,aka(ElemT...)
 *
 *         sizeof...(T): range of i(0,sizeof...(T)
 *
 *
 *     Restriction of T:
 *
 *         This instantiation has some restriction [see following contents for detail]:
 *
 *             for ElemT : must satisfy std::is_trivial_destructible and type(self) is std::trivial_destructible
 *
 *             for ElemT : must satisfy std::is_move_assignable|std::is_copy_assignable(due to allocator feature
 *
 *             for default-noarg-constructor: ElemT must satisfy std::is_default_constructible
 *
 *             for sizeof...(ElemT) is non-zero
 *
 *
 *    Terrible implementation of std::tuple, just stores a copy of ElemTs,and assign them
 *    No lrreference
 */
template <class... ElemT> // ElemT == T  UTypes == U
struct my_tuple
{
public:
    static_assert((std::is_trivially_destructible_v<std::remove_cvref_t<ElemT>> && ...));
    static_assert(((std::is_copy_assignable_v<std::remove_cvref_t<ElemT>> |
                    std::is_move_assignable_v<std::remove_cvref_t<ElemT>>) &&
                   ...));
    using _allocator = std::allocator<std::byte>;
    // 让我们一起做梦 不再一样尽情说着!
    // 执意去抱着孤单 破灭无力一起失落!
    // 我跟你一起冒险 不离不弃一起说过!
    // 你无情岁月到来为何总是把你错过!
    //todo : custom allocator support and std::polymorphic_allocator specialization

    /**
     *
     * To using this default construct func, ElemT must satisfy std::is_trivial_constructible
     *
     * inner template type of __allocator type MUST be std::byte
     *
     * type of the actual storage is std::remove_cvref_t<__ElemT>
     *
     */

    template <class __allocator = _allocator, class... __ElemT> struct elements_manager
    {
        static_assert((std::is_trivially_destructible_v<std::remove_cvref_t<__ElemT>> && ...));
        // Why do not use std::unique_ptr ?? for more detailed memory control
        __allocator allocator;
        std::vector<std::pair<std::byte *, size_t>> original_elements;
        bool is_init;

        // todo : initialize per __ElemT and put them into a sequence container which consists of "ptr,size",... : ok
        // todo : manipulation interfaces for __ELemT
        void initialize_memory_without_arg()
        {
            this->is_init = true;
            static_assert(((std::is_default_constructible_v<std::remove_cvref_t<ElemT>>) && ...));

            for (unsigned int idx : std::ranges::views::iota(0, (int)(sizeof...(__ElemT))))
            {
                // Do not use allocator, because of deprecated-function(allocator::construct)
                std::byte *_base_ptr = nullptr;

                auto _count = at_variadic_template_metainfo(
                    idx, [&]<class FirstT>(FirstT *) constexpr { return sizeof(std::remove_cvref_t<FirstT>); },
                    reinterpret_cast<__ElemT *>((int *)nullptr)...);

                at_variadic_template_metainfo<__ElemT...>(
                    idx,
                    // I love lambda
                    // I have to deduce explicitly type,due to initialize:
                    [&]<class FirstT>(FirstT *) mutable {
                        _base_ptr = std::allocator_traits<__allocator>::allocate(this->allocator, _count);
                        // No support for std::remove_cvref_t<FirstT>(); so, I have to deduce type explicitly to make
                        // FirstT precisely
                        _base_ptr = FirstT();
                        // default construct
                        std::println("[DEBUG]: elements_manager: allocated bytes: {}", _count);
                        // else just filled-with 0
                        return;
                    },
                    reinterpret_cast<std::remove_cvref_t<__ElemT> *>((int *)nullptr)...);
                this->original_elements.emplace_back(std::make_pair(_base_ptr, _count));
            }
        }

        ~elements_manager()
        {
            std::println("[DEBUG] elements_manager: untrivial destructor");
            this->destroy();
        }

        void initialize_memory(__ElemT &&...__elements)
        {
            this->is_init = true;
            for (unsigned int idx : std::ranges::views::iota(0, (int)(sizeof...(__ElemT))))
            {
                // Why didn't use remove_cvref_t(__ElemT)... in here? let the compiler deduce the type to facilitate
                // forwarding argument for elements!
                at_variadic_template(
                    idx,
                    [&]<class FirstT>(FirstT &&first) mutable {
#if __cplusplus >= 202302L
                        // THIS STATE IS FOR ALLOCATOR, CONFLICT WITH CONSTRUCT INITIALIZE BEGIN
                        auto [_base_ptr, _count] = std::allocator_traits<__allocator>::allocate_at_least(
                            this->allocator,
                            // Why didn't use remove_cvref_t(__ElemT)... type deducing in here? let the compiler deduce
                            // the type to facilitate forwarding argument for elements!
                            at_variadic_template(
                                idx,
                                [&]<class _FirstT>(_FirstT &&first) constexpr {
                                    return sizeof(std::remove_cvref_t<_FirstT>);
                                },
                                std::forward<__ElemT>(__elements)...));
#else
                        size_t _count = at_variadic_template(
                            idx,
                            [&]<class FirstT>(FirstT &&first) constexpr { return sizeof(std::remove_cvref_t<FirstT>); },
                            std::forward<__ElemT>(__elements)...);
                        decltype(auto) _base_ptr = std::allocator_traits<__allocator>::allocate(this->allocator, _count);
#endif

                        std::println("[DEBUG]: elements_manager: allocated bytes: {}", _count);
                        if constexpr (std::is_move_assignable_v<std::remove_cvref_t<FirstT>> &&
                                      std::is_rvalue_reference_v<decltype(std::forward<FirstT>(first))>)
                        {
                            std::println("[DEBUG]: elements_manager :move assignment");
                            *reinterpret_cast<std::remove_cvref_t<FirstT> *>(_base_ptr) = std::forward<FirstT>(first);
                        }
                        else if constexpr (std::is_copy_assignable_v<std::remove_cvref_t<FirstT>>)
                        {
                            std::println("[DEBUG]: elements_manager: copy assignment");
                            (*reinterpret_cast<std::remove_cvref_t<FirstT> *>(_base_ptr)) = std::forward<FirstT>(first);
                        }
                        // Fuck! there's no allocator.construct!

                        // else:else:else: directly return, no assignment
                        this->original_elements.emplace_back(std::make_pair(_base_ptr, _count));
                        return;
                        // THIS STATE IS FOR ALLOCATOR, CONFLICT WITH CONSTRUCT INITIALIZE END
                    },
                    std::forward<__ElemT>(__elements)...);
            }
        }

        template <size_t idx> decltype(auto) _get_ptr()
        {
            return this->is_init
                       ? std::make_pair(this->original_elements.at(idx).first, this->original_elements.at(idx).second)
                       : std::make_pair((std::byte *)nullptr, 0ul);
        }
        void destroy()
        {
            if (this->is_init)
            {
                size_t idx = 0;
                for (auto elements_information : this->original_elements)
                {
                    at_variadic_template_metainfo(
                        idx,
                        [&]<class FirstT>(FirstT *) {
                            (*reinterpret_cast<std::remove_cvref_t<FirstT> *>(elements_information.first)).~FirstT();
                            std::allocator_traits<__allocator>::deallocate(this->allocator, elements_information.first,
                                                                           elements_information.second);
                        },
                        reinterpret_cast<std::remove_cvref_t<__ElemT> *>((int *)nullptr)...);

                    idx++;
                }
            }
            return;
        }
    };

    elements_manager<_allocator, ElemT...> elemgr;

  public:
    // todo : provides encapsulation of "elements_manager" and some interfaces about <elements_manipulation>

    constexpr my_tuple()
    {
        // This overload participates in overload resolution only if std::is_default_constructible<Ti>::value is true
        // for all i.
        // to guarantee that the __ElemT(Ti) is default_constructible
        // As the same as default-constructor in elements_manager
        static_assert(sizeof...(ElemT) != 0);
        static_assert(((std::is_default_constructible_v<std::remove_cvref_t<ElemT>>) && ...));
        this->elemgr.initialize_memory_without_arg();
    };

    constexpr my_tuple(const ElemT &...args)
    {
        static_assert(sizeof...(ElemT) != 0);
        // This overload participates in overload resolution only if sizeof...(Types) >= 1 and
        // std::is_copy_constructible<Ti>::value is true for all i. I didn't implement it. this is personal
        // implementation
        static_assert(((std::is_copy_assignable_v<std::remove_cvref_t<ElemT>> |
                        std::is_move_assignable_v<std::remove_cvref_t<ElemT>>) &&
                       ...));
        this->elemgr.initialize_memory(std::forward<ElemT>(args)...);
    };

    template <class... UTypes> constexpr my_tuple(UTypes &&...args)
    {
        static_assert(sizeof...(ElemT) != 0);
        static_assert(sizeof...(ElemT) == sizeof...(UTypes));
        //                                                   The actual storing type
        static_assert(
            (std::is_assignable_v<std::remove_cvref_t<ElemT> &, decltype(std::forward<UTypes>(args))> && ...));
        // Why std::remove_cvref_t<ElemT>& ?
        // Actually,the elemgr only stores sizeof(std::remove_cvref_t<ElemT>) bytes for each elems in internal, so we
        // only need to take care about the remove_cvref_t<ElemT>.
        //     The program will pass the UType to assignment in elemgr.initialize_memory finally
        // This constructor is defined as deleted if the initialization of any element that is a reference would bind it
        // to a temporary object.
        // I won't act it
        this->elemgr.initialize_memory(std::forward<UTypes>(args)...);
    };

    // hack : do not use this func!!
    template <class... UTypes> constexpr my_tuple(const my_tuple<UTypes...> &other)
    {
        static_assert(sizeof...(ElemT) == sizeof...(UTypes));
        static_assert(false);
    };

#if __cplusplus >= 202302L
    // since C++ 23
    // Hack : do not use this func
    template <class... UTypes> constexpr my_tuple(my_tuple<UTypes...> &&other)
    {
        static_assert(sizeof...(ElemT) == sizeof...(UTypes));
        static_assert(false);
    };
#endif

    my_tuple(const my_tuple &other) = default;

    my_tuple(my_tuple &&other) = default;

    /**
       template< class U1, class U2 > constexpr tuple( std::pair<U1, U2>& p );

       template< class U1, class U2 > tuple( const std::pair<U1, U2>& p );

       template< class U1, class U2 > tuple( std::pair<U1, U2>&& p );

       won't implement (temporarily)
     */
    ~my_tuple() = default;

    template <size_t idx> decltype(auto) _get_ptr()
    {
        return this->elemgr._get_ptr<idx>();
    }
};

//   爱是我所要表达 一股汹涌的冲动~

// todo: partial specialization for ElemTs which are not trivial_destructible
// ill-formed while using partial specialization? i won't obey it!
class demonstration01
{
  public:
    int a;
    demonstration01() = delete;

    demonstration01(const demonstration01 &other) = delete;
    demonstration01(const demonstration01 &&other) = delete;
    demonstration01(int b)
    {
        a = b;
    }
    void operator=(demonstration01 &other)
    {
        this->a = other.a;
        std::println("[DEBUG]: custom copy assignment");
    }

    void operator=(demonstration01 &&other)
    {
        this->a = other.a;
        std::println("[DEBUG]: custom move assignment");
    }

    ~demonstration01() = default;
};

// I love you,CTAD
// CTAD你是真屌啊！
template <typename... ElemT> my_tuple(ElemT &&...) -> my_tuple<ElemT...>;

template <typename... ElemT> my_tuple(ElemT &...) -> my_tuple<ElemT...>;

int main()
{
    int a = 10;
    my_tuple tuple(1, 2, 1);
    return 0;
}
