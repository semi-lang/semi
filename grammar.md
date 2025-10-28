# Grammar

```
~ = (WHITESPACE | NEWLINE)*
```

## Program

```
program = BOM? SHEBANG? ( statement ( NEWLINE statement )* )?
```

## Statement

```
statement =
  | assignment_statement
  | if_statement
  | for_statement
  | use_statement
  | fn_statement
  | return_statement
  | raise_statement
  | defer_statement
  | expr
```

### Assignment-Statement

```
export_single_assignment_statement = ""export" NON_TYPE_IDENTIFIER "=" expr
single_assignment_statement = addressable_expr "=" expr
pair_assignment_statement = addressable_expr "," addressable_expr "=" expr

assignment_statement =
  | export_single_assignment_statement
  | single_assignment_statement
  | pair_assignment_statement
```

### If-Statement

```
if_clause = "if" expr "{" statements "}"
else_if_clause = "elif expr "{" statements "}"
else_clause = "else" "{" statements "}"

if_statement = if_clause ( else_if_clause )* ( else_clause )?
```

### For-Statement


```
for_segment = 
  | "for NON_TYPE_IDENTIFIER "in" expr ("step" expr)?
  | "for NON_TYPE_IDENTIFIER "," NON_TYPE_IDENTIFIER "in" expr ("step" expr)?
for_statement = ("for" | for_segment) "{" statements "}"
```

### Import-Statement

```
import_simple_head = "import" STRING
import_named_segment = "as" IDENTIFIER

import_extract_item = IDENTIFIER ("as" IDENTIFIER)?
import_extract_list = import_extract_item ( "," import_extract_item )* ","?

import_extract_segment = "import" STRING "for" "{" import_extract_list "}"
import_statement = import_simple_statement ( import_named_segment | import_extract_segment )?
```

### Fn-Statement

```
fn_arg_list = NON_TYPE_IDENTIFIER ( "," NON_TYPE_IDENTIFIER )* ","?
fn_statement = "export"? "fn" NON_TYPE_IDENTIFIER "(" fn_arg_list ")" "{" statements "}"
```

### Return-Statement

```
return_statement = "return" expr?
```

### Raise-Statement

```
raise_statement = "raise" expr
```

### Defer-Statement

```
defer_statement = "defer" "{" statements "}"
```

### Struct-Statement

struct_field = TYPE_IDENTIFIER | NON_TYPE_IDENTIFIER TYPE_IDENTIFIER
struct_field_list = struct_field (NEWLINE struct_field)* NEWLINE?
struct_statement = "struct" TYPE_IDENTIFIER "{" struct_field_list "}"

### Expression

```
collection_list_loop = expr ":" expr for_segment
collection_list_loop = expr for_segment
collection_map_literal = expr ":" expr ( "," expr ":" expr )* ","?
collection_list_literal = expr ( "," expr )* ","?

collection_content =
  | collection_map_loop
  | collection_list_loop
  | collection_map_literal
  | collection_list_literal

expr = Pratt(
    # Literals recognized in the LHS block
    | LITERAL
    | NON_TYPE_IDENTIFIER

    # Handled in the LHS block
    # (because it starts with a prefix)
    | "(" expr ")"
    | TYPE_IDENTIFIER "[" collection_content "]"
    | ("!" | "-" ) expr


    # Handled in the RHS block
    # (because it starts with the current expr)
    | expr (
        | ( "?." | "." ) IDENTIFIER
        | "[" expr "]"
      )*
    | expr binary_ops expr
    | expr "?" expr ":" expr
)
```
