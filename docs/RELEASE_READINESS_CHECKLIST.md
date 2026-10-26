# FastIndex v0.1.0 — Release readiness checklist

**Status: PREPARED FOR FINAL REVIEW, NOT CLEARED FOR PUBLICATION.**

## Verified by archive inspection

- [x] Supplied Windows x64 package contains `FastIndex.exe` and Qt 6.12.0 DLLs.
- [x] Repacked application ZIP retains byte-for-byte identities of **every original application file** (compare `docs/APP_FILE_SHA256.tsv`).
- [x] Original Qt 6.12.0 SBOM attached; 86 Qt DLLs matched by filenames to SPDX inventory.
- [x] MIT application license, GPLv3/LGPLv3 texts, Qt notice and SPDX evidence included.
- [x] Microsoft DirectX DLL embedded versions documented; Qt identifies likely `opengl32sw.dll` as Mesa llvmpipe.

## ACTION REQUIRED before public release

- [ ] Attach the original `qt-everywhere-src-6.12.0.zip` to the **same draft GitHub release** as the FastIndex Windows ZIP. It was downloaded on the user's Windows machine, but is *not* uploaded to this conversation and is *not contained* in this archive.
- [ ] Verify the matching source contains the used Qt components and required third-party source/licensing materials, and ensure all Qt modifications (if any) are supplied. Confirm the source can be downloaded from the published release after release.
- [ ] Verify/copy the complete required third-party copyright notices and full license texts for shipped Qt code (review Qt source + official Qt module third-party listings); the 45-row attribution list is helpful evidence but not independent clearance.
- [ ] Confirm SDK/Qt-kit source, redistribution terms and component-specific notices for the four standalone DLLs in `docs/EXTERNAL_DLL_AUDIT.md`.
- [ ] Install the bundled MIT and notices files in source repository root (`LICENSE`, `THIRD_PARTY_NOTICES.md`, and `licenses/`), then commit and push; confirm CI green at release tag.
- [ ] Validate source tag `v0.1.0` points to the commit whose `FastIndex.exe` generated this binary. Current binary was submitted to chat independently; exact build provenance cannot be confirmed.
- [ ] Test the **new archive**, after extraction, on a clean Windows environment without Qt on PATH; verify Worker selector number, indexing, full-text search, file opening.
- [ ] Verify appropriate Microsoft Visual C++ runtime is available/required and document it in release notes.
- [ ] Confirm license notices and source references are also prominent on the release page; select **Pre-release**, then publish only after completing review.

## Source and binary deployment notes

Qt is bundled as **shared libraries** (DLLs). The LGPL generally calls for permitting replacement with compatible user-provided versions, and for distribution of the corresponding library source or an appropriate compliant offer. The application is MIT-licensed separately, not a relicensing of Qt, DirectX, Mesa or bundled libraries. Details: https://www.qt.io/development/open-source-lgpl-obligations

## Delivery

- Proposed release title: `FastIndex v0.1.0 – First Preview Release`
- Binary asset from this work: `FastIndex-v0.1.0-PACKAGED-AWAITING-FINAL-REVIEW.zip`
- Qt source companion asset: `qt-everywhere-src-6.12.0.zip` (downloaded by user, not embedded)
- Release draft: https://github.com/JonasMundt/FastIndex/releases

This is a practical compliance-preparation checklist, **not legal advice or a legal opinion**. For definitive legal validation, consult an open-source licensing professional.
