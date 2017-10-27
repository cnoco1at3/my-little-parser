#ifndef MY_LITTLE_ARG_PARSER_H
#define MY_LITTLE_ARG_PARSER_H


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
            if (sizeof (T) != size_ || 0 > idx || (int) nargs_ <= idx)
                throw this;
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
            : argc_ (argc), argv_ (argv), help_(false)
        { 
            help_ = search<false> ("--help", "-h") < argc_;
        }


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

            int idx = search<required> (name, abbr);
            parse<U, nargs> (name, idx);
        }


        ParseArgs parse_args () 
        { 
            if (help_) exit (0);
            return parse_args_; 
        }


    private:


        template<bool required> int search (const char* name, const char* abbr)
        {
            int idx = 1;
            while (idx < argc_ && strcoll (name, argv_[idx]) != 0) idx++;
            return idx;
        }


        template<> int search<true> (const char* name, const char* abbr)
        {
            int idx = search<false> (name, abbr);
            if (argc_ == idx) { throw this; }
            return idx;
        }


        template<typename U, int nargs> void parse (const char* name, int idx)
        {
            parse_args_[name].resize<U> (nargs);
            for (int i = 1; i <= nargs && idx + i < argc_; ++i)
                parse_args_[name].set<U> (
                    i - 1, ParseType<U, nargs>::parse (argv_[idx + i]));
        }


        template<> void parse<bool, 0> (const char* name, int idx)
        {
            parse_args_[name].resize<bool> (1);
            parse_args_[name].set<bool> (0, idx < argc_);
        }


        template<typename T, int nargs> struct ParseType { };


        template<int nargs> struct ParseType<bool, nargs>
        {
            static typename std::enable_if<nargs == 0, bool>::type parse () { }
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
        bool help_;
        ParseArgs parse_args_;
    };
}


#endif
