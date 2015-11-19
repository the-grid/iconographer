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

Create thumb.png from video.ogv

    $ iconographer video.ogv thumb.png

Create thumb.png from video.ogv, and store analysis in analysis.png

    $ iconographer video.ogv -oa analysis.png thumb.png

Create and store analysis in analysis.png from video.ogv

    $ iconographer video.ogv -oa analysis.png

Use existing analysis analysis.png with video.ogv and store best frame in
thumb.png

    $ iconographer video.ogv -ia analysis.png thumb.png

Stop analysing after 120 seconds if video is not done yet, store resulting
thumb in thumb.png

    $ iconographer video.ogv thumb.png -t 120

Make last frame for analysis/consideration be frame number 420

    $ iconographer video.ogv thumb.png -t 120

Optional arguments can be passed in addition

     -p  show progress in terminal
     -t  timeout, in seconds - only keep extracting frame data until this much time
         has passed
     -e  end-frame last frame in video to extract, 0; the default means all frames
         of input video
     -d  compute sum of pixel differences (makes computation per frame take longer)

The analysis images contain a scanline per frame of the original video,
interpreted as 24bit (8bpc) RGB data the following struct maps the
datastructure:

```c

#define NEGL_RGB_HEIGHT      42
#define NEGL_RGB_THEIGHT     42
#define NEGL_RGB_HIST_DIM    6 
#define NEGL_RGB_HIST_SLOTS (NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM)

typedef struct FrameInfo
{
  uint8_t rgb_thumb[NEGL_RGB_THEIGHT*3];
  uint8_t rgb_square_diff[3];
  uint8_t audio_energy[3];
  uint8_t rgb_hist[NEGL_RGB_HIST_SLOTS];
  uint8_t rgb_mid_col[NEGL_RGB_HEIGHT*3];
  uint8_t rgb_mid_row[NEGL_RGB_HEIGHT*3];
} FrameInfo;
```
