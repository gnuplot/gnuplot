/*
 * Dump an image block as a png image.
 * This routine is used by several terminal drivers so it gets a file by itself.
 * May 2016 Daniel Sebald: Support routines to write image in Base64 encoding.
 */
#ifdef  TERM_BODY
#ifndef WRITE_PNG_IMAGE
#define WRITE_PNG_IMAGE

typedef struct base64state {
  int           shift;
  unsigned char bit6;
  unsigned int  byte4;
  FILE*         out;
} base64s;

static const unsigned char base64_lut[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void
init_base64_state_data (base64s *b64, FILE *out) {
  b64->shift = 6;
  b64->bit6 = 0;
  b64->byte4 = 0;
  b64->out = out;
}

static int
piecemeal_write_base64_data_finish (base64s *b64) {

  if (b64->shift < 6) {
    if (fputc(base64_lut[b64->bit6 & 0x3F], b64->out) == EOF)
      return 1;
    if (b64->byte4 == 0)
      b64->byte4 = 3;
    else
      b64->byte4--;
  }
  while (b64->byte4 > 0) {
    if (fputc('=', b64->out) == EOF)
      return 1;
    b64->byte4--;
  }

  return 0;
}

/* To use repeatedly, must initialize base64state first, then when done call finish.  See write_base64_data(). */
static int
piecemeal_write_base64_data (const unsigned char *data, unsigned int length, base64s *b64) {
  unsigned int i_data = 0;

  while (1) {
    unsigned int databyte = 0;
    if (b64->shift > 0) {
      if (i_data >= length)
        break;
      else {
        databyte = data[i_data++];
        b64->shift -= 8;
        if (b64->shift >= 0)
          b64->bit6 |= (databyte << b64->shift);
        else
          b64->bit6 |= (databyte >> -b64->shift);
      }
    }
    if (fputc(base64_lut[b64->bit6 & 0x3F], b64->out) == EOF)
      return 1;
    b64->shift += 6;
    b64->bit6 = (databyte << b64->shift);
    if (b64->byte4 == 0)
      b64->byte4 = 3;
    else
      b64->byte4--;
  }

  return 0;
}

#ifdef HAVE_CAIROPDF

#include "cairo-pdf.h"
#include "wxterminal/gp_cairo.h"
#include "wxterminal/gp_cairo_helpers.h"

/* cairo PNG code */
static int
write_png_image (unsigned m, unsigned n, coordval *image, t_imagecolor color_mode, const char *filename) {
  cairo_surface_t *image_surface;
  cairo_status_t cairo_stat;
  unsigned int *image255;
  int retval = 0;

  image255 = gp_cairo_helper_coordval_to_chars(image, m, n, color_mode);
  image_surface = cairo_image_surface_create_for_data((unsigned char*) image255, CAIRO_FORMAT_ARGB32, m, n, 4*m);
  cairo_stat = cairo_surface_write_to_png(image_surface, filename);
  cairo_surface_destroy(image_surface);
  if (cairo_stat != CAIRO_STATUS_SUCCESS) {
    int_warn(NO_CARET, "write_png_image cairo: could not write image file '%s': %s.", filename, cairo_status_to_string(cairo_stat));
    retval = 1;
  } else
    retval = 0;

  free(image255);
  return retval;
}

cairo_status_t
cairo_write_base64_callback (void *closure, const unsigned char *data, unsigned int length) {
  if (piecemeal_write_base64_data (data, length, closure) == 0)
    return CAIRO_STATUS_SUCCESS;
  else
    return CAIRO_STATUS_WRITE_ERROR;
}

static int 
write_png_base64_image (unsigned m, unsigned n, coordval *image, t_imagecolor color_mode, FILE *out) {
  cairo_surface_t *image_surface;
  cairo_status_t cairo_stat;
  unsigned int *image255;
  base64s *b64;
  int retval = 0;

  b64 = gp_alloc(sizeof(base64s), "base64s");
  if (b64 == NULL)
    return 1;

  image255 = gp_cairo_helper_coordval_to_chars(image, m, n, color_mode);
  image_surface = cairo_image_surface_create_for_data((unsigned char*) image255, CAIRO_FORMAT_ARGB32, m, n, 4*m);

  init_base64_state_data (b64, out);
  cairo_stat = cairo_surface_write_to_png_stream(image_surface, cairo_write_base64_callback, b64);
  cairo_surface_destroy(image_surface);
  if (cairo_stat != CAIRO_STATUS_SUCCESS) {
    int_warn(NO_CARET, "write_png_image cairo: could not write image file: %s.", cairo_status_to_string(cairo_stat));
    retval = 1;
  } else
    retval = piecemeal_write_base64_data_finish (b64);

  free(b64);
  free(image255);

  return retval;
}

#else      /* libgd PNG code mainly taken from gd.trm */
#include <gd.h>

gdImagePtr 
construct_gd_image (unsigned M, unsigned N, coordval *image, t_imagecolor color_mode) {
  int m, n, pixel;
  gdImagePtr im;

  im = gdImageCreateTrueColor(M, N);
  if (!im) {
    int_warn(NO_CARET, "libgd: failed to create image structure");
    return im;
  }
  /* gdImageColorAllocateAlpha(im, 255, 255, 255, 127); */
  gdImageSaveAlpha(im, 1);
  gdImageAlphaBlending(im, 0);

  if (color_mode == IC_RGBA) {
    /* RGB + Alpha channel */
    for (n=0; n<N; n++) {
      for (m=0; m<M; m++) {
        rgb_color rgb1;
        rgb255_color rgb255;
        int alpha;
        rgb1.r = *image++;
        rgb1.g = *image++;
        rgb1.b = *image++;
        alpha  = *image++;
        alpha  = 127 - (alpha>>1);  /* input is [0:255] but gd wants [127:0] */
        rgb255_from_rgb1( rgb1, &rgb255 );
        pixel = gdImageColorResolveAlpha(im, (int)rgb255.r, (int)rgb255.g, (int)rgb255.b, alpha);
        gdImageSetPixel( im, m, n, pixel );
      }
    }
  } else if (color_mode == IC_RGB) {
    /* TrueColor 24-bit color mode */
    for (n=0; n<N; n++) {
      for (m=0; m<M; m++) {
        rgb_color rgb1;
        rgb255_color rgb255;
        rgb1.r = *image++;
        rgb1.g = *image++;
        rgb1.b = *image++;
        rgb255_from_rgb1( rgb1, &rgb255 );
        pixel = gdImageColorResolve(im, (int)rgb255.r, (int)rgb255.g, (int)rgb255.b );
        gdImageSetPixel( im, m, n, pixel );
      }
    }
  } else if (color_mode == IC_PALETTE) {
    /* Palette color lookup from gray value */
    for (n=0; n<N; n++) {
      for (m=0; m<M; m++) {
        rgb255_color rgb;
        if (isnan(*image)) {
          /* FIXME: tried to take the comment from gd.trm into account but needs a testcase */
          pixel = gdImageColorResolveAlpha(im, 0, 0, 0, 127);
          image++;
        } else {
          rgb255maxcolors_from_gray( *image++, &rgb );
          pixel = gdImageColorResolve( im, (int)rgb.r, (int)rgb.g, (int)rgb.b );
        }
        gdImageSetPixel( im, m, n, pixel );
      }
    }
  }

  return im;
}

static int 
write_png_image (unsigned M, unsigned N, coordval *image, t_imagecolor color_mode, const char *filename) {
  gdImagePtr im;
  FILE *out;

  im = construct_gd_image (M, N, image, color_mode);
  if (!im)
    return 1;

  out = fopen(filename, "wb");
  if (!out) {
    int_warn(NO_CARET, "write_png_image libgd: could not write image file '%s'", filename);
    gdImageDestroy(im);
    return 1;
  }
  gdImagePng(im, out);
  fclose(out);
  gdImageDestroy(im);

  return 0;
}

static int
write_base64_data (const unsigned char *data, unsigned int length, FILE *out) {
  base64s *b64;
  int retval = 0;

  b64 = gp_alloc(sizeof(base64s), "base64s");
  if (b64 == NULL)
    return 1;

  init_base64_state_data (b64, out);

  if (piecemeal_write_base64_data (data, length, b64) != 0)
    retval = 1;
  else
    retval = piecemeal_write_base64_data_finish (b64);

  free(b64);

  return retval;
}

static int
write_png_base64_image (unsigned M, unsigned N, coordval *image, t_imagecolor color_mode, FILE *out) {
  gdImagePtr im;
  void *pngdata;
  int pngsize;
  int retval = 0;

  im = construct_gd_image (M, N, image, color_mode);
  if (!im)
    return 1;

  if ((pngdata = gdImagePngPtr(im, &pngsize)) == NULL) {
    gdImageDestroy(im);
    return 1;
  }

  retval = write_base64_data (pngdata, pngsize, out);

  gdFree(pngdata);
  gdImageDestroy(im);

  return retval;
}
#endif /* HAVE_CAIRO_PDF */
#endif /* WRITE_PNG_IMAGE */
#endif /* TERM_BODY */
