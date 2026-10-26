# External DLL verification, FastIndex v0.1.0

Extracted from user-supplied original release ZIP; hashes computed on exact bytes, not from upstream downloads. The PE metadata is a claim inside each binary, not a cryptographic guarantee of provenance.

| Library | Size (bytes) | SHA-256 | Embedded version | Reported origin |
|---|---:|---|---|---|
| `d3dcompiler_47.dll` | 4741568 | `3bd6d7acfe8a0a84acd166fba1a537ce68bd21c8edce2bd9bb863cfa034136d4` | `10.0.26100.9169` | Microsoft Corporation — Direct3D HLSL Compiler for Redistribution |
| `dxcompiler.dll` | 14261192 | `b01d5fc880a41e0034772022961e4d3ce04d127244e6e48e7a2bdb5a3a02a45e` | `1.9.2602.17` | Microsoft Corporation — DirectX Compiler - Out Of Band |
| `dxil.dll` | 1508432 | `e69b838018a1e0025201edbe9f166d061807cab2f02221c18c53ea6f5e96f0b5` | `101.6.2112.13` | Microsoft Corporation — DirectX IL for Redistribution |
| `opengl32sw.dll` | 20639888 | `b04de4541863bc7d8879040a78889c4849c1b1da2784c4630f734c146c2998ce` | `not embedded in readable VERSIONINFO` | Probable Mesa llvmpipe as documented in Qt Windows deployment; verify against installed Qt kit |

### Verification work remaining

- Inspect the install-kit or SDK path from which each file was copied and compare SHA-256 with the packaged file.
- Review Microsoft's applicable redistributable terms for the exact DirectX DLL versions, and preserve required notices.
- Verify the origin, component list, and copyright/license notices of the shipped Mesa software rasterizer.
- Alternatively produce and smoke-test a new deployment without unnecessary components, using documented `windeployqt` options (`--no-system-d3d-compiler`, `--no-system-dxc-compiler`, `--no-opengl-sw`), but do not remove runtime files solely on the basis of these notes.
