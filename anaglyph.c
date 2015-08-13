/*
 * (C) 2015 Alex < raziel at eml dot cc >
 *
 * LICENSE GNU GENERAL PUBLIC LICENSE Version 2, June 1991
 * see the file
 *
 */

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <stdio.h>
#include <string.h>

#include "gtk_mui_macro.h"

#define AVERAGE(A, B) ((guchar)((((unsigned long)A) + ((unsigned long)B)) / 2))

#define VECSIZE 256

#define Red 0
#define Green 1
#define Blue 2
#define Alpha 3

#define GREEN_MAGENTA "green/magenta"
#define RED_CYAN "red/cyan"

typedef enum {
	GreenMagenta = 1,
	RedCyan = 2,
} output_type;

typedef struct {
	gint32             image;
	gint32			   width;
	gint32             height;
	// TODO re-factor to AnaglyphSurface source, depth, out;
	GimpDrawable     *depth_drawable;
	gint             depth_bpp;
	GimpPixelRgn     depth_rgn;
	GimpDrawable     *source_drawable;
	gint             source_bpp;
	GimpPixelRgn     source_rgn;
	GimpDrawable     *out_drawable;
	gint             out_bpp;
	GimpPixelRgn     out_rgn;
	GtkWidget        *curve;
	gfloat           vec[VECSIZE];
	gint             pixmap_width;
	float			 max_displace;
	output_type		 type;
} AnaglyphInfo;

static void query       (void);
static void run         (const gchar      *name,
                         gint              nparams,
                         const GimpParam  *param,
                         gint             *nreturn_vals,
                         GimpParam       **return_vals);


static void process_image(AnaglyphInfo *ai);

static gboolean on_enter_notify (GtkWidget *widget, GdkEvent  *event,
		AnaglyphInfo *ai);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

MAIN()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    {
      GIMP_PDB_INT32,
      "run-mode",
      "Run mode"
    },
    {
      GIMP_PDB_IMAGE,
      "image",
      "Input image"
    }
  };

  gimp_install_procedure (
    "plug-in-anaglyph",
    "Anaglyph",
    "Create Anaglyph",
    "Alex <raziel@eml.cc>",
    "Copyright Alex <raziel@eml.cc>",
    "2015",
    "_Anaglyph...",
    "RGB*",
    GIMP_PLUGIN,
    G_N_ELEMENTS (args), 0,
    args, NULL);

  gimp_plugin_menu_register ("plug-in-anaglyph",
                             "<Image>/Filters/Map");
}

static void on_max_displace_changed(GtkSpinButton *spin_button, AnaglyphInfo *ai)
{
	double value;
	g_object_get(G_OBJECT(spin_button), "value", &value, NULL);
	ai->max_displace = value;
    fprintf(stderr, "gimp-plugin-anaglyph: max_displace=%f\n", ai->max_displace);
}

static gboolean on_curve_change(GtkWidget *widget, GdkEvent  *event, AnaglyphInfo *ai)
{
	const int veclen = VECSIZE;
	int i;

	// TODO vector values should be cached and change only when the curve change
	gtk_curve_get_vector(GTK_CURVE(ai->curve), veclen, &ai->vec[0]);
	process_image(ai);
	return TRUE;
}

static void on_button_click( GtkWidget *widget, AnaglyphInfo *ai) {
	fprintf(stderr, "gimp-plugin-anaglyph: button clicked\n");
    fprintf(stderr, "gimp-plugin-anaglyph: ai=%p\n", ai);

	process_image(ai);
}

static void on_type_changed(GtkWidget *widget, AnaglyphInfo *ai) {
	char *active = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
	fprintf(stderr, "gimp-plugin-anaglyph: type=%s\n", active);
	if (active == NULL)
		return;

	if (strcmp(active, GREEN_MAGENTA) == 0)
		ai->type = GreenMagenta;
	else if (strcmp(active, RED_CYAN) == 0)
		ai->type = RedCyan;
}

static gboolean on_enter_notify (GtkWidget *widget, GdkEvent  *event, AnaglyphInfo *ai)
{
	fprintf(stderr, "gimp-plugin-anaglyph: notify-enter\n");
    fprintf(stderr, "gimp-plugin-anaglyph: ai=%p\n", ai);

	process_image(ai);

	return FALSE;
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunMode       run_mode;
  gint32             image, layer, channel;
  AnaglyphInfo     *ai;

  fprintf(stderr, "gimp-plugin-anaglyph: run() entering...\n");

  ai = g_new(AnaglyphInfo, 1);
  if (ai == NULL) {
	  fprintf(stderr, "gimp-plugin-anaglyph: error allocating AnaglyphInfo...\n");
	  return;
  }
  memset(ai, 0, sizeof(AnaglyphInfo));
  ai->max_displace = 50;
  fprintf(stderr, "gimp-plugin-anaglyph: allocated ai=%p\n", ai);


//  images = gimp_image_list(&num_images);
//  for (i = 0; i++; i < num_images)
//	  fprintf(stderr, "[%d]:0x%08x\n", i, images[i]);

  /* Setting mandatory output values */
  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /* Getting run_mode - we won't display a dialog if 
   * we are in NONINTERACTIVE mode */
  run_mode = param[0].data.d_int32;

  image = param[1].data.d_image;
  ai->image = image;
  fprintf(stderr, "gimp-plugin-anaglyph: image=0x%08x '%s'\n", image, gimp_image_get_name(image));

  layer = gimp_image_get_active_layer(image);
  if (layer == -1) {
	  gimp_message("please select a layer to be the input source\n");
	  fprintf(stderr, "gimp-plugin-anaglyph: no active layer\n");
	  return;
  }
  fprintf(stderr, "gimp-plugin-anaglyph: layer=0x%08x '%s'\n", layer, gimp_item_get_name(layer));
  if ((ai->source_drawable = gimp_drawable_get(layer)) == NULL) {
	  fprintf(stderr, "gimp-plugin-anaglyph: error getting source drawable\n");
  }

  ai->width = gimp_image_width(image);
  ai->height = gimp_image_height(image);

  layer = gimp_image_get_layer_by_name(image, "anaglyph-output");
  fprintf(stderr, "gimp-plugin-anaglyph: layer=0x%08x\n", layer);
  if (layer == -1) {
	  layer = gimp_layer_new(image, "anaglyph-output", ai->width, ai->height, GIMP_RGB, 100, GIMP_NORMAL_MODE);
	  if (gimp_image_insert_layer(image, layer, 0, -1) != TRUE) {
		  fprintf(stderr, "gimp-plugin-anaglyph: error adding output layer!\n");
	  }
  }
  if (gimp_image_set_active_layer(image, layer) != TRUE) {
	  fprintf(stderr, "gimp-plugin-anaglyph: error setting output as the active layer\n");
  }
  if ((ai->out_drawable = gimp_drawable_get(layer)) == NULL) {
	  fprintf(stderr, "gimp-plugin-anaglyph: error getting output drawable\n");
  }
  fprintf(stderr, "gimp-plugin-anaglyph: out_drawable=%p id=%d\n", ai->out_drawable, ai->out_drawable->drawable_id);

  channel = gimp_image_get_channel_by_name(image, "anaglyph-depth");
  if (channel == -1) {
	  struct GimpRGB { gdouble r, g, b, a; } rgb = { 0, 0, 0, 0 };
	  channel = gimp_channel_new(image, "anaglyph-depth", ai->width, ai->height, 0,(const GimpRGB *) &rgb);
	  if (gimp_image_insert_channel(image, channel, 0, -1) != TRUE) {
		  fprintf(stderr, "gimp-plugin-anaglyph: error adding depth channel\n");
	  }
  }
  if (gimp_image_set_active_channel(image, channel) != TRUE) {
	  fprintf(stderr, "gimp-plugin-anaglyph: error setting the active channel\n");
  }

  //
  if ((ai->depth_drawable = gimp_drawable_get(channel)) == NULL) {
	  fprintf(stderr, "gimp-plugin-anaglyph: error getting channel drawable\n");
  }

  ai->source_bpp = gimp_drawable_bpp (ai->source_drawable->drawable_id);
  ai->depth_bpp = gimp_drawable_bpp (ai->depth_drawable->drawable_id);
  ai->out_bpp = gimp_drawable_bpp (ai->out_drawable->drawable_id);

  fprintf(stderr, "gimp-plugin-anaglyph: bpp source=%d depth=%d out=%d\n", ai->source_bpp, ai->depth_bpp, ai->out_bpp);

  {
	  const gint tile_width = gimp_tile_width(), tile_height = gimp_tile_height();
	  gulong kilobytes = 0;

	  kilobytes += ((((ai->source_bpp * ai->width) /  tile_width) + 1) * tile_width) * (((ai->height / tile_height) + 1) * tile_height);
	  kilobytes += ((((ai->depth_bpp * ai->width) /  tile_width) + 1) * tile_width) * (((ai->height / tile_height) + 1) * tile_height);
	  kilobytes += ((((ai->out_bpp * ai->width) /  tile_width) + 1) * tile_width) * (((ai->height / tile_height) + 1) * tile_height);

	  fprintf(stderr, "gimp-plugin-anaglyph: tile size=%d\n", kilobytes);
	  gimp_tile_cache_size(kilobytes/1024);
	  //gimp_tile_cache_ntiles (2 * (drawable->width / gimp_tile_width () + 1));
  }

	if (run_mode != GIMP_RUN_NONINTERACTIVE) {
		GtkBuilder *builder;
		GtkWidget *window;
		GtkWidget *button, *refresh, *curve, *cache, *max_displace, *type;
		GError *error = NULL;
		fprintf(stderr, "gimp-plugin-anaglyph: entering gtk_main()...\n");

		gtk_init(NULL, NULL);

		/* kinda amiga mui... glade ask me for gtk3 :P */
		UIStart window = UIWindow(GTK_WINDOW_TOPLEVEL)
				UIConnect("destroy", gtk_main_quit, NULL)
				UIChild UIVBox
					UIChild curve = UICurve(200, 200) /* deprecated :| */
						UIPackExpand
					UIEnd
					UIChild UIHBox
						UIChild UILabel(" Type: ") UIPack UIEnd
						UIChild type = UIComboBox
							UICBAppendText(GREEN_MAGENTA)
							UICBAppendText(RED_CYAN)
							UIConnect("changed", on_type_changed, ai)
							UIPackExpand
						UIEnd
					UIEnd
					UIChild UIHBox
						UIChild UILabel(" Max displacement: ") UIPack UIEnd
						UIChild max_displace = UISpinButton UIPackExpand
							UIConnect("value-changed", on_max_displace_changed, ai)
						UIEnd
					UIEnd
					UIChild UIHBox
						UIChild UILabel(" Tiles cache in KB: ") UIPack UIEnd
						UIChild cache = UIEntry UIPackExpand UIEnd
					UIEnd
					UIChild refresh = UIEventBox
						UIConnect("enter-notify-event", on_curve_change, ai)
							UIChild UILabel("do it") UIContainerAdd UIEnd
						UIPack
					UIEnd
				UIEnd
			UIEnd

		ai->curve = curve;
		gtk_widget_add_events (refresh, GDK_ENTER_NOTIFY_MASK);

		gtk_combo_box_set_active(GTK_COMBO_BOX(type), 1);

		gtk_widget_show_all(window);
		gtk_window_set_keep_above(GTK_WINDOW(window), TRUE);

		gtk_main();

	} else {
		fprintf(stderr, "gimp-plugin-anaglyph: non interactive not implemented\n");
	}

//  process_image(&ai);

	fprintf(stderr, "gimp-plugin-anaglyph: cleanup\n");

	gimp_displays_flush (); // TODO necessary?
	gimp_drawable_detach (ai->depth_drawable);
	gimp_drawable_detach (ai->source_drawable);
	gimp_drawable_detach (ai->out_drawable);

	g_free(ai);

	return;
}

static inline void red_cyan_displace(const gint width,
		const gint x,
		const guchar *src, const gint src_bpp,
		guchar *out, const gint out_bpp,
		const gint xoff, guchar *pixmap_row)
{
	// actually we move red component to right, but being rgb additive it will appear on left
	// TODO ^ is that so?
	const gint left_x = x + xoff;

	if (left_x >= 0 && left_x < width) {
		if (pixmap_row[left_x >> 3] & (1 << (left_x & 3))) {
			// pixel already in pixmap.. average
			out[out_bpp * left_x + Red] = src[src_bpp * x + Red];// AVG(out[out_bpp * left_x + Red], src[src_bpp * x + Red]);
		} else {
			// pixel not in pixmap.. add it
			out[out_bpp * left_x + Red] = src[src_bpp * x + Red];
			pixmap_row[left_x >> 3] |= 1 << (left_x & 3);
		}
	}
}

static inline void green_magenta_displace(const gint width,
		const gint x,
		const guchar *src, const gint src_bpp,
		guchar *out, const gint out_bpp,
		const gint xoff, guchar *pixmap_row)
{

	// actually we move green component to right, but being rgb additive it will appear on left
	// TODO ^ is that so?
	const gint left_x = x + xoff;

	if (left_x >= 0 && left_x < width) {
		if (pixmap_row[left_x >> 3] & (1 << (left_x & 3))) {
			// pixel already in pixmap.. average
			out[out_bpp * left_x + Green] = src[src_bpp * x + Green]; //AVERAGE(out[out_bpp * left_x + Green], src[src_bpp * x + Green]);
		} else {
			// pixel not in pixmap.. add it
			out[out_bpp * left_x + Green] = src[src_bpp * x + Green];
			pixmap_row[left_x >> 3] |= 1 << (left_x & 3);
		}
	}
}

static void process_image(AnaglyphInfo *ai) {
	gint         x1, y1, x2, y2;
	gint         width, height;
    guchar      *out_row, *depth_row, *source_row;
    gint		x_offset;
//    gboolean    undo_status;
	int i, j, k;
	gint xoff;
	guchar tmp;
	guchar *pixmap;

	fprintf(stderr, "gimp-plugin-anaglyph: process_image... ai=%p\n", ai);


	gimp_progress_init ("Anaglyphing...");
////	gimp_image_undo_group_start(ai->image);
//	undo_status = gimp_image_undo_is_enabled(ai->image);
//	if (undo_status) {
//		gimp_image_undo_disable(ai->image);
//	}

    fprintf(stderr, "gimp-plugin-anaglyph: out_drawable=%p\n", ai->out_drawable);
    fprintf(stderr,  "gimp-plugin-anaglyph: id=%d\n", ai->out_drawable->drawable_id);

	gimp_drawable_mask_bounds (ai->out_drawable->drawable_id,
							 &x1, &y1,
							 &x2, &y2);
	width  = x2 - x1;
	height = y2 - y1;

	ai->pixmap_width = ai->width / 8 + 1;
	pixmap = g_new(guchar, ai->pixmap_width * ai->height);
	memset(pixmap, 0, ai->pixmap_width * ai->height);

	source_row = g_new (guchar, ai->source_bpp * width);
	if (source_row == NULL) {
		fprintf(stderr, "gimp-plugin-anaglyph: error allocating a source row\n");
	}
	depth_row = g_new (guchar, ai->depth_bpp * width);
	if (depth_row == NULL) {
		fprintf(stderr, "gimp-plugin-anaglyph: error allocating a depth row\n");
	}
	out_row = g_new (guchar, ai->out_bpp * width);
	if (out_row == NULL) {
		fprintf(stderr, "gimp-plugin-anaglyph: error allocating a out row\n");
	}
	if (source_row == NULL || depth_row == NULL || out_row == NULL) {
		if (source_row != NULL) g_free(source_row);
		if (depth_row != NULL) g_free(depth_row);
		if (out_row != NULL) g_free(out_row);
		return;
	}

	gimp_pixel_rgn_init (&ai->source_rgn, ai->source_drawable,
			x1, y1, width, height, FALSE, FALSE);

	gimp_pixel_rgn_init (&ai->depth_rgn, ai->depth_drawable,
			x1, y1, width, height, FALSE, FALSE);

	gimp_pixel_rgn_init (&ai->out_rgn, ai->out_drawable,
			x1, y1, width, height, TRUE, TRUE);

	for (i=y1; i < y2; i++) {

		gimp_pixel_rgn_get_row (&ai->source_rgn, source_row, x1, i, width);
		gimp_pixel_rgn_get_row (&ai->depth_rgn, depth_row, x1, i, width);

		for (j = x1; j < x2; j++) {

			tmp = (guchar)(ai->max_displace * (((gfloat)depth_row[j]) / ((gfloat)255)));
			xoff = (guchar) (((gfloat)tmp) * ((gfloat)ai->vec[tmp]));

			// TODO something is wrong with the *displaced* channel copy (maybe a precision problem)
			// even on zero displace there are missing pixel or weird patterns (maybe the *blend*)
			if (ai->type == GreenMagenta) {
				green_magenta_displace(width, (j - x1), source_row, ai->source_bpp, out_row, ai->out_bpp,
						xoff, pixmap + ai->pixmap_width * i);
				out_row[ai->out_bpp * (j - x1) + Red] = source_row[ai->source_bpp * (j - x1) + Red];
				out_row[ai->out_bpp * (j - x1) + Blue] = source_row[ai->source_bpp * (j - x1) + Blue];
			} else if (ai->type == RedCyan) {
				red_cyan_displace(width, j, source_row, ai->source_bpp, out_row, ai->out_bpp,
						xoff, pixmap + ai->pixmap_width * i);
				out_row[ai->out_bpp * (j - x1) + Green] = source_row[ai->source_bpp * (j - x1) + Green];
				out_row[ai->out_bpp * (j - x1) + Blue] = source_row[ai->source_bpp * (j - x1) + Blue];
			}

			if (ai->source_bpp == 4 && ai->out_bpp == 4) {
				out_row[ai->out_bpp * (j - x1) + Alpha] = source_row[ai->source_bpp * (j - x1) + Alpha];
			} else if (ai->out_bpp == 4) {
				out_row[ai->out_bpp * (j - x1) + Alpha] = 0xff;
			}
		}

		gimp_pixel_rgn_set_row(&ai->out_rgn, out_row, x1, i, width);

		if (i % 20 == 0) {
			gimp_progress_update ((gdouble) i / (gdouble) height);
		}
	}
	gimp_progress_end();

////	gimp_image_undo_group_end(ai->image);
//	if (undo_status) {
//		gimp_image_undo_enable(ai->image);
//	}

	g_free(pixmap);
	pixmap = NULL;

	g_free(source_row);
	g_free(depth_row);
	g_free(out_row);

	gimp_drawable_flush (ai->out_drawable);
	gimp_drawable_merge_shadow (ai->out_drawable->drawable_id, TRUE);
	gimp_drawable_update (ai->out_drawable->drawable_id, x1, y1, width, height);

//	gimp_drawable_update (ai->out_drawable->drawable_id,
//						0, 0,
//						gimp_image_width(ai->image), gimp_image_height(ai->image));

	gimp_displays_flush (); // else the main edit view isn't refreshed...

	while (gtk_events_pending()) {
		gtk_main_iteration();
	}
}
