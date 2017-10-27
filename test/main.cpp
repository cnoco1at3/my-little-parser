#include "../src/argparser.hpp"


int main (int argc, char** argv)
{
    mylittleparser::ArgumentParser parser (argc, argv);
    parser.add_argument<bool, 0> (nullptr, nullptr, "");
    parser.add_argument<int, 0> (nullptr, nullptr);
    getchar ();
    return 0;
}