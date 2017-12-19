#include "../src/argparser.h"
#include <iostream>


int main(int argc, char** argv) {
    cnocobot::argument_parser parser;
    parser.add_argument<int, 2>("--awssd", "Random option");
    parser.add_argument<std::string, 2>("--sth", "-s", "Try somthing");
    parser.add_argument<bool>("--world", "World file");
    cnocobot::parsed_arguments parse_args = parser.parse_args();
    std::cout << (parse_args["--world"].get<bool>(0) ? "true" : "false") << std::endl;
    std::string p = parse_args["--sth"][0];
    std::string q = parse_args["--sth"][1];
    std::cout << p << std::endl << q << std::endl;

    getchar();

    return 0;
}
