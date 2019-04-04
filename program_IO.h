#ifndef PROGRAM_IO_H
#define PROGRAM_IO_H

/**
    Author: Robert Crocombe
    Classification: Unclassified
    Initial Release Date: 2004-06-14

    I very much to prefer to use printf-like output statements.  They are
    more easily wrapped in macros that can be compiled away, etc.  This file
    provides the means by which this can be accomplished and yet still
    eventually push the output to std::cout.  I don't know of this is better
    than sync_with_stdio(), but since that routine is supposedly Performance
    Death, I thought I'd try.

    This code is gcc-specific due to use of '##args'.  See C-99 and
    google and so forth for details.  Can be made to work without varargs
    using double parens:
        cprint(("foo: %d", foo))
    If you use something stupid like the MipsPro IRIX compiler.  In fact,
    that was how I first did it.  Hate!

    See, gcc is smart enough to trim the comma out of snprintf(), etc. when
    you don't have any arguments, i.e. cprint("Moo") becomes sprintf("Moo")
    and not snprintf("Moo",) <- dang!  Others not so smart.
*/

#include <iostream>
#include <string>               // std::string
#include <stdexcept>            // std::runtime_error

#include <errno.h>              // errno
#include <cstring>

enum
{
    DEFAULT_BUFFER_SIZE = 2047 + 1,

    DEBUG_0 =   0,
    DEBUG_1 =   1,
    DEBUG_2 =   2,
    DEBUG_3 =   3,
    DEBUG_4 =   4,
    DEBUG_5 =   5

    // Add 'INFO', 'CRITICAL', whatever.
};

extern int debug_level;

/**
    Transforms printf-like output into a single 'char *' of maximum length
    DEFAULT_BUFFER_SIZE (truncates if input data is longer) and outputs it
    via std::cout.

    'c' for compact
*/

#define cprint(format, args...) \
({ \
    char __paste[DEFAULT_BUFFER_SIZE]; \
    snprintf(__paste, sizeof(__paste), format, ##args); \
    std::cout << __paste << std::flush; \
    __paste; \
})

//* 'v' for verbose: add various location-type info to print statement.
#define vprint(format, args...) \
cprint("%s:%s:%d: " format, __FILE__, __func__, __LINE__, ##args)

//* Flavor of vprint -- add "warning" to enhance warny-ness
#define warning(format, args...) vprint("WARNING: " format, ##args)

/**
    Throw an exception of 'except_type' with the string that results from
    feeding 'format, args...' to vprint.
*/
#define exception(except_type, format, args...) \
do { \
    const char *__result = vprint(format "\n", ##args); \
    throw except_type(__result); \
} while (0)

//*  Use this for OS-related errors: adds strerror().
#define error(format, args...) \
exception(std::runtime_error, "ERROR: " format " -- %s", ## args, std::strerror(errno))

/**
    As above, but use this for things that we have screwed up: assertions
    we've failed or the failure of data to arrive as we expected, etc.  By
    far the more likely case.

    The distinction is that we don't need a strerror() for non-OS-related
    junk.
*/

#define runtime(format, args...) \
exception(std::runtime_error, "RUNTIME error: "format, ##args)

/**
    This is like error(), above, but doesn't throw an exception: it just
    reports what error is thinking.  I use it in destructors.
*/

#define report_error(format, args...) \
vprint("BADNESS: " format " -- %s", ## args, std::strerror(errno))

/**
    Takes a variable name 'field', and print out both the field name and the
    value of the field in hexidecial: intended for numeric values.  May not
    work on uint8_t since that may get treated as a char.

    Use like:

    cout << HEX_THIS(some_variable);
*/
#include <iomanip>
#define HEX_THIS(field) \
    "\n" << #field << ": 0x" \
         << std::setw(2 * sizeof(field)) << std::setfill('0') << std::hex \
         << (field)

//* I get tired of writing BLAH.c_str() or BLAH->c_str() over and over.
#define C(string) (string).c_str()
#define CP(string_p) (string_p)->c_str()

////////////////////////////////////////////////////////////////////////////////

/**
    According to the 2005 GCC Summit Proceedings, it's relatively important
    to fully remove debugging code and not just have comparisons keep from
    outputting any data.  They weren't specific, but were annoyed at the
    libc folks.  I would think branch prediction here would be very easy,
    but I dunno.  Maybe code just gets to be too branchy.
*/
#if !DEBUG_ON
    #define XPRINT(format, args...) do {} while (0)
    #define CPRINT_WITH_NAME(name, format, args...) do {} while(0)
    #define VPRINT_WITH_NAME(name, format, args...) do {} while(0)
    #define DP(level, name, format, args...)        do {} while(0)
    #define DEBUG_DECLARE(x)
#else

    /**
        I don't want the name on the front, but I *do* want it to go away
        with -DDEBUG_ON=0.  For more intricate statements: multiple print
        statements generating only a single line of output.
    */
    #define XPRINT(format, args...) \
        cprint(format, ##args)

    //* 'name' is a std::string.  Otherwise like printf().
    #define CPRINT_WITH_NAME(name, format, args...) \
        cprint("%s: "format, name.c_str(), ##args)

    #define VPRINT_WITH_NAME(name, format, args...) \
        vprint("%s: "format, name.c_str(), ##args)

    /**
        Only print the info in 'format, args...' if priority attached to
        debugging info (specified by 'debug') is of a higher priority (lower
        numeric value) than the debugging print level (as specified by
        debug_level).  So a DP(DEBUG_2, args...) or DP(DEBUG_1, args...)
        statement prints if debug_level is == 2, while a DP(DEBUG_5, args...)
        would not.
    */
    #define DP(debug, format, args...) \
    do { \
        if ((debug) <= debug_level) \
            cprint(format, ##args); \
    } while (0)

    /**
        Conditionally declare variables so that CPRINT()s and stuff that go
        away with -DDEBUG_ON=0 don't result in warnings for unused
        variables, e.g. DEBUG_DECLARE(int overran_the_thingy;)
            -- note semi-colon inside the parens.

        Not the best thing I've ever done.

        See above where DEBUG_DECLARE() goes to nothing.
    */
    #define DEBUG_DECLARE(x) x

#endif  // DEBUG_ON

/*
    These seem important enough to keep at all times.
*/

#define ALWAYS_WITH_NAME(name, format, args...) \
    cprint("%s: "format, name.c_str(), ##args)

#define WARNING_WITH_NAME(name, format, args...) \
    warning("%s: "format, name.c_str(), ##args)

#define ERROR_WITH_NAME(name, format, args...) \
    error("%s: "format, name.c_str(), ##args)

#define RUNTIME_WITH_NAME(name, format, args...) \
    runtime("%s: "format, name.c_str(), ##args)

//* For when you want an exception that isn't runtime_error.
#define EXCEPTION_WITH_NAME(exception_type, name, format, args...) \
    exception(exception_type, "%s: "format, name.c_str(), ##args)

#define REPORT_WITH_NAME(name, format, args...) \
    report_error("%s: "format, name.c_str(), ##args)

#endif  // PROGRAM_IO_H

