#ifndef B6024EEC_47B1_43D8_A5E2_7E0CE27C1E65
#define B6024EEC_47B1_43D8_A5E2_7E0CE27C1E65

#include "encode.h"

void decode_add_element(int prefix, unsigned char new_char, int code);

dict_element *decode_search_table(int code);

void decode_initialize_table(int table_size);

void rebuild_string(int code);

int getcode();

int get_first_char(int code);

void dump_decode_dictionary(int table_size, int append);

int *prune_decode(int table_size);

void decode_free_table(dict_element **table, int table_size);

int getBitCode(int nbits);

int decode_calculate_nbits(int current_code, int max_bit_length);

void decode(int stage);

#endif /* B6024EEC_47B1_43D8_A5E2_7E0CE27C1E65 */