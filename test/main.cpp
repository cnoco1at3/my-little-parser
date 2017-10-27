#include "../src/argparser.hpp"


int main (int argc, char** argv)
{
    mylittleparser::ArgumentParser parser (argc, argv);
    parser.add_argument<const int, 2> ("--aws", nullptr);
    parser.add_argument<const char*, 2> ("--sth", nullptr);
    mylittleparser::ParseArgs parse_args = parser.parse_args ();
    std::cout << parse_args["--sth"].get<const char*> (1);
    getchar ();
    return 0;
}