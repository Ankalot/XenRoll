* Add tests?
* Remake it to CLAP + Visage? https://github.com/VitalAudio/visage.
* Upgrade partials finding algorithm. Use algorithm like in SPEAR.
   https://www.klingbeil.com/spear/
* Roughness calculated with the use of partials from several tones is bad,
  it's nonsense, it is not a smooth function at all and makes little sense.
* Upgrade pitch memory model (there is #TODO in PitchMemory.h).
* Actually we can have >128 notes with distinct pitch (in one midi track).
   We don't have to reserve midi notes for all notes in the track instantly,
   we can reserve them as needed during playback.
* Can GrFNN and Harmonic Entropy be useful?