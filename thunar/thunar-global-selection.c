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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "thunar-global-selection.h"

#include <gio/gio.h>
#include <stdio.h>



static void
entry_free (gpointer data)
{
  GlobalSelectionEntry *entry = data;

  g_print ("ENTRY_FREE: Called with entry=%p\n", entry);
  fflush (stdout);

  if (G_UNLIKELY (entry != NULL))
    {
      g_print ("ENTRY_FREE: Freeing entry for %s/%s\n", entry->dir_path, entry->filename);
      fflush (stdout);
      g_free (entry->dir_path);
      g_free (entry->filename);
      g_free (entry);
    }
  else
    {
      g_print ("ENTRY_FREE: entry is NULL\n");
      fflush (stdout);
    }
}



static void
sub_hash_free (gpointer data)
{
  GHashTable *sub_hash = data;

  g_print ("SUB_HASH_FREE: Called with sub_hash=%p\n", sub_hash);
  fflush (stdout);

  if (G_UNLIKELY (sub_hash != NULL))
    {
      g_print ("SUB_HASH_FREE: Destroying sub_hash\n");
      fflush (stdout);
      g_hash_table_destroy (sub_hash);
    }
  else
    {
      g_print ("SUB_HASH_FREE: sub_hash is NULL\n");
      fflush (stdout);
    }
}



GlobalSelection *
global_selection_new (void)
{
  GlobalSelection *selection;

  selection = g_new0 (GlobalSelection, 1);
  selection->dirs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, sub_hash_free);
  selection->total_files = 0;
  selection->total_dirs = 0;
  selection->total_size = 0;

  return selection;
}



void
global_selection_free (GlobalSelection *selection)
{
  g_print ("GLOBAL_SELECTION_FREE: Called with selection=%p\n", selection);
  fflush (stdout);

  if (G_UNLIKELY (selection == NULL))
    {
      g_print ("GLOBAL_SELECTION_FREE: selection is NULL, returning\n");
      fflush (stdout);
      return;
    }

  g_print ("GLOBAL_SELECTION_FREE: Freeing selection with %d files, %d dirs\n",
           selection->total_files, selection->total_dirs);
  fflush (stdout);

  if (G_UNLIKELY (selection->dirs != NULL))
    {
      g_print ("GLOBAL_SELECTION_FREE: Destroying dirs hash table\n");
      fflush (stdout);
      g_hash_table_destroy (selection->dirs);
    }

  g_print ("GLOBAL_SELECTION_FREE: Freeing selection struct\n");
  fflush (stdout);
  g_free (selection);
  g_print ("GLOBAL_SELECTION_FREE: Done\n");
  fflush (stdout);
}



static GHashTable *
get_or_create_sub_hash (GlobalSelection *selection,
                        const gchar     *dir_path)
{
  GHashTable *sub_hash;

  sub_hash = g_hash_table_lookup (selection->dirs, dir_path);
  if (G_UNLIKELY (sub_hash == NULL))
    {
      sub_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, entry_free);
      g_hash_table_insert (selection->dirs, g_strdup (dir_path), sub_hash);
    }

  return sub_hash;
}



void
global_selection_add (GlobalSelection *selection,
                      const gchar     *dir_path,
                      const gchar     *filename,
                      gboolean         is_dir,
                      guint64          size)
{
  GHashTable            *sub_hash;
  GlobalSelectionEntry  *entry;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (dir_path != NULL);
  g_return_if_fail (filename != NULL);

  sub_hash = get_or_create_sub_hash (selection, dir_path);

  /* check if already exists */
  if (G_UNLIKELY (g_hash_table_lookup (sub_hash, filename) != NULL))
    return; /* already exists, no toggle */

  /* create new entry */
  entry = g_new0 (GlobalSelectionEntry, 1);
  entry->dir_path = g_strdup (dir_path);
  entry->filename = g_strdup (filename);
  entry->is_dir = is_dir;
  entry->size = size;
  entry->dir_hash = g_str_hash (dir_path);

  /* add to sub-hash */
  g_hash_table_insert (sub_hash, g_strdup (filename), entry);

  /* update totals */
  if (G_UNLIKELY (is_dir))
    selection->total_dirs++;
  else
    selection->total_files++;
  selection->total_size += size;
}



void
global_selection_remove (GlobalSelection *selection,
                         const gchar     *dir_path,
                         const gchar     *filename)
{
  GHashTable            *sub_hash;
  GlobalSelectionEntry  *entry;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (dir_path != NULL);
  g_return_if_fail (filename != NULL);

  sub_hash = g_hash_table_lookup (selection->dirs, dir_path);
  if (G_UNLIKELY (sub_hash == NULL))
    return; /* directory not in selection */

  entry = g_hash_table_lookup (sub_hash, filename);
  if (G_UNLIKELY (entry == NULL))
    return; /* file not in selection */

  /* update totals */
  if (G_UNLIKELY (entry->is_dir))
    selection->total_dirs--;
  else
    selection->total_files--;
  selection->total_size -= entry->size;

  /* remove from sub-hash */
  g_hash_table_remove (sub_hash, filename);

  /* clean up empty sub-hash */
  if (G_UNLIKELY (g_hash_table_size (sub_hash) == 0))
    g_hash_table_remove (selection->dirs, dir_path);
}



GList *
global_selection_get_filenames_in_dir (GlobalSelection *selection,
                                       const gchar     *dir_path)
{
  GHashTable *sub_hash;
  GList      *filenames = NULL;

  g_return_val_if_fail (selection != NULL, NULL);
  g_return_val_if_fail (dir_path != NULL, NULL);

  sub_hash = g_hash_table_lookup (selection->dirs, dir_path);
  if (G_LIKELY (sub_hash != NULL))
    {
      GHashTableIter iter;
      gpointer       key, value;

      g_hash_table_iter_init (&iter, sub_hash);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          filenames = g_list_prepend (filenames, g_strdup ((const gchar *) key));
        }
    }

  return filenames;
}



void
global_selection_save (GlobalSelection *selection,
                       const gchar     *file_path)
{
  GKeyFile *key_file;
  GError   *error = NULL;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (file_path != NULL);

  key_file = g_key_file_new ();

  /* iterate through all directories */
  if (G_LIKELY (selection->dirs != NULL))
    {
      GHashTableIter dir_iter;
      gpointer       dir_key, dir_value;

      g_hash_table_iter_init (&dir_iter, selection->dirs);
      while (g_hash_table_iter_next (&dir_iter, &dir_key, &dir_value))
        {
          const gchar *dir_path = (const gchar *) dir_key;
          GHashTable  *sub_hash = (GHashTable *) dir_value;

          /* iterate through files in this directory */
          if (G_LIKELY (sub_hash != NULL))
            {
              GHashTableIter file_iter;
              gpointer       file_key, file_value;

              g_hash_table_iter_init (&file_iter, sub_hash);
              while (g_hash_table_iter_next (&file_iter, &file_key, &file_value))
                {
                  const gchar          *filename = (const gchar *) file_key;
                  GlobalSelectionEntry *entry = (GlobalSelectionEntry *) file_value;

                  /* create group name from full path */
                  gchar *full_path = g_build_filename (dir_path, filename, NULL);
                  g_key_file_set_uint64 (key_file, full_path, "size", entry->size);
                  g_key_file_set_boolean (key_file, full_path, "is_dir", entry->is_dir);
                  g_free (full_path);
                }
            }
        }
    }

  /* save to file */
  if (!g_key_file_save_to_file (key_file, file_path, &error))
    {
      g_warning ("Failed to save global selection to %s: %s", file_path, error->message);
      g_error_free (error);
    }

  g_key_file_free (key_file);
}



void
global_selection_load (GlobalSelection *selection,
                       const gchar     *file_path)
{
  GKeyFile *key_file;
  GError   *error = NULL;
  gchar   **groups;
  gsize     n_groups;
  gsize     i;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (file_path != NULL);

  key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, file_path, G_KEY_FILE_NONE, &error))
    {
      g_warning ("Failed to load global selection from %s: %s", file_path, error->message);
      g_error_free (error);
      g_key_file_free (key_file);
      return;
    }

  groups = g_key_file_get_groups (key_file, &n_groups);

  for (i = 0; i < n_groups; i++)
    {
      const gchar *full_path = groups[i];
      GFile       *file = g_file_new_for_path (full_path);
      gchar       *dir_path = g_path_get_dirname (full_path);
      gchar       *filename = g_path_get_basename (full_path);
      guint64      size = g_key_file_get_uint64 (key_file, full_path, "size", NULL);
      gboolean     is_dir = g_key_file_get_boolean (key_file, full_path, "is_dir", NULL);

      global_selection_add (selection, dir_path, filename, is_dir, size);

      g_free (dir_path);
      g_free (filename);
      g_object_unref (file);
    }

  g_strfreev (groups);
  g_key_file_free (key_file);
}



gchar *
global_selection_get_totals (GlobalSelection *selection)
{
  g_return_val_if_fail (selection != NULL, NULL);

  return g_strdup_printf ("%u files, %u dirs (%s)",
                          selection->total_files,
                          selection->total_dirs,
                          g_format_size (selection->total_size));
}