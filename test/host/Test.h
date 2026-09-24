#pragma once
#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
struct TestCase { const char* name; std::function<void()> run; };
inline std::vector<TestCase>& tests() { static std::vector<TestCase> v; return v; }
struct RegisterTest {
  RegisterTest(const char* name, std::function<void()> run) { tests().push_back({name, run}); }
};
#define TEST(name) static void name(); static RegisterTest reg_##name(#name, name); static void name()
#define CHECK(expr) do { if (!(expr)) throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " " #expr); } while (0)
