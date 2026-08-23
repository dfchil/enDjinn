# HDR Rendering Roadmap

## Parked implementation

The `wip/pc-endjinn-hdr` branch contains a tested, opt-in HDR presentation
path for pc-enDjinn. Set `PC_ENDJINN_HDR=1` at runtime to request an FP16
`VK_FORMAT_R16G16B16A16_SFLOAT` swapchain using the extended linear-sRGB
colorspace. SDR remains the default, and unsupported surfaces fall back to
the existing SDR swapchain.

The implementation converts rendered sRGB colors to linear scRGB, anchors
ordinary artwork to a 203-nit reference white, and allows bright translucent
primitives to approach 1000 nits. It was build-tested with dRxLaX and runtime
tested on MoltenVK in both explicit HDR and default SDR modes.

## Known limitations

- An advertised FP16 scRGB surface does not prove that the physical display is
  HDR-capable, which is why HDR must remain explicitly enabled.
- Treating every bright translucent primitive as emissive is too broad and can
  make trails, UI elements, and effects look aggressively overexposed.
- Paper white and peak brightness are fixed rather than calibrated for the
  display or selected by the application.
- Source textures and packed PVR colors are sRGB-range assets, so the current
  path adds luminance headroom but not native wide-gamut artwork.
- Screenshot and streaming software may mishandle or tone-map scRGB output.

## PC follow-up

1. Add explicit emissive/material intent instead of inferring it from the
   translucent render list.
2. Add configurable paper-white, peak-nit, and exposure controls with safe
   defaults.
3. Query platform display capabilities where reliable, while retaining an
   explicit user override.
4. Add visual test scenes for SDR parity, translucent blending, clipping, and
   HDR calibration.
5. Consider an internal FP16 scene target and a dedicated output transform if
   HDR10/PQ output is needed in addition to linear scRGB.

## Web follow-up

The current web-enDjinn renderer uses WebGL 2. A staged web implementation
should keep WebGL 2 as the compatibility path:

1. Add an optional RGBA16F WebGL scene framebuffer and tone-map it into the SDR
   canvas for linear-light blending and HDR-like post-processing.
2. Add a WebGPU renderer that reuses the shared PVR packet decoder.
3. Prefer WebGPU only when adapter/device creation succeeds; otherwise retain
   the existing WebGL 2 backend.
4. Make genuine display HDR a separate opt-in using an `rgba16float` WebGPU
   canvas with extended tone mapping where the browser supports it.

