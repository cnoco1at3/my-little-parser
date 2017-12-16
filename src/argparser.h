#ifndef SRC_ARGPARSER_H_
#define SRC_ARGPARSER_H_


#include <assert.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>
#include <utility>
#include <set>
#include <map>
#include <unordered_map>


namespace mylittleparser {

namespace details {
/*!
Type verfier
*/
template<typename T, class enable = void>
struct is_valid_type {
    static const bool value = false;
};

template<typename T>
struct is_valid_type<
    T,
    typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value >::type> {
    static const bool value = true;
};

template<typename T>
struct is_valid_type<T, typename std::enable_if<std::is_floating_point<T>::value >::type> {
    static const bool value = true;
};

template<typename T>
struct is_valid_type<T, typename std::enable_if<std::is_same<T, char*>::value>::type> {
    static const bool value = true;
};

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
}   // namespace details

class basic_argument {
public:
    typedef struct dscrpt {
        dscrpt() : name(nullptr), help(nullptr), abbr(nullptr), required(false) { }
        dscrpt(const char* name, const char* help, bool required, const char* abbr = nullptr)
            : name(name), help(help), required(required), abbr(abbr) { }
        const char* name;
        const char* help;
        const char* abbr;
        bool required;
    } dscrpt_t;

    basic_argument()
        : data_(nullptr), size_(0), nargs_(0) { }

    basic_argument(size_t size, size_t nargs, dscrpt_t description)
        : size_(size), nargs_(nargs), description_(description) {
        data_ = new unsigned char[size * nargs];
    }

    basic_argument(const basic_argument& other)
        : size_(other.size_), nargs_(other.nargs_), description_(other.description_) {
        data_ = new unsigned char[size_ * nargs_];
        std::memcpy(data_, other.data_, size_ * nargs_);
    }

    basic_argument(basic_argument&& other)
        : size_(other.size_), nargs_(other.nargs_), description_(other.description_) {
        data_ = other.data_;
        other.data_ = nullptr;
    }

    ~basic_argument() { if (data_) delete[] data_; }

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
        assert(sizeof(T) == size_ && 0 <= idx
               && static_cast<int>(nargs_) > idx);
        return *(reinterpret_cast<T*>(data_) + idx);
    }

private:
    friend class argument_parser;
    template<typename T>
    inline void set(int idx, T val) {
        *(reinterpret_cast<T*>(data_) + idx) = val;
    }

    dscrpt_t description_;
    size_t size_;
    size_t nargs_;
    void* data_;
};
typedef std::unordered_map<std::string, basic_argument> parsed_arguments;

class argument_parser {
public:
    explicit argument_parser(int argc = __argc, char** argv = __argv)
        : argc_(argc), argv_(argv) {
        for (int i = 1; i < argc; ++i)
            if (argi_.find(argv[i]) == argi_.end()) argi_[argv[i]] = i;
        prsd_args_.emplace(std::string("--help"),
                           basic_argument(1, 1,
                                          basic_argument::dscrpt_t("--help",
                                                                   "Show this message and exit",
                                                                   false,
                                                                   "-h")));
    }

    ~argument_parser() { }

    template <typename T, int nargs = 0, bool required = false>
    void add_argument(const char* name, const char* help, const char* abbr = nullptr) {
        validate<T, nargs, required>(name, abbr);
        typedef typename std::decay<T>::type U;
        argument_adder<U, nargs, required>::add(this, name, help, abbr);
    }

    const parsed_arguments& parse_args() const {
        if (search("--help", "-h") != argc_) {
            help();
            exit(0);
        }
        return prsd_args_;
    }

private:
    template<typename T, int nargs, bool required>
    struct argument_adder {
        static const void add(argument_parser* self,
                              const char* name, const char* help, const char* abbr) {
            basic_argument arg(sizeof(T), nargs, basic_argument::dscrpt_t(name, help, required, abbr));
            int idx = self->search(name, abbr);
            if (idx != self->argc_) {
                for (int i = 0; i < nargs && idx + i + 1 < self->argc_; ++i)
                    arg.set(i, details::parser<T>::parse(self->argv_[idx + i + 1]));
                self->prsd_args_.emplace(name, arg);
            }
        }
    };

    template<typename T, bool required>
    struct argument_adder<T, 0, required> {
        static const void add(argument_parser* self,
                              const char* name, const char* help, const char* abbr) {
            basic_argument arg(1, 1, basic_argument::dscrpt_t(name, help, required, abbr));
            int idx = self->search(name, abbr);
            arg.set<bool>(0, idx != self->argc_);
            self->prsd_args_.emplace(name, arg);
        }
    };

    template <typename T, int nargs, bool required>
    void validate(const char*name, const char* abbr) {
        static_assert(details::is_valid_argument<std::decay<T>::type, nargs>::value,
                      "Invalid combination of type and number of arguments");
        if (name == nullptr) {
            throw std::domain_error("Argument name could not be null");
            return;
        }
        if (strcmp(name, "--help") == 0
            || (abbr != nullptr && strcmp(abbr, "-h") == 0)) {
            throw std::domain_error("Argument --help or -h is reserved.");
            return;
        }
        if (!validate_required<required>(name, abbr)) {
            throw std::domain_error(std::string("Required argument [") + name + "] not provided.");
        }
    }

    template<bool required> bool validate_required(const char* name, const char* abbr) {
        return true;
    }

    template<> bool validate_required<true>(const char* name, const char* abbr) {
        return !(search(name, abbr) == argc_);
    }

    int search(const char* name, const char* abbr) const {
        int r(argc_);
        if (name && argi_.find(name) != argi_.end()) {
            r = argi_.at(name);
            return r;
        }
        if (abbr && argi_.find(abbr) != argi_.end()) {
            r = argi_.at(abbr);
            return r;
        }
        return r;
    }

    void help() const {
        std::cout << "Usage ";
        for (auto kv : prsd_args_) {
            std::string name = kv.second.description_.required ?
                kv.second.description_.name
                : "[" + std::string(kv.second.description_.name) + "]";
            std::cout << name << " ";
        }
        std::cout << "\n" << std::endl;
        for (auto kv : prsd_args_) {
            basic_argument::dscrpt_t& description = kv.second.description_;
            std::cout << std::left << std::setw(12) << description.name << " ";
            if (description.abbr)
                std::cout << std::left << std::setw(8) << description.abbr;
            else
                std::cout << std::setw(8) << "none ";
            if (description.help)
                std::cout << std::left << description.help;
            std::cout << std::endl;
        }
    }

    const int argc_;                                //!< arguments count
    char** argv_;                                   //!< arguments values
    std::unordered_map<std::string, int> argi_;     //!< arguments indices
    parsed_arguments prsd_args_;
};

}   // namespace mylittleparser


#endif  // SRC_ARGPARSER_H_
