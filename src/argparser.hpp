#ifndef MY_LITTLE_ARG_PARSER_H
#define MY_LITTLE_ARG_PARSER_H


#include <unordered_map>


namespace mylittleparser
{
    class ParseEntry
    {
    public:


        ParseEntry () : data_ (nullptr), size_ (0), nargs_ (0) { }


        ~ParseEntry () { if (data_) delete[] data_; }


        template<typename T>
        inline T get (int idx)
        {
            /*!
                \note This only provide basic type safety, we still don't have
                      the capability to know the type of the stored data.
            */
            if (sizeof (T) != size_ || 0 > idx || (int) nargs_ <= idx)
                throw this;
            return reinterpret_cast<T*>(data_)[idx];
        }


    private:


        friend class ArgumentParser;


        template<typename T, int nargs> inline void resize ()
        {
            size_ = sizeof (T);
            nargs_ = nargs;
            if (data_) delete[] data_;
            data_ = new unsigned char[size_ * nargs_] ();
        }


        template<typename T> inline void set (int idx, const T& val)
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
    private:


        template<int type> struct ParseType;


        // base type
        template<typename T, int nargs = 0, class enable = void> struct TypeId
        {
            static const int value = -1;
        };


        // bool type
        template<int nargs>
        struct TypeId <bool, nargs, typename std::enable_if<nargs == 0>::type>
        {
            static const int value = 0;
        };


        // int type
        template<int nargs>
        struct TypeId <int, nargs, typename std::enable_if<nargs >= 1>::type>
        {
            static const int value = 1;
        };


        // c style string type
        template<int nargs>
        struct TypeId <char *, nargs, typename std::enable_if<nargs >= 1>::type>
        {
            static const int value = 2;
        };


        // c style const string type
        template<int nargs>
        struct TypeId <const char *, nargs,
            typename std::enable_if<nargs >= 1>::type>
        {
            static const int value = 3;
        };


        template<bool required> int search (const char* name, const char* abbr);


        template<typename U, int nargs> void parse (const char* name, int idx);


        int argc_;                                  //!< arguments count
        char** argv_;                               //!< arguments values
        std::unordered_map<std::string, int> argi_; //!< arguments indices
        bool help_;
        ParseArgs parse_args_;


    public:


        ArgumentParser (int argc = __argc, char** argv = __argv)
            : argc_ (argc), argv_ (argv), help_ (false)
        {
            for (int i = 0; i < argc; ++i) argi_[argv[i]] = i;
            help_ = search<false> ("--help", "-h") != argc_;
        }


        ~ArgumentParser () { }


        template <typename T, int nargs = 0, bool required = false>
        typename std::enable_if <
            TypeId<typename std::remove_cv<T>::type, nargs>::value >= 0>::type
            add_argument (const char* name,
                          const char* help,
                          const char* abbr = nullptr)
        {
            typedef typename std::remove_cv<T>::type U;

            parse<U, nargs> (name, search<required> (name, abbr));
        }


        ParseArgs parse_args ()
        {
            if (help_) exit (0);
            return parse_args_;
        }

    };


    template<> struct ArgumentParser::ParseType<
        ArgumentParser::TypeId<bool, 0>::value>
    {
        static bool parse (const char* str) { return true; }
    };


    template<>
    struct ArgumentParser::ParseType<ArgumentParser::TypeId<int, 1>::value>
    {
        static int parse (const char* str) { return std::atoi (str); }
    };


    template<> struct ArgumentParser::ParseType<
        ArgumentParser::TypeId<char*, 1>::value>
    {
        static char *parse (const char *str)
        {
            return const_cast<char*>(str);
        }
    };


    template<> struct ArgumentParser::ParseType<
        ArgumentParser::TypeId<const char*, 1>::value>
    {
        static const char *parse (const char *str) { return str; }
    };


    template<bool required>
    int ArgumentParser::search (const char* name, const char* abbr)
    {
        return argi_.find (name) != argi_.end () ? argi_.at (name) : argc_;
    }


    template<>
    int ArgumentParser::search<true> (const char* name, const char* abbr)
    {
        if (argi_.find (name) == argi_.end ()) throw this;
        return argi_.at (name);
    }


    template<typename U, int nargs>
    inline void ArgumentParser::parse (const char* name, int idx)
    {
        parse_args_[name].resize<U, nargs> ();
        for (int i = 1; i <= nargs && idx + i < argc_; ++i)
            parse_args_[name].set<U> (
                i - 1,
                ParseType <TypeId<U, nargs>::value>::parse (
                    argv_[idx + i]));
    }


    template<>
    inline void ArgumentParser::parse<bool, 0> (const char* name, int idx)
    {
        parse_args_[name].resize<bool, 1> ();
        parse_args_[name].set<bool> (0, idx < argc_);
    }
}


#endif
