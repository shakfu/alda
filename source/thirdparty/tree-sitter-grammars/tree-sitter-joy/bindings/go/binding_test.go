package tree_sitter_joy_test

import (
	"testing"

	tree_sitter "github.com/smacker/go-tree-sitter"
	"github.com/tree-sitter/tree-sitter-joy"
)

func TestCanLoadGrammar(t *testing.T) {
	language := tree_sitter.NewLanguage(tree_sitter_joy.Language())
	if language == nil {
		t.Errorf("Error loading Joy grammar")
	}
}
