#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define VERIFY

#ifdef VERIFY
#include <klee/klee.h>
#endif

// clang -I./klee_src/include -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone dedot.c
// klee --libc=uclibc --posix-runtime --emit-all-errors --only-output-states-covering-new dedot.bc

#define ol_strcpy(dst,src) memmove(dst,src,strlen(src)+1)

// MAIN
int main(int argc, char** argv)
{
  int l;

#ifdef VERIFY
#define MAX_SIZE 20
  char f[MAX_SIZE];
  klee_make_symbolic (f, sizeof f, "f");
  klee_assume (f[MAX_SIZE-1] == '\0');
#else
  char* f = argv[1];
#endif

  char* cp;
  char* cp2;

  while ( ( cp = strstr( f, "//") ) != (char*) 0 )
  {
    for ( cp2 = cp + 2; *cp2 == '/'; ++cp2 )
      continue;
    (void) ol_strcpy( cp + 1, cp2 );
  }

  /* Remove leading ./ and any /./ sequences. */
  while ( strncmp( f, "./", 2 ) == 0 )
    (void) ol_strcpy( f, f + 2 );


  while ( ( cp = strstr( f, "/./") ) != (char*) 0 )
    (void) ol_strcpy( cp, cp + 2 );

  /* Alternate between removing leading ../ and removing xxx/../ */
  for (;;)
  {
    while ( strncmp( f, "../", 3 ) == 0 )
        (void) ol_strcpy( f, f + 3 );

    cp = strstr( f, "/../" );
    if ( cp == (char*) 0 )
      break;
    for ( cp2 = cp - 1; cp2 >= f && *cp2 != '/'; --cp2 )
      continue;
    (void) ol_strcpy( cp2 + 1, cp + 4 );
  }

  /* Also elide any xxx/.. at the end. */
  while ( ( l = strlen( f ) ) > 3 && strcmp( ( cp = f + l - 3 ), "/.." ) == 0 )
  {
    for ( cp2 = cp - 1; cp2 >= f && *cp2 != '/'; --cp2 )
      continue;
    if ( cp2 < f )
      break;
    *cp2 = '\0';
  }

#ifdef VERIFY
  if (strstr (f, "/../")) {
    klee_assert (0);
  }
#endif

#ifndef VERIFY
  printf ("%s\n", f);
#endif
  return 0;
}
