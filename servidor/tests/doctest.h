#ifndef DOCTEST_H
#define DOCTEST_H

#include <iostream>
#include <cstdlib>

// Minimal stub of doctest for local compilation checks in this repo's tests directory.
// Provides TEST_CASE and CHECK macros and a main when DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN is defined.

#define TEST_CASE(name) static void DOCTEST_TEST_##__LINE__(); static int DOCTEST_REG_##__LINE__ = (DOCTEST_TEST_##__LINE__(),0); static void DOCTEST_TEST_##__LINE__()

#define CHECK(expr) do { if (!(expr)) { std::cerr << "CHECK failed: " << #expr << std::endl; std::exit(1); } } while(0)

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
int main() { return 0; }
#endif

#endif // DOCTEST_H
