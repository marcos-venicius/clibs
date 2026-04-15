Language Design:

    Operators:

        not equal:          !=                  int, float, string, nil, bool
        equal:              =                   int, float, string, nil, boolean
        greater:            >                   int, float
        less:               <                   int, float
        greater or equal:   >=                  int, float
        less or equal:      <=                  int, float
        negation:           !                   bool (boolean expressions count)

    Logical Operators:

        or:                 or
        and:                and

    Keywords:

        true:               true
        false:              false
        nil:                nil

    Data Types:

        array:              [1, 2, 3]           you cannot have mixed types in an array
        float:              [0-9]+\.[0-9]*      C double
        int:                [0-9]+              C int
        bool:               true|false
        str:                '.*'                C strings           (always heap allocated)
                                                quote scaping       \'
                                                line break escaping \n
                                                tab escaping        \t
                                                backslash escaping  \\

    Functions:

        str  contains(self: str, str);
        str  trim(self: str);
        str  ltrim(self: str);
        str  rtrim(self: str);
        bool starts_with(self: str, str);
        bool ends_with(self: str, str);
        str  lower(self: str);
        str  upper(self: str);
        str  replace(self: str, old: str, new: str);
        int  len(self: <str|array>);
        bool to_bool(self: any); ---------------------- true values: non empty arrays and strings; non zero floats and integers; not nil values
                                                        false values: empty arrays and strings; zero floats and integers; nil values
        debug(self: any); ----------------------------- any type (always returns true and print the data to stderr)
                                                        usage: 1.debug, 'hello world'.debug, field.debug

        All functions have at least the self argument.
        For example when calling `'Hello'.lower`, `'Hello'` is the self and `lower` is the function
        that is receiving this as the first parameter.

        You can concatenate function calls just by putting a dot right after the previous call:
        `'Hello! World'.replace('!', ',').lower.to_bool`

    Debugging:

        At any time, you can call `debug` for any data type.
        This function will always return true and print the self data type.

