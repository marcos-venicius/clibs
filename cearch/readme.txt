Cearch Language.

Ideas:
  
  FFI (draft):
    It would be very cool if the user could use C functions to transform data during the query.
    The user should provide the signature of the function and it's name.
    Then, he would be able to call it later by linking his functions to the
    compiler unit.

    Some examples would be:

    The user would need to specify the function signature, maybe via another place.
    I mean, a different entry point that is not the query entry point.
    It would be something like a preprocessing of FFI functions.

    ```
    // if left is greater than right returns 1
    // if left is less than right returns -1
    // if left is equal to right returns 0
    @define(compare_dates(str, str): int);
    ```

    Then, the query would look like:
    
    ```
    // so, this query basically searchs for any registers
    // that starts at 2025-10-10 and ends at 2025-11-10
    #compare_dates(created_at, '2025-10-10') >= 0 and #compare_dates(created_at, '2025-11-10') <= 0
    ```

    This would cause the program to call a function with a signature more or less like this:

    `Cearch_Value compare_date(int args_count, Cearch_Value *args);`

    Then, the user and the program would know which is the return type,
    the arguments type and how many of then.

    Since the user knows, the implementation could be something like:

    ```
    Cearch_Value compare_dates(int args_count, Cearch_Value *args) {
      // null terminated strings. The struct would have { int size, char *value }
      Cearch_Value left  = args[0].as_str;
      Cearch_Value right = args[1].as_str;

      int result = 0;

      ... pseudo code
      if (left.value == right.value) result = 0;
      if (left.value >  right.value) result = 1;
      if (left.value <  right.value) result = -1;

      return (Cearch_Value){
        .as_int = result
      };
    }
    ```

    Then, this would satisfies the program because by the previous load of the FFI file specfication
    the program now knows the return type and will directly access `as_int` field.

Language Design:

    Data Types:

        nil:                nil                 represents a non-existent data. imagine like C stdio NULL.
        array:              [1, 2, 3]           you cannot have mixed types arrays
        float:              -?[0-9]+\.[0-9]*    C double
        int:                -?[0-9]+            C int
        bool:               true|false
        str:                '.*'                C strings           (always heap allocated)
                                                quote scaping       \'
                                                line break escaping \n
                                                tab escaping        \t
                                                backslash escaping  \\

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

Language Compiler:

  Constant folder:

    We propabably are going to need to have a `constant folder` step.
    Some kind of optimizer.

    For example, if we have expressions like this:

    - 'Hello'.upper
    - 'Hello'.len
    - 'Testing it.'.replace('.', '!').lower

    We can just replace this with:

    - 'HELLO'
    - 5
    - 'testing it.'

    That's good because you won't need to do these operations every single search.
    Once compile, it's going to be constant folded.

  Type Checker:  

    After lexing and parsing, we should type check the whole expression.

    That's needed because this language is supposed to be strictly typed.

    We cannnot have mixed type arrays and we have functions that accept and returns specific
    data types.
