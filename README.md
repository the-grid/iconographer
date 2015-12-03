iconographer - tool to extract representative frame from video
--------------------------------------------------------------

iconographer is a tool to extract a representative frame from videos.
This is done as a two stage process the first produces an image with
a scanline per frame in the original the second analyses this data dump
aiming to get a good salient/representative frame suitable for use
for example as a thumbnail.

The analysis from stage one is also suitable for scene-detection and
probably some other forms of content based analsysis.

Example usage:
--------------

Create frame.png from video.ogv

    $ iconographer video.ogv frame.png

Create frame.png from video.ogv, and store analysis in analysis.png
if analysis.png already exists it will be reused instead of recreated.

    $ iconographer video.ogv -a analysis.png frame.png

Stop analysing after 120 seconds if video is not done yet, store resulting
frame in frame.png

    $ iconographer video.ogv frame.png -t 120

Make last frame for analysis/consideration be frame number 420

    $ iconographer video.ogv frame.png -t 120

Optional arguments can be passed in addition

     -p  show progress in terminal
     -t  timeout, in seconds - only keep extracting frame data until this much time
         has passed
     -s  start-frame first frame in video to extract
     -e  end-frame last frame in video to extract, 0; the default means all frames
         of input video
     -h, --horizontal   store a horizontal strata instead of vertical.
     -f  format string, specify which forms of analysis to put in the analysis file,
         the default format is: "histogram audio 4 thumb 40 mid-col 20"

The analysis images contain a scanline per frame of the original video,
interpreted as 24bit (8bpc) RGB data, or just as visual thumbnail references for the
video. Valid entries in the format string are:

    histogram
    audio 4
    thumb 50
    mid-row 20
    mid-col 30

where the numbers represents the number of pixels to store from the video per frame.

Horizontal, thumbnail only render from part of movie sintel:

![Example analysis](http://pippin.gimp.org/tmp/sintel-horizontal.png)

And a vertical analysis, with default format setting:

![Example analysis](http://pippin.gimp.org/tmp/sintel-vertical.png)

For representative frame extraction to work properly the first element of the
format string should be the histogram analysis, the expected defaults also map
to the following c in-memory structure.
```c
#define RGB_HIST_DIM    6 
#define RGB_HIST_SLOTS (RGB_HIST_DIM * RGB_HIST_DIM * RGB_HIST_DIM)
typedef struct FrameInfo
{
  uint8_t rgb_hist[RGB_HIST_SLOTS];
  uint8_t rgb_square_diff[3];
  uint8_t audio_energy[3];
} FrameInfo;
```
