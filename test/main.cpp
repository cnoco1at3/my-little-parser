#include "../src/argparser.hpp"
#include <iostream>


int main (int argc, char** argv)
{
    mylittleparser::ArgumentParser parser(argc, argv);
    parser.add_argument<int, 2> ("--awssd", "no idea");
    parser.add_argument<const char*, 2> ("--sth", nullptr, "-s");
    parser.add_argument<bool> ("--world", nullptr);
    mylittleparser::ParseArgs parse_args = parser.parse_args ();
    std::cout << (parse_args["--world"].get<bool> (0) ? "true" : "false");
    const char* p = parse_args["--sth"][0];
    const char* q = parse_args["--sth"][1];
    std::cout << p << q;
    getchar ();

    return 0;
}