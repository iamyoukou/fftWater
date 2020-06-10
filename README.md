# FFT Water

Many video games,
for example, [PUBG](https://en.wikipedia.org/wiki/PlayerUnknown%27s_Battlegrounds),
[Uncharted 4](https://en.wikipedia.org/wiki/Uncharted_4:_A_Thief%27s_End),
and [Sea of Thieves](https://en.wikipedia.org/wiki/Sea_of_Thieves),
contain wide ocean scenes.
Those oceans move in realtime and have realistic appearance.
Basically, they are created using the well-known method proposed by [Tessendorf, 2001].

Tessendorf states that the height of wind-waves in the open ocean
can be decomposed into a sum of sine and cosine waves.
Given a set of frequencies and sampling points,
the height can be efficiently calculated using Fast Fourier Transform (FFT).
That's why I call it FFT water.

Technically, the method uses Inverse FFT, as it creates wave heights from frequencies.
But essentially, IFFT is same as FFT.
Their differences are just the coefficient and the conjugate exponent.

# About this project

This project is based on the paper [Tessendorf, 2001] and the code from [Scrawk](https://github.com/Scrawk/Phillips-Ocean).

Although Scrawk's implementation already has a nice result, some problems exist.

1. The ocean shows obvious artifacts when the viewer moves far away.

2. The shading method is not realistic enough.

3. It cannot create foam.

4. The ocean cannot interact with objects.

I will try to fix those problems and produce a better result.

# Reference
[Tessendorf, 2001] Tessendorf, Jerry. "Simulating ocean water." Simulating nature: realistic and interactive techniques. SIGGRAPH 1.2 (2001): 5.

[Cooley–Tukey FFT](https://rosettacode.org/wiki/Fast_Fourier_transform#C.2B.2B)
