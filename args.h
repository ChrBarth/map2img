#ifndef ARGS_H
#define ARGS_H

#define MAX_ARGS 256

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// a (very) simple argparse-library
//
// usage:
//
// 1. create an arglist-struct and initialize it:
// arglist myarglist;
// init_list(&myarglist);
//
// 2. create arguments and their descriptions:
// add_arg(&myarglist, "-n", STRING, "Argument description", true);
//
// 3. parse the args:
// parse_args(&myarglist, argc, argv);
// this checks if any of the before defined arguments have been set.
// If they have, argument.var points to the string that got passed.
// If not, argument.var points to NULL
//
// 4. do stuff:
// char* s = get_val(&myarglist, "-n");
// printf("s: %s\n", s);
//
// 5. free up some memory:
// free_args(&myarglist);

typedef enum Argtype {
    INTEGER,
    FLOAT,
    STRING,
    BOOL
} argtype;

// argname = name, e.g. "-x"
// val = pointer to the value (NULL if arg was not passed)
// type = the type of variable we want to set
// info = the string that is used in the help function
// required = if true this argument is required
typedef struct {
    char* argname;
    void* val;
    argtype type;
    char* info;
    bool required;
} argument;

typedef struct {
    argument arguments[MAX_ARGS];
    char** positional_args;
    size_t count;
    size_t pos_count;
} arglist;

// functions:
void print_arg(argument *a);
void append_arg(arglist* a, argument arg);
void print_help(arglist* a);
void add_arg(arglist* arglist, char* argname, argtype type, char* info, bool required);
bool parse_args(arglist* arglist, int argc, char** argv);
void free_args(arglist* arglist);
void init_list (arglist* arglist);
double get_float_val(arglist* arglist, const char* argname);
char* get_string_val(arglist* arglist, const char* argname);
int get_int_val(arglist* arglist, const char* argname);
bool get_bool_val(arglist* arglist, const char* argname);
bool is_set(arglist* arglist, const char* argname);

#ifdef ARG_IMPLEMENTATION
void* get_val(arglist* arglist, const char* argname);
bool check_optionals(arglist* arglist);

void print_arg(argument *a) {
    printf("  %s (type: ", a->argname);
    switch(a->type) {
        case INTEGER: printf("integer"); break;
        case STRING: printf("string"); break;
        case FLOAT: printf("float"); break;
        case BOOL: printf("bool"); break;
    }
    printf("): %s (%s)\n", a->info, a->required ? "required" : "optional");
    if (a->val != NULL) {
        printf("current value: ");
        switch(a->type) {
            case BOOL: printf("%s\n", (*(bool*)a->val) ? "true" : "false"); break;
            case INTEGER:
                       printf("%d\n", *(int*)a->val); break;
            case FLOAT:
                       printf("%f\n", *(double*)a->val); break;
            default: printf("%s\n", (char*)a->val); break;
        }
    }
}

void append_arg(arglist* a, argument arg) {
    if (a->count < MAX_ARGS) {
        a->arguments[a->count++] = arg;
    }
    else {
        fprintf(stderr, "MAX_ARGS (%d) reached!\n", MAX_ARGS);
    }
}

void print_help(arglist* a) {
    fprintf(stderr, "Help:\n");
    for (size_t i=0; i<a->count; ++i) {
        print_arg(&a->arguments[i]);
    }
}

void add_arg(arglist* arglist, char* argname, argtype type, char* info, bool required) {
    argument arg = { argname, NULL, type, info, required};
    append_arg(arglist, arg);
}

bool check_optionals(arglist* arglist) {
    for (int i=0; i<arglist->count; ++i) {
        if (arglist->arguments[i].required && arglist->arguments[i].val == NULL) {
            fprintf(stderr, "missing required argument %s (%s)!\n", arglist->arguments[i].argname, arglist->arguments[i].info);
            return false;
        }
    }
    return true;
}

bool parse_args(arglist* arglist, int argc, char** argv) {
    int c = 0;
    arglist->positional_args = malloc(argc * sizeof(char*)); // the maximum number of positional arguments we can have
    while(c < argc-1) {
        c++;
        bool is_arg = false;
        for (int i=0; i<arglist->count; ++i) {
            argument a = arglist->arguments[i];
            if(strcmp(argv[c], a.argname) == 0) {
                is_arg = true;
                switch(a.type) {
                    case BOOL:
                    case INTEGER:
                        if (a.val == NULL) {
                            int* val = malloc(sizeof(int));
                            if (a.type == BOOL) {
                                *val = 1;
                            }
                            else {
                                if (c == argc-1) {
                                    fprintf(stderr, "missing argument to %s\n", argv[c]);
                                    free(val);
                                    return false;
                                }
                                c++;
                                *val = atoi(argv[c]);
                            }
                            arglist->arguments[i].val = val;
                        }
                        break;
                    case FLOAT:
                        if (a.val == NULL && c < argc-1) {
                            c++;
                            double* val = malloc(sizeof(double));
                            *val = atof(argv[c]);
                            arglist->arguments[i].val = val;
                        }
                            break;
                    case STRING:
                        if (a.val != NULL) {
                            fprintf(stderr, "double assignment of %s not allowed!\n", a.argname);
                            break;
                        }
                        if (c == argc-1) {
                            // last element in argv reached but expecting one more argument:
                            fprintf(stderr, "missing argument to %s\n", argv[c]);
                            return false;
                            }
                        c++;
                        char* val = malloc(sizeof(char) * (strlen(argv[c])+1));
                        strcpy(val, argv[c]);
                        arglist->arguments[i].val = val;
                        break;
                }
                // TODO: do something if argument does not match any arglist->arguments[]

            }
        }
        if (!is_arg) {
            // add to positional arguments array:
            arglist->positional_args[arglist->pos_count] = argv[c];
            arglist->pos_count++;
        }
    }
    return check_optionals(arglist);
}

void free_args(arglist* arglist) {
    for (int i=0; i<arglist->count; ++i) {
        if (arglist->arguments[i].val != NULL) {
            free(arglist->arguments[i].val);
        }
    }
    free(arglist->positional_args);
    arglist->count = 0;
    arglist->pos_count = 0;
}

void init_list (arglist* arglist) {
    // without setting count to 0 valgrind will pop up some errors because of uninitialized memory:
    arglist->count = 0;
    arglist->pos_count = 0;
}

void* get_val(arglist* arglist, const char* argname) {
    for (int i=0; i<arglist->count; ++i) {
        if (strcmp (arglist->arguments[i].argname, argname) == 0) {
            return arglist->arguments[i].val;
        }
    }
    return NULL;
}

double get_float_val(arglist* arglist, const char* argname) {
    for (int i=0; i<arglist->count; ++i) {
        if (strcmp (arglist->arguments[i].argname, argname) == 0 && arglist->arguments[i].type == FLOAT && arglist->arguments[i].val != NULL) {
            double* d = arglist->arguments[i].val;
            return *d;
        }
    }
    return 0.0;
}

char* get_string_val(arglist* arglist, const char* argname) {
    for (int i=0; i<arglist->count; ++i) {
        if (strcmp (arglist->arguments[i].argname, argname) == 0 && arglist->arguments[i].type == STRING && arglist->arguments[i].val != NULL) {
            char* s = arglist->arguments[i].val;
            return s;
        }
    }
    return "";
}

int get_int_val(arglist* arglist, const char* argname) {
    for (int i=0; i<arglist->count; ++i) {
        if (strcmp (arglist->arguments[i].argname, argname) == 0 && arglist->arguments[i].type == INTEGER && arglist->arguments[i].val != NULL) {
            int* n = arglist->arguments[i].val;
            return *n;
        }
    }
    return 0;
}

bool get_bool_val(arglist* arglist, const char* argname) {
    for (int i=0; i<arglist->count; ++i) {
        if (strcmp (arglist->arguments[i].argname, argname) == 0 && arglist->arguments[i].type == BOOL && arglist->arguments[i].val != NULL) {
            bool* b = arglist->arguments[i].val;
            return *b;
        }
    }
    return false;
}

bool is_set(arglist* arglist, const char* argname) {
    if (get_val(arglist, argname) != NULL) return true;
    return false;
}

#endif // ARGS_IMPLEMENTATION
#endif // ARGS_H
