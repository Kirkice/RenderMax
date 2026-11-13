# Mali Offline Compiler runtime (Windows x64)

Source: Arm Performance Studio 2024.6, local installed distribution.

This directory intentionally versions the Windows runtime so developers do not need a
machine-specific Arm installation path. The compiler is a separate process, not embedded
inside qrenderdoc.exe.

Included runtime:
- malioc.exe
- graphics/Mali-Gxx_r51p0-00rel0.dll
- graphics/Mali-Gxx_r53p0-00rel0.dll
- graphics/Mali-T600_r23p0-00rel0.dll
- external/glslang.exe
- license_terms/license_agreement.txt
- license_terms/third_party_licenses/mali_offline_compiler/third_party_licenses.txt

`qrenderdoc/malioc.props` copies this runtime next to qrenderdoc.exe after a build.
`util/buildscripts/scripts/make_package_win32.sh` includes it in the x64 Windows
package and enables the corresponding Installer64.wxs components. An explicit
MALIOC_ROOT overrides the repository runtime. The x86 installer does not include
this x64 compiler. MaliOCEnabled=no disables package bundling.

The SHA256SUMS file records the included vendor files. Samples, local test shaders
and the manual PDF are intentionally excluded.

## Licensing

These are third-party Arm binaries, not covered by RenderMax's source license.
Preserve the accompanying Arm and third-party notices. The supplied Arm EULA
restricts redistribution; copying these files into a repository or installer does
not itself establish permission to redistribute them. Confirm the applicable Arm
redistribution authorization before publishing the repository or shipping packages.
