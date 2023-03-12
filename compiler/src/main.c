#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

long file_size(FILE *file) {
    if (!file) { return 0; }
    fpos_t original;
    if (fgetpos(file, &original) != 0) {
        printf("fgetpos() failed: %i\n", errno);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long out = ftell(file);
    if (fsetpos(file, &original) != 0) {
        printf("fsetpos() failed: %i\n", errno);
    }

    return out;
}

char *file_contents(char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("Could not open file at %s\n", path);
        return NULL;
    }

    long size = file_size(file);
    char *contents = malloc(size + 1);
    assert(contents && "Could not allocate buffer for contents.");
    char *write_it = contents;
    size_t bytes_read = 0;
    while (bytes_read < size) {
        size_t bytes_read_this_iteration = fread(write_it, 1, size - bytes_read, file);
        if (ferror(file)) {
            printf("Error while reading file: %i\n", errno);
            free(contents);
            return NULL;
        }
        bytes_read += bytes_read_this_iteration;
        write_it += bytes_read_this_iteration;

        if (feof(file)) {
            break;
        }
    }
    contents[bytes_read] = '\0';
    return contents;
}

void print_usage(char **argv) {
    printf("USAGE: %s <path_to_file>\n", argv[0]);
}

typedef struct Error {
    enum ErrorType {
        ERROR_NONE = 0,
        ERROR_ARGUMENTS,
        ERROR_TYPE,
        ERROR_GENERIC,
        ERROR_SYNTAX,
        ERROR_TODO,
        ERROR_MAX
    } type;
    char *msg;
} Error;

Error ok = {ERROR_NONE, NULL};

void print_error(Error err) {
    if (err.type == ERROR_NONE) {
        return;
    }
    printf("ERROR: ");
    assert(ERROR_MAX == 6);
    switch (err.type) {
        default:
            printf("Unknown error type...");
            break;
        case ERROR_TYPE:
            printf("Mismatched types");
            break;
        case ERROR_TODO:
            printf("TODO (not implemented)");
            break;
        case ERROR_SYNTAX:
            printf("Invalid syntax");
            break;
        case ERROR_ARGUMENTS:
            printf("Invalid arguments");
            break;
        case ERROR_GENERIC:
            break;
        case ERROR_NONE:
            break;
    }
    putchar('\n');
    if (err.msg) {
        printf("     : %s\n", err.msg);
    }
}

#define ERROR_CREATE(n, t, msg) Error (n) = { (t) , (msg) }
#define ERROR_PREP(e, t, message)   \
(e).type = (t);                     \
(e).msg = (message);

const char *whitespace = " \r\n";
const char *delimiters = " \r\n,():+";

typedef struct Token {
    char *beginning;
    char *end;
} Token;


void token_print(const Token t) {
    printf("%.*s", t.end - t.beginning, t.beginning);
}

Error lex(char *source, Token *token) {
    Error err = ok;
    if (!source || !token) {
        ERROR_PREP(err, ERROR_ARGUMENTS, "Cannot lex empty source.");
        return err;
    }
    token->beginning = source;
    // skip any whitespace
    token->beginning += strspn(token->beginning, whitespace);
    token->end = token->beginning;
    if (*(token->end) == '\0') {
        return err;
    }
    // get characters until a delimiter is found
    token->end += strcspn(token->beginning, delimiters);
    if (token->end == token->beginning) {
        token->end += 1;
    }
    return ok;
}

/// @return 0 if id is a valid identifier, 1 otherwise
int valid_identifier(const char *id) {
    // returns null if any of the chars in delimiters is contained by id
    return strpbrk(id, delimiters) == NULL ? 1 : 0;
}

typedef long long integer_t;

typedef struct Node {
    enum NodeType {
        NODE_TYPE_NONE,
        NODE_TYPE_INTEGER,
        NODE_TYPE_SYMBOL,
        NODE_TYPE_VARIABLE_DECLARATION,
        NODE_TYPE_VARIABLE_DECLARATION_INITIALIZED,
        NODE_TYPE_PROGRAM,
        NODE_TYPE_BINARY_OPERATOR,
        NODE_TYPE_UNARY_OPERATOR,
        NODE_TYPE_MAX
    } type;
    union NodeValue {
        integer_t integer;
        char *symbol;
    } value;
    struct Node *children;
    struct Node *next_child;
} Node;

#define nonep(node) ((node).type == NODE_TYPE_NONE)
#define integerp(node) ((node).type == NODE_TYPE_INTEGER)
#define symbolp(node) ((node).type == NODE_TYPE_SYMBOL)

int node_cmp(Node *a, Node *b) {
    if (!a || !b) {
        if (!a && !b) {
            return 1;
        }
        return 0;
    }
    assert(NODE_TYPE_MAX == 3 && "node_cmp() requires all node types to be handled.");
    if (a->type != b->type) {
        return 0;
    }

    switch (a->type) {

        case NODE_TYPE_NONE:
            if (nonep(*b)) {
                return 1;
            }
            return 0;
        case NODE_TYPE_INTEGER:
            if (a->value.integer == b->value.integer) {
                return 1;
            }
            return 0;
        case NODE_TYPE_PROGRAM:
            break;

    }
    return 1;
}

void node_free(Node *root) {
    if (!root) {
        return;
    }

    Node *child = root->children;
    Node *next = NULL;
    while (child) {
        next = child->next_child;
        node_free(child);
        child = next;
    }

    if (symbolp(*root) && root->value.symbol) {
        free(root->value.symbol);
    }

    free(root);
}

void node_print_impl(const Node *node) {
    if (!node) return;
    assert(NODE_TYPE_MAX == 8 && "node_print() must handle all node types.");
    switch (node->type) {

        case NODE_TYPE_INTEGER:
            printf("INT:%lld", node->value.integer);
            break;
        case NODE_TYPE_PROGRAM:
            printf("PROGRAM");
        case NODE_TYPE_NONE:
            printf("NONE");
            break;
        case NODE_TYPE_SYMBOL:
            printf("SYM");
            if (node->value.symbol) {
                printf(":%s", node->value.symbol);
            }
            break;
        case NODE_TYPE_BINARY_OPERATOR:
            printf("BINARY OPERATOR");
            break;
        case NODE_TYPE_UNARY_OPERATOR:
            printf("UNARY OPERATOR");
            break;
        case NODE_TYPE_VARIABLE_DECLARATION:
            printf("VARIABLE DECLARATION");
            break;
        case NODE_TYPE_VARIABLE_DECLARATION_INITIALIZED:
            printf("VARIABLE DECLARATION INITIALIZED");
            break;
        default:
            printf("UNKNOWN");

    }
}

void node_print(const Node *node, const size_t indent_level) {
    if (!node) return;
    for (size_t i = 0; i < indent_level; ++i) {
        putchar(' ');
    }
    node_print_impl(node);
    putchar('\n');

    Node *child = node->children;
    while (child) {
        node_print(node, indent_level + 4);
        child = child->next_child;
    }

}

typedef struct Binding {
    Node id;
    Node value;
    struct Binding *next;
} Binding;

typedef struct Environment {
    struct Environment *parent;
    Binding *bind;
} Environment;

Environment *environment_create(Environment *parent) {
    Environment *env = malloc(sizeof(Environment));
    assert(env && "Could not allocate memory for environment");
    env->bind = NULL;
    env->parent = parent;
    return env;
}

void environment_set(Environment env, Node id, Node value) {
    Binding *binding = malloc(sizeof(Binding));
    assert(binding && "Could not allocate new binding for environment");
    binding->id = id;
    binding->value = value;
    env.bind = binding;
}

Node environment_get(Environment env, Node id) {

    Binding *binding_it = env.bind;

    while (binding_it) {
        if (node_cmp(&binding_it->id, &id)) {

            return binding_it->id;

        }
        binding_it = binding_it->next;
    }
    Node value;
    value.type = NODE_TYPE_NONE;
    value.children = NULL;
    value.next_child = NULL;
    value.value.integer = 0;
    return value;
}

int token_equalp(char *string, Token *token) {
    if (!token) {
        return 0;
    }
    char *beg = token->beginning;
    char *end = token->end;
    if (!string || !beg || !end) {
        return 0;
    }
    while (*string && beg < end) {
        if (*string != *beg) {
            return 0;
        }
        string++;
        beg++;
    }
    return 1;
}

#define token_length(token) token->end - token->beginning

int parse_integer(Token *token, Node *node) {
    if (!token || !node) return 0;
    char *end = NULL;
    size_t token_length = token->end - token->beginning;

    if (token_length == 1 && *(token->beginning) == '0') {
        node->type = NODE_TYPE_INTEGER;
        node->value.integer = 0;
    } else if ((node->value.integer = strtoll(token->beginning, &end, 10)) != 0) {
        if (end != token->end) {
            return 0;
        }
        node->type = NODE_TYPE_INTEGER;
    } else {

        return 0;
    }

    return 1;

}

Error parse_symbol(Token *current, Node *symbol) {
    Error err = ok;
    size_t token_length = current->end - current->beginning;
    symbol->type = NODE_TYPE_SYMBOL;
    symbol->children = NULL;
    symbol->value.symbol = NULL;
    symbol->next_child = NULL;

    char *symbol_string = malloc(token_length + 1);
    assert(symbol_string && "Could not allocate memory for symbol");
    memcpy(symbol_string, (*current).beginning, token_length);
    symbol_string[token_length] = '\0';
    symbol->value.symbol = symbol_string;
    if (!valid_identifier(symbol_string)) {
        char *message;
        asprintf(&message, "%s%s", "Invalid identifier: ", symbol_string);
        ERROR_PREP(
                err, ERROR_TODO, message
        );
    }
    return err;
}

Error parse_expr(char *source, char **end, Node *result) {
    Token current;
    current.beginning = source;
    current.end = source;
    Error err = ok;

    while ((err = lex(current.end, &current)).type == ERROR_NONE) {
        *end = current.end;
        size_t token_length = token_length((&current));
        if (token_length == 0) break;
        if (parse_integer(&current, result)) {
            Node lhs_integer = *result;
            err = lex(current.end, &current);
            if (err.type != ERROR_NONE) {
                return err;
            }
            *end = current.end;
        } else if (token_equalp("let", &current)) {
            err = lex(current.end, &current);
            if (err.type != ERROR_NONE) {
                return err;
            }
            *end = current.end;
            err = parse_symbol(&current, result);
            if (err.type != ERROR_NONE) {
                return err;
            }
            err = lex(current.end,&current);
            if(err.type != ERROR_NONE) return err;
            *end = current.end;
            if((token_length((&current))) == 0) break;
            if(token_equalp(":", &current)) {
                err = lex(current.end,&current);
                if(err.type != ERROR_NONE) return err;
                *end = current.end;
                if((token_length((&current))) == 0) break;
                if(token_equalp("integer", &current)) {
                    Node var_decl;
                    var_decl.children = NULL;
                    var_decl.next_child = NULL;
                    var_decl.type = NODE_TYPE_VARIABLE_DECLARATION;
                    *result = var_decl;
                    return ok;
                }
            }
            ERROR_PREP(err, ERROR_SYNTAX, "Unexpected token");


        } else {
            err = parse_symbol(&current, result);

        }
        printf("Found node: ");
        node_print(result, 0);
    }

    return err;
}


int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv);
        exit(0);
    }
    char *path = argv[1];
    char *contents = file_contents(path);

    if (!contents) {
        return 1;
    }
    printf("Contents of %s:\n---\n\"%s\"\n---\n", path, contents);
    Node expression;
    memset(&expression, 0, sizeof(Node));
    char *contents_it = contents;
    Error err = parse_expr(contents, &contents_it, &expression);
    node_print(&expression, 0);
    print_error(err);
    free(contents);
    return 0;
}
