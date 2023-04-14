
struct prog {
    const char *name;
    unsigned nbytes;
    uint8_t code[];
};

struct bin_header {
    uint32_t cookie;
    uint32_t header_nbytes;
    uint32_t link_addr;
    uint32_t code_nbytes;
    uint32_t bss_nbytes;
};

