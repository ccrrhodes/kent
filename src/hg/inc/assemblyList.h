/* assemblyList.h was originally generated by the autoSql program, which also 
 * generated assemblyList.c and assemblyList.sql.  This header links the database and
 * the RAM representation of objects. */

#ifndef ASSEMBLYLIST_H
#define ASSEMBLYLIST_H

#include "jksql.h"
#define ASSEMBLYLIST_NUM_COLS 13

extern char *assemblyListCommaSepFieldNames;

struct assemblyList
/* listing all UCSC genomes, and all NCBI assemblies, with search priority, and status if browser available or can be requested */
    {
    struct assemblyList *next;  /* Next in singly linked list. */
    char *name;	/* UCSC genome: dbDb name or GenArk/NCBI accession */
    unsigned *priority;	/* assigned search priority */
    char *commonName;	/* a common name */
    char *scientificName;	/* binomial scientific name */
    unsigned *taxId;	/* Entrez taxon ID: www.ncbi.nlm.nih.gov/taxonomy/?term=xxx */
    char *clade;	/* approximate clade: primates mammals birds fish ... etc ... */
    char *description;	/* other description text */
    unsigned char *browserExists;	/* 1 == this assembly is available at UCSC, 0 == can be requested */
    char *hubUrl;	/* path name to hub.txt: GCF/000/001/405/GCF_000001405.39/hub.txt */
    unsigned *year;	/* year of assembly construction */
    char *refSeqCategory;	/* one of: reference, representative or na */
    char *versionStatus;	/* one of: latest, replaced or suppressed */
    char *assemblyLevel;	/* one of: complete, chromosome, scaffold or contig */
    };

void assemblyListStaticLoadWithNull(char **row, struct assemblyList *ret);
/* Load a row from assemblyList table into ret.  The contents of ret will
 * be replaced at the next call to this function. */

struct assemblyList *assemblyListLoadByQuery(struct sqlConnection *conn, char *query);
/* Load all assemblyList from table that satisfy the query given.  
 * Where query is of the form 'select * from example where something=something'
 * or 'select example.* from example, anotherTable where example.something = 
 * anotherTable.something'.
 * Dispose of this with assemblyListFreeList(). */

void assemblyListSaveToDb(struct sqlConnection *conn, struct assemblyList *el, char *tableName, int updateSize);
/* Save assemblyList as a row to the table specified by tableName. 
 * As blob fields may be arbitrary size updateSize specifies the approx size
 * of a string that would contain the entire query. Arrays of native types are
 * converted to comma separated strings and loaded as such, User defined types are
 * inserted as NULL. This function automatically escapes quoted strings for mysql. */

struct assemblyList *assemblyListLoadWithNull(char **row);
/* Load a assemblyList from row fetched with select * from assemblyList
 * from database.  Dispose of this with assemblyListFree(). */

struct assemblyList *assemblyListLoadAll(char *fileName);
/* Load all assemblyList from whitespace-separated file.
 * Dispose of this with assemblyListFreeList(). */

struct assemblyList *assemblyListLoadAllByChar(char *fileName, char chopper);
/* Load all assemblyList from chopper separated file.
 * Dispose of this with assemblyListFreeList(). */

#define assemblyListLoadAllByTab(a) assemblyListLoadAllByChar(a, '\t');
/* Load all assemblyList from tab separated file.
 * Dispose of this with assemblyListFreeList(). */

struct assemblyList *assemblyListCommaIn(char **pS, struct assemblyList *ret);
/* Create a assemblyList out of a comma separated string. 
 * This will fill in ret if non-null, otherwise will
 * return a new assemblyList */

void assemblyListFree(struct assemblyList **pEl);
/* Free a single dynamically allocated assemblyList such as created
 * with assemblyListLoad(). */

void assemblyListFreeList(struct assemblyList **pList);
/* Free a list of dynamically allocated assemblyList's */

void assemblyListOutput(struct assemblyList *el, FILE *f, char sep, char lastSep);
/* Print out assemblyList.  Separate fields with sep. Follow last field with lastSep. */

#define assemblyListTabOut(el,f) assemblyListOutput(el,f,'\t','\n');
/* Print out assemblyList as a line in a tab-separated file. */

#define assemblyListCommaOut(el,f) assemblyListOutput(el,f,',',',');
/* Print out assemblyList as a comma separated list including final comma. */

void assemblyListJsonOutput(struct assemblyList *el, FILE *f);
/* Print out assemblyList in JSON format. */

/* -------------------------------- End autoSql Generated Code -------------------------------- */

#define defaultAssemblyListTableName "assemblyList"
/* Name of table that maintains the list of all assemblies, whether
 *  available here or could be built */

#define assemblyListTableConfVariable    "hub.assemblyListTableName"
/* the name of the hg.conf variable to use something other than the default */

char *assemblyListTableName();
/* return the assemblyList table name from the environment,
 * or hg.conf, or use the default.  Cache the result */

char *asmListMatchAllWords(char *searchString);
/* given a multiple word search string, fix it up so it will be
 *  a 'match all words' MySQL FULLTEXT query, with the required + signs
 *  in front of the words when appropriate
 */

#endif /* ASSEMBLYLIST_H */
