# FFT Water

Many video games,
for example, [PUBG](https://en.wikipedia.org/wiki/PlayerUnknown%27s_Battlegrounds),
[Uncharted 4](https://en.wikipedia.org/wiki/Uncharted_4:_A_Thief%27s_End),
and [Sea of Thieves](https://en.wikipedia.org/wiki/Sea_of_Thieves),
contain wide ocean scenes.
Those oceans move in realtime and have realistic appearance.
Basically, they are created using the well-known method proposed by [1].

The paper states that the height of wind-waves in the open ocean
can be decomposed into a sum of sine and cosine waves.
Given a set of frequencies and sampling points,
the height can be calculated using Discrete Fourier Transform (DFT).
And Fast Fourier Transform (FFT) is an efficient way to calculate DFT.
That's why I call it FFT Water.

Technically, the method uses Inverse FFT, as it creates wave heights from frequencies.
But essentially, IFFT is same as FFT.
Their differences are just the coefficient and the conjugate exponent.

![withLOD](./withLOD.gif) ![withLOD3](./withLOD3.gif)

# About this project

[Keith Lantz](https://www.keithlantz.net/2011/10/ocean-simulation-part-one-using-the-discrete-fourier-transform/) has provided a nice implementation of [1], and I use it as a start point.
The goal of this project is to create an ocean scene which I have seen in many video games.
It will have features as following:

-   Realistic appearance: reflections, refractions, foams, caustics, etc.

-   Can interact with objects (i.e. buoyancy).

-   Everything works in real time.

-   and so on.

# Result

I have modified [Keith Lantz's code](https://github.com/klantz81/ocean-simulation/tree/master/src) and it can be run on `OSX` now.

## Level of Detail (LOD)

When rendering a wide ocean,
LOD is always a good choice to improve performance.

For each frame, first, render a height map, a xz-displacement map and a normal map [3].
Second, sampling those maps both in tessellation shader and fragment shader.

For example, in the demo, a 4-vertex quad is subdivided into a `64x64` grid based on the distance between `eyePoint` and a vertex.

The shading code is based on [4].

## Periodic artifacts

Generally, the wide ocean area is created by tiling an FFT water quad.
As a result, when the field of view is large enough,
periodic artifacts can be observed at the far place.

According to [5], there are two ways to reduce periodic (or tiling) artifacts.
I have tried the one that blends the result with some noise [6, 7] and it works well.

![withLOD2](./withLOD2.gif)

## Underwater

When the eye point is underwater, mix the frame with a blue-green color
to obtain an underwater effect.

This can be achieved by using the [post-processsing](https://en.wikibooks.org/wiki/OpenGL_Programming/Post-Processing) of OpenGL,
and `eyePoint.y < threshold` is used as the condition of underwater.

To avoid the artifact when `eyePoint.y` is near the ocean surface,
it is better to set a diving/rising area `[-someValue, someValue]`.
Always keep `eyePoint.y` outside of this area.
When diving or rising, quickly go through this area.
This strategy can make the artifact unnoticeable.

![underwater](./underwater.gif)

# Problem

## Performance

Currently, the result (`512x512` FFT resolution) is not rendered in realtime.

I have tried OpenMP with `4` threads (my CPU is `2.5 GHz Quad-Core Intel Core i7`).
It can reduce the rendering time from about `1.2s` per frame to about `0.7s` per frame.
But this is not enough for realtime purpose.

A GPU-based parallelization is needed.

## Shading

To bring more reality into the shading part, [2, 8, 9] consider the sub-surface scattering (SSS) of water.
[2] blends a deep water color with a sub-surface water color to create a fake SSS effect.
[8] designs a BRDF for the sun light, and modifies the sea color with the sky light.
[9] applies BRDFs to both the sun light and sky light, and proposes an scattering equation to fake the SSS effect of water.

# Note

Using standard FFT codes (e.g. [Cooley–Tukey FFT](https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B)) results in incorrect geometry changes.
Generally, standard FFT assumes that the sampling position is non-negative.
However, the wave vectors used in [1] have negative components.
This constraint guarantees the variety of wave directions.
For example, if a wave vector only has non-negative components,
its direction will always reside in `[+x, +z]` and `[-x, -z]` orthants, and hence,
waves with directions within `[-x, +z]` and `[+x, -z]` orthants are lost.

Furthermore, a standard FFT assumes that the range of summation is also non-negative.
Based on this constraint, it designs the complex exponent set and iterations.
However, the range used in [1] is `[-N/2, N/2)`, with `N` representing the number of sampling points.
It means that a different set of complex exponents should be used.
Therefore, a special IFFT must be designed for [1].

# Reference

[1] Tessendorf, Jerry. "Simulating ocean water." Simulating nature: realistic and interactive techniques. SIGGRAPH 1.2 (2001): 5.

[2] Ang, Nigel, et al. "The technical art of sea of thieves." ACM SIGGRAPH 2018 Talks. 2018. 1-2. ([link](https://dl.acm.org/doi/10.1145/3214745.3214820), including video)

[3] An introduction to Realistic Ocean Rendering through FFT - Fabio Suriano - Codemotion Rome 2017 ([slide](https://www.slideshare.net/Codemotion/an-introduction-to-realistic-ocean-rendering-through-fft-fabio-suriano-codemotion-rome-2017), [video](https://www.youtube.com/watch?v=ClW3fo94KR4))

[4] Claes, J. "Real-time water rendering-introducing the projected grid concept." Master's thesis (2004).

[5] Gamper, Thomas. "Ocean Surface Generation and Rendering." (2018).

[6] Rydahl, Björn. "A VFX ocean toolkit with real time preview." (2009).

[7] Ocean Surface Simulation ([slide](http://www-evasion.imag.fr/~Fabrice.Neyret/images/fluids-nuages/waves/Jonathan/articlesCG/NV_OceanCS_Slides.pdf))

[8] Bruneton, Eric, Fabrice Neyret, and Nicolas Holzschuch. "Real‐time realistic ocean lighting using seamless transitions from geometry to BRDF." Computer Graphics Forum. Vol. 29. No. 2. Oxford, UK: Blackwell Publishing Ltd, 2010.

[9] Advanced Graphics Techniques Tutorial: Wakes, Explosions and Lighting: Interactive Water Simulation in 'Atlas', GDC 2019 ([slide](https://gpuopen.com/gdc-presentations/2019/gdc-2019-agtd6-interactive-water-simulation-in-atlas.pdf))
