/* vi:set et ai sw=2 sts=2 ts=2: */
/*-
 * Copyright (c) 2005-2026 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2005-2026 Jannis Pohlmann <jannis@xfce.org>
 * Copyright (c) 2005-2026 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __THUNAR_GLOBAL_SELECTION_H__
#define __THUNAR_GLOBAL_SELECTION_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GlobalSelectionEntry GlobalSelectionEntry;

struct _GlobalSelectionEntry
{
  gchar    *dir_path;
  gchar    *filename;
  gboolean  is_dir;
  guint64   size;
  guint     dir_hash; /* optional, for faster lookups */
};

typedef struct _GlobalSelection GlobalSelection;

struct _GlobalSelection
{
  GHashTable *dirs;      /* key: gchar* dir_path (g_strdup), value: GHashTable* sub_hash */
  guint       total_files;
  guint       total_dirs;
  guint64     total_size;
};

GlobalSelection *global_selection_new            (void);
void             global_selection_free           (GlobalSelection *selection);

void             global_selection_add            (GlobalSelection *selection,
                                                 const gchar     *dir_path,
                                                 const gchar     *filename,
                                                 gboolean         is_dir,
                                                 guint64          size);
void             global_selection_remove         (GlobalSelection *selection,
                                                 const gchar     *dir_path,
                                                 const gchar     *filename);

GList           *global_selection_get_filenames_in_dir (GlobalSelection *selection,
                                                        const gchar     *dir_path);

void             global_selection_save          (GlobalSelection *selection,
                                                const gchar     *file_path);
void             global_selection_load          (GlobalSelection *selection,
                                                const gchar     *file_path);

gchar           *global_selection_get_totals    (GlobalSelection *selection);

G_END_DECLS

#endif /* __THUNAR_GLOBAL_SELECTION_H__ */