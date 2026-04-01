# picotoml

A single-header, small, C99-compliant TOML parser following the [TOML v1.0.0](https://toml.io/en/v1.0.0) specification.

## Features

- **Single-Header**: Just drop `picotoml.h` into your project.
- **C99 Compliant**: No C++ or modern C dependencies.
- **Robust Parsing**: Handles dotted keys, inline tables, array of tables, and all TOML data types.
- **Order Preserving**: Maintains the order of keys as they appear in the file.
- **Zero Dependencies**: Requires only standard C library headers.

## Installation

Simply copy `picotoml.h` into your project. In **one** C file, define `PICOTOML_IMPLEMENTATION` before including the header to create the implementation:

```c
#define PICOTOML_IMPLEMENTATION
#include "picotoml.h"
```

## Basic Usage

```c
#include "picotoml.h"
#include <stdio.h>

int main() {
    const char* toml_str = "title = 'My TOML' \n [owner] \n name = 'Tom'";
    char* err = NULL;
    
    // Parse the document
    picotoml_node* root = picotoml_parse(toml_str, &err);
    if (!root) {
        printf("Error: %s\n", err);
        return 1;
    }

    // Access data
    picotoml_node* title = picotoml_get(root, "title");
    if (title) printf("Title: %s\n", title->value.string);

    picotoml_node* owner = picotoml_get(root, "owner");
    if (owner) {
        picotoml_node* name = picotoml_get(owner, "name");
        printf("Owner: %s\n", name->value.string);
    }

    // Cleanup
    picotoml_free(root);
    return 0;
}
```

## Documentation

- [API Reference](docs/API.md)
- [Examples](docs/EXAMPLES.md)

## License

MIT License. See [LICENSE](LICENSE) for details.
