#include <gegl.h>
#include <glib/gprintf.h>
#include <stdint.h>

// XXX: sort rgb histogram by lightness
// XXX: implement audio extraction

#define NEGL_RGB_HEIGHT    42
#define NEGL_RGB_HIST_DIM   6 // if changing make dim * dim * dim divisible by 3
#define NEGL_RGB_HIST_SLOTS (NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM)
#define NEGL_FFT_DIM       64

typedef struct FrameInfo  
{
  uint8_t rgb_middle[NEGL_RGB_HEIGHT*3];
  uint8_t rgb_hist[NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM];
  uint8_t audio_energy[3];
  uint8_t audio_fft[NEGL_FFT_DIM*3];
  uint8_t scene_change_estimate[3];
} FrameInfo;

int frame_start = 0;
int frame_end   = 0+ 1800;
int frame_thumb = 0;

char *video_path = NULL;
char *thumb_path = NULL;
char *input_analysis_path = NULL;
char *output_analysis_path = NULL;

typedef enum NeglRunMode {
  NEGL_NO_UI = 0,
  NEGL_VIDEO,
  NEGL_TERRAIN
} NeglRunmode;

int show_progress = 0;

//int run_mode = NEGL_VIDEO;
//int run_mode = NEGL_TERRAIN;
int run_mode = NEGL_NO_UI;

void parse_args (int argc, char **argv)
{
  int i;
  int stage = 0;
  for (i = 1; i < argc; i++)
  {
    if (g_str_equal (argv[i], "-oa") ||
        g_str_equal (argv[i], "--output-analysis"))
    {
      output_analysis_path = g_strdup (argv[i+1]);
      i++;
    } 
    else if (g_str_equal (argv[i], "-p") ||
        g_str_equal (argv[i], "--progress"))
    {
      show_progress = 1;
    }
    else if (g_str_equal (argv[i], "-ia") ||
        g_str_equal (argv[i], "--input-analysis"))
    {
      input_analysis_path = g_strdup (argv[i+1]);
      i++;
    } 
    else if (g_str_equal (argv[i], "-s"))
    {
      frame_start = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "-e"))
    {
      frame_end = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "--video"))
    {
      run_mode = NEGL_VIDEO;
    }
    else if (g_str_equal (argv[i], "--no-ui"))
    {
      run_mode = NEGL_NO_UI;
    }
    else if (g_str_equal (argv[i], "--terrain"))
    {
      run_mode = NEGL_TERRAIN;
    }
    else if (stage == 0)
    {
      video_path = g_strdup (argv[i]);
      stage = 1;
    } else if (stage == 1)
    {
      thumb_path = g_strdup (argv[i]);
      stage = 2;
    }
  }

  printf ("frames: %i - %i\n", frame_start, frame_end);
  printf ("video: %s\n", video_path);
  printf ("thumb: %s\n", thumb_path);
  printf ("input analysis: %s\n", input_analysis_path);
  printf ("output analysis: %s\n", output_analysis_path);
}

#define TERRAIN_STRIDE sizeof(FrameInfo)
#define TERRAIN_WIDTH  (TERRAIN_STRIDE/3)

#include <string.h>

GeglNode   *gegl_decode  = NULL;
GeglNode   *gegl_display = NULL;
GeglNode   *display      = NULL;
GeglBuffer *video_frame  = NULL;
GeglBuffer *terrain      = NULL;

uint8_t rgb_hist_shuffler[NEGL_RGB_HIST_SLOTS];
uint8_t rgb_hist_unshuffler[NEGL_RGB_HIST_SLOTS];

typedef struct {
  int r;
  int g;
  int b;
  int no; 
} Entry;

static int rgb_hist_inited = 0;

static gint sorted_color (gconstpointer a, gconstpointer b)
{
  const Entry *ea = a;
  const Entry *eb = b;
  return (ea->r + ea->g + ea->b) -
         (eb->r + eb->g + eb->b);
}

static inline void init_rgb_hist (void)
{
  /* sort RGB histogram slots by luminance for human readability
   */
  if (!rgb_hist_inited)
    {
      GList *list = NULL;
      GList *l;
      int r, g, b, i;
      i = 0;
      for (r = 0; r < NEGL_RGB_HIST_DIM; r++)
        for (g = 0; g < NEGL_RGB_HIST_DIM; g++)
          for (b = 0; b < NEGL_RGB_HIST_DIM; b++, i++)
          {
            Entry *e = g_malloc0 (sizeof (Entry));
            e->r = r;
            e->g = g;
            e->b = b;
            e->no = i;
            list = g_list_prepend (list, e);
          }
      list = g_list_sort (list, sorted_color);
      rgb_hist_inited = 1;
      i = 0;
      for (l = list; l; l = l->next, i++)
      {
        Entry *e = l->data;
        int no = e->no;
        int li = i;
#if 0
        if (i < NEGL_RGB_HIST_SLOTS / 3)
        {
          li = i * 3 + 0;
        } 
        else if (i < (NEGL_RGB_HIST_SLOTS * 2) / 3)
        {
          li = (i - ((NEGL_RGB_HIST_SLOTS * 1) / 3))* 3 + 1;
        } 
        else
        {
          li = (i - ((NEGL_RGB_HIST_SLOTS * 2) / 3))* 3 + 2;
        } 
#endif
        rgb_hist_shuffler[li] = no;
        rgb_hist_unshuffler[no] = li;
        g_free (e);
      }
      g_list_free (list);
    }
}

int rgb_hist_shuffle (int in)
{
  init_rgb_hist();
  return rgb_hist_shuffler[in];
}

int rgb_hist_unshuffle (int in)
{
  init_rgb_hist();
  return rgb_hist_unshuffler[in];
}

GeglNode *store, *load;

static void decode_frame_no (int frame)
{
  if (video_frame)
    g_object_unref (video_frame);
  video_frame = NULL;
  gegl_node_set (load, "frame", frame, NULL);
  gegl_node_process (store);
}

void find_best_thumb (void)
{
  frame_thumb = 1000;
}

gint
main (gint    argc,
      gchar **argv)
{
  gegl_init (&argc, &argv);
  parse_args (argc, argv);

  gegl_decode = gegl_node_new ();
  gegl_display = gegl_node_new ();


  store = gegl_node_new_child (gegl_decode,
                                         "operation", "gegl:buffer-sink",
                                         "buffer", &video_frame, NULL);
  load = gegl_node_new_child (gegl_decode,
                              "operation", "gegl:ff-load",
                              "frame", 0,
                              "path", video_path,
                              NULL);
  gegl_node_link_many (load, store, NULL);


  GeglNode *display = NULL;
  GeglNode *readbuf;

  GeglBuffer *terrain = NULL;
  GeglRectangle terrain_rect = {0, 0,
                                frame_end - frame_start + 1,
                                TERRAIN_WIDTH
};
  terrain = gegl_buffer_new (&terrain_rect, babl_format ("R'G'B' u8"));

  switch(run_mode)
  {
    case NEGL_NO_UI:
      break;
    case NEGL_VIDEO:
      readbuf = gegl_node_new_child (gegl_display, "operation", "gegl:buffer-source", NULL);
      display = gegl_node_create_child (gegl_display, "gegl:display");
      gegl_node_link_many (readbuf, display, NULL);
      break;
    case NEGL_TERRAIN:
      readbuf = gegl_node_new_child (gegl_display, "operation", "gegl:buffer-source", NULL);
      display = gegl_node_create_child (gegl_display, "gegl:display");
      gegl_node_set (readbuf, "buffer", terrain, NULL);
      gegl_node_link_many (readbuf, display, NULL);
      break;
  }

  /* a graph for looking at the video */
  {
    FrameInfo info = {0};
    gint frame;
    for (frame = frame_start; frame <= frame_end; frame++)
      {
  	int rgb_hist[NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM]={0,};
        int sum = 0;
        //int second_max_hist = 0;
        int max_hist = 0;

        if (show_progress)
        {
          fprintf (stdout, "\r%2.1f%% %i/%i (%i)", 
                   (frame-frame_start) * 100.0 / (frame_end-frame_start),
                   frame-frame_start,
                   frame_end-frame_start,
                   frame);
          fflush (stdout);
        }

        GeglRectangle terrain_row = {frame-frame_start, 0, 1, TERRAIN_WIDTH};

	decode_frame_no (frame);

	GeglBufferIterator *it = gegl_buffer_iterator_new (video_frame, NULL, 0,
                babl_format ("R'G'B' u8"),
                GEGL_BUFFER_READ,
                GEGL_ABYSS_NONE);


        int slot;
        for (slot = 0; slot < NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM; slot ++)
          rgb_hist[slot] = 0;

	while (gegl_buffer_iterator_next (it))
        {
          uint8_t *data = (void*)it->data[0];
          int i;
          for (i = 0; i < it->length; i++)
          {
            int r = data[i * 3 + 0] / 256.0 * NEGL_RGB_HIST_DIM;
            int g = data[i * 3 + 1] / 256.0 * NEGL_RGB_HIST_DIM;
            int b = data[i * 3 + 2] / 256.0 * NEGL_RGB_HIST_DIM;
            int slot = r * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM +
		       g * NEGL_RGB_HIST_DIM +
		       b;
            if (slot < 0) slot = 0;
            if (slot >= NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM)
                slot = NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM;

            rgb_hist[slot]++;
            if (rgb_hist[slot] > max_hist)
            {
              //second_max_hist = max_hist;
              max_hist = rgb_hist[slot];
            }
            sum++;
          }
        }
        {
           int slot;
           for (slot = 0; slot < NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM; slot ++)
{
     	   //int val = rgb_hist[slot] / ((second_max_hist + max_hist) * 0.5) * 255;
           int val = rgb_hist[slot] / (sum * 1.0) * 4096;
           if (val > 255)
             val = 255; 

     	   info.rgb_hist[rgb_hist_shuffle (slot)] = val;
}
        }

        GeglRectangle mid_row;
        mid_row.x = 
           gegl_buffer_get_extent (video_frame)-> width * 1.0 *
            mid_row.height / gegl_buffer_get_extent (video_frame)->height / 2;
        mid_row.y = 0;
        mid_row.width = 1;
        mid_row.height = NEGL_RGB_HEIGHT;
        gegl_buffer_get (video_frame, &mid_row, 
           1.0 * mid_row.height / gegl_buffer_get_extent (video_frame)->height,
           babl_format ("R'G'B' u8"),
           (void*)&(info.rgb_middle)[0],
           GEGL_AUTO_ROWSTRIDE,
           GEGL_ABYSS_NONE);
        gegl_buffer_set (terrain, &terrain_row, 0, babl_format("RGB u8"),
                         &(info.rgb_middle[0]),
                         3);

        switch(run_mode)
        {
          case NEGL_NO_UI:
            break;
          case NEGL_VIDEO:
	    gegl_node_set (readbuf, "buffer", video_frame, NULL);
	    gegl_node_process (display);
            break;
          case NEGL_TERRAIN:
	    gegl_node_set (readbuf, "buffer", terrain, NULL);
	    gegl_node_process (display);
            break;
        }
      }
      if (show_progress)
      {
        fprintf (stdout, "\n");
        fflush (stdout);
      }
  }

  if (output_analysis_path)
  {
    GeglNode *save_graph = gegl_node_new ();
    GeglNode *readbuf = gegl_node_new_child (gegl_display, "operation", "gegl:buffer-source", "buffer", terrain, NULL);
    GeglNode *save = gegl_node_new_child (gegl_display, "operation", "gegl:save",
      "path", output_analysis_path, NULL);
      gegl_node_link_many (readbuf, save, NULL);
    gegl_node_process (save);
    g_object_unref (save_graph);
  }
  
  find_best_thumb ();
  decode_frame_no (frame_thumb);

  if (thumb_path)
  {
    GeglNode *save_graph = gegl_node_new ();
    GeglNode *readbuf = gegl_node_new_child (gegl_display, "operation", "gegl:buffer-source", "buffer", video_frame, NULL);
    GeglNode *save = gegl_node_new_child (gegl_display, "operation", "gegl:save",
      "path", thumb_path, NULL);
      gegl_node_link_many (readbuf, save, NULL);
    gegl_node_process (save);
    g_object_unref (save_graph);
  }

  if (video_frame)
    g_object_unref (video_frame);
  video_frame = NULL;
  if (terrain)
    g_object_unref (terrain);
  terrain = NULL;
  g_object_unref (gegl_decode);
  g_object_unref (gegl_display);

  gegl_exit ();
  return 0;
}

