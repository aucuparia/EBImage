/* -------------------------------------------------------------------------
Counting objects determined in segmentations like watershed
Copyright (c) 2006 Oleg Sklyar
See: ../LICENSE for license, LGPL
------------------------------------------------------------------------- */

#include "common.h"

/*----------------------------------------------------------------------- */
#define BG 0.0
#define N_FEATURES 6

/*----------------------------------------------------------------------- */
SEXP
lib_assignFeatures (SEXP x, SEXP ref) {
    SEXP res;

    PROTECT ( res = Rf_duplicate(x) );
    assign_features (res, ref);
    UNPROTECT (1);
    return res;    
};

/*----------------------------------------------------------------------- */
/* will assign features field to the argument, modifying argument */
/* used in watershed and other routines. We assume that supplied x is
ALREADY a duplicate of that sent from R, so we modify it */
void assign_features (SEXP x, SEXP ref) {
    SEXP features, * fm, * dm;
    int nx, ny, nz, nprotect, im, i, j, nobj, obj, perdone;
    double * data, * refdata, * fmdata;
    
    if ( !isImage(x) ) return;
    if ( !isImage(ref) && ref != R_NilValue ) return;
    
    nx = INTEGER ( GET_DIM(x) )[0];
    ny = INTEGER ( GET_DIM(x) )[1];
    nz = INTEGER ( GET_DIM(x) )[2];
    nprotect = 0;

    /* will be freed automatically */
    fm = (SEXP *) R_alloc (nz, sizeof(SEXP) );
    for ( im = 0; im < nz; im++ )
        fm[im] = R_NilValue;
    dm = (SEXP *) R_alloc (nz, sizeof(SEXP) );
    
    refdata = NULL;
    for ( im = 0; im < nz; im++ ) {
        /* get image data */
        data = &( REAL(x)[ im * nx * ny ] );
        if ( ref != R_NilValue )
            refdata = &( REAL(ref)[ im * nx * ny ] );
        /* get number of objects -- max index */
        nobj = 0;
        for ( i = 0; i < nx * ny; i++ )
            if ( data[i] > nobj ) nobj = data[i];
        if ( nobj < 1 ) continue;
        /* create features matrix */
        PROTECT ( fm[im] = allocVector(REALSXP, nobj * N_FEATURES) );
        nprotect++;
        fmdata = REAL (fm[im]);
        for ( i = 0; i < nobj * N_FEATURES; i++ )
            fmdata [i] = 0.0;
        /* set dimensions of the feature matrix */
        PROTECT ( dm[im] = allocVector(INTSXP, 2) );
        nprotect++;
        INTEGER ( dm[im] )[0] = nobj;
        INTEGER ( dm[im] )[1] = N_FEATURES;
        SET_DIM ( fm[im], dm[im] );
        /* go through pixels and collect descriptors */
        for ( i = 0; i < nx; i++ )
            for ( j = 0; j < ny; j++ ) {
                obj = data[i + j * nx];
                if ( obj < 1 ) continue;
                /* all indexes were 1, 2, 3, but C has 0-based indexes!!! */
                obj--;
                /* update x (+0), y (+1) */
                fmdata [obj] += i;
                fmdata [obj + nobj] += j;
                /* size (+2) */
                fmdata [obj + 2 * nobj] += 1.0;
                /* per (+3) */
                perdone = 0;
                if ( i > 0 )
                    if ( (int) data [i - 1 + j * nx] != obj + 1 ) { /* +1 because obj-- above */
                        perdone = 1;
                        fmdata [obj + 3 * nobj] += 1.0;
                    }
                if ( j > 0 && !perdone )
                    if ( (int) data [i + (j - 1) * nx] != obj + 1 ) { /* +1 because obj-- above */
                        perdone = 1;
                        fmdata [obj + 3 * nobj] += 1.0;
                    }
                if ( i < nx - 1 && !perdone )
                    if ( (int) data [i + 1 + j * nx] != obj + 1 ) { /* +1 because obj-- above */
                        perdone = 1;
                        fmdata [obj + 3 * nobj] += 1.0;
                    }
                if ( j < ny - 1 && !perdone )
                    if ( (int) data [i + (j + 1) * nx] != obj + 1 ) { /* +1 because obj-- above */
                        perdone = 1;
                        fmdata [obj + 3 * nobj] += 1.0;
                    }
                /* int (+4)  */
                if ( refdata && ref != R_NilValue ) 
                    fmdata [obj + 4 * nobj] += refdata [i + j * nx];
                /* edge (+5) */
                if ( i == 0 || j == 0 || i == nx - 1 || j == ny - 1 )
                    fmdata [obj + 5 * nobj] += 1.0;
                    
            }
        for ( obj = 0; obj < nobj; obj++ )
            if ( fmdata[obj + 2 * nobj] > 0 ) {
                fmdata [obj] = floor (fmdata [obj] / fmdata [obj + 2 * nobj] * 10.0) / 10.0;
                fmdata [obj + nobj] = floor (fmdata [obj + nobj] / fmdata [obj + 2 * nobj] * 10.0) / 10.0;
            }        
    }
    PROTECT ( features = allocVector(VECSXP, nz) );
    nprotect++;
    /* put fm into the list */
    for ( im = 0; im < nz; im++ )
        SET_VECTOR_ELT (features, im, fm[im] );
    SET_SLOT (x, mkString("features"), features);
    UNPROTECT(nprotect);
}
