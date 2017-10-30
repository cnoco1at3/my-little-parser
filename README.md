# My Little Parser


**My Little Parser** is an ongoing header only C++ lib for ini config files and command line arguments parsing.


## Requirements


C++ version later than 11 is expected, tested with **MSVC 14 (Visual Studio 2015)** on Windows and **LLVM 5.0.0** on Linux. Please feel free to kindly test them on Mac OS, but since I don't own any Apple device this condition might not change soon. Some features are still relying on **STL**, but I guess we are removing these dependencies in the near future.


## How To


These libraries are **Header-Only**, so simply include them should be everything you need to do.


## Argument Parser


For now we only have limited number of interfaces. These examples should demostrate all the features we have.


```c++
// Input: <name-of-your-executable> --hello world -s 1 2 3 --flag

int main (int argc, char** argv)
{
    using mylittleparser;

    ArgumentParser parser = ArgumentParser();   // With no arguments provided, the parser would automatically
                                                // take __argc and __argv as input.

    /*
     * add_argument<typename T, int nargs, bool required>(const char* name, const char* help, const char* abbr)
     *
     * params: 
     *    typename  T:          The type of the incoming argument
     *         int  nargs:      Numbers of elements of this argument
     *        bool  required:   True if this argument is manditory
     * const char*  name:       The name of this argument
     * const char*  help:       The help information for the argument
     * const char*  abbr:       Shortcut
     */
    parser.add_argument<const char*, 1, true>("--hello", "Echo the string");    // True required flag to indicate
                                                                                // this argument is manditory.

    parser.add_argument<int, 3>("--someint", "Some integer", "-s");             // Use more multiple elements for
                                                                                // single argument, and work with 
                                                                                // shortcut.

    parser.add_argument<bool, 0>("--flag", "Manditory")                         // Be careful that, bool type can
                                                                                // only accept 0 elements.

    ParseArgs parsed = parser.parse_args();

    /*
     * There is two ways to retrieve the data, using subscripts or the get<T> function.
     */
    int a = parsed["--someint"][2];
    int b = parsed["--someint"][1];
    int c = parsed["--someint"].get<int>(0);

    printf("Received string %s, counting down %d %d %d ...\n",
           parsed["--hello"].get<const char*>(0), a, b, c);

    return 0;
}

// Expected Output: Receiving string world, counting down 3 2 1 ...
```


Some known issues


1. Retrieving parsed data by neither subscripts nor get<T> function style provides exactly type safe feature, we only perform size evaluation and bound check. For example, if you are trying to retrieve a **const char\*** while you defined this argument as a **int** type, it would results in unexpected behavior.


2. While a single set of command line arguments with multiple flags that map to the same argument, we always pick the first appeared one. A pitfall is when some of these flags are valid, but the first appeared one is yet invalid, it would still leads to exceptions.


3. Help message still need some polishing.


## Config Parser


Under construction.