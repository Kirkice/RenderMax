<p align="center"><img src="https://user-images.githubusercontent.com/661798/36482670-f81601c0-170b-11e8-8adb-2365b346ac27.png" /></p>

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
[![CI](https://github.com/baldurk/renderdoc/actions/workflows/ci.yml/badge.svg?branch=v1.x&event=push)](https://github.com/baldurk/renderdoc/actions)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](docs/CODE_OF_CONDUCT.md) 

RenderMax is a RenderDoc-compatible frame-capture, graphics debugging, and performance analysis tool for Vulkan, D3D11, D3D12, OpenGL, and OpenGL ES development on Windows, Linux, Android, and Nintendo Switch&trade;. It is open-source under the MIT license.

RenderMax is intended for debugging and analysing programs that you are authorised to inspect. Capture files, the `renderdoc` API, and compatibility command-line tools retain their established identifiers to preserve interoperability with the RenderDoc ecosystem.

For local builds, see the [compilation instructions](docs/CONTRIBUTING/Compiling.md). Project-specific release, support, and community links should be configured before publishing RenderMax distributions.

* **Documentation**: Included source documentation is under [docs](docs), with a CHM generated in Windows builds.
* **Compatibility**: `.rdc` captures, the `renderdoc` API, and the `qrenderdoc`/`renderdoccmd` compatibility executables remain supported.
* **Licensing**: [MIT license](LICENSE.md) and third-party acknowledgements are retained.
* **Contributing**: [Contribution information](docs/CONTRIBUTING.md) and [compilation instructions](docs/CONTRIBUTING/Compiling.md).

Screenshots
--------------

| [ ![Texture view](https://renderdoc.org/fp/ts_screen1.jpg?2) ](https://renderdoc.org/fp/screen1.jpg) | [ ![Pixel history & shader debug](https://renderdoc.org/fp/ts_screen2.jpg?2) ](https://renderdoc.org/fp/screen2.png) |
| --- | --- |
| [ ![Mesh viewer](https://renderdoc.org/fp/ts_screen3.jpg?2) ](https://renderdoc.org/fp/screen3.png) | [ ![Pipeline viewer & constants](https://renderdoc.org/fp/ts_screen4.jpg?2) ](https://renderdoc.org/fp/screen4.png) |

API Support
--------------

|                          | Windows                  | Linux                    | Android                   |
| ------------------------ | ------------------------ | ------------------------ | ------------------------  |
| Vulkan                   | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:        |
| OpenGL ES 2.0 - 3.2      | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:        |
| OpenGL 3.2 - 4.6 Core    | :heavy_check_mark:       | :heavy_check_mark:       |  N/A                      |
| D3D11 & D3D12            | :heavy_check_mark:       |  N/A                     |  N/A                      |
| OpenGL 1.0 - 2.0 Compat  | :heavy_multiplication_x: | :heavy_multiplication_x: |  N/A                      |
| D3D9 & 10                | :heavy_multiplication_x: |  N/A                     |  N/A                      |
| Metal                    |  N/A                     |  N/A                     |  N/A                      |

* Nintendo Switch&trade; support is distributed separately for authorized developers as part of the NintendoSDK. For more information, consult the Nintendo Developer Portal.

Downloads
--------------

RenderMax release locations are project-managed. Build from source using the [compilation instructions](docs/CONTRIBUTING/Compiling.md) until a RenderMax release channel is configured.

Documentation
--------------

The text documentation is built from [reStructuredText with Sphinx](docs). Windows builds package the generated local help as `renderdoc.chm` for compatibility.

License
--------------

RenderMax is released under the MIT license; see [LICENSE.md](LICENSE.md) for the full text and third-party library acknowledgements.

Compiling
---------

Building RenderMax is straightforward on most platforms. See [Compiling.md](docs/CONTRIBUTING/Compiling.md) for details.

Contributing & Development
--------------

I've added some notes on how to contribute, as well as where to get started looking through the code in [Developing-Change.md](docs/CONTRIBUTING/Developing-Change.md). All contribution information is available under [CONTRIBUTING.md](docs/CONTRIBUTING.md).

