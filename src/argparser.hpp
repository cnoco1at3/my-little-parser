#ifndef MY_LITTLE_ARG_PARSER_H
#define MY_LITTLE_ARG_PARSER_H


#include <fstream>
#include <unordered_map>


namespace mylittleparser
{
    class ArgumentParser
    {
    public:
        constexpr ArgumentParser (int argc, char** argv)
            : argc_ (argc), argv_ (argv)
        { }

        template <typename T, int nargs = 0, bool required = false>
        void add_argument (const char* name, const char* help,
                           const char* abbr = nullptr,
                           typename std::decay<decltype(ParseType<T, nargs>::parse)>::type* = nullptr)
        {
            ParseType<T, nargs>::parse ("");
        }

    private:
        template<typename T, int nargs> struct ParseType { };

        template<int nargs> struct ParseType<bool, nargs>
        {
            static typename std::enable_if<nargs == 0, bool>::type
                parse (const char* str)
            {
                return str != nullptr;
            }
        };
        template<int nargs> struct ParseType<int, nargs>
        {
            static typename std::enable_if<nargs >= 1, int>::type
                parse (const char* str)
            {
                return std::atoi (str);
            }
        };
        template<int nargs> struct ParseType<char*, nargs>
        {
            static typename std::enable_if<nargs >= 1, char*>::type
                parse (const char* str)
            {
                return const_cast<char*>(str);
            }
        };
        template<int nargs> struct ParseType<const char*, nargs>
        {
            static typename std::enable_if<nargs >= 1, const char*>::type
                parse (const char* str)
            {
                return str;
            }
        };


        int argc_;
        char** argv_;
    };
}


#endif
