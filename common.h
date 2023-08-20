#pragma once

// va_list is a type that represents a variable argument
// list. It is used to declare a list of arguments whose
// number and types are not known at compile time.
#define va_list __builtin_va_list
// va_start is a macro that initializes a va_list object
// with the first argument after the format string. It takes
// two arguments: the first is the va_list object to be
// initialized, and the second is the name of the last fixed
// argument in the function (in this case, fmt).
#define va_start __builtin_va_start
// va_end is a macro that cleans up a va_list object after
// use. It takes one argument: the va_list object to be
// cleaned up.
#define va_end __builtin_va_end
// After va_arg is used to retrieve an
// argument from the variable argument
// list, the argument is removed from the
// list. This means that subsequent calls
// to va_arg will retrieve the next
// argument in the list.
#define va_arg __builtin_va_arg

void printf(const char* fmt, ...);