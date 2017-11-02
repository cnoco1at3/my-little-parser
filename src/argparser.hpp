/*
MIT License

Copyright (c) 2017 cnocobot

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef MY_LITTLE_ARG_PARSER_H
#define MY_LITTLE_ARG_PARSER_H


#include <assert.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <unordered_map>


namespace mylittleparser
{
    class ArgEntry
    {
    public:
        ArgEntry () : data_ (nullptr), size_ (0), nargs_ (0) { }


        ArgEntry (const ArgEntry& other)
            : size_ (other.size_), nargs_ (other.nargs_)
        {
            data_ = new unsigned char[size_ * nargs_];
            std::memcpy (data_, other.data_, size_ * nargs_);
        }


        ArgEntry (ArgEntry&& other)
            : size_ (other.size_), nargs_ (other.nargs_)
        {
            data_ = other.data_;
            other.data_ = nullptr;
        }


        ~ArgEntry () { if (data_) delete[] data_; }


        struct ArgProxy
        {
        public:
            template<typename T> operator T() { return entry_->get<T> (idx_); }


        private:
            friend class ArgEntry;


            ArgProxy (const ArgEntry* entry, int idx)
                : entry_ (entry), idx_ (idx) { }


            const ArgEntry* entry_;
            int idx_;
        };


        ArgProxy operator[](int idx) const { return ArgProxy (this, idx); }


        template<typename T>
        inline T get (int idx = 0) const
        {
            /*!
                \note This only provide basic type safety, we still don't have
                      the capability to know the type of the stored data.
            */
            assert (sizeof (T) == size_ && 0 <= idx && (int) nargs_ > idx);
            return *(reinterpret_cast<T*>(data_) + idx);
        }


    private:
        friend class ArgParser;


        template<typename T, int nargs> inline void resize ()
        {
            size_ = sizeof (T);
            nargs_ = nargs;
            if (data_) delete[] data_;
            data_ = new unsigned char[sizeof (T) * nargs];
        }


        template<typename T> inline void set (int idx, T&& val)
        {
            if (idx >= (int) nargs_) return;
            *(reinterpret_cast<T*>(data_) + idx) = val;
        }


        unsigned char* data_;
        size_t size_;
        size_t nargs_;
    };


    typedef std::unordered_map<std::string, ArgEntry> ParseArgs;


    class ArgParser
    {
    private:
        // base type quilifier
        template<typename T, int nargs = 0, class enable = void> struct Ty;

        // parser template
        template<typename T> struct PrsTy;


        template<bool required> int search (const char name[],
                                            const char abbr[]) const;


        template<typename U, int nargs> void parse (const char* name, int idx);


        void show_help () const
        {
            std::cout << "Usage ";
            for (std::pair<std::string, description_t> d : argd_)
                std::cout << d.second.name << " ";
            std::cout << "\n" << std::endl;
            for (std::pair<std::string, description_t> d : argd_)
            {
                std::cout << std::left << std::setw (12) << d.second.name << " ";
                if (d.second.abbr)
                    std::cout << std::left << std::setw (8) << d.second.abbr;
                else std::cout << std::setw (8) << "none ";
                if (d.second.help)
                    std::cout << std::left << d.second.help;
                std::cout << std::endl;
            }
            exit (0);
        }


    public:


        ArgParser (int argc = __argc, char** argv = __argv)
            : argc_ (argc), argv_ (argv), showh_ (false)
        {
            for (int i = 1; i < argc; ++i)
                if (argi_.find (argv[i]) == argi_.end ()) argi_[argv[i]] = i;
            if ((showh_ = search<false> ("--help", "-h") != argc_))
                argd_["--help"] = description_t ("--help",
                                                 "Show this message and exit",
                                                 "-h");
        }


        ~ArgParser () { }


        template <typename T, int nargs = 0, bool required = false>
        void add_argument (const char* name,
                           const char* help,
                           const char* abbr = nullptr,
                           typename std::decay<decltype(
                               PrsTy<typename
                               Ty<typename std::decay<T>::type, nargs>::type
                               >::parse
                               )>::type* = nullptr)
        {
            typedef typename std::decay<T>::type U;
            if (showh_)
                argd_[name] = description_t (name, help, abbr);
            else
                parse<U, nargs> (name, search<required> (name, abbr));
        }


        const ParseArgs& parse_args () const
        {
            if (showh_) show_help ();
            return parse_args_;
        }


    private:
        typedef struct description
        {
            description () { }
            description (const char* name,
                         const char* help,
                         const char* abbr = nullptr)
                : name (name), help (help), abbr (abbr)
            { }
            const char* name;
            const char* help;
            const char* abbr;
        } description_t;
        std::map<std::string, description_t> argd_;


        const int argc_;                            //!< arguments count
        char** argv_;                   //!< arguments values
        std::unordered_map<std::string, int> argi_; //!< arguments indices


        bool showh_;
        ParseArgs parse_args_;
    };


    // specialization for bool
    template<int nargs>
    struct ArgParser::Ty<bool, nargs,
        typename std::enable_if<nargs == 0>::type>
    {
        typedef bool type;
    };


    // other types
    template<typename T, int nargs>
    struct ArgParser::Ty<T, nargs,
        typename std::enable_if<
        !std::is_same<bool, T>::value && nargs >= 1>::type>
    {
        typedef T type;
    };


    /*!
        \note Useless placeholder here.
    */
    template<> struct ArgParser::PrsTy<bool>
    {
        static bool parse (const char*) { return true; }
    };


    template<> struct ArgParser::PrsTy<int>
    {
        static int parse (const char* str) { return std::atoi (str); }
    };


    template<> struct ArgParser::PrsTy<const char*>
    {
        static const char *parse (const char *str) { return str; }
    };


    template<bool required>
    int ArgParser::search (const char name[], const char abbr[]) const
    {
        int r (argc_);
        if (name && argi_.find (name) != argi_.end ()) r = argi_.at (name);
        if (abbr && argi_.find (abbr) != argi_.end ())
            r = std::min (r, argi_.at (abbr));
        return r;
    }


    template<>
    int ArgParser::search<true> (const char name[], const char abbr[]) const
    {
        int i = search<false> (name, abbr);
        if (argc_ <= i) throw this;
        return i;
    }


    template<typename U, int nargs>
    inline void ArgParser::parse (const char* name, int idx)
    {
        parse_args_[name].resize<U, nargs> ();
        for (int i = 1; nargs >= i && argc_ > idx + i; ++i)
            parse_args_[name].set<U> (
                i - 1,
                PrsTy <typename Ty<U, nargs>::type>::parse (argv_[idx + i]));
    }


    template<>
    inline void ArgParser::parse<bool, 0> (const char* name, int idx)
    {
        parse_args_[name].resize<bool, 1> ();
        parse_args_[name].set<bool> (0, idx < argc_);
    }
}


#endif
