int reloc_global_without(int b) {
    return b + 1;
}

static int reloc_static_without(int b) {
    return b + 2;
}

int __attribute__((visibility ("hidden"))) reloc_global_hidden(int b) {
    return reloc_static_without(b) + 3;
}

int __attribute__((visibility ("default"))) reloc_global_default(int b) {
    return b + 4;
}
