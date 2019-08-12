#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define VERIFY

#ifdef VERIFY
#include <klee/klee.h>
#endif

#define MAX_DIRECTORIES 50
#define MAX_TOKEN_SIZE 15

// clang -I./klee_src/include -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone implementation.c
// klee --libc=uclibc --posix-runtime --emit-all-errors --only-output-states-covering-new implementation.bc

/*
 * Tokenize the string into array of strings
 */
char**
tokenize (char* directory_string)
{
	char** tokens = malloc (MAX_DIRECTORIES * sizeof(char*));
	memset (tokens, 0, sizeof(char*) * MAX_DIRECTORIES);

	char* tmp = directory_string;

	uint32_t idx = 0;

	while ((tokens[idx] = strtok_r (tmp, "/", &tmp)))
	{
		idx++;
	}

	return tokens;
}

int
num_tokens (char** tokens)
{
	int idx = 0;

	while (tokens[idx])
	{
		idx++;
	}

	return idx;
}

uint8_t
free_tokens (char** tokens)
{
	if (tokens)
	{
		free (tokens);
		memset (tokens, 0, MAX_DIRECTORIES);
		return 1;
	}

	return 0;
}

/*
 * Stack implementation
 * https://groups.csail.mit.edu/graphics/classes/6.837/F04/cpp_notes/stack1.html
 */
struct Stack {
    char*   data[MAX_DIRECTORIES];
    int     size;
};
typedef struct Stack Stack;


void
Stack_Init (Stack *S)
{
    S->size = 0;
}

char*
Stack_Top (Stack *S)
{
    if (S->size == 0) {
        fprintf(stderr, "Error: stack empty\n");
        return (char*) NULL;
    } 

    return S->data[S->size-1];
}

void
Stack_Push (Stack *S, char* d)
{
    if (S->size < MAX_DIRECTORIES)
        S->data[S->size++] = d;
    else
        fprintf(stderr, "Error: stack full\n");
}

void
Stack_Pop (Stack *S)
{
    if (S->size == 0)
        fprintf(stderr, "Error: stack empty\n");
    else
        S->size--;
}

char*
Stack_to_string (Stack *S)
{
	if (S->size == 0)
	{
		return "";
	}

	/* MAX_TOKEN_SIZE + 1 to hold sandwhiching slashes ("/") */
	char* result = malloc (sizeof(char) * MAX_DIRECTORIES * (MAX_TOKEN_SIZE + 1));
	char* result_ptr = result;
	char* slash = "/";

	*result_ptr++ = *slash;

	for (int i = 0; i < S->size; i++)
	{
		char* current_token = S->data[i];
		size_t current_token_size = strlen (current_token);
		memcpy (result_ptr, current_token, current_token_size);

		result_ptr += current_token_size;

		if (i != (S->size - 1))
			*result_ptr++ = *slash;
	}

	*result_ptr = '\0';
	return result;
}


/*
 * The actual stack, directory-traversal-sanitizing algorithm
 */
char*
sanitize (char* target_file_path)
{
	Stack* s = malloc (sizeof (Stack));
	Stack_Init (s);

	char** tokens = tokenize (target_file_path);
	int count = num_tokens (tokens);

	for (int i = 0; i < count; i++)
	{
		char* current_token = tokens[i];

		if (!strncmp (current_token, "..", MAX_TOKEN_SIZE))
		{
			if (s->size == 0)
			{
				continue;
			}
			else
			{
				Stack_Pop (s);
			}
		}
		else if (!strncmp (current_token, ".", MAX_TOKEN_SIZE))
		{
			continue;
		}
		else
		{
			Stack_Push (s, current_token);
		}
	}

	char* canonicalizedString = Stack_to_string (s);
#ifdef VERIFY
  /// First example
  /*klee_assert (strncmp(canonicalizedString, "/a/b/c", MAX_TOKEN_SIZE) != 0);*/

  /// Second example
  klee_assert (!strstr (canonicalizedString, "/../"));
  klee_assert (!strstr (canonicalizedString, "//"));
  klee_assert (!strstr (canonicalizedString, "/./"));
#endif
	return canonicalizedString;
}

int
main (int argc, char** argv)
{

#ifdef VERIFY
#define MAX_SIZE 12
	char path_string[MAX_SIZE];
	klee_make_symbolic (path_string, sizeof path_string, "path_string");
	klee_assume (path_string[MAX_SIZE-1] == '\0');
	sanitize (path_string);
#else
	char* input = malloc (MAX_DIRECTORIES * MAX_TOKEN_SIZE * sizeof (char));
	memset (input, 0, MAX_DIRECTORIES * MAX_TOKEN_SIZE * sizeof (char));

	size_t input_len = strnlen (argv[1], MAX_DIRECTORIES * MAX_TOKEN_SIZE * sizeof (char) - 1);

	memcpy (input, argv[1], input_len);
	printf ("%s\n", sanitize (input));
#endif

	return 0;
}
