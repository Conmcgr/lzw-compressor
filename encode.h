#ifndef AB1D5576_F986_4B13_AE08_ABCF50CDF907
#define AB1D5576_F986_4B13_AE08_ABCF50CDF907

typedef struct dict_element
{
    int prefix;
    int code;
    unsigned char last_char;
    struct dict_element *next;
} dict_element;

int hash_function(int prefix, unsigned char new_char, int table_size);

void add_element(int prefix, unsigned char new_char, int code, int table_size);

int search_table(int prefix, unsigned char new_char, int table_size);

void initialize_table(int table_size);

void dump_encode_dictionary(int table_size, int append);

int *prune_encode(int table_size);

void free_table(dict_element **table, int table_size);

void printBits(unsigned int code, int nbits);

int calculate_nbits(int current_code, int max_bit_length);

void encode(int p, int max_bit_length, int stage);

#endif /* AB1D5576_F986_4B13_AE08_ABCF50CDF907 */