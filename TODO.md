* Add tests?
* Remake it to CLAP + Visage? https://github.com/VitalAudio/visage.
* Upgrade partials finding algorithm. Use algorithm like in SPEAR.
   https://www.klingbeil.com/spear/
* Roughness calculated with the use of partials from several tones is bad,
  it's nonsense, it is not a smooth function at all and makes little sense.
* Upgrade pitch memory model (there is #TODO in PitchMemory.h).
* Can GrFNN and Harmonic Entropy be useful?