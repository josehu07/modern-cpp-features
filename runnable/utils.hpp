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
        ss << "assertion failed @ " << file << ":" << line;
        message = ss.str();
    }
    ~AssertionFailure() {}

    const char *what() const noexcept override {
        return message.c_str();
    }
};

void assert_or_raise(const std::string file, int line, bool condition) {
    if (!condition) {
        throw AssertionFailure(file, line);
    }
}

#define ASSERT(cond)    assert_or_raise(__FILE__, __LINE__, (cond))
#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NE(a, b) ASSERT((a) != (b))


///////////////////////////////
// Exception raising support //
///////////////////////////////

// TODO


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
    } catch (...) {                                         \
        throw;                                              \
    }


#endif
