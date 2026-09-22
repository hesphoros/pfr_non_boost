/// Test the pfr library with a simple struct and demonstrate
/// accessing its fields using pfr::get and printing the entire struct using pfr::io.

/// Compile with: g++ -std=c++23 -I../include test1.cpp -o bin/test1
/// Author: hesphoros@gmail.com

#include "../include/pfr.hpp"
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

typedef struct person_s
{
    std::string     name;
    unsigned int    birth_year;
    std::string     email;
    std::string     gender;
    std::string     address;

} person_t;


template <typename T>
constexpr std::string_view type_name() {
    std::string_view name = __PRETTY_FUNCTION__;

    constexpr std::string_view prefix = "T = ";
    constexpr std::string_view suffix = "]";

    const auto begin = name.find(prefix);
    if (begin == std::string_view::npos)
        return {};

    const auto start = begin + prefix.size();
    const auto end = name.find(suffix, start);

    if (end == std::string_view::npos)
        return name.substr(start);

    return name.substr(start, end - start);
}

void get_tuple_from_struct_demo()
{

    struct foo { int a,b;}
    struct other {
        char c;
        foo nested;
    };

    other var{'A', {3, 4}};
    std::tuple<char, foo> t = pfr::structure_to_tuple(var);
    assert(std::get<0>(t) == 'A');// A = 65
    assert(pfr::eq(std::get<1>(t), foo{3, 4}));
}

void tuple_to_reference_demo()
{

    struct foo { int a, b; };
    struct other {
        char c;
        foo f;
    };

    other var{'A', {14, 15}};
    std::tuple<char&, foo&> t = pfr::structure_tie(var); // other
    std::get<1>(t) = foo{1, 2};

    std::cout << pfr::io(var.f); // Outputs: {1, 2}
}

int main()
{
    person_t p1{"hesphoros", 2008, "hesphoros@example.com", "male", "123 Main St"};
    
    std::cout << "Name: " << pfr::get<0>(p1) << std::endl;
    std::cout << "Birth Year: " << pfr::get<1>(p1) << std::endl;
    std::cout << "Email: " << pfr::get<2>(p1) << std::endl;
    std::cout << "Gender: " << pfr::get<3>(p1) << std::endl;
    std::cout << "Address: " << pfr::get<4>(p1) << std::endl;
    std::cout << pfr::io(p1) << std::endl;

    /// Get name of the fields using pfr::get_name and print them along with their values.
    std::cout << "Struct size: " << pfr::tuple_size<person_t>::value << std::endl;
    std::cout << "Struct fields with names: " << 
        pfr::get_name<0, person_t>() << ": " << pfr::get<0>(p1) << ", " <<
        pfr::get_name<1, person_t>() << ": " << pfr::get<1>(p1) << ", " <<
        pfr::get_name<2, person_t>() << ": " << pfr::get<2>(p1) << ", "<<
        pfr::get_name<3, person_t>() << ": " << pfr::get<3>(p1) << ", " <<
        pfr::get_name<4, person_t>() << ": " << pfr::get<4>(p1) << std::endl;


    // using for_each_filed_with_name
    std::cout << "Using for_each_filed_with_name \n"; 
    pfr::for_each_field_with_name(p1,[](std::string_view whatever, const auto& value){
        std::cout << whatever << ": " << value << std::endl;
    });

    struct my_struct {
        int firstl
        short second;
    };

    // Use near the type definition to define the whole set of operators for your type
    PFR_FUNCTIONS_FOR(my_struct); 
    assert(my_struct(1, 2) < my_struct(1, 3));
    

    // Iterate over fields of a variable and output index and
    // type of a variable.

    struct tag0{};
    struct tag1{};
    struct sample {
        tag0 a;
        tag1 b;
    };

    // Outputs:
    //  0: tag0
    //  1: tag1
    // boost::pfr::for_each_field(sample{}, [](const auto& field, std::size_t idx) {
    //     std::cout << '\n' << idx << ": "
    //         << boost::typeindex::type_id_runtime(field);
    // });
    std::size_t idx = 0;

    pfr::for_each_field(
        sample{},
        [&](const auto& field) {
            std::cout
                << '\n'
                << idx++
                << ": "
                << type_name<std::remove_cvref_t<decltype(field)>>();
        }
    );

}

