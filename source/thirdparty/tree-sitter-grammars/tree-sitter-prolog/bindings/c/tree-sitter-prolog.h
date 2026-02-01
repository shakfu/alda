#ifndef TREE_SITTER_PROLOG_H_
#define TREE_SITTER_PROLOG_H_

typedef struct TSLanguage TSLanguage;

#ifdef __cplusplus
extern "C" {
#endif

const TSLanguage *tree_sitter_prolog(void);
const TSLanguage *tree_sitter_problog(void);

#ifdef __cplusplus
}
#endif

#endif // TREE_SITTER_PROLOG_H_
