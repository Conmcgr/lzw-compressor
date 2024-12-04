#include "decode.h"
#include <stdio.h>
#include <stdlib.h>

dict_element **string_table;
int current_code;

unsigned long input_bits = 0;
int input_bits_count = 0;

void decode_add_element(int prefix, unsigned char new_char, int code)
{
    dict_element *new_entry = (dict_element *)malloc(sizeof(dict_element));
    new_entry->prefix = prefix;
    new_entry->last_char = new_char;
    new_entry->code = code;
    new_entry->next = NULL;
    string_table[code] = new_entry;
}

void decode_initialize_table(int table_size)
{
    string_table = (dict_element **)malloc(table_size * sizeof(dict_element *));
    if (string_table == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    for (int i = 0; i < table_size; i++)
    {
        string_table[i] = NULL;
    }
    for (int i = 0; i < 256; i++)
    {
        decode_add_element(-1, i, i);
    }
    current_code = 256;
}
void rebuild_string(int code)
{
    if (code == -1)
    {
        return;
    }

    dict_element *entry = string_table[code];

    if (entry == NULL)
    {
        return;
    }

    if (entry->prefix != -1)
    {
        rebuild_string(entry->prefix);
    }

    unsigned char last_char = (unsigned char)entry->last_char; // Cast to unsigned char
    putchar(last_char);
}
int getcode()
{
    int code;

    if (scanf("%d", &code) == 1)
    {
        return code;
    }

    else
    {
        return EOF;
    }
}
int get_first_char(int code)
{
    dict_element *entry = string_table[code];

    if (entry == NULL)
    {
        return -1;
    }

    while (entry->prefix != -1)
    {
        entry = string_table[entry->prefix];
        if (entry == NULL)
        {
            return -1;
        }
    }

    return entry->last_char;
}
void dump_decode_dictionary(int table_size, int append)
{
    /*if (!getenv("DBG"))
    {
        return;
    }*/

    FILE *fp;

    if (append == 0)
    {
        fp = fopen("./DBG.decode", "w");
    }
    else
    {
        fp = fopen("./DBG.decode", "a");
    }
    if (fp == NULL)
    {
        perror("Failed to open DBG.decode");
        return;
    }

    fprintf(fp, "Code\tPrefix\tCharacter\n");

    for (int i = 0; i < table_size; i++)
    {
        dict_element *entry = string_table[i];
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
        }
    }

    fclose(fp);
}
int *prune_decode(int table_size)
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
        dict_element *entry = string_table[i];
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
            new_current_code++;
        }
    }

    dict_element **table_copy = (dict_element **)malloc(table_size * sizeof(dict_element *));

    if (table_copy == NULL)
    {
        perror("Decode: failed to allocate memory for table_copy");
        free(is_prefix);
        free(code_mapping);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < table_size; i++)
    {
        table_copy[i] = string_table[i];
    }

    for (int i = 0; i < table_size; i++)
    {
        string_table[i] = NULL;
    }

    for (int i = 0; i < 256; i++)
    {
        decode_add_element(-1, i, i);
    }

    // dump_decode_dictionary(table_size, 1);
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

            // fprintf(stderr, "Decode inserting old code: %d, as new code: %d \n", entry->code, new_code);
            decode_add_element(new_prefix, entry->last_char, new_code);
        }
    }
    // dump_decode_dictionary(table_size, 1);
    current_code = new_current_code;
    free(is_prefix);
    free(table_copy);

    return code_mapping;
}

void decode_free_table(dict_element **table, int table_size)
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

int getBitCode(int nbits)
{
    while (input_bits_count < nbits)
    {
        int input_byte = getc(stdin);
        if (input_byte == EOF)
        {
            if (input_bits_count == 0)
            {
                return EOF;
            }
            else
            {
                break;
            }
        }
        input_bits = input_bits << 8;
        input_bits = input_bits | (unsigned char)input_byte;
        input_bits_count += 8;
    }

    if (input_bits_count < nbits)
    {
        return EOF;
    }

    int shift_amount = input_bits_count - nbits;
    int code = (input_bits >> shift_amount) & ((1 << nbits) - 1);

    input_bits_count -= nbits;
    input_bits = input_bits & ((1 << input_bits_count) - 1);

    return code;
}

int decode_calculate_nbits(int current_code, int max_bit_length)
{
    int bits = 9;
    while ((1 << bits) <= current_code && bits < max_bit_length)
    {
        bits++;
    }
    return bits;
}

void decode(int stage)
{
    if (stage == 3)
    {
        int prune = getBitCode(8);
        int max_bits = getBitCode(8);
        int table_size = 1 << max_bits;
        int max_code = table_size - 1;

        int nbits = 9;

        decode_initialize_table(table_size);

        int C = getBitCode(nbits);

        if (C == EOF)
        {
            return;
        }

        rebuild_string(C);

        int finalK = get_first_char(C);
        int newC;

        while ((newC = getBitCode(nbits)) != EOF)
        {
            // fprintf(stderr, "Decode: Read code %d with %d bits, current_code=%d, nbits=%d\n", newC, nbits, current_code, nbits);
            dict_element *target = string_table[newC];
            if (target != NULL)
            {
                if (current_code == (1 << nbits) - 2 && nbits < max_bits)
                {
                    nbits++;
                    // fprintf(stderr, "Decode: Incremented nbits to %d\n", nbits);
                }
                if (current_code == max_code && prune == 1)
                {
                    // dump_decode_dictionary(table_size, 1);
                    int *code_mapping = prune_decode(table_size);
                    // dump_decode_dictionary(table_size, 1);
                    nbits = decode_calculate_nbits(current_code, max_bits);
                    ;

                    if (C != -1)
                    {
                        C = code_mapping[C];
                    }

                    free(code_mapping);
                }

                rebuild_string(newC);
                int K = get_first_char(newC);

                if (current_code <= max_code)
                {
                    decode_add_element(C, (unsigned char)K, current_code);
                    current_code++;
                }

                finalK = K;
            }
            else
            {
                rebuild_string(C);
                putchar((unsigned char)finalK);

                if (current_code == (1 << nbits) - 2 && nbits < max_bits)
                {
                    nbits++;
                }

                if (current_code == max_code && prune == 1)
                {
                    // dump_decode_dictionary(table_size, 1);
                    int *code_mapping = prune_decode(table_size);
                    // dump_decode_dictionary(table_size, 1);
                    nbits = decode_calculate_nbits(current_code, max_bits);
                    ;

                    if (C != -1)
                    {
                        C = code_mapping[C];
                    }

                    free(code_mapping);
                }

                if (current_code <= max_code)
                {
                    decode_add_element(C, (unsigned char)finalK, current_code);
                    current_code++;
                }
            }

            C = newC;
        }
        if (getenv("DBG"))
        {
            dump_decode_dictionary(table_size, 0);
        }

        decode_free_table(string_table, table_size);
    }
    else
    {
        // Stage 1 or 2
        int prune = getcode();
        int max_bits = getcode();
        int table_size = 1 << max_bits;
        int max_code = table_size - 1;

        decode_initialize_table(table_size);

        int C = getcode();

        if (C == EOF)
        {
            return;
        }

        rebuild_string(C);

        int finalK = get_first_char(C);
        int newC;

        while ((newC = getcode()) != EOF)
        {
            dict_element *target = string_table[newC];
            if (target != NULL)
            {
                if (current_code == max_code && prune == 1)
                {
                    // dump_decode_dictionary(table_size, 1);
                    int *code_mapping = prune_decode(table_size);
                    // dump_decode_dictionary(table_size, 1);

                    if (C != -1)
                    {
                        C = code_mapping[C];
                    }

                    free(code_mapping);
                }

                rebuild_string(newC);
                int K = get_first_char(newC);

                if (current_code <= max_code)
                {
                    decode_add_element(C, (unsigned char)K, current_code);
                    current_code++;
                }

                finalK = K;
            }
            else
            {
                rebuild_string(C);
                putchar((unsigned char)finalK);

                if (current_code == max_code && prune == 1)
                {
                    // dump_decode_dictionary(table_size, 1);
                    int *code_mapping = prune_decode(table_size);
                    // dump_decode_dictionary(table_size, 1);

                    if (C != -1)
                    {
                        C = code_mapping[C];
                    }

                    free(code_mapping);
                }

                if (current_code <= max_code)
                {
                    decode_add_element(C, (unsigned char)finalK, current_code);
                    current_code++;
                }
            }

            C = newC;
        }
        if (getenv("DBG"))
        {
            dump_decode_dictionary(table_size, 0);
        }

        decode_free_table(string_table, table_size);
    }
}
