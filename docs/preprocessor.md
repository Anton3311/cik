# Preprocessor

The preprocessor is the next step after the tokenization in the pipeline. It's API is rather similar to the one of the tokenizer:

```c
Token preprocessor_view_next(Preprocessor* state);
Token preprocessor_next_token(Preprocessor* state);
```

Same as the tokenizer, the preprocessor is built around returning tokens one by one, instead of generating a huge array of all tokens in the source file. Tokens are short-lived transient structs, they are not stored anywhere, they get created by one stage and get consumed by the next, and that's it.

The preprocessor acts as an iterator over the stream, cosuming tokens from the tokenizer and returning preprocessed tokens. Same as the tokenizer, which can be considered an iterator over the source code.

Internally it is implemented as a set of state machines. They are required to be able to keep preprocessing from the spot where the last call to the preprocessor has ended.

The preprocessor consits of multiple internal structures:
1. `MacroCallStack` - a call stack for macros
2. `IncludeStack` - a stack for file included files
3. `MacroTable` - a table used to track defined macros
4. `branch stack` - used for tracking the current state of conditional directives

## Main state machine

The main state machine isn't explicitly defined, rather its state is determined by the `MacroCallStack`.
 
When the `MacroCallStack` is empty the preprocessor just forwards the results of the tokenizer.

Otherwise, if the `MacroCallStack` is not empty, the preprocessor returns the next token from the macro expansion.

At this point the preprocessor might have encourntered an identifier token (returned either by the tokenizer, or the macro expansion), this token can potentially be a macro call. In case it is, the preprocessor parses it and pushes it onto the `MacroCallStack`.

In the above step the tokens were consumed by the macro call parser, but the preprocessor always must return a valid token (EOF is also a valid token), thus the described state machine is wrapped into a loop, which loops until it produces a token.

## Structure of a preprocessor macro

Before going into the second state machine, it is important to first describe how the macro is structured internally.

Here is the `MacroDefinitionn` struct:
```c
struct MacroDefinition {
    SourceString name;

    // What kind of a builtin macro it is
    BuiltinMacroKind builtin_kind;

    // Function-like or not
    MacroStyle style;
    
    String* parameter_names;
    size_t parameter_count;
    
    // Whether it uses __VA_ARGS__
    bool has_va_args;
    
    // An array of tokens the macro expands to
    Token* tokens;

    // A hint on how to interpret each of the tokens in the above array.
    MacroTokenHint* token_hints;
    size_t token_count;
};
```

All of the above fields are pretty much self-explanatory, however it is worth exaplining the token hints, as they are used to guide the macro expansion.

Token hints are defined by the `MacroTokenHintKind` enum:
```c
// Tells the preprocessor how to interpret the identifier token
// in the defintion of the macro
typedef enum {
    MACRO_TOKEN_HINT_NONE,
    MACRO_TOKEN_HINT_PARAMETER,
    MACRO_TOKEN_HINT_STRING_OPERATOR,
    MACRO_TOKEN_HINT_TOKEN_INSERT_OPERATOR,
    MACRO_TOKEN_HINT_VA_ARGS,
} MacroTokenHintKind;
```

The meaning of these hints:
1. `MACRO_TOKEN_HINT_NONE` - the token doesn't require special treatment, it should be returned from the expansion as is.
2. `MACRO_TOKEN_HINT_PARAMETER` - the token is a macro call argument, and must be replaced by the tokens of the corresponding argument.
3. `MACRO_TOKEN_HINT_STRING_OPERATOR` - the current token is ignored and the next token is turned into a string.
4. `MACRO_TOKEN_HINT_TOKEN_INSERT_OPERATOR` - the current and all the subsuquent tokens with the same hint are merged into one token.
5. `MACRO_TOKEN_HINT_VA_ARGS` - the token is replaced by the `__VA_ARGS__` tokens.

Example macro:
```c
#define min(a, b) a < b ? a : b
```

The tokens stream for this macros look like this:
```
IDENT("a"), LESS, IDENT("b"), QUESTION_MARK, IDENT("a"), COLON, IDENT("b")
```

Since it is a function-like macro, it also has a stream of token hints:
```
PARAMETER, NONE, PARAMETER, NONE, PARAMETER, NONE, PAREMETER
```

## Macro expansion state machine

The second state machine is implemented in the macro expansion process by `_preprocessor_expand_user_defined_macro` in `parser/src/parser/preprocessor.c`.

This state machine's sole purpose is to emit tokens one by one from the macro's token stream while also interpreting the hints stored along side the stream.

The states of this state machine are defined by this enum:
```c
typedef enum {
    MACRO_CALL_TOKEN,
    MACRO_CALL_ARGUMENT_EXPANSION,
    MACRO_CALL_VA_ARGS_EXPANSION,
} MacroCallState;
```

The `MACRO_CALL_TOKEN` is the default state where either the current token gets returned or the state machine gets transfered into a different state based on the current token hint.

`MACRO_CALL_ARGUMENT_EXPANSION` returns the tokens of the corresponding argument one by one.

`MACRO_CALL_VA_ARGS_EXPANSION` same as the above, returns tokens of the `__VA_ARGS__`.
