# picotoml Examples

## Basic Parsing

```c
#define PICOTOML_IMPLEMENTATION
#include "picotoml.h"
#include <stdio.h>

int main() {
    const char* toml = "title = \"picotoml example\"";
    char* err = NULL;
    picotoml_node* root = picotoml_parse(toml, &err);
    
    if (root) {
        picotoml_node* title = picotoml_get(root, "title");
        printf("Title: %s\n", title->value.string);
        picotoml_free(root);
    }
    return 0;
}
```

## Accessing Nested Tables

```c
const char* toml = "[owner]\nname = \"Tom\"\n";
picotoml_node* root = picotoml_parse(toml, NULL);

picotoml_node* owner = picotoml_get(root, "owner");
if (owner) {
    picotoml_node* name = picotoml_get(owner, "name");
    printf("Name: %s\n", name->value.string);
}
```

## Iterating Arrays

```c
const char* toml = "ports = [ 8000, 8001, 8002 ]";
picotoml_node* root = picotoml_parse(toml, NULL);

picotoml_node* ports = picotoml_get(root, "ports");
if (ports && ports->type == PICOTOML_TYPE_ARRAY) {
    for (size_t i = 0; i < picotoml_size(ports); i++) {
        picotoml_node* p = picotoml_at(ports, i);
        printf("Port %zu: %lld\n", i, (long long)p->value.integer);
    }
}
```

## Array of Tables

```c
const char* toml = 
    "[[products]]\nname = \"Hammer\"\n"
    "[[products]]\nname = \"Nail\"\n";
picotoml_node* root = picotoml_parse(toml, NULL);

picotoml_node* products = picotoml_get(root, "products");
if (products && products->type == PICOTOML_TYPE_ARRAY) {
    for (size_t i = 0; i < picotoml_size(products); i++) {
        picotoml_node* p = picotoml_at(products, i);
        picotoml_node* name = picotoml_get(p, "name");
        printf("Product %zu: %s\n", i, name->value.string);
    }
}
```
