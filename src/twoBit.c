#include "kent/common.h"
#include "kent/dnaseq.h"
#include "kent/twoBit.h"

#include "IRanges_interface.h"

#include "twoBit.h"

void twoBit_finalizer(SEXP s)
{
    if (R_ExternalPtrAddr(s) == NULL)
        return;
    struct twoBit *twoBit = (struct twoBit *) R_ExternalPtrAddr(s);
    twoBitFree(&twoBit);
    R_ClearExternalPtr(s); /* not strictly necessary, but good practice */
}

/* --- .Call ENTRY POINT --- */
SEXP DNAString_to_twoBit(SEXP r_dna, SEXP r_mask, SEXP r_seqname) {
    dnaUtilOpen();

    /* Input validation */
    if (!isString(r_dna) || length(r_dna) != 1)
        error("'r_dna' must be a single character string");
    if (!isString(r_seqname) || length(r_seqname) != 1)
        error("'r_seqname' must be a single character string");

    const DNA *dna = CHAR(asChar(r_dna));
    struct dnaSeq *seq = newDnaSeq((DNA *)dna, strlen(dna),
                                   (char *)CHAR(asChar(r_seqname)));

    if (seq == NULL)
        error("Failed to create DNA sequence");

    /* Convert DNA sequence to twoBit */
    struct twoBit *twoBit = twoBitFromDnaSeq(seq, FALSE);
    if (twoBit == NULL) {
        /* Clean up seq before reporting error */
        seq->dna = NULL; /* do not free memory owned by R */
        freeDnaSeq(&seq);
        error("Failed to convert DNA sequence to twoBit format");
    }

    /* Process mask information */
    int *mask_start = NULL;
    int *mask_width = NULL;
    int mask_count = 0;

    /* Check that mask is an IRanges object */
    if (!isNull(r_mask)) {
        mask_start = INTEGER(get_IRanges_start(r_mask));
        mask_width = INTEGER(get_IRanges_width(r_mask));
        mask_count = get_IRanges_length(r_mask);

        if (mask_count < 0) {
            /* Clean up before reporting error */
            seq->dna = NULL;
            freeDnaSeq(&seq);
            twoBitFree(&twoBit);
            error("Invalid mask ranges");
        }
    }

    /* Allocate memory for mask data if needed */
    if (mask_count > 0) {
        twoBit->maskStarts = (uint *)R_alloc(mask_count, sizeof(uint));
        if (twoBit->maskStarts == NULL) {
            seq->dna = NULL;
            freeDnaSeq(&seq);
            twoBitFree(&twoBit);
            error("Memory allocation failed for mask starts");
        }

        twoBit->maskSizes = (uint *)R_alloc(mask_count, sizeof(uint));
        if (twoBit->maskSizes == NULL) {
            seq->dna = NULL;
            freeDnaSeq(&seq);
            twoBitFree(&twoBit);
            error("Memory allocation failed for mask sizes");
        }

        /* Copy mask data */
        for (int i = 0; i < mask_count; i++) {
            if (mask_start[i] <= 0) {
                warning("Mask start position must be positive, adjusting start position %d", i+1);
                twoBit->maskStarts[i] = 0;
            } else {
                twoBit->maskStarts[i] = mask_start[i] - 1; /* Convert to 0-based indexing */
            }

            if (mask_width[i] <= 0) {
                warning("Mask width must be positive, ignoring mask %d", i+1);
                twoBit->maskSizes[i] = 0;
            } else {
                twoBit->maskSizes[i] = mask_width[i];
            }
        }

        twoBit->maskBlockCount = mask_count;
    }

    /* Clean up */
    seq->dna = NULL; /* do not free memory owned by R */
    freeDnaSeq(&seq);

    /* Create and return external pointer */
    SEXP ans;
    PROTECT(ans = R_MakeExternalPtr(twoBit, R_NilValue, R_NilValue));

    /* Set up finalizer to avoid memory leaks */
    R_RegisterCFinalizerEx(ans, (R_CFinalizer_t)twoBit_finalizer, TRUE);

    /* Set class attribute */
    setAttrib(ans, R_ClassSymbol, mkString("twoBit"));
    UNPROTECT(1);

    return ans;
}
