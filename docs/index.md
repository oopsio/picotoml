---
layout: default
title: picotoml - A Pico TOML Parser for C99
---

# picotoml

Welcome to the documentation for **picotoml**, a single-header, small, C99-compliant TOML parser following the [TOML v1.0.0](https://toml.io/en/v1.0.0) specification.

## Quick Links

- [🚀 Getting Started](https://github.com/oopsio/picotoml#installation)
- [📖 API Reference](api.md)
- [💻 Examples](examples.md)
- [⚖️ MIT License](https://github.com/oopsio/picotoml/blob/main/LICENSE)

## Why picotoml?

- **Painless Integration**: One file, one include. No complex build systems.
- **Strict Compliance**: Built directly against the offical TOML specification.
- **Compact & Fast**: Optimized for size and simplicity, perfect for embedded systems or small projects.
- **Order Preservation**: Maintains the internal structure and order of keys for consistent re-serialization.

## Basic Usage

```c
#define PICOTOML_IMPLEMENTATION
#include "picotoml.h"

int main() {
    char* err = NULL;
    picotoml_node* root = picotoml_parse("key = 'value'", &err);
    // ... use the node ...
    picotoml_free(root);
    return 0;
}
```

---

*This documentation is automatically generated and deployed via GitHub Pages.*
