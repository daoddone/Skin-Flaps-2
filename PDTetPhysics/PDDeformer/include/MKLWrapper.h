//#####################################################################
// Copyright (c) 2019, Eftychios Sifakis, Yutian Tao, Qisi Wang
// Distributed under the FreeBSD license (see license.txt)
//#####################################################################
#pragma once

#ifdef WIN32
#include <mkl.h>
#include <mkl_types.h>
#else

#ifdef __APPLE__
// Use Eigen's sparse solvers or UMFPACK on macOS
#include <Accelerate/Accelerate.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <type_traits>
#include <cstring>

// Map MKL types to standard types for macOS
typedef int MKL_INT;

// Map LAPACK constants
#define LAPACK_ROW_MAJOR 101
#define LAPACK_COL_MAJOR 102

// Use the existing CBLAS_ORDER from Accelerate framework
typedef CBLAS_ORDER CBLAS_LAYOUT;

#ifdef USE_SUITESPARSE
// Use UMFPACK on macOS
#include <umfpack.h>
template<class T, class IntType> struct PardisoPolicy {
    static inline IntType exec(void** pt, const IntType maxfct, const IntType mnum, const IntType mtype, 
                              const IntType phase, const IntType n, T* a, IntType* ia, IntType* ja, 
                              IntType* perm, const IntType nrhs, IntType* iparm, const IntType msglvl, 
                              T* b, T* x) {
        // UMFPACK implementation will go here
        std::cout << "Using UMFPACK solver" << std::endl;
        return 0;
    }
};
#elif defined(USE_EIGEN_SPARSE)
// Use Eigen's SparseLU solver on macOS
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
#include <Eigen/SparseQR>
#include <vector>
#include <iostream>
#include <memory>

template<class T, class IntType> struct PardisoPolicy {
    static std::unique_ptr<Eigen::SparseLU<Eigen::SparseMatrix<T>>> solver;
    static Eigen::SparseMatrix<T> A;
    static bool initialized;
    static bool symbolic_done;
    static bool numeric_done;
    static bool solve_done;
    static Eigen::Matrix<T, Eigen::Dynamic, 1> cached_solution;
    
    static inline IntType exec(void** pt, const IntType maxfct, const IntType mnum, const IntType mtype, 
                              const IntType phase, const IntType n, T* a, IntType* ia, IntType* ja, 
                              IntType* perm, const IntType nrhs, IntType* iparm, const IntType msglvl, 
                              T* b, T* x) {
        try {
            if (phase == 11) { // Symbolic factorization
                std::cout << "Eigen SparseLU: Symbolic factorization for " << n << "x" << n << " matrix" << std::endl;
                
                // Convert CSR to Eigen sparse matrix format
                int nnz = ia[n] - ia[0];
                std::cout << "Eigen SparseLU: Converting CSR with " << nnz << " non-zeros" << std::endl;
                std::cout << "Eigen SparseLU: ia[0] = " << ia[0] << ", ia[n] = " << ia[n] << std::endl;
                
                std::vector<Eigen::Triplet<T>> triplets;
                triplets.reserve(nnz);
                
                // CSR format uses 1-based indexing, convert to 0-based for Eigen
                int base = ia[0]; // Usually 1 for Fortran-style, 0 for C-style
                
                for (int i = 0; i < n; i++) {
                    int row_start = ia[i] - base;
                    int row_end = ia[i + 1] - base;
                    
                    for (int idx = row_start; idx < row_end; idx++) {
                        int col = ja[idx] - base;
                        
                        // Bounds checking
                        if (col < 0 || col >= n) {
                            std::cerr << "Eigen SparseLU: Invalid column index " << col 
                                      << " (original: " << ja[idx] << ") for row " << i << std::endl;
                            return 1;
                        }
                        
                        triplets.push_back(Eigen::Triplet<T>(i, col, a[idx]));
                    }
                }
                
                // Create sparse matrix
                A.resize(n, n);
                A.setFromTriplets(triplets.begin(), triplets.end());
                
                // Add small regularization to diagonal for numerical stability
                T regularization = 1e-8;
                for (int i = 0; i < n; i++) {
                    A.coeffRef(i, i) += regularization;
                }
                
                // Check matrix properties
                T norm = A.norm();
                std::cout << "Eigen SparseLU: Matrix norm = " << norm << std::endl;
                
                // Create a new solver instance
                solver.reset(new Eigen::SparseLU<Eigen::SparseMatrix<T>>());
                
                // Perform the full decomposition (analyzePattern + factorize)
                solver->compute(A);
                
                // Check if decomposition was successful
                if (solver->info() != Eigen::Success) {
                    std::cerr << "Eigen SparseLU: Decomposition failed!" << std::endl;
                    return 1;
                }
                
                std::cout << "Eigen SparseLU: Symbolic factorization complete" << std::endl;
                symbolic_done = true;
                numeric_done = true;  // compute() does both symbolic and numeric
                initialized = true;
                return 0;
            }
            else if (phase == 22) { // Numeric factorization
                std::cout << "Eigen SparseLU: Numeric factorization" << std::endl;
                
                if (!symbolic_done) {
                    std::cerr << "Eigen SparseLU: Symbolic factorization must be done first" << std::endl;
                    return 1;
                }
                
                // If compute() was already called in phase 11, we're done
                if (numeric_done) {
                    std::cout << "Eigen SparseLU: Numeric factorization already completed" << std::endl;
                    return 0;
                }
                
                // Otherwise, update matrix values and refactorize
                // Convert CSR to Eigen sparse matrix format
                int nnz = ia[n] - ia[0];
                std::vector<Eigen::Triplet<T>> triplets;
                triplets.reserve(nnz);
                
                // CSR format uses 1-based indexing, convert to 0-based for Eigen
                int base = ia[0]; // Usually 1 for Fortran-style, 0 for C-style
                
                for (int i = 0; i < n; i++) {
                    int row_start = ia[i] - base;
                    int row_end = ia[i + 1] - base;
                    
                    for (int idx = row_start; idx < row_end; idx++) {
                        int col = ja[idx] - base;
                        
                        // Bounds checking
                        if (col < 0 || col >= n) {
                            std::cerr << "Eigen SparseLU: Invalid column index " << col 
                                      << " (original: " << ja[idx] << ") for row " << i << std::endl;
                            return 1;
                        }
                        
                        triplets.push_back(Eigen::Triplet<T>(i, col, a[idx]));
                    }
                }
                
                A.resize(n, n);
                A.setFromTriplets(triplets.begin(), triplets.end());
                
                // Add small regularization to diagonal for numerical stability
                T regularization = 1e-8;
                for (int i = 0; i < n; i++) {
                    A.coeffRef(i, i) += regularization;
                }
                
                // Factorize with existing symbolic pattern
                solver->factorize(A);
                
                if (solver->info() != Eigen::Success) {
                    std::cerr << "Eigen SparseLU: Numeric factorization failed" << std::endl;
                    return 1;
                }
                
                numeric_done = true;
                std::cout << "Eigen SparseLU: Numeric factorization complete" << std::endl;
                return 0;
            }
            else if (phase == 33) { // Solve
                if (!numeric_done) {
                    std::cerr << "Eigen SparseLU: Numeric factorization must be done first" << std::endl;
                    return 1;
                }
                
                // Convert RHS to Eigen vector
                Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>> b_vec(b, n);
                
                // Solve
                cached_solution = solver->solve(b_vec);
                
                if (solver->info() != Eigen::Success) {
                    std::cerr << "Eigen SparseLU: Solve failed" << std::endl;
                    return 1;
                }
                
                // Check for NaN in solution
                if (!cached_solution.allFinite()) {
                    std::cerr << "Eigen SparseLU: Solution contains NaN or Inf values!" << std::endl;
                    std::cerr << "  RHS norm: " << b_vec.norm() << std::endl;
                    std::cerr << "  Solution norm: " << cached_solution.norm() << std::endl;
                    // Try to recover by setting solution to zero
                    cached_solution.setZero();
                    return 1;
                }
                
                // Copy solution to output
                Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>>(x, n) = cached_solution;
                
                solve_done = true;
                return 0;
            }
            else if (phase == 331 || phase == 332 || phase == 333) {
                // These are forward/diagonal/backward substitution phases
                // Eigen SparseLU doesn't support separate phases, so we do the full solve
                if (!solve_done && (phase == 331)) {
                    // First phase - do the full solve
                    if (!numeric_done) {
                        std::cerr << "Eigen SparseLU: Numeric factorization must be done first" << std::endl;
                        return 1;
                    }
                    
                    // Convert RHS to Eigen vector
                    Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>> b_vec(b, n);
                    
                    // Solve
                    cached_solution = solver->solve(b_vec);
                    
                    if (solver->info() != Eigen::Success) {
                        std::cerr << "Eigen SparseLU: Solve failed" << std::endl;
                        return 1;
                    }
                    
                    // Check for NaN in solution
                    if (!cached_solution.allFinite()) {
                        std::cerr << "Eigen SparseLU: Solution contains NaN or Inf values!" << std::endl;
                        std::cerr << "  RHS norm: " << b_vec.norm() << std::endl;
                        std::cerr << "  Solution norm: " << cached_solution.norm() << std::endl;
                        // Try to recover by setting solution to zero
                        cached_solution.setZero();
                        return 1;
                    }
                    
                    solve_done = true;
                }
                
                // For all phases, copy the cached solution
                if (solve_done) {
                    Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>>(x, n) = cached_solution;
                }
                
                // Reset solve_done flag after phase 333
                if (phase == 333) {
                    solve_done = false;
                }
                
                return 0;
            }
            else if (phase == -1) { // Release memory
                solver.reset();
                initialized = false;
                symbolic_done = false;
                numeric_done = false;
                solve_done = false;
                return 0;
            }
            else {
                std::cerr << "Eigen SparseLU: Unsupported phase " << phase << std::endl;
                return 1;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Eigen SparseLU: Exception: " << e.what() << std::endl;
            return 1;
        }
    }
};

// Static member definitions
template<class T, class IntType>
std::unique_ptr<Eigen::SparseLU<Eigen::SparseMatrix<T>>> PardisoPolicy<T, IntType>::solver;

template<class T, class IntType>
Eigen::SparseMatrix<T> PardisoPolicy<T, IntType>::A;

template<class T, class IntType>
bool PardisoPolicy<T, IntType>::initialized = false;

template<class T, class IntType>
bool PardisoPolicy<T, IntType>::symbolic_done = false;

template<class T, class IntType>
bool PardisoPolicy<T, IntType>::numeric_done = false;

template<class T, class IntType>
bool PardisoPolicy<T, IntType>::solve_done = false;

template<class T, class IntType>
Eigen::Matrix<T, Eigen::Dynamic, 1> PardisoPolicy<T, IntType>::cached_solution;

#else // USE_UMFPACK
// Pardiso is not available in Accelerate, provide minimal placeholder
template<class T, class IntType> struct PardisoPolicy {
    static inline IntType exec(void** pt, const IntType maxfct, const IntType mnum, const IntType mtype, 
                              const IntType phase, const IntType n, T* a, IntType* ia, IntType* ja, 
                              IntType* perm, const IntType nrhs, IntType* iparm, const IntType msglvl, 
                              T* b, T* x) {
        std::cerr << "Error: SuiteSparse not found. Sparse solver not available on macOS." << std::endl;
        std::cerr << "Please install SuiteSparse: brew install suite-sparse" << std::endl;
        return -1;
    }
};
#endif // USE_SUITESPARSE

#else
// Use MKL on other platforms
#include "mkl_types.h"

template<class T, class IntType> struct PardisoPolicy;
    template<class T> struct PardisoPolicy<T, int> {
        using IntType = int;
        static inline IntType exec(void** pt, const IntType maxfct, const IntType mnum, const IntType mtype, const IntType phase, const IntType n, T* a, IntType* ia, IntType* ja, IntType* perm, const IntType nrhs, IntType* iparm, const IntType msglvl, T* b, T* x) {
            IntType error;
            pardiso(pt, &maxfct, &mnum, &mtype, &phase, &n, a, ia, ja, perm, &nrhs, iparm, &msglvl, b, x, &error);
            return error;
        }
    };

    template<class T> struct PardisoPolicy<T, long long int> {
        using IntType = long long int;
        static inline IntType exec(void** pt, const IntType maxfct, const IntType mnum, const IntType mtype, const IntType phase, const IntType n, T* a, IntType* ia, IntType* ja, IntType* perm, const IntType nrhs, IntType* iparm, const IntType msglvl, T* b, T* x) {
            IntType error;
            pardiso_64(pt, &maxfct, &mnum, &mtype, &phase, &n, a, ia, ja, perm, &nrhs, iparm, &msglvl, b, x, &error);
            return error;
        }
    };
#endif


template<class T> struct LAPACKPolicy;
template<> struct LAPACKPolicy<double> {
        static constexpr int matrix_order=LAPACK_ROW_MAJOR;
        static constexpr char uplo = 'U';

        using T = double;
        using IntType = int;

        static inline IntType potrf(const int matrix_order, const char uplo, const IntType m, double* a) {
            if (matrix_order == LAPACK_ROW_MAJOR) {
#ifdef __APPLE__
                // Accelerate uses column-major, need to transpose
                char uplo_trans = (uplo == 'U') ? 'L' : 'U';
                IntType info;
                IntType m_copy = m;  // Remove const for LAPACK call
                dpotrf_(&uplo_trans, &m_copy, a, &m_copy, &info);
                return info;
#else
                return (int)LAPACKE_dpotrf(matrix_order,uplo,m,a,m);
#endif
            }
            return -1; // Should not reach here
        }
        static inline IntType potrs(const int matrix_order, const char uplo, const IntType m, const IntType nrhs, double* a, double* b) {
            if (matrix_order == LAPACK_ROW_MAJOR) {
#ifdef __APPLE__
                // Accelerate uses column-major, need to transpose
                char uplo_trans = (uplo == 'U') ? 'L' : 'U';
                IntType info;
                IntType m_copy = m;      // Remove const for LAPACK call
                IntType nrhs_copy = nrhs; // Remove const for LAPACK call
                dpotrs_(&uplo_trans, &m_copy, &nrhs_copy, a, &m_copy, b, &nrhs_copy, &info);
                return info;
#else
                return (int)LAPACKE_dpotrs(matrix_order,uplo,m,nrhs,a,m,b,nrhs);
#endif
            }
            return -1; // Should not reach here
        }
        
        // Add the missing fact and solve methods used by PardisoWrapper
        static inline IntType fact(IntType m, T* schur) {
            return potrf(matrix_order, uplo, m, schur);
        }
        
        static inline IntType solve(IntType m, IntType nrhs, T* schur, T* rhs) {
            return potrs(matrix_order, uplo, m, nrhs, schur, rhs);
        }
    };

    template<> struct LAPACKPolicy<float> {
        static constexpr int matrix_order=LAPACK_ROW_MAJOR;
        static constexpr char uplo = 'U';

        using T = float;
        using IntType = int;

        static inline IntType potrf(const int matrix_order, const char uplo, const IntType m, float* a) {
            if (matrix_order == LAPACK_ROW_MAJOR) {
#ifdef __APPLE__
                // Accelerate uses column-major, need to transpose
                char uplo_trans = (uplo == 'U') ? 'L' : 'U';
                IntType info;
                IntType m_copy = m;  // Remove const for LAPACK call
                spotrf_(&uplo_trans, &m_copy, a, &m_copy, &info);
                return info;
#else
                return (int)LAPACKE_spotrf(matrix_order,uplo,m,a,m);
#endif
            }
            return -1; // Should not reach here
        }
        static inline IntType potrs(const int matrix_order, const char uplo, const IntType m, const IntType nrhs, float* a, float* b) {
            if (matrix_order == LAPACK_ROW_MAJOR) {
#ifdef __APPLE__
                // Accelerate uses column-major, need to transpose
                char uplo_trans = (uplo == 'U') ? 'L' : 'U';
                IntType info;
                IntType m_copy = m;      // Remove const for LAPACK call
                IntType nrhs_copy = nrhs; // Remove const for LAPACK call
                spotrs_(&uplo_trans, &m_copy, &nrhs_copy, a, &m_copy, b, &nrhs_copy, &info);
                return info;
#else
                return (int)LAPACKE_spotrs(matrix_order,uplo,m,nrhs,a,m,b,nrhs);
#endif
            }
            return -1; // Should not reach here
        }
        
        // Add the missing fact and solve methods used by PardisoWrapper
        static inline IntType fact(IntType m, T* schur) {
            return potrf(matrix_order, uplo, m, schur);
        }
        
        static inline IntType solve(IntType m, IntType nrhs, T* schur, T* rhs) {
            return potrs(matrix_order, uplo, m, nrhs, schur, rhs);
        }
    };

template<class T> struct CBLASPolicy;
template<> struct CBLASPolicy<double> {
    static constexpr CBLAS_LAYOUT matrix_order = CblasRowMajor;
    static constexpr CBLAS_UPLO uplo = CblasUpper;
    using T = double;
    static inline void mutiplyAdd(T* result, const int n, const T alpha, const T* a, const T* x, const T beta) {
        cblas_dsymv (matrix_order, uplo, n, alpha, a, n, x, 1, beta, result, 1);
    }
};

template<> struct CBLASPolicy<float> {
    static constexpr CBLAS_LAYOUT matrix_order = CblasRowMajor;
    static constexpr CBLAS_UPLO uplo = CblasUpper;
    using T = float;
    static inline void mutiplyAdd(T* result, const int n, const T alpha, const T* a, const T* x, const T beta) {
        cblas_ssymv (matrix_order, uplo, n, alpha, a, n, x, 1, beta, result, 1);
    }
};

#endif // WIN32
