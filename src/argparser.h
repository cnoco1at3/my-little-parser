#ifndef SRC_ARGPARSER_H_
#define SRC_ARGPARSER_H_


#include <assert.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>

#define ARGPARSER_TMPL template<    \
typename T,     \
int nargs,      \
bool required   \
>

namespace mylittleparser {

/* Declarations */
class basic_argument;
typedef std::unordered_map<std::string, basic_argument> parsed_arguments;

namespace details {
ARGPARSER_TMPL
struct arg_maker;
}   //  namespace details

class basic_argument {
public:
    typedef struct dscrpt {
        dscrpt() : required(false) { }
        dscrpt(const std::string& name,
               const std::string& help,
               const std::string& flag,
               bool required)
            : name(name), help(help), flag(flag), required(required) { }
        std::string name;
        std::string help;
        std::string flag;
        bool required;
    } dscrpt_t;

    basic_argument()
        : size_(0), nargs_(0), data_(nullptr) { }

    basic_argument(size_t size, size_t nargs, dscrpt_t description)
        : description_(description), size_(size), nargs_(nargs) {
        data_ = std::unique_ptr<unsigned char[]>(new unsigned char[size_ * nargs_]());
    }

    basic_argument(const basic_argument& other)
        : description_(other.description_), size_(other.size_), nargs_(other.nargs_) {
        data_ = std::unique_ptr<unsigned char[]>(new unsigned char[size_ * nargs_]);
        std::memcpy(data_.get(), other.data_.get(), size_ * nargs_);
    }

    basic_argument(basic_argument&& other)
        : description_(other.description_), size_(other.size_), nargs_(other.nargs_) {
        data_.swap(other.data_);
    }

private:
    struct proxy {
    public:
        template<typename T> operator T() { return entry_->get<T>(idx_); }

    private:
        friend class basic_argument;
        proxy(const basic_argument* entry, int idx)
            : entry_(entry), idx_(idx) { }

        const basic_argument* entry_;
        int idx_;
    };

public:
    proxy operator[](int idx) const { return proxy(this, idx); }

    template<typename T>
    inline T get(int idx = 0) const {
        /*!
            \note This only provide basic type safety, we still don't have
                  the capability to know the type of the stored data.
        */
        assert(sizeof(T) == size_ && 0 <= idx && static_cast<int>(nargs_) > idx);
        return *(reinterpret_cast<T*>(data_.get()) + idx);
    }

    dscrpt_t description() const {
        return description_;
    }

private:
    ARGPARSER_TMPL
        friend struct details::arg_maker;
    template<typename T>
    inline void set(int idx, T val) {
        *(reinterpret_cast<T*>(data_.get()) + idx) = val;
    }

    dscrpt_t description_;
    size_t size_;
    size_t nargs_;
    std::unique_ptr<unsigned char[]> data_;
};

namespace details {
/*!
Is string helper
*/
template<typename T>
struct is_string
    : public std::integral_constant<
    bool,
    std::is_same<char*, typename std::decay<T>::type>::value
    || std::is_same<const char*, typename std::decay<T>::type>::value> { };

template<>
struct is_string<std::string> : std::true_type { };

/*!
Type verfier
*/
template<typename T, class enable = void>
struct is_valid_type : std::false_type { };

template<typename T>
struct is_valid_type<
    T,
    typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value >::type>
    : std::true_type { };

template<typename T>
struct is_valid_type<T, typename std::enable_if<std::is_floating_point<T>::value >::type>
    : std::true_type { };

template<typename T>
struct is_valid_type<T, typename std::enable_if<is_string<T>::value>::type>
    : std::true_type { };

/*!
Type and number of arguments combination verifier
*/
template<typename T, int nargs, class enable = void>
struct is_valid_argument {
    static const bool value = false;
};

template<>
struct is_valid_argument <bool, 0> {
    static const bool value = true;
};

template<typename T, int nargs>
struct is_valid_argument<T, nargs, typename std::enable_if<nargs >= 1>::type> {
    static const bool value = is_valid_type<T>::value;
};

/*!
Type parser
*/
template<typename T, class enable = void> struct parser;

template<>
struct parser<char*> {
    static char* parse(char* s) {
        return s;
    }
};

template<>
struct parser<std::string> {
    static std::string parse(char* s) {
        return std::string(s);
    }
};

template<typename T>
struct parser<T, typename std::enable_if<std::is_integral<T>::value>::type> {
    static T parse(char* s) {
        return static_cast<T>(std::atoll(s));
    }
};

template<typename T>
struct parser<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
    static T parse(char* s) {
        return static_cast<T>(std::atof(s));
    }
};

struct args_descriptor {
    std::unordered_map<std::string, int> argi;
    int argc;
    char** argv;

    args_descriptor(int c, char** v) : argc(c), argv(v) {
        for (int i = 1; i < argc; ++i) argi[argv[i]] = i;
    }

    int find(const std::string& name, const std::string& flag) const {
        int r1 = argi.find(name) != argi.end() ? argi.at(name) : -1;
        int r2 = argi.find(flag) != argi.end() ? argi.at(flag) : -1;
        return std::max(r1, r2);
    }
};

ARGPARSER_TMPL
struct arg_maker {
    static const basic_argument make(const args_descriptor& argd,
                                     const std::string& name,
                                     const std::string& help,
                                     const std::string& flag) {
        basic_argument arg(sizeof(T), nargs, basic_argument::dscrpt_t(name, help, flag, required));
        int idx = argd.find(name, flag);
        if (idx != -1) {
            for (int i = 0; i < nargs && idx + i + 1 < argd.argc; ++i)
                arg.set(i, parser<T>::parse(argd.argv[idx + i + 1]));
        }
        return arg;
    }
};

template<typename T, bool required>
struct arg_maker<T, 0, required> {
    static const basic_argument make(const args_descriptor& argd,
                                     const std::string& name,
                                     const std::string& help,
                                     const std::string& flag) {
        basic_argument arg(1, 1, basic_argument::dscrpt_t(name, help, flag, required));
        int idx = argd.find(name, flag);
        arg.set<bool>(0, idx != -1);
        return arg;
    }
};

ARGPARSER_TMPL
struct arg_validator {
    static void validate(const args_descriptor& argd,
                         const std::string& name, const std::string& flag) {
        static_assert(details::is_valid_argument<typename std::decay<T>::type, nargs>::value,
                      "Invalid combination of type and number of arguments");
        if (name.empty())
            throw std::domain_error("Argument name could not be empty");
        if (name == "--help" || flag == "-h")
            throw std::domain_error("Argument --help or -h is reserved.");
    }
};

template<typename T, int nargs>
struct arg_validator<T, nargs, true> {
    static void validate(const args_descriptor& argd,
                         const std::string& name, const std::string& flag) {
        arg_validator<T, nargs, false>::validate(argd, name, flag);
        if (argd.find(name, flag) == -1)
            throw std::domain_error(std::string("Required argument [") + name + "] not provided.");
    }
};

}   // namespace details

class argument_parser {
public:
    explicit argument_parser(std::string prog = "",
                             std::string description = "",
                             std::string epilog = "")
        : argd_(__argc, __argv),
        prog_(prog),
        description_(description),
        epilog_(epilog) {
        init();
    }

    explicit argument_parser(int argc, char** argv,
                             std::string prog = "",
                             std::string description = "",
                             std::string epilog = "")
        : argd_(argc, argv),
        prog_(prog),
        description_(description),
        epilog_(epilog) {
        init();
    }

    ~argument_parser() { }

    template<typename T, int nargs = 0, bool required = false>
    void add_argument(std::string name, std::string flag, std::string help) {
        typedef typename std::decay<T>::type U;
        if (!help_) {
            details::arg_validator<T, nargs, required>::validate(argd_, name, flag);
            prsd_args_.emplace(
                name,
                details::arg_maker<U, nargs, required>::make(argd_, name, help, flag));
        }
        else {
            details::arg_validator<T, nargs, false>::validate(argd_, name, flag);
            prsd_args_.emplace(
                name,
                details::arg_maker<U, 0, false>::make(argd_, name, help, flag));
        }
    }

    template<typename T, int nargs = 0, bool required = false>
    void add_argument(std::string name, std::string help) {
        add_argument<T, nargs, required>(name, "", help);
    }

    const parsed_arguments& parse_args() const {
        if (help_) {
            print_help();
            exit(0);
        }
        return prsd_args_;
    }

private:
    void init() {
        help_ = argd_.find(std::string("--help"), std::string("-h")) != -1;
        if (prog_.empty()) prog_ = __argv[0];
    }

    void print_help() const {
        std::cout
            << "NAME " << prog_ << "\n\n"
            << "SYNOPSIS " << prog_ << " ";
        for (auto kv : prsd_args_) {
            std::string name = kv.second.description().required ?
                kv.second.description().name
                : "[" + std::string(kv.second.description().name) + "]";
            std::cout << name << " ";
        }
        std::cout << "\n\n";
        if (!description_.empty())
            std::cout << description_ << "\n\n";
        print_description(basic_argument::dscrpt_t(
            "--help",
            "Show this messsage and exit",
            "-h",
            false
        ));
        for (auto kv : prsd_args_)
            print_description(kv.second.description());
        if (!epilog_.empty())
            std::cout << "\n\n" << epilog_ << std::endl;
        getchar();
    }

    static void print_description(basic_argument::dscrpt_t&& description) {
        std::cout << std::left << std::setw(12) << description.name << " ";
        if (!description.flag.empty())
            std::cout << std::left << std::setw(8) << description.flag << " ";
        else
            std::cout << std::setw(8) << "none" << " ";
        if (!description.help.empty())
            std::cout << std::left << description.help;
        std::cout << std::endl;
    }

    details::args_descriptor argd_;
    parsed_arguments prsd_args_;

    std::string prog_;
    std::string description_;
    std::string epilog_;

    bool help_;
};

}   // namespace mylittleparser

#endif  // SRC_ARGPARSER_H_
