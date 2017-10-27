#ifndef MY_LITTLE_ARG_PARSER_H
#define MY_LITTLE_ARG_PARSER_H


#include <fstream>
#include <iostream>
#include <unordered_map>


namespace mylittleparser
{
    class ParseEntry
    {
    public:


        ParseEntry () : data_ (nullptr), size_ (0), nargs_ (0) { }


        ~ParseEntry () { if (data_) delete[] data_; data_ = nullptr; }


        template<typename T>
        inline T operator[](int idx) { return get<T> (idx); }


        template<typename T>
        inline T get (int idx)
        {
            if (sizeof (T) != size_ || 0 > idx || nargs_ <= idx) throw this;
            return reinterpret_cast<T*>(data_)[idx];
        }


    private:


        friend class ArgumentParser;


        template<typename T>
        inline void resize (size_t nargs)
        {
            size_ = sizeof (T);
            nargs_ = nargs;
            if (data_) delete[] data_;
            data_ = new unsigned char[size_ * nargs_];
        }


        template<typename T>
        inline void set (int idx, const T& val)
        {
            reinterpret_cast<T*>(data_)[idx] = val;
        }


        unsigned char* data_;
        size_t size_;
        size_t nargs_;
    };


    typedef std::unordered_map<std::string, ParseEntry> ParseArgs;


    class ArgumentParser
    {
    public:


        ArgumentParser (int argc, char** argv)
            : argc_ (argc), argv_ (argv)
        { }


        ~ArgumentParser ()
        {

        }


        template <typename T, int nargs = 0, bool required = false>
        void add_argument (const char* name, const char* help,
                           const char* abbr = nullptr,
                           typename std::decay<decltype(
                               ParseType<std::remove_cv<T>::type,
                               nargs>::parse)>::type* = nullptr)
        {
            typedef std::remove_cv<T>::type U;

            int idx;
            for (idx = 1; idx < argc_; ++idx)
                if (strcoll (name, argv_[idx]) == 0) break;

            if (required && argc_ == idx) { throw name; }

            parse_args_[name].resize<U> (nargs);
            for (int i = 1; i <= nargs && idx + i < argc_; ++i)
                parse_args_[name].set<U> (
                    i - 1, ParseType<U, nargs>::parse (argv_[idx + i]));
        }


        ParseArgs parse_args () { return std::move (parse_args_); }


    private:


        template<typename T, int nargs> struct ParseType { };


        template<int nargs> struct ParseType<bool, nargs>
        {
            static typename std::enable_if<nargs == 0, bool>::type
                parse (const char* str)
            {
                return true;
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
        std::unordered_map<std::string, ParseEntry> parse_args_;
    };
}


#endif
