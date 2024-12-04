#include "encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dict_element **string_table;
dict_element **code_table;
int current_code;
unsigned long bits_to_print = 0;
int bits_to_print_count = 0;

int hash_function(int prefix, unsigned char new_char, int table_size)
{
    return (((unsigned long)(prefix) << 8) | (unsigned)(new_char)) % table_size;
}

void add_element(int prefix, unsigned char new_char, int code, int table_size)
{
    dict_element *new = (dict_element *)malloc(sizeof(dict_element));
    new->prefix = prefix;
    new->last_char = new_char;
    new->code = code;
    new->next = NULL;

    int index = hash_function(prefix, new_char, table_size);

    new->next = string_table[index];
    string_table[index] = new;

    dict_element *new_code_entry = (dict_element *)malloc(sizeof(dict_element));
    new_code_entry->prefix = prefix;
    new_code_entry->last_char = new_char;
    new_code_entry->code = code;
    new_code_entry->next = NULL;

    code_table[code] = new_code_entry;
}

int search_table(int prefix, unsigned char new_char, int table_size)
{
    int index = hash_function(prefix, new_char, table_size);
    dict_element *target = string_table[index];

    while (target != NULL)
    {
        if (target->prefix == prefix && target->last_char == new_char)
        {
            return target->code;
        }
        target = target->next;
    }

    return -1;
}

void initialize_table(int table_size)
{
    string_table = (dict_element **)malloc(table_size * sizeof(dict_element *));
    code_table = (dict_element **)malloc(table_size * sizeof(dict_element *));

    if (string_table == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    if (code_table == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    for (int i = 0; i < table_size; i++)
    {
        string_table[i] = NULL;
        code_table[i] = NULL;
    }

    for (int i = 0; i < 256; i++)
    {
        add_element(-1, i, i, table_size);
    }

    current_code = 256;
}

void dump_encode_dictionary(int table_size, int append)
{
    FILE *fp;

    if (append == 0)
    {
        fp = fopen("./DBG.encode", "w");
    }
    else
    {
        fp = fopen("./DBG.encode", "a");
    }

    if (fp == NULL)
    {
        perror("Failed to open DBG.encode");
        return;
    }

    fprintf(fp, "Code\tPrefix\tCharacter\n");

    for (int i = 0; i < table_size; i++)
    {
        dict_element *entry = code_table[i];
        if (entry != NULL)
        {
            fprintf(fp, "%d\t%d\t", entry->code, entry->prefix);
            if (entry->last_char >= 32 && entry->last_char <= 126)
            {
                fprintf(fp, "%c\n", entry->last_char);
            }
            else
            {
                fprintf(fp, "\\x%02x\n", (unsigned char)entry->last_char);
            }
            // entry = entry->next;
        }
    }

    fclose(fp);
}

int *prune_encode(int table_size)
{
    int *is_prefix = (int *)malloc(table_size * sizeof(int));
    if (is_prefix == NULL)
    {
        perror("Encode: Failed to allocate memory for is_prefix");
        exit(EXIT_FAILURE);
    }

    int *code_mapping = (int *)malloc(table_size * sizeof(int));
    if (code_mapping == NULL)
    {
        perror("Encode: Failed to allocate memory for code_mapping");
        free(is_prefix);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < table_size; i++)
    {
        is_prefix[i] = 0;
        code_mapping[i] = -1;
    }

    for (int i = 256; i < table_size; i++)
    {
        dict_element *entry = code_table[i];
        if (entry != NULL && entry->prefix != -1)
        {
            is_prefix[entry->prefix] = 1;
        }
    }

    for (int i = 0; i < 256; i++)
    {
        code_mapping[i] = i;
    }

    int new_current_code = 256;

    for (int i = 256; i < table_size; i++)
    {
        if (is_prefix[i] == 1)
        {
            code_mapping[i] = new_current_code;
            // fprintf(stderr, "Encode: %d --> %d \n", i, new_current_code);
            new_current_code++;
        }
    }

    dict_element **table_copy = (dict_element **)malloc(table_size * sizeof(dict_element *));

    if (table_copy == NULL)
    {
        perror("Encode: failed to allocate memory for table_copy");
        free(is_prefix);
        free(code_mapping);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < table_size; i++)
    {
        table_copy[i] = code_table[i];
    }

    for (int i = 0; i < table_size; i++)
    {
        string_table[i] = NULL;
        code_table[i] = NULL;
    }

    for (int i = 0; i < 256; i++)
    {
        add_element(-1, i, i, table_size);
    }

    for (int i = 256; i < table_size; i++)
    {
        dict_element *entry = table_copy[i];

        if (entry != NULL && is_prefix[i] == 1)
        {
            int new_code = code_mapping[i];
            int new_prefix;

            if (entry->prefix == -1)
            {
                new_prefix = -1;
            }
            else
            {
                new_prefix = code_mapping[entry->prefix];
            }

            add_element(new_prefix, entry->last_char, new_code, table_size);
        }
    }

    current_code = new_current_code;
    free(is_prefix);
    free(table_copy);

    return code_mapping;
}

void free_table(dict_element **table, int table_size)
{
    for (int i = 0; i < table_size; i++)
    {
        dict_element *entry = table[i];
        while (entry != NULL)
        {
            dict_element *temp = entry;
            entry = entry->next;
            free(temp);
        }
    }
    free(table);
}

void printBits(unsigned int code, int nbits)
{
    bits_to_print = bits_to_print << nbits;
    bits_to_print = bits_to_print | code;
    bits_to_print_count += nbits;

    while (bits_to_print_count >= 8)
    {
        unsigned int byte_to_print = bits_to_print >> (bits_to_print_count - 8);
        byte_to_print = byte_to_print & 0xFF;
        putc(byte_to_print, stdout);
        bits_to_print_count -= 8;
    }
}

void printLastBits()
{
    if (bits_to_print_count > 0)
    {
        unsigned int byte_to_print = (bits_to_print << (8 - bits_to_print_count));
        byte_to_print = byte_to_print & 0xFF;
        putc(byte_to_print, stdout);
        bits_to_print_count = 0;
    }
}

int calculate_nbits(int current_code, int max_bit_length)
{
    int bits = 9;
    while ((1 << bits) <= current_code && bits < max_bit_length)
    {
        bits++;
    }
    return bits;
}

void encode(int p, int max_bit_length, int stage)
{

    int table_size = 1 << max_bit_length;
    int max_code = table_size - 1;
    int nbits;

    if (stage == 3)
    {
        nbits = 9;
        printBits(p, 8);
        printBits(max_bit_length, 8);
        initialize_table(table_size);

        int K_char;
        int prefix = -1;

        while ((K_char = getchar()) != EOF)
        {
            unsigned char K = (unsigned char)K_char;
            int code = search_table(prefix, K, table_size);

            if (code != -1)
            {
                prefix = code;
            }
            else
            {
                printBits(prefix, nbits);
                // fprintf(stderr, "Encode: Wrote code %d with %d bits, current_code=%d, nbits=%d\n", prefix, nbits, current_code, nbits);
                if (current_code == (1 << nbits) - 1 && nbits < max_bit_length)
                {
                    nbits++;
                    // fprintf(stderr, "Encode: Incremented nbits to %d\n", nbits);
                }

                if (current_code == max_code && p == 1)
                {
                    dump_encode_dictionary(table_size, 1);
                    int *code_mapping = prune_encode(table_size);
                    dump_encode_dictionary(table_size, 1);

                    nbits = calculate_nbits(current_code, max_bit_length);

                    if (prefix != -1)
                    {
                        prefix = code_mapping[prefix];
                    }

                    free(code_mapping);
                }
                if (current_code <= max_code)
                {
                    add_element(prefix, K, current_code, table_size);
                    current_code++;
                }
                prefix = K;
            }
        }

        printBits(prefix, nbits);
        printLastBits();
    }

    else
    {
        // Stage 1 or 2
        printf("%d\n", p);
        printf("%d\n", max_bit_length);
        initialize_table(table_size);

        int K_char;
        int prefix = -1;

        while ((K_char = getchar()) != EOF)
        {
            unsigned char K = (unsigned char)K_char;
            int code = search_table(prefix, K, table_size);

            if (code != -1)
            {
                prefix = code;
            }
            else
            {
                printf("%d\n", prefix);

                if (current_code == max_code && p == 1)
                {
                    // dump_encode_dictionary(table_size, 1);
                    int *code_mapping = prune_encode(table_size);
                    // dump_encode_dictionary(table_size, 1);

                    if (prefix != -1)
                    {
                        prefix = code_mapping[prefix];
                    }

                    free(code_mapping);
                }

                if (current_code <= max_code)
                {
                    add_element(prefix, K, current_code, table_size);
                    current_code++;
                }

                prefix = K;
            }
        }

        printf("%d\n", prefix);
    }

    /* if (getenv("DBG"))
    {
        dump_encode_dictionary(table_size, 0);
    }*/

    free_table(code_table, table_size);
    free_table(string_table, table_size);
}
