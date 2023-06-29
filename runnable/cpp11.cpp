#include <iostream>
#include <vector>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <exception>
#include <thread>
#include <mutex>
#include <chrono>
#include <type_traits>
#include <typeinfo>
#include <algorithm>
#include <numeric>
#include <future>
#include <cmath>
#include "utils.hpp"

////////////////////
// Move semantics //
////////////////////

void test_std_move()
{
    std::unique_ptr<std::vector<int>> p1(new std::vector<int>{1, 2, 3});
    std::unique_ptr<std::vector<int>> p2 = std::move(p1);
    // std::move(x) returns a "moved" rvalue
    // p2 thus takes over "ownership" without copying vector content
    // p1 is now unsafe to dereference
    ASSERT_EQ(*p2, (std::vector<int>{1, 2, 3}));
    // when p2 goes out of scope, deletes (frees) the object
}

struct ObjA
{
    std::string s;
    std::string last_op;
    // constructor:
    ObjA() = delete;
    ObjA(std::string arg) : s(arg)
    {
        last_op = "normal-construct";
    }
    // copy constructor & copy assignment operator:
    ObjA(const ObjA &o) : s(o.s)
    {
        last_op = "copy-construct";
    }
    ObjA &operator=(const ObjA &o)
    {
        last_op = "copy-assignment";
        s = o.s;
        return *this;
    }
    // move constructor & move assignment operator:
    ObjA(ObjA &&o) : s(std::move(o.s))
    {
        last_op = "move-construct";
    }
    ObjA &operator=(ObjA &&o)
    {
        last_op = "move-assignment";
        s = std::move(o.s);
        return *this;
    }
};

ObjA make_A_rvalue(ObjA a)
{
    return a;
}

void test_move_ctor_assign_op()
{
    ObjA a0("dummy0");        // normally constructed
    ObjA a1 = ObjA("dummy1"); // normally constructed
    ObjA a2 = a1;             // copy constructed
    ObjA a3("dummy3");
    a3 = a1;                                 // copy assignment
    ObjA a4 = make_A_rvalue(ObjA("dummy4")); // move constructed from rvalue temporary
    ObjA a5 = std::move(a1);                 // move constructed using std::move
    ObjA a6("dummy6");
    a6 = ObjA("temp2"); // move assignment from rvalue temporary
    ObjA a7("dummy7");
    a7 = std::move(a2); // move assignment using std::move
    ASSERT_EQ(a0.last_op, "normal-construct");
    ASSERT_EQ(a1.last_op, "normal-construct");
    ASSERT_EQ(a2.last_op, "copy-construct");
    ASSERT_EQ(a3.last_op, "copy-assignment");
    ASSERT_EQ(a4.last_op, "move-construct");
    ASSERT_EQ(a5.last_op, "move-construct");
    ASSERT_EQ(a6.last_op, "move-assignment");
    ASSERT_EQ(a7.last_op, "move-assignment");
}

////////////////////////////////////
// Rvalue references & forwarding //
////////////////////////////////////

std::string which_variant_called(int &x)
{
    return "lvalue-reference";
}

std::string which_variant_called(int &&x)
{
    return "rvalue-reference";
}

void test_rvalue_references()
{
    int x = 0;    // `x` is an lvalue of type `int`
    int &xl = x;  // `xl` is an lvalue of type `int&` (lvalue reference)
    int &&xr = 1; // `xr` is an lvalue of type `int&&` (rvalue reference)
    // cannot do `int&& xr = x` since `x` is an lvalue
    ASSERT_EQ(which_variant_called(x), "lvalue-reference");
    ASSERT_EQ(which_variant_called(xl), "lvalue-reference");
    ASSERT_EQ(which_variant_called(2), "rvalue-reference");
    ASSERT_EQ(which_variant_called(std::move(x)), "rvalue-reference");
    ASSERT_EQ(which_variant_called(xr), "lvalue-reference"); // attention
    ASSERT_EQ(which_variant_called(std::move(xr)), "rvalue-reference");
}

void test_forwarding_references()
{
    int x = 0;
    auto &&al = x; // `al` is an lvalue reference (`int&`) since we bind to lvalue `x`
    auto &&ar = 0; // `ar` is an rvalue reference (`int&&`) since we bind to rvalue 0
    // besides `auto`, `&&` also works on template typenames
    ASSERT_EQ(which_variant_called(al), "lvalue-reference");
    (void)ar;
}

template <typename T>
ObjA make_A_by_forwarding(T &&a)
{
    return ObjA{std::forward<T>(a)};
}

void test_std_forward()
{
    ObjA a0("dummy");
    ObjA a1 = make_A_by_forwarding(a0);
    ObjA a2 = make_A_by_forwarding(std::move(a0));
    ASSERT_EQ(a1.last_op, "copy-construct");
    ASSERT_EQ(a2.last_op, "move-construct");
}

////////////////////////
// Variadic templates //
////////////////////////

template <typename... T>
struct ObjB
{
    constexpr static int ntargs = sizeof...(T);
};

static_assert(ObjB<>::ntargs == 0, "incorrect ntargs");
static_assert(ObjB<char, int, long>::ntargs == 3, "incorrect ntargs");

template <size_t S, size_t... Args>
std::array<bool, S> create_bool_array()
{
    std::array<bool, S> b{};
    auto lambda = [](...) {};  // variadic lambda that takes a parameter pack as argument
    lambda(b[Args] = true...); // call it giving a parameter unpack -- a trick to avoid recursion
    return b;
}

void test_variadic_templates()
{
    auto b = create_bool_array<5, 0, 3>();
    ASSERT_EQ(b, (std::array<bool, 5>{true, false, false, true, false}));
}

///////////////////////
// Initializer lists //
///////////////////////

void test_initializer_lists()
{
    std::initializer_list<int> list = {1, 2, 3};
    // light-weight immutable replacement for vector in some cases
    int total = 0;
    for (const int &e : list)
        total += e;
    ASSERT_EQ(total, 6);
}

///////////////////////
// Static assertions //
///////////////////////

static_assert(sizeof(uint64_t) == 8, "invalid uint64_t size");

void test_static_asserts()
{
    constexpr int x = 0;
    constexpr int y = 1;
    static_assert(x != y, "x should not be equal to y"); // checked at compile time
    ASSERT(x != y);                                      // this one evaluates at runtime
}

////////////////////
// auto deduction //
////////////////////

template <typename X, typename Y>
auto deduce_return_type(X x, Y y) -> decltype(x + y)
{
    return x + y;
}

void test_auto_type()
{
    // syntax sugar, shorter than std::vector<int>::const_iterator
    std::vector<int> v{1, 2, 3};
    auto cit = v.cbegin();
    (void)cit;
    // deduced return type of function
    int n1 = deduce_return_type(-1, 7);
    double n2 = deduce_return_type(-1, 7.0);
    ASSERT_EQ(static_cast<double>(n1), n2);
    // used in making forwarding references
    int x = 0;
    auto &&al = x; // forwarding reference
    (void)al;
}

////////////////////////
// Lambda expressions //
////////////////////////

void test_lambda_expressions()
{
    int x = 1, y = 2;
    auto capture_nothing = []
    { int x = 0; return x; };
    auto capture_by_value = [=](int z)
    { return x + z; };
    auto capture_by_reference = [&](int z)
    { y = 3; return y + z; };
    auto capture_differently = [x, &y]
    { return x + y; };
    auto capture_mutable_arg = [x]() mutable
    { x = 7; return x; };
    ASSERT_EQ(capture_nothing(), 0);
    ASSERT_EQ(capture_by_value(10), 11);
    ASSERT_EQ(capture_by_reference(10), 13);
    ASSERT_EQ(capture_differently(), 4);
    ASSERT_EQ(capture_mutable_arg(), 7);
    ASSERT_EQ(x, 1);
}

//////////////
// decltype //
//////////////

void test_decltype()
{
    int a = 1;
    decltype(a) b = a;
    decltype(123) c = a;
    const int &d = a;
    decltype(d) e = a;
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 1);
    ASSERT_EQ(c, 1);
    ASSERT_EQ(d, 1);
    ASSERT_EQ(e, 1);
    // also relates to auto deduction of return type
    ASSERT_EQ(deduce_return_type(a, d), 2);
}

//////////////////
// Type aliases //
//////////////////

using String = std::string;

template <typename T>
using Vec = std::vector<T>;

void test_type_aliases()
{
    String s("foo");
    Vec<int> v{1, 2, 3};
    ASSERT_EQ(s, std::string("foo"));
    ASSERT_EQ(v, (std::vector<int>{1, 2, 3}));
}

/////////////
// nullptr //
/////////////

void test_nullptr()
{
    char *x = nullptr;                                // implicitly convertible to any pointer type
    uint64_t y = reinterpret_cast<uint64_t>(nullptr); // not convertible to integral type
    (void)y;
    ASSERT_EQ(x, nullptr);
}

//////////////////////////
// Strongly-typed enums //
//////////////////////////

enum class Color : unsigned int
{ // explicitly specify underlying type
    Red = 0xff0000,
    Green = 0xff00,
    Blue = 0xff
};

enum class Alert : bool
{ // won't pollute parent scope with names
    Red,
    Green
};

void test_strongly_typed_enums()
{
    ASSERT_NE(Color::Green, Color::Red);
    ASSERT_NE(Alert::Green, Alert::Red);
}

////////////////
// Attributes //
////////////////

[[noreturn]] void get_nothing()
{
    throw std::runtime_error("error");
}

// toolchain-specific attributes may be inside a namespace
[[gnu::always_inline, gnu::hot]] inline int get_popular_number()
{
    return 7;
}

void test_attributes()
{
    try
    {
        get_nothing();
    }
    catch (...)
    {
    }
    ASSERT_EQ(get_popular_number(), 7);
}

///////////////
// constexpr //
///////////////

constexpr int square(int x)
{
    return x * x;
}

static constexpr int four = 4;
static_assert(four == 4, "invalid value of four");

void test_constexpr()
{
    int y = square(2);
    // cannot call square(y) -- expression must be evaluatable at compile-time
    int z = four;
    constexpr int another_four = 2 * 2;
    ASSERT_EQ(y, z);
    ASSERT_EQ(y, another_four);
}

// works with classes too
struct Complex
{
    double re;
    double im;
    // when arguments are evaluatable at compile-time, yields compile-time constant
    constexpr Complex(double r, double i) : re(r), im(i) {}
    constexpr double real() const { return re; }
    constexpr double imag() const { return im; }
};

void test_constexpr_class()
{
    constexpr Complex I(0, 1);
    const Complex J(0, 1); // this is a run-time constructed variable
    ASSERT(I.re == J.re && I.im == J.im);
}

////////////////////////////
// Delegating constructor //
////////////////////////////

struct ObjC
{
    int foo;
    int bar;
    ObjC(int foo, int bar) : foo(foo), bar(bar) {}
    ObjC(int foo) : ObjC(foo, 0) {} // calls other constructor, giving an intializer list
};

void test_delegating_ctor()
{
    ObjC c(3);
    ASSERT_EQ(c.bar, ObjC(7, 0).bar);
}

///////////////////////////
// User-defined literals //
///////////////////////////

// `unsigned long long` parameter required for integer literal.
long long operator"" _celsius(unsigned long long tempCelsius)
{
    return std::llround(tempCelsius * 1.8 + 32);
}

// `const char*` and `std::size_t` required as parameters for char pointer.
int operator"" _int(const char *str, std::size_t)
{
    return std::stoi(str);
}

void test_user_defined_literals()
{
    ASSERT_EQ(24_celsius, 75);
    ASSERT_EQ("123"_int, 123);
}

///////////////////////////////
// Explicit override & final //
///////////////////////////////

struct ObjD
{
    virtual std::string foo() { return "base"; }
    void bar() {}
};

struct ObjE : ObjD
{
    std::string foo() override { return "derived"; } // compiles
    // void bar() override;     // compile error -- A::bar is not virtual
    // void baz() override;     // compile error -- A has no baz
};

void test_explicit_override()
{
    ObjE e;
    ObjD *d = &e;
    ASSERT_EQ(d->foo(), "derived");
}

struct ObjF final : ObjE
{ // ObjF cannot be derived from any further
    std::string foo() override { return "further-derived"; }
};

void test_final_specifier()
{
    ObjF f;
    ASSERT_EQ(f.foo(), "further-derived");
}

///////////////////////
// default & deleted //
///////////////////////

struct ObjG
{
    int x = 0;
    ObjG() : x(1) {}
};

struct ObjH : ObjG
{
    ObjH() = default; // uses ObjG::ObjG
    ObjH(const ObjH &) = delete;
    ObjH &operator=(const ObjH &) = delete;
};

void test_default_specifier()
{
    ObjH h;
    ASSERT_EQ(h.x, 1);
}

void test_delete_specifier()
{
    ObjH h0;
    ASSERT_EQ(h0.x, 1);
    // cannot do `ObjH h1 = h0` -- copy constructor deleted
    // cannot do `h1 = h0` -- assignment operator= deleted
}

///////////////////////////
// Range-based for loops //
///////////////////////////

void test_range_based_for()
{
    std::array<int, 5> a{1, 2, 3, 4, 5};
    for (auto x : a)
        x *= 2;
    ASSERT_EQ(a, (std::array<int, 5>{1, 2, 3, 4, 5}));
    for (auto &x : a)
        x *= 2;
    ASSERT_EQ(a, (std::array<int, 5>{2, 4, 6, 8, 10}));
}

////////////////////////////
// Converting constructor //
////////////////////////////

struct ObjI
{
    int c = 0;
    ObjI() = default;
    ObjI(int, int) { c = 2; }
    ObjI(int, int, int) { c = 3; }
};

struct ObjJ : ObjI
{
    bool list_version_called = false;
    ObjJ(int a, int b) : ObjI(a, b) {}
    ObjJ(int a, int b, int c) : ObjI(a, b, c) {}
    ObjJ(std::initializer_list<int>) { list_version_called = true; }
};

void test_converting_ctors()
{
    ObjI i0{1, 2}; // curly braced syntax converted into constructor args
    ASSERT_EQ(i0.c, 2);
    ObjI i1 = {1, 2, 3};
    ASSERT_EQ(i1.c, 3);
    ObjJ j{4, 5, 6}; // has one that accepts initializer_list, calls that instead
    ASSERT(j.list_version_called);
}

//////////////////////////////
// Member initializer sugar //
//////////////////////////////

class Person
{
private:
    unsigned age{7}; // avoids writing a ctor only for doing such default initialization
public:
    unsigned get_age() const { return age; }
};

void test_member_initializer()
{
    Person p;
    ASSERT_EQ(p.get_age(), 7);
}

////////////////////////////////////
// Ref-qualified member functions //
////////////////////////////////////

struct ObjK
{
    std::vector<int> get_bar(std::string &s) &
    {
        s = "lr";
        return bar;
    }
    std::vector<int> get_bar(std::string &s) const &
    {
        s = "clr";
        return bar;
    }
    std::vector<int> get_bar(std::string &s) &&
    {
        s = "rr";
        return std::move(bar);
    }
    std::vector<int> get_bar(std::string &s) const &&
    {
        s = "crr";
        return std::move(bar);
    }

private:
    std::vector<int> bar;
};

void test_ref_qualified_methods()
{
    std::string s0, s1, s2, s3, s4;
    ObjK foo0;
    auto bar0 = foo0.get_bar(s0); // calls `get_bar() &`
    const ObjK foo1;
    auto bar1 = foo1.get_bar(s1); // calls `get_bar() const&`
    ObjK{}.get_bar(s2);           // calls `get_bar() &&`
    std::move(foo0).get_bar(s3);  // calls `get_bar() &&`
    std::move(foo1).get_bar(s4);  // calls `get_bar() const&&`
    ASSERT_EQ(s0, "lr");
    ASSERT_EQ(s1, "clr");
    ASSERT_EQ(s2, "rr");
    ASSERT_EQ(s3, "rr");
    ASSERT_EQ(s4, "crr");
}

////////////////////////
// noexcept specifier //
////////////////////////

void may_throw_something()
{
    throw std::runtime_error("something");
}

void declared_nonthrowing() noexcept
{
    may_throw_something(); // allowed, even if it throws
    // calling `throw ..` here is valid, but effectively triggers std::terminate
}

void test_noexcept_specifier()
{
    EXPECT_THROW(may_throw_something);
}

/////////////////////////
// Raw string literals //
/////////////////////////

void test_raw_string_literal()
{
    const std::string msg1 = "\n    Hello,\n        \"world\"!\n    ";
    const std::string msg2 = R"deli(
    Hello,
        "world"!
    )deli"; // anything enclosed within parentheses are preserved
    ASSERT_EQ(msg1, msg2);
}

/////////////////
// std::thread //
/////////////////

static int counter = 0;
static std::mutex counter_mutex;

static void thread_func(int arg1, int arg2)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::lock_guard<std::mutex> g(counter_mutex);
    counter++;
}

void test_std_thread()
{
    std::vector<std::thread> threads;
    // could give a lambda function
    threads.emplace_back([]()
                         {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::lock_guard<std::mutex> g(counter_mutex);
        counter++; });
    // could give a standard function
    threads.emplace_back(thread_func, 1, 2);
    for (auto &t : threads)
        t.join();
    ASSERT_EQ(counter, 2);
}

////////////////////
// std::to_string //
////////////////////

void test_std_to_string()
{
    ASSERT_EQ(std::to_string(123u), "123");
    ASSERT_EQ(std::to_string(1.2f), "1.200000"); // assumes 6 significant digits
}

////////////////////////
// Type traits & info //
////////////////////////

static_assert(std::is_integral<int>::value, "is_integral<int> not true"); // usable at compile-time
static_assert(std::is_same<std::conditional<true, int, double>::type, int>::value, "is_same not working");

void test_type_traits_info()
{
    typedef std::conditional<(sizeof(int) > sizeof(double)), int, double>::type MyNumT;
    ASSERT_NE(typeid(MyNumT).name(), std::string("int"));
    // this will be a string literal generated at compile-time,
    // and is compiler-dependent -- gcc may yield "d" for double
}

////////////////////
// Smart pointers //
////////////////////

struct ObjL
{
    int x = -1;
};

void test_unique_ptr()
{
    {
        std::unique_ptr<ObjL> p0(new ObjL()); // using constructor is not recommended;
                                              // with C++14 onward, use std::make_unique
        // p0 owns this heap object at this point
        p0->x = 0;
        ASSERT_EQ(p0->x, 0);
    } // when p0 goes out of scope, object is destructed (freed)
    {
        std::unique_ptr<ObjL> p1(new ObjL());
        p1->x = 1;
        {
            std::unique_ptr<ObjL> p2(std::move(p1));
            // move semantics, now p1 owns this object, p0 no longer safe to dereference
            // std::unique_ptr is not copyable
            p2->x = 2;
            ASSERT_EQ(p2->x, 2);
            p1 = std::move(p2); // move back to p1
        }
        p1->x = 3;
        ASSERT_EQ(p1->x, 3);
    }
}

void test_shared_ptr()
{
    // essentially a reference counted pointer that automatically destroys the heap object it
    // holds when the last pointer goes out of scope, calls reset(), or assigned another object;
    // std::shared_ptr does not guarantee mutual exclusion on the object it holds when there's
    // multi-threading -- proper synchronization is still required!
    {
        std::shared_ptr<ObjL> pn(new ObjL()); // using constructor is not recommended
    }
    {
        auto p0 = std::make_shared<ObjL>(); // std::make_shared is recommended, see doc
        p0->x = 0;
        ASSERT_EQ(p0->x, 0);
        {
            std::shared_ptr<ObjL> p1 = p0;
            std::shared_ptr<ObjL> p2 = p0;
            p1->x = 1;
            p2->x = 2;
            ASSERT_EQ(p1->x, 2);
            ASSERT_EQ(p0->x, 2);
        }
        p0->x = 3;
        ASSERT_EQ(p0->x, 3);
    } // all references go out-of-scope here -- object destructed
}

/////////////////
// std::chrono //
/////////////////

void test_std_chrono()
{
    std::chrono::time_point<std::chrono::steady_clock> tps, tpe;
    tps = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    tpe = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed_ms = tpe - tps;
    ASSERT(elapsed_ms.count() >= 30);
}

///////////////////////
// Tuples & std::tie //
///////////////////////

void test_tuples_std_tie()
{
    auto profile = std::make_tuple(24, "Jose", 179.5);
    // has type std::tuple<int, const char *, double>
    // use std::get to get element at index
    ASSERT_EQ(std::get<0>(profile), 24);
    ASSERT_EQ(std::get<1>(profile), std::string("Jose"));
    ASSERT_EQ(std::get<2>(profile), 179.5);
    // use std::tie to bound a tuple of lvalue references (for unpacking)
    std::string name;
    double height;
    std::tie(std::ignore, name, height) = profile;
    ASSERT_EQ(name, std::string("Jose"));
    ASSERT_EQ(height, 179.5);
}

//////////////////////////////////
// Array & unordered containers //
//////////////////////////////////

void test_std_array()
{
    std::array<int, 3> a = {2, 1, 3};
    std::sort(a.begin(), a.end());
    ASSERT_EQ(a, (std::array<int, 3>{1, 2, 3}));
    ASSERT_EQ(a[1], 2);
    for (int &x : a)
        x *= 2;
    ASSERT_EQ(a, (std::array<int, 3>{2, 4, 6}));
}

void test_unordered_containers()
{
    // unordered_set
    std::unordered_set<int> s;
    s.insert(7);
    s.insert(9);
    int x = s.count(7);
    ASSERT_EQ(x, 1);
    // unordered_map
    std::unordered_map<int, std::string> m;
    m[4] = "nice";
    std::string y = m.at(4);
    ASSERT_EQ(y, "nice");
    // see unordered_multiset, _multimap, and all useful methods in doc
}

//////////////
// std::ref //
//////////////

void test_std_ref()
{
    auto val = 99;
    auto _ref = std::ref(val);
    _ref++;
    auto _cref = std::cref(val);
    ASSERT_EQ(_cref.get(), 100);
    // can be used to make a vector of "references" (actually references wrappers)
    std::vector<std::reference_wrapper<int>> vec;
    vec.push_back(_ref);
    vec[0].get() = 77;
    ASSERT_EQ(val, 77);
}

///////////////////////////
// std::begin & std::end //
///////////////////////////

template <typename T>
int count_twos(const T &container)
{
    return std::count_if(std::begin(container), std::end(container),
                         [](int e)
                         { return e == 2; });
}

void test_std_begin_end()
{
    std::vector<int> vec{1, 2, 2, 2, 3, 4, 5};
    int arr[7] = {8, 7, 7, 5, 3, 2, 1};
    auto vec_2s = count_twos(vec);
    auto arr_2s = count_twos(arr); // std::begin/end also works with raw arrays
    ASSERT_EQ(vec_2s, 3);
    ASSERT_EQ(arr_2s, 1);
}

//////////////////////////////
// Async, future, & promise //
//////////////////////////////

static int return_a_thousand()
{
    // maybe some time-consuming work here...
    return 1000;
}

void test_std_async_future()
{
    auto handle0 = std::async(std::launch::async, return_a_thousand);
    // do work on a new thread
    auto handle1 = std::async(std::launch::deferred, return_a_thousand);
    // laze evaluation on current thread (evaluate when trying to get)
    auto handle2 = std::async(std::launch::deferred | std::launch::async, return_a_thousand);
    // up to implementation to decide
    ASSERT_EQ(handle0.get(), 1000);
    ASSERT_EQ(handle1.get(), 1000);
    ASSERT_EQ(handle2.get(), 1000);
}

static void accumulate_func(std::vector<int>::const_iterator begin,
                            std::vector<int>::const_iterator end,
                            std::promise<int> accumulate_promise)
{
    int sum = std::accumulate(begin, end, 0);
    accumulate_promise.set_value(sum); // notifies future
}

static void barrier_demo(std::promise<void> barrier_promise)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    barrier_promise.set_value(); // notifies future
}

void test_std_promise()
{
    // promise is a lower-level implementation and can be used explicitly when your task (that
    // makes the result of future ready) is too complicated and thus cannot fit to be passed
    // in std::async
    // an std::promise assumes only one owner at any time, so need to std::move
    std::vector<int> vec{1, 2, 3, 4, 5};
    std::promise<int> accumulate_promise;
    std::future<int> accumulate_future = accumulate_promise.get_future();
    std::thread t0(accumulate_func, vec.begin(), vec.end(), std::move(accumulate_promise));
    ASSERT_EQ(accumulate_future.get(), 15);
    t0.join();
    // a special "void" promise is useful as a handy barrier
    std::promise<void> barrier_promise;
    std::future<void> barrier_future = barrier_promise.get_future();
    std::thread t1(barrier_demo, std::move(barrier_promise));
    barrier_future.wait();
    t1.join();
}

int main(int argc, char *argv[])
{
    std::cout << "C++11 features runnable tests:" << std::endl;

    RUN_EXAMPLE(test_std_move);
    RUN_EXAMPLE(test_move_ctor_assign_op);
    RUN_EXAMPLE(test_rvalue_references);
    RUN_EXAMPLE(test_forwarding_references);
    RUN_EXAMPLE(test_std_forward);
    RUN_EXAMPLE(test_variadic_templates);
    RUN_EXAMPLE(test_initializer_lists);
    RUN_EXAMPLE(test_static_asserts);
    RUN_EXAMPLE(test_auto_type);
    RUN_EXAMPLE(test_lambda_expressions);
    RUN_EXAMPLE(test_decltype);
    RUN_EXAMPLE(test_type_aliases);
    RUN_EXAMPLE(test_nullptr);
    RUN_EXAMPLE(test_strongly_typed_enums);
    RUN_EXAMPLE(test_attributes);
    RUN_EXAMPLE(test_constexpr);
    RUN_EXAMPLE(test_constexpr_class);
    RUN_EXAMPLE(test_delegating_ctor);
    RUN_EXAMPLE(test_user_defined_literals);
    RUN_EXAMPLE(test_explicit_override);
    RUN_EXAMPLE(test_final_specifier);
    RUN_EXAMPLE(test_default_specifier);
    RUN_EXAMPLE(test_delete_specifier);
    RUN_EXAMPLE(test_range_based_for);
    RUN_EXAMPLE(test_converting_ctors);
    RUN_EXAMPLE(test_member_initializer);
    RUN_EXAMPLE(test_ref_qualified_methods);
    RUN_EXAMPLE(test_noexcept_specifier);
    RUN_EXAMPLE(test_raw_string_literal);
    RUN_EXAMPLE(test_std_thread);
    RUN_EXAMPLE(test_std_to_string);
    RUN_EXAMPLE(test_type_traits_info);
    RUN_EXAMPLE(test_unique_ptr);
    RUN_EXAMPLE(test_shared_ptr);
    RUN_EXAMPLE(test_std_chrono);
    RUN_EXAMPLE(test_tuples_std_tie);
    RUN_EXAMPLE(test_std_array);
    RUN_EXAMPLE(test_unordered_containers);
    RUN_EXAMPLE(test_std_ref);
    RUN_EXAMPLE(test_std_begin_end);
    RUN_EXAMPLE(test_std_async_future);
    RUN_EXAMPLE(test_std_promise);

    return 0;
}
