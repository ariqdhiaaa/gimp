/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationburnmode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
 *               2017 Øyvind Kolås <pippin@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "gimpoperationburn.h"


static gboolean gimp_operation_burn_process (GeglOperation       *operation,
                                             void                *in_buf,
                                             void                *aux_buf,
                                             void                *aux2_buf,
                                             void                *out_buf,
                                             glong                samples,
                                             const GeglRectangle *roi,
                                             gint                 level);


G_DEFINE_TYPE (GimpOperationBurn, gimp_operation_burn,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_burn_class_init (GimpOperationBurnClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:burn",
                                 "description", "GIMP burn mode operation",
                                 NULL);

  point_class->process = gimp_operation_burn_process;
}

static void
gimp_operation_burn_init (GimpOperationBurn *self)
{
}

static gboolean
gimp_operation_burn_process (GeglOperation       *operation,
                             void                *in_buf,
                             void                *aux_buf,
                             void                *aux2_buf,
                             void                *out_buf,
                             glong                samples,
                             const GeglRectangle *roi,
                             gint                 level)
{
  gfloat opacity = GIMP_OPERATION_POINT_LAYER_MODE (operation)->opacity;

  return gimp_operation_burn_process_pixels (in_buf, aux_buf, aux2_buf, out_buf, opacity, samples, roi, level);
}

gboolean
gimp_operation_burn_process_pixels (gfloat              *in,
                                    gfloat              *layer,
                                    gfloat              *mask,
                                    gfloat              *out,
                                    gfloat               opacity,
                                    glong                samples,
                                    const GeglRectangle *roi,
                                    gint                 level)
{
  const gboolean has_mask = mask != NULL;

  while (samples--)
    {
      gfloat comp_alpha;

      comp_alpha = layer[ALPHA] * opacity;
      if (has_mask)
        comp_alpha *= *mask;

      if (comp_alpha != 0.0)
        {
          gint   b;

          for (b = RED; b < ALPHA; b++)
            {
              gfloat comp = 1.0 - (1.0 - in[b]) / layer[b];
              /* The CLAMP macro is deliberately inlined and
               * written to map comp == NAN (0 / 0) -> 1
               */
              comp = comp < 0 ? 0.0 : comp < 1.0 ? comp : 1.0;

              out[b] = comp * comp_alpha + in[b] * (1.0 - comp_alpha);
            }
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }
        }

      out[ALPHA] = in[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;

      if (has_mask)
        mask++;
    }

  return TRUE;
}