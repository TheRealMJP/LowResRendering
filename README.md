# Low-Resolution Rendering Sample

This is a D3D11 sample project that demonstrates a technique for using MSAA to render transparents at half resolution. Using MSAA effectively allows shading at a low resolution, while still rasterizing and depth testing at full resolution. This helps reduce haloing and other artifacts commonly associated with low-resolution transparents. The sample also implements nearest-depth upsampling as well as full-resolution rendering for comparison.

To make sure that the low-resolution MSAA sample points line up with the sample points from the full-resolution render target, the sample makes use of the programmable sample point functionality found in Nvidia's second generation Maxwell GPU's. This is done by using the D3D11 driver extensions exposed through Nvidia's properietary NVAPI. If you don't have an Nvidia GPU, or you have an Nvidia GPU that doesn't support programmable sample points, the sample will automatically disable the functionality and remove the corresponding UI.

This code sample is meant to go along with an article I'm writing for my blog, which should be released shortly.

# Build Instructions

The repository contains a Visual Studio 2013 project and solution file that's ready to build on Windows. All external dependencies are included in the repository, with the exception of NVAPI, which is excluded due to its licensing terms. To build with NVAPI, you'll want to download it from [Nvidia's developer website](https://developer.nvidia.com/nvapi), and unzip the file into Externals\NVAPI-352. If you already have it somewhere else, you can change the path to the include and the lib at the top of LowResRendering.cpp. Alternatively, you can also disable NVAPI entirely by defining "UseNVAPI_" to 0 at the top of LowResRendering.cpp

