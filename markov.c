/* Markov Text Generator
 *
 * Raises warnings if not compiled with a C89 or newer compiler.
 * Tested with GCC 5.1.
 *
 * Copyright ©2015 Joel Burton <joel@joelburton.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *
 * This file contains the functions to make a hashmap of bigram chains
 * and to generate text from those chains.
 *
 * */

#include <glib.h>
#include <glib/gprintf.h>


#define MAX_LINE_LENGTH 60  ///< Maximum length of generated text line


/** Bigram: a two-word tuple. */

typedef struct _bigram {
    gchar *first;   ///< first word
    gchar *second;  ///< second word
} Bigram;


/** Hash a bigram.
 *
 * @param bigram: bigram to hash.
 *
 * Creates a hash by hashing both strings and combining with bitwise OR.
 * This is needed for the hashmap, since we'll be storing bigrams as keys
 * in our hashmap.
 *
 * @return Hash
 */

guint bigram_hash(gconstpointer bigram)
{
    return (g_str_hash(((Bigram *) bigram)->first) |
            g_str_hash(((Bigram *) bigram)->second));
}


/** Check two bigrams for equality.
 *
 * @param bigram1: bigram to compare
 * @param bigram2: bigram to compare
 *
 * Bigrams are equal if both words are equal. This is needed since we're
 * storing bigrams in our hashmap, and need to see if a key is equal to
 * another bigram.
 *
 * @return TRUE or FALSE
 */

gboolean bigram_equal(gconstpointer bigram1, gconstpointer bigram2)
{
    return (g_str_equal(((Bigram *) bigram1)->first,
                        ((Bigram *) bigram2)->first) &&
            g_str_equal(((Bigram *) bigram1)->second,
                        ((Bigram *) bigram2)->second));
}


/** Add a word to a bigram chain.
 *
 * @param chains: chains hashmap
 * @param first: first word of bigram
 * @param second: second word of bigram
 * @param follows: word to add to bigram chain
 *
 * Find or create the chain of (first, second), and add follows to it.
 */

void add_word_to_chain(GHashTable *chains,
                       const gchar *first,
                       const gchar *second,
                       gchar *follows)
{
    GPtrArray *follow_words;

    Bigram *bigram = g_malloc(sizeof(Bigram));
    bigram->first = g_strdup(first);
    bigram->second = g_strdup(second);

    follow_words = g_hash_table_lookup(chains, bigram);
    if (!follow_words)
        follow_words = g_ptr_array_new();
    g_ptr_array_add(follow_words, follows);

    g_hash_table_insert(chains, bigram, follow_words);
}


/** Create hash table of (word-1, word-2) => [follow-word-1, follow-word-2, ...]
 *
 * @param in_string: C string of input text
 *
 * @return pointer to hash table
 */

GHashTable *makeChains(const gchar *in_string)
{
    gchar *first = NULL;
    gchar *second = NULL;
    gchar *follows = NULL;

    GHashTable *chains = g_hash_table_new(bigram_hash, bigram_equal);

    // Split input string on whitespace into words list
    gchar **words = g_strsplit_set(in_string, " \n", -1);

    // We need at least two words of input text
    if (!words[0] || !words[1]) {
        g_printf("ERROR: too short!\n");
        return NULL;
    }

    first = words[0];
    second = words[1];

    for (int i = 2; words[i]; i++) {
        follows = words[i];

        // Skip empty words (which we get with double-whitespace in input)
        if (g_str_equal(follows, ""))
            continue;

        add_word_to_chain(chains, first, second, follows);

        // Move words down so our next chain is (curr-second, curr-follows)
        first = second;
        second = follows;
    }

    // Add the last two words of source with a NULL entry as follows;
    // we'll use this to stop our text generation
    add_word_to_chain(chains, first, second, NULL);

    return chains;
}


/** Make markov chain from bigram hashtable.
 *
 * @param chains: hashmap of bigram chains
 *
 * Prints generated text, breaking at MAX_LINE_LENGTH.
 */

void makeText(GHashTable *chains)
{
    Bigram *bigram;
    guint line_length = 0;
    guint n_chains;

    gpointer *keys = g_hash_table_get_keys_as_array(chains, &n_chains);

    // Find an uppercase word to start
    while (1) {
        bigram = keys[g_random_int_range(0, n_chains)];
        if (g_ascii_isupper(*(bigram->first)))
            break;
    }

    line_length += g_printf("%s %s ", bigram->first, bigram->second);

    // Generate text forever, until we hit end of our source text
    while (1) {
        GPtrArray *chain;
        gchar *follows;

        chain = g_hash_table_lookup(chains, (gconstpointer) bigram);

        gint32 rand_chain_index = g_random_int_range(0, chain->len);
        follows = chain->pdata[rand_chain_index];

        if (!follows)
            // We've reached the sentinel end of source text; stop
            break;

        // Handle breaking lines
        line_length += sizeof(follows);
        if (line_length > MAX_LINE_LENGTH) {
            g_printf("\n");
            line_length = sizeof(follows);
        }
        g_printf("%s ", follows);

        // Shuffle down: new bigram is (second, follows)
        bigram->first = bigram->second;
        bigram->second = follows;
    }
    g_printf("\n");
}


/** Handle command-line
 *
 * Called with one parameter, the file to read text from.
 *
 * @return 0 for success, 1 for error
 */

int main(int argc, char *argv[])
{
    g_set_application_name("Markov Generator");
    const gchar *prg_name = g_path_get_basename(argv[0]);
    if (argc != 2) {
        g_printf("%s: Generate Markov chain\n\n", prg_name);
        g_printf("  usage: %s [file]\n", prg_name);
        return 1;
    }

    gchar *in_str;
    if (g_file_get_contents(argv[1], &in_str, NULL, NULL) == FALSE)
        return 1;

    GHashTable *chains = makeChains(in_str);
    if (!chains)
        return 1;

    makeText(chains);
    return 0;
}