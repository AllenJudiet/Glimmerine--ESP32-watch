#pragma once
//orbital.
// ─── Orbital element struct — defined here so ALL .ino files see it first ────
struct OrbitalElements {
  double a, e, I, L, lp, Om;       // J2000.0 epoch values
  double da, de, dI, dL, dlp, dOm; // rates per Julian century
  float  orb_a;   // screen semi-major axis (px)
  float  orb_b;   // screen semi-minor axis (px)  — tilted perspective
  int    dot_r;   // planet dot radius (px)
};
