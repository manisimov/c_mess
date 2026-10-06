int shared_global_without(int b) {
    return b + 1;
}

static int shared_static_without(int b) {
    return b + 2;
}

int __attribute__((visibility ("hidden"))) shared_global_hidden(int b) {
    return shared_static_without(b) + 3;
}

int __attribute__((visibility ("default"))) shared_global_default(int b) {
    return b + 4;
}
