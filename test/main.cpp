#include "../src/argparser.hpp"
#include <iostream>


int main (int argc, char** argv)
{
    mylittleparser::ArgumentParser parser;
    parser.add_argument<int, 2> ("--aws", nullptr);
    parser.add_argument<const char*, 2> ("--sth", nullptr);
    parser.add_argument<bool> ("--world", nullptr);
    mylittleparser::ParseArgs parse_args = parser.parse_args ();
    std::cout << (parse_args["--world"].get<bool> (0) ? "true" : "false");
    std::cout << parse_args["--aws"].get<int> (1);
    getchar ();

    return 0;
}