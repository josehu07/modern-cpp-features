/**
 * Test utils do not use features > cpp11, so that all runnables can compile
 * with this header.
 */


#include <iostream>
#include <sstream>
#include <exception>
#include <functional>
#include <string>


#ifndef __UTILS_HPP__
#define __UTILS_HPP__


///////////////////////
// Assertion support //
///////////////////////

class AssertionFailure : public std::exception {
private:
    const std::string file;
    const int line;
    std::string message;

public:
    AssertionFailure(const std::string file, int line)
            : file(file), line(line) {
        std::stringstream ss;
        ss << "condition assertion failed @ " << file << ":" << line;
        message = ss.str();
    }
    ~AssertionFailure() {}

    const char *what() const noexcept override {
        return message.c_str();
    }
};

#define ASSERT(cond)                                    \
    do {                                                \
        if (!(cond)) {                                  \
            throw AssertionFailure(__FILE__, __LINE__); \
        }                                               \
    } while (0)
#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))


///////////////////////////////
// Exception raising support //
///////////////////////////////

class ThrowingFailure : public std::exception {
private:
    const std::string file;
    const int line;
    std::string message;

public:
    ThrowingFailure(const std::string file, int line)
            : file(file), line(line) {
        std::stringstream ss;
        ss << "no exception thrown as expected @ " << file << ":" << line;
        message = ss.str();
    }
    ~ThrowingFailure() {}

    const char *what() const noexcept override {
        return message.c_str();
    }
};

#define EXPECT_THROW(func)                             \
    do {                                               \
        bool thrown = false;                           \
        try {                                          \
            func();                                    \
        } catch (...) {                                \
            thrown = true;                             \
        }                                              \
        if (!thrown) {                                 \
            throw ThrowingFailure(__FILE__, __LINE__); \
        }                                              \
    } while (0)


///////////////////////
// Run stub for main //
///////////////////////

#define RUN_EXAMPLE(func)                                   \
    try {                                                   \
        std::cout << "  " << #func << "... " << std::flush; \
        func();                                             \
        std::cout << "OK" << std::endl;                     \
    } catch (const AssertionFailure& e) {                   \
        std::cout << "FAILED" << std::endl                  \
                  << "    " << e.what() << std::endl;       \
    } catch (const ThrowingFailure& e) {                    \
        std::cout << "FAILED" << std::endl                  \
                  << "    " << e.what() << std::endl;       \
    } catch (...) {                                         \
        throw;                                              \
    }


#endif
