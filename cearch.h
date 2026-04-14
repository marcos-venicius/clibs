#ifndef CL_CEARCH_H_
#define CL_CEARCH_H_

/**

Language Design:

Operators:

    not equal:          !=                  integers, floats, strings, nil, booleans
    equal:              =                   integers, floats, strings, nil, booleans
    greater:            >                   integers, floats
    less:               <                   integers, floats
    greater or equal:   >=                  integers, floats
    less or equal:      <=                  integers, floats
    negation:           !                   booleans (boolean expressions count)

Logical Operators:

    or:                 or
    and:                and

Keywords:

    true:               true
    false:              false
    nil:                nil

Data Types:

    arr:                [1, 2, 3]           you cannot have mixed types in an array
    floats:             [0-9]+(\.[0-9]*)?   C double
    integers:           [0-9]+              C int
    strings:            '.*'                C string (always allocated)
                                            scape quote with \'
                                            line break sequence \n
                                            tab sequence \t

Functions:

    in:                 arrays (of same type), strings ; args: data (array [of same type], string)
    trim:               strings
    ltrim:              strings
    rtrim:              strings
    lower:              strings
    upper:              strings
    replace:            strings ; args: old (string), new (string)
    len:                strings, array
    to_bool:            strings, arrays, integers, floats, nil
                        true values: non empty arrays and strings; non zero floats and integers; not nil values
                        false values: empty arrays and strings; zero floats and integers; nil values
    debug:              any type (always returns true and print the data to stderr)
                        usage: 1.debug, 'hello world'.debug, field.debug

    All functions have the self reference,
    can have zero or more arguments and always returns
    a single value.
    
    How to call a function with zero arguments:
    
        ' Hello, World   '.lower
    
        result: ' hello, world   '
    
    How to join function calls:
    
        ' Hello, World   '.lower.ltrim.rtrim
    
        result: 'hello, world'
    
    How to call a function with arguments:
    
        'Hello! World'.replace('!', ',')

Debugging:

    At any time, you can call `debug` for any data type.
    This function will always return true and print the self data type.

*/

#endif // !CL_CEARCH_H_
