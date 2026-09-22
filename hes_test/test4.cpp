/// @file test4.cpp
/// @brief Demonstrate using a member pointer as a template parameter and printing its name.
/// Compile with: g++ -std=c++20  test4.cpp -o bin/t4
/// output: void print() [member_ptr = &s.the_member_name]

#include <iostream>

template <auto member_ptr>
void print() {
    // Print the name of the member pointer using __PRETTY_FUNCTION__
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

struct S {
    int the_member_name;
} s;

int main() {
    print<&s.the_member_name>();
}