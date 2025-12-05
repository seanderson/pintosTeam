#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  block_sector_t inode_sector = 0;
  struct dir *dir = dir_open_root ();
  bool success = false;

  if (dir == NULL)
  {
    printf("Error: Could not open root directory.\n");
    return false;
  }

  success = free_map_allocate (1, &inode_sector);
  if (!success) {
    printf("Error: Could not allocate inode sector.\n");
    dir_close (dir);
    return false;
  }

  success = inode_create (inode_sector, initial_size);
  if (!success) {
    printf("Error: Could not create inode for the file.\n");
    free_map_release (inode_sector, 1);
    dir_close (dir);
    return false;
  }

  success = dir_add (dir, name, inode_sector);
  if (!success) {
    printf("Error: Could not add file to the directory.\n");
    free_map_release (inode_sector, 1);
  }

  dir_close (dir);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise. */
struct file *
filesys_open (const char *name)
{
  struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;

  if (dir != NULL)
    dir_lookup (dir, name, &inode);
  dir_close (dir);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure. */
bool
filesys_remove (const char *name) 
{
  struct dir *dir = dir_open_root ();
  bool success = false;

  if (dir != NULL) {
    success = dir_remove (dir, name);
  } else {
    printf("Error: Could not open root directory.\n");
  }

  dir_close (dir); 
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  if (!free_map_create ()) {
    PANIC ("free map creation failed");
  }

  if (!dir_create (ROOT_DIR_SECTOR, 16)) {
    PANIC ("root directory creation failed");
  }

  free_map_close ();
  printf ("done.\n");
}
