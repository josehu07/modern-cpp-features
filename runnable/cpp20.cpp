#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <coroutine>
#include <span>
#include <bit>
#include <numbers>
#include <numeric>
#include <cstdlib>
#include <ctime>
#include "utils.hpp"

////////////////
// Coroutines //
////////////////

// I certainly cannot understand the point of having coroutines -- it seems everything it
// could do can be rather easily implemented with lower-level mechanisms. Is it just a
// standardized syntax sugar for user-managed heap objects that contain a promise?
// Someone please save me...
template <typename T>
class Generator
{
public:
    // must supply a `promise_type` implementation for this class to be qualified as a
    // return type of a coroutine
    struct promise_type
    {
        using Handle = std::coroutine_handle<promise_type>;
        Generator<T> get_return_object() { return Generator(Handle::from_promise(*this)); }

        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T value)
        {
            state = std::move(value);
            return {};
        }
        void await_transform() = delete; // disallow co_await
        [[noreturn]] static void unhandled_exception() { throw; }

        std::optional<T> state;
    };

    // constructors, etc.
    explicit Generator(promise_type::Handle handle) : handle(handle) {}
    ~Generator()
    {
        if (handle)
            handle.destroy();
    }
    // disable copy constructor and assignment
    Generator(const Generator &) = delete;
    Generator &operator=(const Generator &) = delete;
    // support move constructor and assignment
    Generator(Generator &&g) noexcept : handle(g.handle) { g.handle = {}; }
    Generator &operator=(Generator &&g) noexcept
    {
        if (this == &g)
            return *this;
        if (handle)
            handle.destroy();
        handle = g.handle;
        g.handle = {};
        return *this;
    }

    // user API implementation -- could have implemented an `Iter` subclass to support
    // range-based for loops, etc.
    std::optional<T> next()
    {
        if (!handle)
            return std::optional<T>{};
        handle.resume();
        auto state = handle.promise().state;
        if (handle.done())
            state.reset();
        return state;
    }

private:
    promise_type::Handle handle;
};

template <std::integral T>
Generator<T> range_gen(T start, const T end)
{
    while (start < end)
        co_yield start++; // execution suspends and resumes here
                          // `co_yield x;` == `co_await promise.yield_value(x);`
    // implicit co_return at the end of this func, where the associated state on
    // heap for this coroutine gets destroyed
    // co_return;
}

void test_coroutines()
{
    std::vector<int> vec;
    auto gen = range_gen(0, 5);
    // we did not support standard iterators yet and only uses an std::optional to store
    // the state
    while (auto n = gen.next())
        vec.push_back(n.value());
    ASSERT_EQ(vec, (std::vector<int>{0, 1, 2, 3, 4}));
}

//////////////
// Concepts //
//////////////

// concept can be a constexpr bool to constraint the type it accepts
template <typename T>
concept integral = std::is_integral_v<T>;
template <typename T>
concept signed_integral = integral<T> && std::is_signed_v<T>;

// some examples of enforcing concepts when defining generic templates -- for complete list of
// all syntactic forms, see doc
template <signed_integral T>
T func1(T v) { return v - 1; }

template <typename T>
    requires signed_integral<T>
T func2(T v)
{
    return v - 1;
}

decltype(auto) func3(signed_integral auto v) { return v - 1; }

void test_concepts_basic()
{
    signed_integral auto var = -1;
    auto lambda = [](signed_integral auto v)
    { return v - 1; };
    ASSERT_EQ(var, -1);
    ASSERT_EQ(func1(var), -2);
    ASSERT_EQ(func2(var), -2);
    ASSERT_EQ(func3(var), -2);
    ASSERT_EQ(lambda(var), -2);
}

// a more interesting usage is to constraint the set of interface functions the type must
// support, making concepts much like Rust's traits
struct ObjA
{
    using ValueType = int;
    ValueType value;
    ValueType increment() { return ++value; }
};

template <typename T>
concept incrementable = requires(T x) {
    typename T::ValueType; // T has a member typename ValueType
    sizeof(T) > 1;         // size of T greater than 1 byte
    // T has member function `increment()` that gives type ValueType
    {
        x.increment()
    } -> std::same_as<typename T::ValueType>;
};

void test_concepts_exprs()
{
    auto lambda = [](incrementable auto v)
    { return v.increment(); };
    ObjA a{0};
    int result = lambda(a);
    ASSERT_EQ(result, 1);
}

//////////////////////////////////////
// Range-based for loop initializer //
//////////////////////////////////////

void test_range_based_for_initializer()
{
    std::string s;
    for (auto v = std::vector{1, 2, 3}; auto &e : v)
        s += std::to_string(e);
    ASSERT_EQ(s, "123");
}

///////////////////////
// likely & unlikely //
///////////////////////

void test_likely_unlikely()
{
    // if branch
    std::srand(std::time(nullptr));
    int rv = std::rand();
    if (rv > 0) [[likely]]
    {
        ASSERT(rv > 0);
    }
    else
    {
        ASSERT_EQ(rv, 0);
    }
    // switch case
    switch (rv)
    {
    [[unlikely]] case 0:
        ASSERT_EQ(rv, 0);
        break;
    case 1:
        ASSERT_EQ(rv, 1);
        break;
    default:
        ASSERT(rv > 1);
    }
    // loop body
    while (rv == 0) [[unlikely]]
    {
        rv = std::rand();
    }
    ASSERT(rv > 0);
}

////////////////////
// explicit(bool) //
////////////////////

struct ObjB
{
    template <typename T>
    explicit(!std::is_integral_v<T>) ObjB(T) {}
    // explicit(true) means explicit
};

void test_explicit_ctor()
{
    ObjB b0{123};   // ok
    ObjB b1 = 123;  // ok, 123 is integral and turns off the explcit
    ObjB b2{"123"}; // ok
    // ObjB b3 = "123";     // error: must use explicit ctor
    (void)b0;
    (void)b1;
    (void)b2;
}

/////////////////////////
// Immediate functions //
/////////////////////////

consteval int sqr(int n)
{
    return n * n;
}

void test_consteval()
{
    // consteval function (called an immediate function) is essentially a constexpr
    // function that must take a constant at compile-time and does not implicitly fallback
    // to normal
    static_assert(sqr(10) == 100);
}

////////////////
// using enum //
////////////////

// requires GCC version 11+, turning off for now

// enum class Channel { RED, GREEN, BLUE, ALPHA };

// void test_using_enum() {
//     std::string s;
//     Channel c = Channel::RED;
//     switch (c) {
//         using enum Channel;
//         // then no need to write Channel::RED etc. in case
//         case RED:   s = "red";   break;
//         case GREEN: s = "green"; break;
//         case GREEN: s = "blue";  break;
//         case ALPHA: s = "alpha"; break;
//     }
//     ASSERT_EQ(s, "red");
// }

///////////////
// std::span //
///////////////

// a view into a container that further hides the pointer-length information; think of a span
// as a container of references
int set_zero_then_sum(std::span<int> span)
{
    if (!span.empty())
        span[0] = 0;
    int sum = 0;
    std::for_each(span.begin(), span.end(), [&](auto &v)
                  { sum += v; });
    return sum;
}

void test_std_span()
{
    std::vector<int> vec{1, 2, 3};
    int sum0 = set_zero_then_sum(vec);
    std::array<int, 5> arr{4, 5, 6, 7, 8};
    int sum1 = set_zero_then_sum(arr);
    ASSERT_EQ(sum0, 5);
    ASSERT_EQ(sum1, 26);
}

/////////////////
// Bit helpers //
/////////////////

void test_bit_helpers()
{
    auto count = std::popcount(0b1111'0100u);
    ASSERT_EQ(count, 5);
}

////////////////////
// Math constants //
////////////////////

void test_math_constants()
{
    ASSERT(std::numbers::pi > 3);
    ASSERT(std::numbers::e < 3);
}

////////////////////////////////
// std::is_constant_evaluated //
////////////////////////////////

constexpr bool is_compile_time()
{
    return std::is_constant_evaluated();
}

void test_std_is_constant_evaluated()
{
    constexpr bool b0 = is_compile_time();
    volatile bool b1 = is_compile_time();
    ASSERT(b0);
    ASSERT(!b1);
}

////////////////////////////////////
// String starts_with & ends_with //
////////////////////////////////////

void test_starts_ends_with()
{
    std::string s = "foobar";
    ASSERT(s.starts_with("foo"));
    ASSERT(!s.ends_with("baz"));
}

////////////////////////
// Map & set contains //
////////////////////////

void test_check_contains()
{
    // avoids writing the tedious "find and check against iterator end" pattern
    std::map<int, char> m{{1, 'a'}, {2, 'b'}};
    ASSERT(m.contains(2));
    ASSERT(!m.contains(7));
    std::set<int> s{1, 2, 3};
    ASSERT(s.contains(2));
    ASSERT(!s.contains(7));
}

///////////////////
// std::midpoint //
///////////////////

void test_std_midpoint()
{
    int mid = std::midpoint(1, 3); // safe from overflows
    ASSERT_EQ(mid, 2);
}

///////////////////
// std::to_array //
///////////////////

void test_std_to_array()
{
    auto arr = std::to_array("foo");
    ASSERT_EQ(arr.size(), 4);
    ASSERT_EQ(arr, (std::array<char, 4>{'f', 'o', 'o', '\0'}));
}

int main(int argc, char *argv[])
{
    std::cout << "C++20 features runnable tests:" << std::endl;

    RUN_EXAMPLE(test_coroutines);
    RUN_EXAMPLE(test_concepts_basic);
    RUN_EXAMPLE(test_concepts_exprs);
    RUN_EXAMPLE(test_range_based_for_initializer);
    RUN_EXAMPLE(test_likely_unlikely);
    RUN_EXAMPLE(test_explicit_ctor);
    RUN_EXAMPLE(test_consteval);
    // RUN_EXAMPLE(test_using_enum);
    RUN_EXAMPLE(test_std_span);
    RUN_EXAMPLE(test_bit_helpers);
    RUN_EXAMPLE(test_math_constants);
    RUN_EXAMPLE(test_std_is_constant_evaluated);
    RUN_EXAMPLE(test_starts_ends_with);
    RUN_EXAMPLE(test_check_contains);
    RUN_EXAMPLE(test_std_midpoint);
    RUN_EXAMPLE(test_std_to_array);

    return 0;
}
