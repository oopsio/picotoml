#define PICOTOML_IMPLEMENTATION
#include "picotoml.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void print_value(picotoml_node* v) {
    if (!v) return;
    switch (v->type) {
        case PICOTOML_TYPE_STRING: printf("\"%s\"", v->value.string); break;
        case PICOTOML_TYPE_INTEGER: printf("%lld", (long long)v->value.integer); break;
        case PICOTOML_TYPE_FLOAT: printf("%g", v->value.floating); break;
        case PICOTOML_TYPE_BOOLEAN: printf("%s", v->value.boolean ? "true" : "false"); break;
        case PICOTOML_TYPE_DATETIME:
            printf("%04d-%02d-%02dT%02d:%02d:%02dZ", 
                   v->value.datetime.year, v->value.datetime.month, v->value.datetime.day,
                   v->value.datetime.hour, v->value.datetime.minute, v->value.datetime.second);
            break;
        case PICOTOML_TYPE_ARRAY:
            printf("[ ");
            for (picotoml_node* item = v->value.array_head; item; item = item->next) {
                print_value(item);
                if (item->next) printf(", ");
            }
            printf(" ]");
            break;
        case PICOTOML_TYPE_TABLE:
            if (v->is_inline) {
                printf("{ ");
                for (picotoml_node* kv = v->value.table_head; kv; kv = kv->next) {
                    printf("%s = ", kv->key);
                    print_value(kv);
                    if (kv->next) printf(", ");
                }
                printf(" }");
            }
            break;
        default: break;
    }
}

void print_node_toml(picotoml_node* node, int indent, bool is_root, const char* prefix) {
    if (!node) return;
    picotoml_node* head = (node->type == PICOTOML_TYPE_TABLE || is_root) ? node->value.table_head : NULL;
    if (!head && !is_root) return;

    /* 1. Flat values */
    for (picotoml_node* curr = head; curr; curr = curr->next) {
        bool is_flat = (curr->type != PICOTOML_TYPE_TABLE && curr->type != PICOTOML_TYPE_ARRAY) ||
                       (curr->type == PICOTOML_TYPE_TABLE && curr->is_inline) ||
                       (curr->type == PICOTOML_TYPE_ARRAY && !(curr->value.array_head && curr->value.array_head->type == PICOTOML_TYPE_TABLE && !curr->value.array_head->is_inline));
        
        if (is_flat) {
            for (int i = 0; i < indent; i++) printf("  ");
            printf("%s = ", curr->key);
            print_value(curr);
            printf("\n");
        }
    }

    /* 2. Sub-sections */
    for (picotoml_node* curr = head; curr; curr = curr->next) {
        char new_prefix[256];
        if (prefix && prefix[0]) snprintf(new_prefix, sizeof(new_prefix), "%s.%s", prefix, curr->key);
        else snprintf(new_prefix, sizeof(new_prefix), "%s", curr->key);

        if (curr->type == PICOTOML_TYPE_TABLE && !curr->is_inline) {
            printf("\n");
            for (int i = 0; i < indent; i++) printf("  ");
            printf("[%s]\n", new_prefix);
            print_node_toml(curr, indent, false, new_prefix);
        } else if (curr->type == PICOTOML_TYPE_ARRAY) {
            picotoml_node* first = curr->value.array_head;
            if (first && first->type == PICOTOML_TYPE_TABLE && !first->is_inline) {
                for (picotoml_node* item = first; item; item = item->next) {
                    printf("\n");
                    for (int i = 0; i < indent; i++) printf("  ");
                    printf("[[%s]]\n", new_prefix);
                    print_node_toml(item, indent, false, new_prefix);
                }
            }
        }
    }
}

int main() {
    const char* toml = 
        "title = \"Edge Case Test\"\n"
        "decimal_underscores = 1_000_000\n"
        "special_floats = { inf = inf, nan = nan, neg_inf = -inf }\n"
        "multi_line = \"\"\"\n"
        "line 1 \\\n"
        "line 2\"\"\"\n"
        "\n"
        "[nested.dotted.key]\n"
        "success = true\n"
        "\n"
        "[[products]]\n"
        "name = \"Hammer\"\n"
        "\n"
        "[[products]]\n"
        "name = \"Nail\"\n"
        "details = { material = \"steel\", count = 100 }\n";

    char* err = NULL;
    picotoml_node* root = picotoml_parse(toml, &err);
    if (!root) { printf("Error: %s\n", err); return 1; }

    printf("# Robust TOML Output:\n");
    print_node_toml(root, 0, true, "");

    picotoml_free(root);
    return 0;
}
