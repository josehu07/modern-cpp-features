#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <any>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <execution>
#include "utils.hpp"

/////////////////////////
// Folding expressions //
/////////////////////////

template <typename... Args>
auto sum(Args... args)
{
    // unary folding
    return (... + args);
}

template <typename... Args>
bool logical_and(Args... args)
{
    // binary folding
    return (true && ... && args);
}

void test_folding_exprs()
{
    // folding expressions avoid writing tedious (and badly-readable) recursive definitions
    // for variadic template functions
    bool b0 = true;
    bool &b1 = b0;
    bool b2 = logical_and(b0, b1, true);
    ASSERT(b2);
    int n0 = 1;
    double n1 = 2.3;
    double &n2 = n1;
    auto n3 = sum(n0, n2, 3);
    ASSERT_EQ(n3, 6.3);
}

///////////////////////
// constexpr lambdas //
///////////////////////

void test_constexpr_lambdas()
{
    auto identity = [](int n) constexpr
    { return n; };
    static_assert(identity(123) == 123);
}

//////////////////////
// inline variables //
//////////////////////

struct ObjA
{
    int x;
};

// inline specifier can be useful for defining global variables in a header-only library
// and be used in multiple source files -- just like inline functions
// for example, could have the following lines in a somelib.hpp
inline std::atomic<int> global_counter(0);
inline ObjA a0 = ObjA{321}; // value collapsed inline

void test_inline_variables()
{
    ObjA a1 = ObjA{321};
    ASSERT_EQ(a0.x, a1.x);
}

///////////////////////
// Nested namespaces //
///////////////////////

namespace DB::Person::Student
{
    static constexpr char ID_PREFIX[] = "stu";
}
// Equivalent to:
// namespace DB {
//   namespace Person {
//     namespace Student {
//       static constexpr char ID_PREFIX[] = "stu";
//     }
//   }
// }

void test_nested_namespaces()
{
    ASSERT_EQ(DB::Person::Student::ID_PREFIX, std::string("stu"));
}

/////////////////////////
// Structured bindings //
/////////////////////////

void test_structured_bindings()
{
    auto [x, y, z] = std::tuple<int, double, std::string>(1, 2.3, "4");
    std::array<int, 2> arr{1, 2};
    auto &[a, b] = arr;
    ASSERT_EQ(x, 1);
    ASSERT_EQ(y, 2.3);
    ASSERT_EQ(z, "4");
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
    // cleaner use of functions returning tuple-like objects
    auto func = [](int i, double j) -> decltype(auto)
    { return std::make_tuple(i, j); };
    auto [i, j] = func(1, 2.3);
    ASSERT_EQ(i, 1);
    ASSERT_EQ(j, 2.3);
    // range-based for loop on maps
    std::unordered_map<std::string, int> map{{"a", 1}, {"b", 2}};
    int sum = 0;
    for (const auto &[key, val] : map)
        sum += val;
    ASSERT_EQ(sum, 3);
}

/////////////////////////////
// if & switch initializer //
/////////////////////////////

static std::vector<int> shared_vec;
static std::mutex lock;

void test_if_initializer()
{
    // keeps scope tight
    if (std::lock_guard<std::mutex> lk(lock); shared_vec.empty())
    {
        shared_vec.push_back(1);
    }
    ASSERT_EQ(shared_vec, std::vector<int>{1});
}

struct ObjB
{
    enum Status
    {
        OK,
        FAILED
    };
    bool valid = true;
    ObjB(bool valid = true) : valid(valid) {}
    Status status() const { return valid ? OK : FAILED; }
    void do_work() {}
    static std::string status_msg(Status s) { return s == OK ? "ok" : "not_ok"; }
};

void test_switch_initializer()
{
    auto should_throw = []()
    {
        // keeps scope tight
        switch (ObjB test_b(false); auto s = test_b.status())
        {
        case ObjB::OK:
            test_b.do_work();
            break;
        case ObjB::FAILED:
            throw std::runtime_error(ObjB::status_msg(s));
            break;
        default:
            break;
        }
    };
    EXPECT_THROW(should_throw);
}

//////////////////
// if constexpr //
//////////////////

template <typename T>
constexpr bool is_integral()
{
    if constexpr (std::is_integral<T>::value)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void test_if_constexpr()
{
    static_assert(is_integral<int>());
    static_assert(!is_integral<double>());
}

/////////////////////
// More attributes //
/////////////////////

[[maybe_unused]] void legacy_func()
{
    return;
}

[[nodiscard]] int make_a_ten()
{
    return 10;
}

void test_more_attributes()
{
    int counter = 0, level = 1;
    switch (level)
    {
    case 1:
        counter++; // falling through to case 1 is intended
        [[fallthrough]];
    case 2:
        counter++;
        break;
    default:
        break;
    }
    ASSERT_EQ(counter, 2);
    // call `make_a_ten()` and ignoring its return value will issue a compiler warning
    int x = make_a_ten();
    ASSERT_EQ(x, 10);
}

///////////////////
// __has_include //
///////////////////

#ifdef __has_include
#if __has_include(<mylib>)
#include <mylib>
#define has_mylib true
#elif __has_include(<experimental/mylib>)
#include <experimental/mylib>
#define has_mylib true
#define experimental_mylib
#else
#define has_mylib false
#endif
#else
#define has_mylib false
#endif

void test_has_include()
{
    ASSERT(!has_mylib);
}

//////////////////
// std::variant //
//////////////////

struct ObjC
{
    int x;
    int y;
};

void test_std_variant()
{
    // some sort of a "strong enum" like Rust's (or called "type-safe union")
    std::variant<int, double, ObjC, std::string> thing{2};
    ASSERT_EQ(thing.index(), 0);
    ASSERT_EQ(std::get<int>(thing), 2);
    ASSERT_EQ(std::get<0>(thing), 2);
    thing = "str";
    ASSERT_EQ(thing.index(), 3);
    ASSERT_EQ(std::get<std::string>(thing), "str");
}

//////////////
// std::any //
//////////////

void test_std_any()
{
    // type-safe union of a single value of any type
    std::any x{5};
    ASSERT(x.has_value());
    ASSERT_EQ(std::any_cast<int>(x), 5);
    std::any_cast<int &>(x) = 10;
    ASSERT_EQ(std::any_cast<int>(x), 10);
}

///////////////////
// std::optional //
///////////////////

std::optional<std::string> create_string(bool success)
{
    if (success)
        return "str";
    else
        return {}; // empty initializer-list casts to an empty optional
}

void test_std_optional()
{
    // some sort of a valid-or-none "Option" like Rust's
    auto s0 = create_string(true).value();
    auto s1 = create_string(false).value_or("none");
    std::string s2;
    if (auto s = create_string(true))
    {
        s2 = s.value();
    }
    ASSERT_EQ(s0, "str");
    ASSERT_EQ(s1, "none");
    ASSERT_EQ(s2, "str");
}

//////////////////////
// std::string_view //
//////////////////////

void test_std_string_view()
{
    // non-owning reference to a std::string, useful for parsing operations
    std::string str("   trim me");
    std::string_view view(str);
    view.remove_prefix(std::min(view.find_first_not_of(" "), view.size()));
    ASSERT_EQ(str, "   trim me");
    ASSERT_EQ(view, "trim me");
    // also useful for declaring a compile-time constexpr string
    constexpr std::string_view const_view = "something constant";
    ASSERT_EQ(const_view, "something constant");
}

///////////////////////////////
// Generic callable invokers //
///////////////////////////////

template <typename Callable>
class Proxy
{
    Callable c;

public:
    Proxy(Callable c) : c(c) {}
    template <class... Args>
    decltype(auto) operator()(Args &&...args)
    {
        return std::invoke(c, std::forward<Args>(args)...);
    }
};

void test_std_invoke()
{
    // useful when implementing a custom callable class
    auto add_func = [](int x, double y)
    { return x + y; };
    Proxy<decltype(add_func)> proxy(add_func);
    auto result = proxy(1, 2.3);
    ASSERT_EQ(result, 3.3);
}

void test_std_apply()
{
    // call any callable object with arguments as a tuple
    auto add_func = [](int x, double y)
    { return x + y; };
    auto result = std::apply(add_func, std::make_tuple(1, 2.3));
    ASSERT_EQ(result, 3.3);
}

/////////////////////
// std::filesystem //
/////////////////////

void test_std_filesystem()
{
    bool exists = std::filesystem::exists("some_cOmpLiCaTed_filename");
    ASSERT(!exists);
}

//////////////////////
// Splicing methods //
//////////////////////

void test_map_splicing()
{
    std::map<int, std::string> master{{1, "one"}, {2, "two"}};
    std::map<int, std::string> backup{{4, "three"}};
    auto entry = backup.extract(4);
    entry.key() = 3;
    master.insert(std::move(entry));
    ASSERT_EQ(master, (std::map<int, std::string>{{1, "one"}, {2, "two"}, {3, "three"}}));
    ASSERT(backup.empty());
}

void test_set_splicing()
{
    std::set<int> src{1, 3, 5};
    std::set<int> dst{2, 4, 5};
    dst.merge(src);
    ASSERT_EQ(dst, (std::set<int>{1, 2, 3, 4, 5}));
}

/////////////////////////
// Parallel algorithms //
/////////////////////////

void test_parallel_algos()
{
    std::vector<int> large_vec(100, 1);
    auto result = std::find(std::execution::par, std::begin(large_vec), std::end(large_vec), 1);
    ASSERT(result != large_vec.end());
    ASSERT_EQ(*result, 1);
}

int main(int argc, char *argv[])
{
    std::cout << "C++17 features runnable tests:" << std::endl;

    RUN_EXAMPLE(test_folding_exprs);
    RUN_EXAMPLE(test_constexpr_lambdas);
    RUN_EXAMPLE(test_inline_variables);
    RUN_EXAMPLE(test_nested_namespaces);
    RUN_EXAMPLE(test_structured_bindings);
    RUN_EXAMPLE(test_if_initializer);
    RUN_EXAMPLE(test_switch_initializer);
    RUN_EXAMPLE(test_if_constexpr);
    RUN_EXAMPLE(test_more_attributes);
    RUN_EXAMPLE(test_has_include);
    RUN_EXAMPLE(test_std_variant);
    RUN_EXAMPLE(test_std_any);
    RUN_EXAMPLE(test_std_optional);
    RUN_EXAMPLE(test_std_string_view);
    RUN_EXAMPLE(test_std_invoke);
    RUN_EXAMPLE(test_std_apply);
    RUN_EXAMPLE(test_std_filesystem);
    RUN_EXAMPLE(test_map_splicing);
    RUN_EXAMPLE(test_set_splicing);
    RUN_EXAMPLE(test_parallel_algos);

    return 0;
}
