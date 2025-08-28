Buffered Multiple for Modular Synths - Powered by Arduino


Takes one sync input signal and produces six individually configurable outputs
 - Shows current BPM on OLED display
 - Each output can be configured with:
      - Normal sync (1:1)
      - Clock division (1/2, 1/4, 1/8)
      - Three groove patterns:
          - GS: Swing (delays off-beats by fixed amount)
          - GF: Shuffle (creates triplet-based rhythm)
          - GH: Humanize (adds random timing variations)


Groove Implementation 

Swing (GS) = consistent delayed off-beats
 -Delays off-beats by shifting timing up to 66% through beat
- Very classic swing feel at 50-75% intensities
- Clean implementation with consistent timing


Shuffle (GF) = polyrhythmic feel with specific timing ratios
- Creates 2-against-3 feel using weighted offset timing
- Distinct from swing - pushes toward triplet territory
- At 75%, approaches dotted 8th feeling (funkier)


Humanize (GH) = unpredictable variations
- Adds random timing variation around off-beats
- Much stronger randomization (up to 40% of beat)
- More chaotic/organic feel compared to others

The implementation provides good sonic variety between the modes. Each mode serves a distinct musical purpose, and the intensity levels (50%/75%) allow meaningful adjustment.
The two intensity levels (50% and 75%) offer enough adjustability without overcomplicating the interface. This gives users six distinct groove feels to choose from (three types × two intensities), which is a good balance for this type of device.
