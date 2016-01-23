#include "scran.h"

double get_proportion (const double* expr, const int * order, const int& npairs, const int& minpairs, const int * m1, const int * m2) {
    // Assumes m1, m2 are 1-based, and order is already shifted by 1 to compensate.
    int was_first=0, was_total=0;
    for (int p=0; p<npairs; ++p) {
        const double& first=expr[order[m1[p]]];
        const double& second=expr[order[m2[p]]];
        if (first > second) { ++was_first; }
        if (first != second) { ++was_total; }      
    }
    if (was_total < minpairs) { return(NA_REAL); }
    return(double(was_first)/was_total);
}

SEXP shuffle_scores (SEXP ncells, SEXP ngenes, SEXP exprs, SEXP marker1, SEXP marker2, SEXP iter, SEXP miniter, SEXP minpair) try {
    if (!isInteger(ncells) || LENGTH(ncells)!=1) { throw std::runtime_error("number of cells must be an integer scalar"); }
    const int nc=asInteger(ncells);
    if (!isInteger(ngenes) || LENGTH(ngenes)!=1) { throw std::runtime_error("number of genes must be an integer scalar"); }
    const int ng=asInteger(ngenes);
    if (!isReal(exprs)) { throw std::runtime_error("matrix of expression values must be double-precision"); }
    if (nc*ng!=LENGTH(exprs)) { throw std::runtime_error("size of expression matrix is not consistent with provided dimensions"); }
    if (!isInteger(marker1) || !isInteger(marker2)) { throw std::runtime_error("vectors of marker pair genes must be integer"); }
    const int npairs = LENGTH(marker1);
    if (npairs!=LENGTH(marker2)) { throw std::runtime_error("vectors of marker pairs must be of the same length"); }

    if (!isInteger(iter) || LENGTH(iter)!=1) { throw std::runtime_error("number of iterations must be an integer scalar"); }
    const int nit=asInteger(iter);
    if (!isInteger(miniter) || LENGTH(miniter)!=1) { throw std::runtime_error("minimum number of iterations must be an integer scalar"); }
    const int minit=asInteger(miniter);
    if (!isInteger(minpair) || LENGTH(minpair)!=1) { throw std::runtime_error("minimum number of pairs must be an integer scalar"); }
    const int minp=asInteger(minpair);

    const double** exp_ptrs=(const double**)R_alloc(nc, sizeof(const double*));
    if (nc) { exp_ptrs[0] = REAL(exprs); }
    int cell;
    for (cell=1; cell<nc; ++cell) { exp_ptrs[cell]=exp_ptrs[cell-1] + ng; }
    const int* m1_ptr=INTEGER(marker1), * m2_ptr=INTEGER(marker2);
    int* reorder=(int*)R_alloc(ng, sizeof(int)) - 1;

    // Checking marker sanity.
    for (int marker=0; marker<npairs; ++marker) {
        if (m1_ptr[marker] > ng || m1_ptr[marker] <= 0) { throw std::runtime_error("first marker indices are out of range"); }
        if (m2_ptr[marker] > ng || m2_ptr[marker] <= 0) { throw std::runtime_error("second marker indices are out of range"); }
    }

    SEXP output=PROTECT(allocVector(REALSXP, nc));
    try {
        double* optr=REAL(output);
        double curscore, newscore;
        int gene;
        int it, below, total;

        for (int cell=0; cell<nc; ++cell) {
            for (gene=0; gene<ng; ++gene) { reorder[gene+1]=gene; }
            curscore=get_proportion(exp_ptrs[cell], reorder, npairs, minp, m1_ptr, m2_ptr);
            if (ISNA(curscore)) { 
                optr[cell]=NA_REAL;
                continue;
            }

            below=total=0;
            for (it=0; it < nit; ++it) {
                std::random_shuffle(reorder+1, reorder+ng+1);
                newscore=get_proportion(exp_ptrs[cell], reorder, npairs, minp, m1_ptr, m2_ptr);
                if (!ISNA(newscore)) { 
                    if (newscore < curscore) { ++below; }
                    ++total;
                }
            }
            
            optr[cell]=(total < minit ?  NA_REAL : double(below)/total);
        }
    } catch (std::exception& e) {
        UNPROTECT(1);
        throw;
    }

    UNPROTECT(1);
    return output;
} catch (std::exception& e) {
    return mkString(e.what());
}

