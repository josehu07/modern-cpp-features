#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "utils.hpp"


/////////////////////
// Binary literals //
/////////////////////

void test_binary_literals() {
    ASSERT_EQ(0b110, 6u);
    ASSERT_EQ(0b1111'1111, 255u);
}


////////////////////////////////
// Generic lambda expressions //
////////////////////////////////

void test_generic_lambda() {
    auto identity = [](auto x) { return x; };
    int num = identity(3);
    std::string str = identity("foo");
    ASSERT_EQ(num, 3);
    ASSERT_EQ(str, "foo");
}


/////////////////////////////////
// Lambda capture initializers //
/////////////////////////////////

int times_10(int i) {
    return 10 * i;
}

void test_lambda_capture_initializers() {
    int j = 1;
    auto generator = [x = times_10(j)] () mutable { return x++; };
    j = 2;
    // lambda capture expression is evaluated at creation, not at when the lambda
    // is invoked, so the state x is 10 at this point
    auto a = generator();
    auto b = generator();
    auto c = generator();
    ASSERT_EQ(a, 10);
    ASSERT_EQ(b, 11);
    ASSERT_EQ(c, 12);
    // the capture initializer expression also makes it possible to pass a move-only
    // thing by value
    auto p = std::make_unique<int>(7);
    auto take_unique = [p = std::move(p)] { *p = 5; };
                        // this p is a new name within lambda scope, shadows outer p
    take_unique();  // p has moved into lambda and then destroyed
}


/////////////////////////
// More type deduction //
/////////////////////////

auto identity_int(int i) {
    return i;
}

template <typename T>
auto& identity_ref(T& i) {
    return i;
}

void test_return_type_deduction() {
    auto identity_ref_lambda = [](auto& x) -> auto& { return identity_ref(x); };
    int x = 123;
    int y = identity_int(x);
    int& z = identity_ref_lambda(x);
    z = 456;
    ASSERT_EQ(y, 123);
    ASSERT_EQ(x, 456);
}

auto identity_auto(const int& i) {
    return i;
}

decltype(auto) identity_decltype_auto(const int& i) {
    return i;
}

void test_decltype_auto() {
    // decltype(auto) behave almost exactly the same as auto, but it keeps references and
    // cv-qualifiers, while auto will not
    const int x = 0;
    auto x1 = x;                // int
    decltype(auto) x2 = x;      // const int
    x1++;
    ASSERT_EQ(x1, 1);
    ASSERT_EQ(x2, 0);
    int y = 0;
    int& yr = y;
    auto y1 = yr;               // int
    decltype(auto) y2 = yr;     // int&
    y1++;
    y2--;
    ASSERT_EQ(y1, 1);
    ASSERT_EQ(y2, -1);
    ASSERT_EQ(y, -1);
    // useful for better "autoness" in generic code
    int z = 123;
    static_assert(std::is_same<int, decltype(identity_auto(z))>::value);
    static_assert(std::is_same<const int&, decltype(identity_decltype_auto(z))>::value);
}


/////////////////////////////////
// Heavier constexpr functions //
/////////////////////////////////

constexpr int factorial(int n) {
    if (n <= 1)
        return 1;
    else
        return n * factorial(n - 1);
}

void test_constexpr_funcs() {
    ASSERT_EQ(factorial(5), 120);
}


////////////////////////
// Variable templates //
////////////////////////

template <typename T>
constexpr T pi = T(3.14159);

template <typename T>
T circular_area(T r) {
    return pi<T> * r * r;
}

void test_variable_templates() {
    double pi_f = pi<double>;
    int pi_i = pi<int>;
    ASSERT_EQ(pi_f, 3.14159);
    ASSERT_EQ(pi_i, 3);
    ASSERT_EQ(circular_area<int>(2), 12);
}


//////////////////////////
// deprecated attribute //
//////////////////////////

[[deprecated("this function is deprecated")]]
int legacy_func() { return 7; }

void test_deprecated_attribute() {
    // calling legacy_func() here will likely give compiler warning
    return;     // nothing to test at run-time
}


///////////////////
// More literals //
///////////////////

void test_more_literals() {
    using namespace std::chrono_literals;
    constexpr auto day_hours = 24h;
    constexpr auto day_minutes = 1440min;
    constexpr auto day_minutes_2 = std::chrono::duration_cast<std::chrono::minutes>(day_hours);
    ASSERT_EQ(day_hours.count(), 24);
    ASSERT_EQ(day_minutes.count(), 1440);
    ASSERT_EQ(day_minutes.count(), day_minutes_2.count());
}


///////////////////////////
// std::integer_sequence //
///////////////////////////

// refer to C++11 variadic templates doc if you get confused by the recursive definition below
template <typename T>
void fill_vec_with_template_ints(std::vector<T>& vec) {
    return;
}

template <typename T, T Int0, T... IntsRest>
void fill_vec_with_template_ints(std::vector<T>& vec) {
    vec.push_back(Int0);
    fill_vec_with_template_ints<T, IntsRest...>(vec);
}

template <typename T, T... Ints>
std::vector<T> sequence_to_vec(std::integer_sequence<T, Ints...> seq) {
    std::vector<T> vec;
    vec.reserve(seq.size());
    fill_vec_with_template_ints<T, Ints...>(vec);
    return vec;
}

void test_std_integer_sequence() {
    // compile-time integer sequence, useful in templating
    constexpr auto seq = std::make_integer_sequence<int, 7>{};
    ASSERT_EQ(sequence_to_vec(seq), (std::vector<int>{0, 1, 2, 3, 4, 5, 6}));
}


//////////////////////
// std::make_unique //
//////////////////////

struct ObjA {
    int x = -1;
};

void test_std_make_unique() {
    // std::make_unique is the recommended way to create C++11 std::unique_ptr
    // interestingly, std::make_shared has been introduced in C++11 already
    auto p = std::make_unique<ObjA>();
    p->x = 0;
    ASSERT_EQ(p->x, 0);
}


int main(int argc, char *argv[]) {
    std::cout << "C++11 features runnable tests:" << std::endl;

    RUN_EXAMPLE(test_binary_literals);
    RUN_EXAMPLE(test_generic_lambda);
    RUN_EXAMPLE(test_lambda_capture_initializers);
    RUN_EXAMPLE(test_return_type_deduction);
    RUN_EXAMPLE(test_decltype_auto);
    RUN_EXAMPLE(test_constexpr_funcs);
    RUN_EXAMPLE(test_variable_templates);
    RUN_EXAMPLE(test_deprecated_attribute);
    RUN_EXAMPLE(test_more_literals);
    RUN_EXAMPLE(test_std_integer_sequence);
    RUN_EXAMPLE(test_std_make_unique);

    return 0;
}
