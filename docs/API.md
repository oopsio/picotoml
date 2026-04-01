# API Reference

## Types

### `picotoml_type`
An enumeration of the supported TOML data types:
- `PICOTOML_TYPE_STRING`
- `PICOTOML_TYPE_INTEGER`
- `PICOTOML_TYPE_FLOAT`
- `PICOTOML_TYPE_BOOLEAN`
- `PICOTOML_TYPE_DATETIME`
- `PICOTOML_TYPE_ARRAY`
- `PICOTOML_TYPE_TABLE`
- `PICOTOML_TYPE_NULL`

### `picotoml_node`
The primary data structure representing a value in the TOML document.
```c
typedef struct picotoml_node {
    picotoml_type type;
    char* key;           // Key name if this is a table entry
    bool is_inline;      // True if table was defined inline
    union {
        char* string;
        int64_t integer;
        double floating;
        bool boolean;
        picotoml_datetime datetime;
        struct picotoml_node* array_head;
        struct picotoml_node* table_head;
    } value;
    struct picotoml_node* next; // Next sibling in structure
} picotoml_node;
```

---

## Functions

### `picotoml_parse`
Parses a TOML string into an AST.
```c
picotoml_node* picotoml_parse(const char* input, char** error_out);
```
- **input**: A null-terminated UTF-8 TOML string.
- **error_out**: (Optional) Pointer to a string that will contain an error message if parsing fails.
- **Returns**: The root table node on success, or `NULL` on failure.

### `picotoml_free`
Recursively frees a `picotoml_node` and all its children.
```c
void picotoml_free(picotoml_node* node);
```

### `picotoml_get`
Looks up a key in a table node.
```c
picotoml_node* picotoml_get(picotoml_node* table, const char* key);
```
- **Returns**: The matching node or `NULL`.

### `picotoml_at`
Retrieves an element from an array by index.
```c
picotoml_node* picotoml_at(picotoml_node* array, size_t index);
```
- **Returns**: The matching node or `NULL`.

### `picotoml_size`
Returns the number of elements in an array or table.
```c
size_t picotoml_size(picotoml_node* node);
```
