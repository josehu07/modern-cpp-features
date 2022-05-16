#include <iostream>
#include <vector>
#include <memory>
#include <exception>
#include <cmath>
#include "utils.hpp"


////////////////////
// Move semantics //
////////////////////

void test_std_move() {
    std::unique_ptr<std::vector<int>> p1(new std::vector<int>{1, 2, 3});
    std::unique_ptr<std::vector<int>> p2 = std::move(p1);
    // std::move(x) returns a "moved" rvalue
    // p2 thus takes over "ownership" without copying vector content
    // p1 is now unsafe to dereference
    ASSERT_EQ(*p2, (std::vector<int>{1, 2, 3}));
    // when p2 goes out of scope, deletes (frees) the object
}

struct ObjA {
    std::string s;
    std::string last_op;
    // constructor:
    ObjA() = delete;
    ObjA(std::string arg) : s(arg) {
        last_op = "normal-construct";
    }
    // copy constructor & copy assignment operator:
    ObjA(const ObjA& o) : s(o.s) {
        last_op = "copy-construct";
    }
    ObjA& operator=(const ObjA& o) {
        last_op = "copy-assignment";
        s = o.s;
        return *this;
    }
    // move constructor & move assignment operator:
    ObjA(ObjA&& o) : s(std::move(o.s)) {
        last_op = "move-construct";
    }
    ObjA& operator=(ObjA&& o) {
        last_op = "move-assignment";
        s = std::move(o.s);
        return *this;
    }
};

ObjA make_A_rvalue(ObjA a) {
    return a;
}

void test_move_ctor_assign_op() {
    ObjA a0("dummy0");                          // normally constructed
    ObjA a1 = ObjA("dummy1");                   // normally constructed
    ObjA a2 = a1;                               // copy constructed
    ObjA a3("dummy3");
    a3 = a1;                                    // copy assignment
    ObjA a4 = make_A_rvalue(ObjA("dummy4"));    // move constructed from rvalue temporary
    ObjA a5 = std::move(a1);                    // move constructed using std::move
    ObjA a6("dummy6");
    a6 = ObjA("temp2");                         // move assignment from rvalue temporary
    ObjA a7("dummy7");
    a7 = std::move(a2);                         // move assignment using std::move
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

std::string which_variant_called(int& x) {
    return "lvalue-reference";
}

std::string which_variant_called(int&& x) {
    return "rvalue-reference";
}

void test_rvalue_references() {
    int x = 0;      // `x` is an lvalue of type `int`
    int& xl = x;    // `xl` is an lvalue of type `int&` (lvalue reference)
    int&& xr = 1;   // `xr` is an lvalue of type `int&&` (rvalue reference)
    // cannot do `int&& xr = x` since `x` is an lvalue
    ASSERT_EQ(which_variant_called(x),             "lvalue-reference");
    ASSERT_EQ(which_variant_called(xl),            "lvalue-reference");
    ASSERT_EQ(which_variant_called(2),             "rvalue-reference");
    ASSERT_EQ(which_variant_called(std::move(x)),  "rvalue-reference");
    ASSERT_EQ(which_variant_called(xr),            "lvalue-reference");     // attention
    ASSERT_EQ(which_variant_called(std::move(xr)), "rvalue-reference");
}

void test_forwarding_references() {
    int x = 0;
    auto&& al = x;      // `al` is an lvalue reference (`int&`) since we bind to lvalue `x`
    auto&& ar = 0;      // `ar` is an rvalue reference (`int&&`) since we bind to rvalue 0
    // besides `auto`, `&&` also works on template typenames
    ASSERT_EQ(which_variant_called(al), "lvalue-reference");
    (void) ar;
}

template <typename T>
ObjA make_A_by_forwarding(T&& a) {
    return ObjA{std::forward<T>(a)};
}

void test_std_forward() {
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
struct ObjB {
    constexpr static int ntargs = sizeof...(T);

};

static_assert(ObjB<>::ntargs == 0);
static_assert(ObjB<char, int, long>::ntargs == 3);

template <size_t S, size_t... Args>
std::array<bool, S> create_bool_array() {
    std::array<bool, S> b{};
    auto lambda = [](...){};    // variadic lambda that takes a parameter pack as argument
    lambda(b[Args] = true...);  // call it giving a parameter unpack -- a trick to avoid recursion
    return b;
}

void test_variadic_templates() {
    auto b = create_bool_array<5, 0, 3>();
    ASSERT_EQ(b, (std::array<bool, 5>{true, false, false, true, false}));
}


///////////////////////
// Initializer lists //
///////////////////////

void test_initializer_lists() {
    std::initializer_list<int> list = {1, 2, 3};
    // light-weight immutable replacement for vector in some cases
    int total = 0;
    for (const int& e : list)
        total += e;
    ASSERT_EQ(total, 6);
}


///////////////////////
// Static assertions //
///////////////////////

static_assert(sizeof(uint64_t) == 8);

void test_static_asserts() {
    constexpr int x = 0;
    constexpr int y = 1;
    static_assert(x != y, "x should not be equal to y");    // checked at compile time
    ASSERT(x != y);     // this one evaluates at runtime
}


////////////////////
// auto deduction //
////////////////////

template <typename X, typename Y>
auto deduce_return_type(X x, Y y) -> decltype(x + y) {
    return x + y;
}

void test_auto_type() {
    // syntax sugar, shorter than std::vector<int>::const_iterator
    std::vector<int> v{1, 2, 3};
    auto cit = v.cbegin();
    (void) cit;
    // deduced return type of function
    int n1 = deduce_return_type(-1, 7);
    double n2 = deduce_return_type(-1, 7.0);
    ASSERT_EQ(static_cast<double>(n1), n2);
    // used in making forwarding references
    int x = 0;
    auto&& al = x;      // forwarding reference
    (void) al;
}


////////////////////////
// Lambda expressions //
////////////////////////

void test_lambda_expressions() {
    int x = 1, y = 2;
    auto capture_nothing = [] { int x = 0; return x; };
    auto capture_by_value = [=](int z) { return x + z; };
    auto capture_by_reference = [&](int z) { y = 3; return y + z; };
    auto capture_differently = [x, &y] { return x + y; };
    auto capture_mutable_arg = [x]() mutable { x = 7; return x; };
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

void test_decltype() {
    int a = 1;
    decltype(a) b = a;
    decltype(123) c = a;
    const int& d = a;
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

void test_type_aliases() {
    String s("foo");
    Vec<int> v{1, 2, 3};
    ASSERT_EQ(s, std::string("foo"));
    ASSERT_EQ(v, (std::vector<int>{1, 2, 3}));
}


/////////////
// nullptr //
/////////////

void test_nullptr() {
    char *x = nullptr;                                  // implicitly convertible to any pointer type
    uint64_t y = reinterpret_cast<uint64_t>(nullptr);   // not convertible to integral type
    (void) y;
    ASSERT_EQ(x, nullptr);
}


//////////////////////////
// Strongly-typed enums //
//////////////////////////

enum class Color : unsigned int {   // explicitly specify underlying type
    Red   = 0xff0000,
    Green = 0xff00,
    Blue  = 0xff
};

enum class Alert : bool {   // won't pollute parent scope with names
    Red,
    Green
};

void test_strongly_typed_enums() {
    ASSERT_NE(Color::Green, Color::Red);
    ASSERT_NE(Alert::Green, Alert::Red);
}


////////////////
// Attributes //
////////////////

[[noreturn]] void get_nothing() {
    throw std::runtime_error("error");
}

// toolchain-specific attributes may be inside a namespace
[[gnu::always_inline, gnu::hot]]
inline int get_popular_number() {
    return 7;
}

void test_attributes() {
    try {
        get_nothing();
    } catch (...) {}
    ASSERT_EQ(get_popular_number(), 7);
}


///////////////
// constexpr //
///////////////

constexpr int square(int x) {
    return x * x;
}

static constexpr int four = 4;
static_assert(four == 4);

void test_constexpr() {
    int y = square(2);
    // cannot call square(y) -- expression must be evaluatable at compile-time
    int z = four;
    constexpr int another_four = 2 * 2;
    ASSERT_EQ(y, z);
    ASSERT_EQ(y, another_four);
}

// works with classes too
struct Complex {
  double re;
  double im;
  // when arguments are evaluatable at compile-time, yields compile-time constant
  constexpr Complex(double r, double i) : re(r), im(i) {}
  constexpr double real() { return re; }
  constexpr double imag() { return im; }
};

void test_constexpr_class() {
    constexpr Complex I(0, 1);
    const Complex J(0, 1);      // this is a run-time constructed variable
    ASSERT(I.re == J.re && I.im == J.im);
}


////////////////////////////
// Delegating constructor //
////////////////////////////

struct ObjC {
    int foo;
    int bar;
    ObjC(int foo, int bar) : foo(foo), bar(bar) {}
    ObjC(int foo) : ObjC(foo, 0) {}     // calls other constructor, giving an intializer list
};

void test_delegating_ctor() {
    ObjC c(3);
    ASSERT_EQ(c.bar, ObjC(7, 0).bar);
}


///////////////////////////
// User-defined literals //
///////////////////////////

// `unsigned long long` parameter required for integer literal.
long long operator "" _celsius(unsigned long long tempCelsius) {
  return std::llround(tempCelsius * 1.8 + 32);
}

// `const char*` and `std::size_t` required as parameters for char pointer.
int operator "" _int(const char* str, std::size_t) {
  return std::stoi(str);
}

void test_user_defined_literals() {
    ASSERT_EQ(24_celsius, 75);
    ASSERT_EQ("123"_int, 123);
}


///////////////////////////////
// Explicit override & final //
///////////////////////////////

struct ObjD {
    virtual std::string foo() { return "base"; }
    void bar() {}
};

struct ObjE : ObjD {
    std::string foo() override { return "derived"; }    // compiles
    // void bar() override;     // compile error -- A::bar is not virtual
    // void baz() override;     // compile error -- A has no baz
};

void test_explicit_override() {
    ObjE e;
    ObjD *d = &e;
    ASSERT_EQ(d->foo(), "derived");
}

struct ObjF final : ObjE {      // ObjF cannot be derived from any further
    std::string foo() { return "further-derived"; }
};

void test_final_specifier() {
    ObjF f;
    ASSERT_EQ(f.foo(), "further-derived");
}


int main(int argc, char *argv[]) {
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

    return 0;
}
