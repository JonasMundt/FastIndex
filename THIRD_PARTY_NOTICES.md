# Third-party notices — FastIndex v0.1.0, Windows x64

> **Distribution review status:** The notices and SBOM match the uploaded binary package. Before public publication, verify the matching Qt source is attached as a separate release asset, that the relevant permissions and copyright notices are complete, and test the *exact* final ZIP on a clean Windows system. This document is not a legal certification.

## FastIndex — MIT

Copyright (c) 2026 Jonas Mundt. The FastIndex application source is offered under MIT; see `LICENSE` and https://github.com/JonasMundt/FastIndex. FastIndex's MIT license does **not** replace the licenses of redistributed components.

## Qt 6.12.0 — dynamically loaded Qt libraries, plugins, QML, translations

Qt and its contributors are the copyright owners of the respective Qt components. The supplied Qt 6.12.0 SDK SPDX SBOM lists the license option `LGPL-3.0-only` for **86 matched Qt DLLs**, alongside other Qt licensing alternatives. This match is based on filenames/path metadata; it is not a cryptographic binary match. Qt-specific GPL-only modules must not be assumed to be LGPL-covered. See `licenses/Qt-SBOM.zip` and `docs/QT_SBOM_MATCHED_FILES.csv` for the mapping.

FastIndex is packaged with replaceable, dynamically linked Qt DLLs. Recipients may replace the relevant LGPL-covered shared Qt libraries with ABI-compatible modified versions and may reverse engineer the application as necessary to debug modifications to the LGPL-covered libraries, within the LGPL conditions. There are no technical measures in the provided application known to enforce a fixed copy of Qt; working correctly with an arbitrary replacement Qt build is not guaranteed.

The text of GNU LGPL version 3 and GNU GPL version 3 (incorporated by LGPLv3) are in `licenses/LGPL-3.0.txt` and `licenses/GPL-3.0.txt`.

- Qt official open-source licensing: https://doc.qt.io/qt-6/licensing.html
- Qt LGPL obligations: https://www.qt.io/development/open-source-lgpl-obligations
- Third-party licensing inventory: https://doc.qt.io/qt-6/licenses-used-in-qt.html
- Qt 6.12 SBOM: https://doc.qt.io/qt-6.12/sbom.html

### Qt corresponding source — separate release asset

The author has downloaded `qt-everywhere-src-6.12.0.zip`. **A matching copy must also be available to recipients** before the Windows runtime package is published. The intended arrangement is to attach `qt-everywhere-src-6.12.0.zip` as a second downloadable asset on the **same GitHub FastIndex v0.1.0 release** as this binary ZIP. Confirm that asset actually appears and can be downloaded. Any modifications made to Qt (if applicable) must also be provided. The source ZIP is not included in this smaller application ZIP.

Expected asset name: `qt-everywhere-src-6.12.0.zip`.
GitHub release listing: https://github.com/JonasMundt/FastIndex/releases

## Third-party code incorporated into Qt components

Qt SBOM evidence identifies 45 candidates directly associated with redistributed Qt DLLs/plugins. Original copyright holders retain their rights. Component/version/license-expression and available copyright details are provided in `docs/QT_DIRECT_DEPENDENCY_ATTRIBUTIONS.csv` and the readable `docs/QT_DIRECT_DEPENDENCY_ATTRIBUTIONS.md`. This attribution inventory may require supplementing with original full license texts and copyright notices for the exact binaries; the Qt source archive and official Qt third-party page include further detail. The SPDX inventory is evidence, not an all-purpose license substitute.

## Other DLLs shipped next to FastIndex.exe

The supplied package contains four DLLs not directly mapped to installed-file entries in the provided Qt SBOM. Exact SHA-256 values and PE-version strings are in `docs/EXTERNAL_DLL_AUDIT.md`.

- `d3dcompiler_47.dll`: Embedded Windows VERSIONINFO describes **Microsoft Corporation**, `Direct3D HLSL Compiler for Redistribution`, version **10.0.26100.9169**. Microsoft's Windows SDK documentation describes application-local redistribution of its `D3DCompiler_47.dll`; actual source/SDK provenance must be verified.
- `dxcompiler.dll`: Embedded VERSIONINFO describes **Microsoft Corporation**, `DirectX Compiler - Out Of Band`, version **1.9.2602.17**. Microsoft's open-source DirectX Shader Compiler project covers related components; its stated upstream license is **University of Illinois/NCSA** with additional third-party notices. The precise redistributed binary's origin/terms must still be checked.
- `dxil.dll`: Embedded VERSIONINFO describes **Microsoft Corporation**, `DirectX IL for Redistribution`, version **101.6.2112.13**. Confirm SDK/redistribution terms for this exact build; do not presume the open-source compiler repository alone proves rights over this supplied binary.
- `opengl32sw.dll`: Likely **Mesa llvmpipe** software OpenGL renderer. The official Qt Windows graphics documentation says Qt's prebuilt packages ship Mesa llvmpipe under this filename, and Qt 6.12 lists `Mesa llvmpipe` under **MIT and Boost Software License 1.0**. This exact file's Mesa version and license inventory have not been independently authenticated.

References:
- Microsoft D3DCompiler: https://learn.microsoft.com/en-us/windows/win32/directx-sdk--august-2009-
- Microsoft DirectX Shader Compiler: https://github.com/microsoft/DirectXShaderCompiler
- Qt graphics/Mesa: https://doc.qt.io/qt-6/windows-graphics.html
- Mesa licensing: https://docs.mesa3d.org/license.html

## SQLite

FastIndex statically includes SQLite amalgamation code, which upstream dedicates to the public domain. https://www.sqlite.org/copyright.html

## GoogleTest

Used to build and test FastIndex. The release runtime ZIP does not contain a GoogleTest test executable or standalone GTest DLL.

## Microsoft Visual C++ runtime

The deployment was created with `windeployqt --no-compiler-runtime`. Installing Microsoft Visual C++ Redistributable (x64) may be required on a clean Windows system: https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist

---

**Before publishing:** Check `docs/RELEASE_READINESS_CHECKLIST.md`. Do not claim that the release is legally fully cleared or independently tested just because these files are included.
