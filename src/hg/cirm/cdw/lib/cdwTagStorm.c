/* cdwTagStorm - create a tag storm out of database contents. */

#include "common.h"
#include "hash.h"
#include "dystring.h"
#include "cheapcgi.h"
#include "jksql.h"
#include "tagStorm.h"
#include "intValTree.h"
#include "cdw.h"
#include "cdwLib.h"

static void addPairedEndTags(struct tagStorm *tagStorm, struct tagStanza *stanza, 
    struct cdwQaPairedEndFastq *pair, char *mateAccession)
/* Add tags from pair to stanza, also add mateAccession */
{
if (mateAccession != NULL)
    tagStanzaAdd(tagStorm, stanza, "paired_end_mate", mateAccession);
tagStanzaAddDouble(tagStorm, stanza, "paired_end_concordance", pair->concordance);
tagStanzaAddDouble(tagStorm, stanza, "paired_end_distance_mean", pair->distanceMean);
tagStanzaAddDouble(tagStorm, stanza, "paired_end_distance_min", pair->distanceMin);
tagStanzaAddDouble(tagStorm, stanza, "paired_end_distance_max", pair->distanceMax);
tagStanzaAddDouble(tagStorm, stanza, "paired_end_distance_std", pair->distanceStd);
}

struct tagStorm *cdwTagStorm(struct sqlConnection *conn)
/* Load  cdwMetaTags.tags, cdwFile.tags, and select other fields into a tag
 * storm for searching */
{
/* Build up a cache of cdwSubmitDir */
char query[512];
sqlSafef(query, sizeof(query), "select * from cdwSubmitDir");
struct cdwSubmitDir *dir, *dirList = cdwSubmitDirLoadByQuery(conn, query);
struct rbTree *submitDirTree = intValTreeNew();
for (dir = dirList; dir != NULL; dir = dir->next)
   intValTreeAdd(submitDirTree, dir->id, dir);
verbose(2, "cdwTagStorm: %d items in submitDirTree\n", submitDirTree->n);

/* First pass through the cdwMetaTags table.  Make up a high level stanza for each
 * row, and save a reference to it in metaTree. */
struct tagStorm *tagStorm = tagStormNew("constructed from cdwMetaTags and cdwFile");
struct rbTree *metaTree = intValTreeNew();
sqlSafef(query, sizeof(query), "select id,tags from cdwMetaTags");
struct sqlResult *sr = sqlGetResult(conn, query);
char **row;
while ((row = sqlNextRow(sr)) != NULL)
    {
    unsigned id = sqlUnsigned(row[0]);
    char *cgiVars = row[1];
    struct tagStanza *stanza = tagStanzaNew(tagStorm, NULL);
    char *var, *val;
    while (cgiParseNext(&cgiVars, &var, &val))
	 {
         tagStanzaAdd(tagStorm, stanza, var, val);
	 }
    intValTreeAdd(metaTree, id, stanza);
    }
sqlFreeResult(&sr);
verbose(2, "cdwTagStorm: %d items in metaTree\n", metaTree->n);


/* Now go through the cdwFile table, adding its files as substanzas to 
 * meta cdwMetaTags stanzas. */
sqlSafef(query, sizeof(query), 
    "select cdwFile.*,cdwValidFile.* from cdwFile,cdwValidFile "
    "where cdwFile.id=cdwValidFile.fileId ");
sr = sqlGetResult(conn, query);
struct rbTree *fileTree = intValTreeNew();
while ((row = sqlNextRow(sr)) != NULL)
    {
    struct cdwFile ef;
    struct cdwValidFile vf;
    cdwFileStaticLoad(row, &ef);
    cdwValidFileStaticLoad(row + CDWFILE_NUM_COLS, &vf);
    struct tagStanza *metaStanza = intValTreeFind(metaTree, ef.metaTagsId);
    if (metaStanza != NULL)
	{
	/* Figure out file name independent of cdw location */
	char name[FILENAME_LEN], extension[FILEEXT_LEN];
	splitPath(ef.cdwFileName, NULL, name, extension);
	char fileName[PATH_LEN];
	safef(fileName, sizeof(fileName), "%s%s", name, extension);


	struct tagStanza *stanza = tagStanzaNew(tagStorm, metaStanza);
	intValTreeAdd(fileTree, ef.id, stanza);

	/** Add stuff we want in addition to what is in ef.tags */

	/* Deprecated is important to know */
	if (!isEmpty(ef.deprecated))
	    tagStanzaAdd(tagStorm, stanza, "deprecated", ef.deprecated);

	/* Basic file name info */
	tagStanzaAdd(tagStorm, stanza, "file_name", fileName);
	tagStanzaAddLongLong(tagStorm, stanza, "file_size", ef.size);
	tagStanzaAdd(tagStorm, stanza, "md5", ef.md5);

	/* Stuff gathered from submission */
	tagStanzaAdd(tagStorm, stanza, "submit_file_name", ef.submitFileName);
	struct cdwSubmitDir *dir = intValTreeFind(submitDirTree, ef.submitDirId);
	if (dir != NULL)
	    {
	    tagStanzaAdd(tagStorm, stanza, "submit_dir", dir->url);
	    }

	tagStanzaAdd(tagStorm, stanza, "cdw_file_name", ef.cdwFileName);
	tagStanzaAdd(tagStorm, stanza, "accession", vf.licensePlate);
	if (vf.itemCount != 0)
	    tagStanzaAddLongLong(tagStorm, stanza, "item_count", vf.itemCount);
	if (vf.depth != 0)
	    tagStanzaAddDouble(tagStorm, stanza, "seq_depth", vf.depth);
	if (vf.mapRatio != 0)
	    tagStanzaAddDouble(tagStorm, stanza, "map_ratio", vf.mapRatio);

	/* Add tag field */
	char *cgiVars = ef.tags;
	char *var,*val;
	while (cgiParseNext(&cgiVars, &var, &val))
	    tagStanzaAdd(tagStorm, stanza, var, val);
	}
    }
sqlFreeResult(&sr);
verbose(2, "cdwTagStorm: %d items in fileTree\n", fileTree->n);

/** Add selected tags from other tables as part of stanza tags. */

/* Add cdwFastqFile info */
sqlSafef(query, sizeof(query), "select * from cdwFastqFile");
sr = sqlGetResult(conn, query);
while ((row = sqlNextRow(sr)) != NULL)
    {
    struct cdwFastqFile *fq = cdwFastqFileLoad(row);
    struct tagStanza *stanza = intValTreeFind(fileTree, fq->fileId);
    if (stanza != NULL)
	{
	if (fq->readSizeMin == fq->readSizeMax)
	    tagStanzaAddLongLong(tagStorm, stanza, "read_size", fq->readSizeMin);
	else
	    {
	    tagStanzaAddDouble(tagStorm, stanza, "read_size_mean", fq->readSizeMean);
	    tagStanzaAddDouble(tagStorm, stanza, "read_size_min", fq->readSizeMin);
	    tagStanzaAddDouble(tagStorm, stanza, "read_size_max", fq->readSizeMax);
	    tagStanzaAddDouble(tagStorm, stanza, "read_size_std", fq->readSizeStd);
	    }
	tagStanzaAdd(tagStorm, stanza, "fastq_qual_type", fq->qualType);
	tagStanzaAddDouble(tagStorm, stanza, "fastq_qual_mean", fq->qualMean);
	tagStanzaAddDouble(tagStorm, stanza, "at_ratio", fq->atRatio);
	cdwFastqFileFree(&fq);
	}
    }
sqlFreeResult(&sr);

/* Add cdwQaPairedEndFastq */
sqlSafef(query, sizeof(query), "select * from cdwQaPairedEndFastq where recordComplete != 0");
sr = sqlGetResult(conn, query);
while ((row = sqlNextRow(sr)) != NULL)
    {
    struct cdwQaPairedEndFastq pair;
    cdwQaPairedEndFastqStaticLoad(row, &pair);
    struct tagStanza *p1 = intValTreeFind(fileTree, pair.fileId1);
    struct tagStanza *p2 = intValTreeFind(fileTree, pair.fileId2);
    if (p1 != NULL && p2 != NULL)
	{
	char *acc1 = tagFindVal(p1, "accession");
	char *acc2 = tagFindVal(p2, "accession");
	addPairedEndTags(tagStorm, p1, &pair, acc2);
	addPairedEndTags(tagStorm, p2, &pair, acc1);
	}
    }
sqlFreeResult(&sr);

/* Build up in-memory random access data structure for enrichment targets */
struct rbTree *enrichTargetTree = intValTreeNew();
sqlSafef(query, sizeof(query), "select * from cdwQaEnrichTarget");
sr = sqlGetResult(conn, query);
while ((row = sqlNextRow(sr)) != NULL)
    {
    struct cdwQaEnrichTarget *target = cdwQaEnrichTargetLoad(row);
    intValTreeAdd(enrichTargetTree, target->id, target);
    }
sqlFreeResult(&sr);

/* Add cdwQaEnrich - here we'll supply exon, chrY, and whatever they put in their enriched_in
 * data, a subset of all */
sqlSafef(query, sizeof(query), "select * from cdwQaEnrich");
sr = sqlGetResult(conn, query);
while ((row = sqlNextRow(sr)) != NULL)
    {
    struct cdwQaEnrich rich;
    cdwQaEnrichStaticLoad(row, &rich);
    struct cdwQaEnrichTarget *target = intValTreeFind(enrichTargetTree, rich.qaEnrichTargetId);
    if (target != NULL)
        {
	char *targetName = target->name;
	struct tagStanza *stanza = intValTreeFind(fileTree, rich.fileId);
	if (stanza != NULL)
	    {
	    char *enrichedIn = tagFindVal(stanza, "enriched_in");
	    boolean onTarget = FALSE;
	    if (enrichedIn != NULL && sameWord(enrichedIn, targetName))
	        onTarget = TRUE;
	    else if (sameWord(targetName, "exon"))
	        onTarget = TRUE;
	    else if (sameWord(targetName, "promoter"))
	        onTarget = TRUE;
	    else if (sameWord(targetName, "chrY"))
	        onTarget = TRUE;
	    if (onTarget)
	        {
		char tagName[128];
		safef(tagName, sizeof(tagName), "enriched_%s", targetName);
		if (!tagFindVal(stanza, tagName))
		    tagStanzaAddDouble(tagStorm, stanza, tagName, rich.enrichment);
		safef(tagName, sizeof(tagName), "coverage_%s", targetName);
		if (!tagFindVal(stanza, tagName))
		    tagStanzaAddDouble(tagStorm, stanza, tagName, rich.coverage);
		}
	    }
	}
    else
        verbose(2, "Missing on qaEnrichTargetId %u\n", rich.qaEnrichTargetId);
    }
sqlFreeResult(&sr);


/* Clean up and go home */
rbTreeFree(&submitDirTree);
rbTreeFree(&metaTree);
rbTreeFree(&fileTree);
tagStormReverseAll(tagStorm);
return tagStorm;
}
