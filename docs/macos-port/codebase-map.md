# Codebase Map and Dependency Graph

This document provides a high-level overview of the SkinFlaps codebase architecture and its platform-specific dependencies.

## Dependency Graph

The following diagram illustrates the major components of the application and their dependencies that are relevant to the macOS porting effort.

```mermaid
graph TD
    subgraph App
        A[SkinFlaps]
    end

    subgraph UI
        B[FacialFlapsGui.h]
    end

    subgraph Physics
        C[PDTetPhysics]
        D[simd-numeric-kernels-new]
        E[vnBccTetCutter_tbb]
    end

    subgraph Dependencies
        F[Windows Registry]
        G[Intel MKL]
        H[Intel TBB]
        I[Intel AVX]
    end
    
    subgraph Mac_Alternatives
        J[Config File]
        K[Accelerate Framework]
        L[TBB on macOS / GCD]
        M[ARM NEON]
    end

    A --> B
    A --> C
    A --> E
    C --> G
    C --> H
    D --> I
    E --> H
    B -- Windows-Specific --> F

    F -.-> J
    G -.-> K
    H -.-> L
    I -.-> M
```

### Component Descriptions:

-   **SkinFlaps**: The main application executable.
-   **FacialFlapsGui.h**: Handles the GUI, user input, and currently contains Windows-specific code (Registry access).
-   **PDTetPhysics**: The core physics simulation library. It depends on a math kernel library (MKL) and a threading library (TBB).
-   **simd-numeric-kernels-new**: Contains low-level, performance-critical math kernels that are implemented using platform-specific SIMD instructions (AVX).
-   **vnBccTetCutter_tbb**: A class used for mesh processing, parallelized with TBB. 