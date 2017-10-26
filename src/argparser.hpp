#ifndef MY_LITTLE_ARG_PARSER_H
#define MY_LITTLE_ARG_PARSER_H


#include <fstream>


namespace mylittleparser
{
    class ArgumentParser
    {
    public:
        constexpr ArgumentParser (int argc, char** argv, char** envp = nullptr)
            : m_argc (argc), m_argv (argv), m_envp (envp)
        { }

        template<typename T>
        struct SupportedType { static const bool value = false; };

        template<> struct SupportedType<bool> { static const bool value = true; };
        template<> struct SupportedType<int> { static const bool value = true; };
        template<> struct SupportedType<char*> { static const bool value = true; };
        template<> struct SupportedType<const char*> { static const bool value = true; };

        template <typename T, int nargs = 0, bool required = false>
        typename std::enable_if<
            SupportedType<typename std::remove_const<T>::type>::value
        >::type
            add_argument (const char* name, const char* help, const char* abbr = nullptr)
        { 
            printf ("%s %s", name, help);
        }

    private:
        int m_argc;
        char** m_argv;
        char** m_envp;
    };
}


#endif
