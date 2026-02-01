# tree-sitter-prolog

[![status-badge](https://ci.codeberg.org/api/badges/13456/status.svg)](https://ci.codeberg.org/repos/13456)

Prolog grammar for [Tree-sitter](https://github.com/tree-sitter/tree-sitter).
The ["References" section](#references) lists the various extensions that
currently have their own grammar besides ISO Prolog.

Feel free to contribute with new ones! It is super easy, as you can just extend
an existing one. See `.woodpecker.yml` for the current version of Tree-sitter
we are testing against.

## Project structure

The project uses pnpm as its package manager. Files `package.json`,
`tree-sitter.json`, and `pnpm-lock.yaml` can be found in the root folder.

### Grammars

A `grammar.js` file for a Prolog extension `xxx` is located at
`grammars/xxx/grammar.js`. They are formatted using dprint, according to
`grammars/dprint.json`.

### Parsers

Parser sources for a Prolog extension `xxx` are located at `grammars/xxx/src/`.
If you are not using the `generate` pnpm script and are manually invoking the
`tree-sitter generate` command instead, make sure your current directory is
`grammars/xxx/`.

### Queries

If a `.scm` file does not need modifications between two or more grammars, it
can be found in `queries/`. Otherwise, its version for grammar `xxx` is located
at `grammars/xxx/queries/`.

### Tests

Corpus tests for a grammar `xxx` are located at `grammars/xxx/tests/corpus/`.
Highlight tests of all grammars are located at `tests/highlight/`.

## References

### ISO Prolog

- [_ISO Prolog standard, part 1: General core, ISO-IEC-13211-1_](https://annas-archive.org/md5/2ab8eeb4b91b6c75ccb9f80a8cc446e4)
- [_ISO Prolog standard, part 2: Modules, ISO-IEC-13211-2_](https://annas-archive.org/md5/476731b3fea9384b12f5e6c2f42960c8)

### ProbLog

- [L. De Raedt, A. Kimmig and H. Toivonen. _ProbLog: A Probabilistic Prolog and
  its Application in Link Discovery_. IJCAI 2007, Proceedings of the 20th
  International Joint Conference on Artificial Intelligence, pp. 2462 - 2467,
  2007.](https://lirias.kuleuven.be/bitstream/123456789/146072/1/ijcai07-problog.pdf)
