#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <map>

class TestRunner {
    public:
    template<typename T, typename U>
    void AssertEqual(const T& t, const U& u, const std::string& hint) {
        if (t == u) {
            tests_passed_++;
        } else {
            tests_failed_++;
            std::cerr << "[ FAIL ] " << hint << "\n"
                      << "         Expected: " << u << "\n"
                      << "         Actual:   " << t << std::endl;
        }
    }

    using TestFunc = std::function<void(TestRunner&)>;

    void AddTest(const std::string& name, TestFunc func) {
        tests_.push_back({name, func});
    }

    void RunAllTests() {
        for (const auto& test : tests_) {
            std::cout << "--- Running test: " << test.name << " ---" << std::endl;
            try {
                test.function(*this);
            } catch (const std::exception& e) {
                tests_failed_++;
                std::cerr << "[ CRASH ] Test '" << test.name << "' threw an exception: " << e.what() << std::endl;
            } catch (...) {
                tests_failed_++;
                std::cerr << "[ CRASH ] Test '" << test.name << "' threw an unknown exception." << std::endl;
            }
        }
        std::cout << "\n====================\n";
        std::cout << "Tests finished.\n";
        std::cout << "PASSED: " << tests_passed_ << "\n";
        std::cout << "FAILED: " << tests_failed_ << "\n";
        std::cout << "====================\n";
    }

    int GetFailedCount() const {
        return tests_failed_;
    }

    private:
    struct TestCase {
        std::string name;
        TestFunc function;
    };

    std::vector<TestCase> tests_;
    int tests_passed_ = 0;
    int tests_failed_ = 0;
};