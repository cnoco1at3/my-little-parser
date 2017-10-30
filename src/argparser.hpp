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


        ~ArgEntry () { if (data_ != nullptr) delete[] data_; }


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


        friend class ArgumentParser;


        template<typename T, int nargs> inline void resize ()
        {
            size_ = sizeof (T);
            nargs_ = nargs;
            if (data_) delete[] data_;
            data_ = new unsigned char[nargs_ * size_];
        }


        template<typename T> inline void set (int idx, const T& val)
        {
            *(reinterpret_cast<T*>(data_) + idx) = val;
        }


        unsigned char* data_;
        size_t size_;
        size_t nargs_;
    };


    typedef std::unordered_map<std::string, ArgEntry> ParseArgs;


    class ArgumentParser
    {
    private:


        template<int type> struct ParseTy;


        // base type
        template<typename T, int nargs = 0, class enable = void> struct TyId
        {
            static const int value = -1;
        };


        // bool type
        template<int nargs>
        struct TyId <bool, nargs, typename std::enable_if<nargs == 0>::type>
        {
            static const int value = 0;
        };


        // int type
        template<int nargs>
        struct TyId <int, nargs, typename std::enable_if<nargs >= 1>::type>
        {
            static const int value = 1;
        };


        // c style string type
        template<int nargs>
        struct TyId <char *, nargs, typename std::enable_if<nargs >= 1>::type>
        {
            static const int value = 2;
        };


        // c style const string type
        template<int nargs>
        struct TyId <const char *, nargs,
            typename std::enable_if<nargs >= 1>::type>
        {
            static const int value = 3;
        };


        inline int search (const char* key)
        {
            return key && argi_.find (key) != argi_.end () ?
                argi_[key] : argc_;
        }
        template<bool required>
        inline int search (const char* name, const char* abbr);


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


        ArgumentParser (int argc = __argc, char** argv = __argv)
            : argc_ (argc), argv_ (argv), showh_ (false)
        {
            for (int i = 1; i < argc; ++i)
                if (argi_.find (argv[i]) == argi_.end ()) argi_[argv[i]] = i;
            if ((showh_ = search<false> ("--help", "-h") != argc_))
                argd_["--help"] = description_t ("--help",
                                                 "Show this message and exit",
                                                 "-h");
        }


        ~ArgumentParser () { }


        template <typename T, int nargs = 0, bool required = false>
        typename std::enable_if <
            TyId<typename std::remove_cv<T>::type, nargs>::value >= 0>::type
            add_argument (const char* name,
                          const char* help,
                          const char* abbr = nullptr)
        {
            typedef typename std::remove_cv<T>::type U;
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


        int argc_;                                  //!< arguments count
        char** argv_;                               //!< arguments values
        std::unordered_map<std::string, int> argi_; //!< arguments indices


        bool showh_;
        ParseArgs parse_args_;
    };


    /*!
        \note Useless placeholder here.
    */
    template<> struct ArgumentParser::ParseTy<
        ArgumentParser::TyId<bool, 0>::value>
    {
        static bool parse (const char*) { return true; }
    };


    template<>
    struct ArgumentParser::ParseTy<ArgumentParser::TyId<int, 1>::value>
    {
        static int parse (const char* str) { return std::atoi (str); }
    };


    template<> struct ArgumentParser::ParseTy<
        ArgumentParser::TyId<char*, 1>::value>
    {
        static char *parse (const char *str)
        {
            return const_cast<char*>(str);
        }
    };


    template<> struct ArgumentParser::ParseTy<
        ArgumentParser::TyId<const char*, 1>::value>
    {
        static const char *parse (const char *str) { return str; }
    };


    template<bool required>
    inline int ArgumentParser::search (const char* name, const char* abbr)
    {
        return std::min (search (name), search (abbr));
    }


    template<>
    inline int ArgumentParser::search<true> (const char* name, const char* abbr)
    {
        int i = search<false> (name, abbr); assert (argc_ > i);
        return i;
    }


    template<typename U, int nargs>
    inline void ArgumentParser::parse (const char* name, int idx)
    {
        parse_args_[name].resize<U, nargs> ();
        for (int i = 1; i <= nargs && idx + i < argc_; ++i)
            parse_args_[name].set<U> (
                i - 1,
                ParseTy <TyId<U, nargs>::value>::parse (argv_[idx + i]));
    }


    template<>
    inline void ArgumentParser::parse<bool, 0> (const char* name, int idx)
    {
        parse_args_[name].resize<bool, 1> ();
        parse_args_[name].set<bool> (0, idx < argc_);
    }
}


#endif
