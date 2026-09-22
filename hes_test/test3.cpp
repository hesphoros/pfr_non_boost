/// @brief Using the C++20 module version of PFR to reflect a simple aggregate type.
/// Compile with:
/// clang++ -std=c++20 -fmodule-file=pfr=../modules/boost_pfr.pcm ../modules/boost_pfr.pcm -I../include test3.cpp -o bin/test3

#include <iostream>
#include <string>

#include <iostream>
#include <string>

import pfr;

struct some_person {
    std::string name;
    unsigned birth_year;
};

int main() {
    some_person val{"hesphoros", 2008};

    std::cout << pfr::get<0>(val)                // No macro!
        << " was born in " << pfr::get<1>(val);  // Works with any aggregate!
        
    std::cout << '\n' << pfr::io(val);           // Outputs: {"hesphoros", 2008}
    std::cout << "\n." << pfr::get_name<0, some_person>()
        << '=' << val.name << '\n';                     // Outputs: .name=hesphoros
}
