#!/usr/bin/env python3
"""
Patch eval.c for psnd's in-process MicroHs runtime.

Always applied:
1. Renames mhs_fopen to mhs_fopen_orig (the original implementation)
2. Renames mhs_opendir to mhs_opendir_orig (for VFS directory support)
3. Renames mhs_readdir to mhs_readdir_orig (for VFS directory support)
4. Renames mhs_closedir to mhs_closedir_orig (for VFS directory support)
5. Adds extern declarations for VFS overrides

With --rename-malloc:
6. Renames mmalloc/mrealloc/mcalloc to mhs_* so the runtime can be linked
   into the psnd binary alongside Csound, which exports the same names.
   Only eval.c defines or calls them; bfile.c, md5.c, lz77.c and extra.c
   do not, so no other runtime file needs patching.

Usage: mhs-patch-eval.py [--rename-malloc] <input_eval.c> <output_eval.c>
"""

import sys
import re


def patch_eval(input_path: str, output_path: str, rename_malloc: bool) -> None:
    with open(input_path, 'r') as f:
        content = f.read()

    # Rename function definitions
    functions_to_rename = ['mhs_fopen', 'mhs_opendir', 'mhs_readdir', 'mhs_closedir']
    for func in functions_to_rename:
        content = re.sub(
            rf'^(from_t) {func}\(',
            rf'\1 {func}_orig(',
            content,
            flags=re.MULTILINE
        )

    # Add extern declarations before ffi_table
    extern_decl = (
        '/* Forward declarations for VFS overrides - provided by mhs_ffi_override.c */\n'
        'extern from_t mhs_fopen(int s);\n'
        'extern from_t mhs_opendir(int s);\n'
        'extern from_t mhs_readdir(int s);\n'
        'extern from_t mhs_closedir(int s);\n\n'
    )

    # Insert before "const struct ffi_entry ffi_table[] = {"
    content, n = re.subn(
        r'^(const struct ffi_entry ffi_table\[\] = \{)',
        extern_decl + r'\1',
        content,
        flags=re.MULTILINE
    )
    if n != 1:
        sys.exit(f"{sys.argv[0]}: expected 1 ffi_table definition in {input_path}, found {n}")

    if rename_malloc:
        # Word-boundary match catches both the definitions and every call site.
        content, n = re.subn(
            r'\bm(m|re|c)alloc\b',
            r'mhs_m\1alloc',
            content
        )
        if n == 0:
            sys.exit(f"{sys.argv[0]}: --rename-malloc found no mmalloc/mrealloc/mcalloc "
                     f"in {input_path}; upstream may have renamed them")

    with open(output_path, 'w') as f:
        f.write(content)

    print(f"Patched: {input_path} -> {output_path}")


if __name__ == '__main__':
    args = sys.argv[1:]
    rename_malloc = False
    if '--rename-malloc' in args:
        rename_malloc = True
        args.remove('--rename-malloc')

    if len(args) != 2:
        print(f"Usage: {sys.argv[0]} [--rename-malloc] <input_eval.c> <output_eval.c>",
              file=sys.stderr)
        sys.exit(1)

    patch_eval(args[0], args[1], rename_malloc)
