#ifndef MY_LITTLE_ARG_PARSER_H
#define MY_LITTLE_ARG_PARSER_H


#include <fstream>
#include <unordered_map>
#include <string>


class ArgumentParser
{
public:
    constexpr ArgumentParser (int argc, char** argv, char** envp)
        : m_argc (argc), m_argv (argv), m_envp (envp)
    { }

    constexpr void addArgument (const char* name, const char* help = "")
    {
        return;
    }

    std::unordered_map<std::string, std::string> parseArgs ()
    {
        return std::unordered_map<std::string, std::string>();
    }

private:
    int m_argc;
    const char** m_argv;
    const char** m_envp;
};


#endif
