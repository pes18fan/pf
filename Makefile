check:
	echo '#define PF_IMPLEMENTATION' > checker.c
	echo '#include "pf.h"' >> checker.c
	echo '#define PF_ARGPARSE_IMPLEMENTATION' >> checker.c
	echo '#include "pf_argparse.h"' >> checker.c
	echo 'int main(void) { return 0; }' >> checker.c
	clang -Wall -Wextra -Wpedantic -Werror checker.c -o checker_bin
	clang --analyze checker.c
	rm -f checker.c checker_bin checker.plist
