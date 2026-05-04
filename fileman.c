/* Allegro port Copyright (C) 2014 Gavin Smith. */
/*-------------------------------------------------------*/
/*                                                       */
/*               F I L E M A N A G E R                   */
/*           [c] copyright 1993 by Alpha-Helix           */
/*               written by Dany Schoch                  */
/*                                                       */
/*   Revision List:                                      */
/*	27. May 93: Memory peak meter added.	 	         */
/*      28.   : >64kB file load error corrected.         */
/*	30. June  : 'shutfilemanager' crash corrected.       */
/*	22. Sept  : 'openfile' added.			             */
/*	30. Nov	  : 'errno.h' error compatibility added.     */
/*  19. Dez   : 'printbuffer' added.                     */
/*  Apr 2026  : 'findbeerfile', 'fivebeerfilename'       */
/*                  added, use linux/limits.h            */


#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "fileman.h"

#define TRUE		1
#define FALSE		0


typedef unsigned long ulong;
typedef unsigned int uint;

#define HDRSIZE			30

/* Size of file header as it appears on disk. sizeof(filestrc) may
   be bigger because of padding. */
#define SIZE_OFFSET 14
#define FLAGS_OFFSET (14 + 4)
#define FPTR_OFFSET (14 + 4 + 2)
#define SIZE_OF_FILE_HEADER (14 + 4 + 2 + 4)

struct filestrc {
  char name[14];		// Name of file.
  ulong size;			// It's size.
  int flags;			// Buffering flags.
  long fptr;			// File pointer to access it's data.
};


struct dbasestrc {
  char basename[PATH_MAX];	// Path and name of the open data base.
  FILE *filvar;			// File handle of data base.
  int nfiles;			// # of files in data base.
  struct filestrc file[];
};


struct entrystrc {
  char name[14];		// Name of the buffered file.
  ulong size;
  char *addr;
  int hits;			// Cache hit counter.
  int flags;
};

static char *xmspointer;	// Pointer to free XMS.
static ulong freexms;		// Free extended mem.
static int nentries;		// # of table entries.
static struct entrystrc *entry;	// Entry table.
static void (*error) (char *text, int code, ...);

static void errf (char *text, int code, ...) {
  return;
}

char beerconfigfile[PATH_MAX] = "0";
char beerdatafile[PATH_MAX] = "0";

/*////// Structures for finding files ////////////////////////
/// The main thing is to not hard-code 
/// any file or directory names (elsewhere) 
/// in code and minimally here
/// there is only one function and one only
/// to get file X - the rest of the code does
/// not know the platform or where the file actually is
/// only findbeerfile is exposed to the rest of the code
///
/// This is probably way overkill for the purposes of this game
/// which uses only a few files, but I made this as 
/// an exercize - one way to try to make (more) portable code
///////////////////////////////////////////////////////// */

enum beerfiletypes {
  DATA,         // data for game, only red
  USERCONFIG,   // user-specific
  SHARED        // highscore etc., can be shared in multi-juser systems
};

/*////////////////////////////////////////////////////
// Fill in according to ./configure and / ot change 
// these accordingly at run-time and platform
// search from directories according to beerfiletype
/////////////////////////////////////////////////// */
struct searchpaths_s {         
    //  = DATADIR;
    char beerdatadir[PATH_MAX];
    // location of beer executable (depends where installed; might not be ".")
    char exedir[PATH_MAX];              // TODO: actually find it!
    // /var... for shared files (highscore) in multi-user systems
    char localstatedir[PATH_MAX];       
    // look in XDG_CONFIG_HOME/beer and HOME/.config/beer...
    char user_beerconfigdir[PATH_MAX];
}beerpaths;

// File and it's metadata, location
struct beerfileentry {
  char name[NAME_MAX];
  char path[PATH_MAX];
  char fullpath[PATH_MAX];
  enum beerfiletypes type;
};

// Information about all files - only hardcoded information is here!!
static struct beerfiles_s {
  struct beerfileentry data;
  struct beerfileentry config;
  struct beerfileentry hiscore;
} beerfiles = {
  {"lastbeer.bin", "", "", DATA},
  {"config.bin",   "", "", USERCONFIG},
  {"hiscore.bin",  "", "", SHARED}
};

/*------------------------------------------------------
Function populate_dirs 

Pupulate a struct searchpaths; platform-specific things 
should go here (and hopefully only here)
------------------------------------------------------*/
void populate_dirs() {
  snprintf(beerpaths.beerdatadir, PATH_MAX, "%s/%s", DATADIR, PACKAGE_NAME);
  readlink("/proc/self/exe", beerpaths.exedir, PATH_MAX);
  snprintf(beerpaths.localstatedir, PATH_MAX, "%s/%s", LOCALSTATEDIR, PACKAGE_NAME);

  if (char *xdgconfhome = getenv ("XDG_CONFIG_HOME")) {
    snprintf (beerpaths.user_beerconfigdir, PATH_MAX, "%s", xdgconfhome);
  } else if (char *userhome = getenv ("HOME")) {
    sprintf (beerpaths.user_beerconfigdir, "%s/%s", userhome, ".config");
    int mdr = mkdir (beerpaths.user_beerconfigdir, 0777);
    if (mdr != 0 && errno != EEXIST)
      printf ("Error creating .config dir: %s\n", strerror (errno));
  }
  sprintf (beerpaths.user_beerconfigdir, "%s/%s", beerpaths.user_beerconfigdir, PACKAGE_NAME);
  int mdr = mkdir (beerpaths.user_beerconfigdir, 0777);   // PACKAGE_NAME = lastbeer (unless changed)
  if (mdr != 0 && errno != EEXIST) {
    printf ("Errror creating %s directory: %i: %s\n", beerpaths.user_beerconfigdir, errno,
        strerror (errno));
  } 
  // Make directories if needed and test access
  // might create ./.config, change this?
  if ( access( beerpaths.user_beerconfigdir, W_OK | X_OK) != 0 ) {
    printf("Could not find a writeable config dir (tried %s), using \".\"!\n", beerpaths.user_beerconfigdir);
    sprintf(beerpaths.user_beerconfigdir, ".");
  }
}

/*------------------------------------------------------
Function: finbeerfiles

Find all files (or a location for them) and populate 
directory for the struct
------------------------------------------------------*/
void findbeerfiles() {
  populate_dirs();

  char *getfullpath(struct beerfileentry beerfile) {
    char testfile[PATH_MAX];
    switch(beerfile.type) {
      case DATA:

        snprintf(testfile, PATH_MAX, "%s/%s", beerpaths.beerdatadir, beerfile.name);
        if (access(testfile, R_OK) == 0) return beerpaths.beerdatadir;
        snprintf(testfile, PATH_MAX, "%s/%s", beerpaths.exedir, beerfile.name);
        if (access(testfile, R_OK) == 0) return beerpaths.exedir;

        if (access(beerfile.name, R_OK) == 0) return ".";

        printf("ERROR: Can not find or access %s!\n", beerfile.name);
        return ".";
        break;
      case USERCONFIG:
        if (access (beerpaths.user_beerconfigdir, W_OK | R_OK | X_OK ) == 0 ) return beerpaths.user_beerconfigdir;
        if (access (".", R_OK | W_OK | X_OK) == 0) return ".";
        printf("ERROR: Can not find writable directory for %s!\n", beerfile.name);
        return NULL;
        break;
      case SHARED:
        sprintf(testfile, "");
        if (access (beerpaths.localstatedir, R_OK | X_OK ) == 0 ) {
          snprintf(testfile, PATH_MAX, "%s/%s", beerpaths.localstatedir, beerfile.name);
          if (access (testfile, R_OK | W_OK ) == 0 ) {
            return beerpaths.localstatedir;
          }
        }
        if (access (beerpaths.user_beerconfigdir, W_OK | X_OK | R_OK ) == 0 ) return beerpaths.user_beerconfigdir;
        if (access (".", R_OK ) == 0) return ".";
        printf("ERROR: Can not find writable directory for %s!\n", beerfile.name);
        return NULL;
        break;
    }
  }

  snprintf(beerfiles.data.path, PATH_MAX, "%s", getfullpath(beerfiles.data));
  snprintf(beerfiles.config.path, PATH_MAX,  "%s", getfullpath(beerfiles.config));
  snprintf(beerfiles.hiscore.path, PATH_MAX, "%s", getfullpath(beerfiles.hiscore));

  sprintf(beerfiles.data.fullpath, "%s/%s", beerfiles.data.path ,beerfiles.data.name );
  sprintf(beerfiles.hiscore.fullpath, "%s/%s", beerfiles.hiscore.path ,beerfiles.hiscore.name );
  sprintf(beerfiles.config.fullpath, "%s/%s", beerfiles.config.path ,beerfiles.config.name );
  
  printf("Found files: %s, %s, %s\n", beerfiles.hiscore.fullpath, beerfiles.data.fullpath, beerfiles.config.fullpath);

}

/*------------------------------------------------------
Function: finbeerfile

Function to return FULL PATH to files.
------------------------------------------------------*/
char *findbeerfile(enum beerfile file_entry) {
  switch (file_entry) {
  case BEER_DATAFILE:
    return beerfiles.data.fullpath;
    break;
  case BEER_CONFIG:
    return beerfiles.config.fullpath;
    break;
  case BEER_HISCORE:
    return beerfiles.hiscore.fullpath;
    break;
  }
}

/*------------------------------------------------------
Function: initfilemanager

Description: Initializes the Alpha-Helix file manager.
    handles: max # of files that can be buffered.
    minsize:\ extended memory that will be allocated
    maxsize:/ (in kB)
    err    : Pointer to alternative error handler.
Returns: kBytes allocated on success.
	      0 not enough free XMS memory.
	     -1 not enough main memory.
------------------------------------------------------*/
int initfilemanager (int handles,
		     int minsize, int maxsize,
		     void (*err) (char *text, int code, ...)) {

// Install custom error handler.
  if (err == NULL)
    error = errf;
  else
    error = err;

  entry = NULL;

  /* Always allocate the maxsize. */
  xmspointer = malloc (freexms = maxsize);
  if (!xmspointer)
    return -1;

// Allocate memory for the buffer table.
  nentries = handles;
  entry = (struct entrystrc *) calloc (nentries, sizeof (struct entrystrc));
  if (entry == NULL) {
    return -1;			// Fatal error.
  }

  return 1;			// Successful

}


/*------------------------------------------------------
Function: shutfilemanager

Description: Leaves the filemanager and frees the allocated
	     memory.
	     NOTE: memory allocated by 'opendatabase'
		   won't be freed.
------------------------------------------------------*/
void shutfilemanager (void) {
  if (entry != NULL)
    free (entry);
#ifdef DEBUG
  printf ("\nDOS free memory: %ld bytes.\n", mindosmem);
#endif
}

#ifdef DEBUG
void printbuffer (void) {
  int i;

  printf ("Name           Size      Addr     Hits\n");
  printf ("--------------------------------------\n");
  for (i = 0; i < nentries; i++) {
    if (entry[i].size != 0)
      printf ("%s       %ld        %lx      %d\n",
	      entry[i].name, entry[i].size, entry[i].addr, entry[i].hits);
  }

}
#endif

// -- Start of private area.

static struct entrystrc *findfreeentry (void) {
  int i;

  for (i = 0; i < nentries; i++) {
    if (entry[i].size == 0)
      break;
  }
  if (entry[i].size == 0)
    return &entry[i];
  else
    return NULL;

}


static struct entrystrc *findentry (char *file) {
  int i;
  struct entrystrc *result;

  result = NULL;
  if (entry != NULL) {
    for (i = 0; i < nentries; i++) {
      if (strcmp (file, entry[i].name) == 0) {
	result = &entry[i];
	break;
      }
    }
  }
  return result;

}


static void freeentry (int i) {
  ulong size;
  struct entrystrc *result;

// Move memory blocks.
  result = &entry[i];
  size = result->size;
  memmove (result->addr, result->addr + size,
	   xmspointer - result->addr - result->size);
  freexms += size;
  xmspointer -= size;		// Adjust free xms pointer.

// Move index table.
  if (i < nentries - 1) {
    memmove (result, result + 1,
	     (nentries - i - 1) * sizeof (struct entrystrc));
  }
  for (; i < nentries - 1; i++)
    entry[i].addr -= size;
  entry[nentries - 1].size = 0;	// Mark free slot.
}

static int lightxms (void) {
  int count;
  int i;
  struct entrystrc *result;

  count = 0;
  do {

// Look for a block to remove.
    result = NULL;
    for (i = 0; i < nentries; i++) {
      if ((entry[i].hits == count) && (entry[i].size != 0)) {
	result = &entry[i];
	break;
      }
    }
    if (result) {
      freeentry (i);
// Decrement hitcounts.
      for (i = 0; i < nentries; i++)
	if (entry[i].hits > 0)
	  entry[i].hits--;

      return TRUE;
    }
  } while (count++ < 20);

  return FALSE;
}


static void buffer (char *file, void *data, ulong size, int flags) {
  struct entrystrc *result;

  if ((flags & M_XMS)) {
    // Buffer in extended memory.
    if (!(flags & M_NOFREEUP)) {
      while (freexms < size)
	if (!lightxms ())
	  break;		// Try to make space.
    }
    result = findfreeentry ();
    if (result != NULL && (freexms >= size)) {
      strcpy (result->name, file);
      result->size = size;
      result->addr = xmspointer;
      result->hits = 0;
      result->flags = flags;
      memmove (result->addr, data, size);
      freexms -= size;
      xmspointer += size;	// Move pointer to remaining free XMS.
    }
  }

}

// --- End of private area.




/*------------------------------------------------------
Function: opendatabase

Description: Opens a special type of file: The Alpha-Helix
	     James Bond Baller game data library file
	     type.
------------------------------------------------------*/
void *opendatabase (char *file) {
  FILE *filvar;
  int nfiles;
  uint nread;
  struct dbasestrc *ptr;
  char hdr[HDRSIZE];
  int mode;
  char byte;

#ifndef DATADIR
#define DATADIR "."
#endif

  printf ("Opening database file %s\n", file);
  if (!(filvar = fopen (file, "rb"))) {
    (*error) ("opendatabase", ENOENT, file);
    return NULL;
  }

  fread (hdr, HDRSIZE, 1, filvar);

  fread (&byte, 1, 1, filvar);
  mode = byte;
  fread (&byte, 1, 1, filvar);
  mode += byte << 8;
  fread (&byte, 1, 1, filvar);
  nfiles = byte;
  fread (&byte, 1, 1, filvar);
  nfiles += byte << 8;

  ptr = malloc (sizeof (struct dbasestrc) + nfiles * SIZE_OF_FILE_HEADER);

  if (ptr == NULL) {
    fclose (filvar);
    (*error) ("opendatabase", ENOENT);
    return NULL;
  }
// Fill in the newly created structure.
  strcpy (ptr->basename, file);
  ptr->filvar = filvar;
  ptr->nfiles = nfiles;
  fread ((void *) ptr->file, nfiles * SIZE_OF_FILE_HEADER, 1, filvar);

  return (void *) ptr;

}

/*------------------------------------------------------
Function: closedatabase

Description: Undoes the effect of a previous 'opendatabase'.
------------------------------------------------------*/
void closedatabase (void *database) {
  struct dbasestrc *ptr;

  ptr = (struct dbasestrc *) database;
  fclose (ptr->filvar);
  free (ptr);
}


/*------------------------------------------------------
Function: loadfile

Description: Reserves enough memory to hold the file
	     in memory and loads it from the database.
------------------------------------------------------*/
unsigned char *loadfile (void *database, char *file) {
  struct entrystrc *result;
  struct dbasestrc *ptr;
  unsigned char *data;
  unsigned char *p;
  ulong fsize, msize;		// file and memory size of file respectively.
  int i, j;
  uint nread;

  ptr = (struct dbasestrc *) database;
  result = findentry (file);
  if (result != NULL) {
    // File is in buffer.
    if (result->hits < 20)
      result->hits++;
    // Allocated memory and copy block.
    if ((data = malloc (result->size)) == NULL) {
      (*error) ("loadfile", ENOMEM);
      return NULL;
    }
    memmove (data, result->addr, result->size);
  } else {
    ulong size;
    int flags;
    long fptr;
    unsigned char *rec_ptr;
    // File must be read from disk.
    for (i = 0, j = -1; i < ptr->nfiles; i++) {
      rec_ptr = (unsigned char *) (&ptr->file) + i * SIZE_OF_FILE_HEADER;

      /* File record begins with name so a pointer to it is a pointer
         to the name. */
      if (strcmp (file, rec_ptr) == 0) {
	j = i;
	break;
      }
    }
    if (j == -1) {
      (*error) ("loadfile", ENOENT, file);
      return NULL;
    }

    /* Read 4 byte size record */
    size = rec_ptr[SIZE_OFFSET];
    size += rec_ptr[SIZE_OFFSET + 1] << 8;
    size += rec_ptr[SIZE_OFFSET + 2] << 16;
    size += rec_ptr[SIZE_OFFSET + 3] << 24;

    /* Read 2 byte flags record */
    flags = rec_ptr[FLAGS_OFFSET];
    flags += rec_ptr[FLAGS_OFFSET + 1] << 8;

    /* Read 4 byte fptr record */
    fptr = rec_ptr[FPTR_OFFSET];
    fptr += rec_ptr[FPTR_OFFSET + 1] << 8;
    fptr += rec_ptr[FPTR_OFFSET + 2] << 16;
    fptr += rec_ptr[FPTR_OFFSET + 3] << 24;

    fseek (ptr->filvar, fptr, SEEK_SET);
    fsize = size;
    msize = (fsize + 1) & 0xfffffffel;	// make it even.
    if ((data = malloc (msize)) == NULL) {
      (*error) ("loadfile", ENOENT);
      return NULL;
    }
    p = data;
    while (fsize > 65000) {
      fread (p, 1, 65000, ptr->filvar);
      fsize -= 65000;
      p += 65000;
    }
    nread = fread (p, 1, fsize, ptr->filvar);

    buffer (file, data, size, flags);
  }

  return data;

}


/*------------------------------------------------------
Function: loadfiledirect

Description: Allocates memory for the file and loads
	     it if present from the buffer, otherwise
	     from disk.
------------------------------------------------------*/
void *loadfiledirect (char *file, int flags) {
  FILE *filvar;
  struct entrystrc *result;
  void *ptr;
  char *p;
  ulong fsize, msize;		// real size and size in memory of file.
  uint nread;

  result = findentry (file);
  if (result != NULL) {
    // File is in buffer.
    if (result->hits < 20)
      result->hits++;
    // Allocated memory and copy block.
    if ((ptr = malloc (result->size)) == NULL) {
      (*error) ("loadfiledirect", ENOMEM);
      return NULL;
    }
    memmove (ptr, result->addr, result->size);
  } else {
    // File must be read from disk.
    if (!(filvar = fopen (file, "rb"))) {
      (*error) ("loadfiledirect", ENOENT, file);
      return NULL;
    }
    /* Calculate length of file */
    fseek (filvar, 0, SEEK_END);
    fsize = ftell (filvar);
    rewind (filvar);

    msize = (fsize + 1) & 0xfffffffel;	// round it up.
    if ((ptr = malloc (msize)) == NULL) {
      fclose (filvar);
      (*error) ("loadfiledirect", ENOMEM);
      return NULL;
    }
    p = ptr;
    while (fsize > 65000) {
      nread = fread (p, 1, 65000, filvar);
      fsize -= 65000;
      p += 65000;
    }
    nread = fread (p, 1, fsize, filvar);
    fclose (filvar);

    buffer (file, ptr, msize, flags);
  }

  return ptr;

}


void unloadfile (void *ptr) {
  free (ptr);
}


/*------------------------------------------------------
Function: openfile

Description: Same as loadfile, but doesn't read it into
	     memory. Just sets the file pointer to
	     the start of the file.
Returns: File handle for further access, null if failed.
------------------------------------------------------*/
FILE *openfile (void *database, char *file) {
  /* Not converted yet. */
  return 0;
#if 0
  struct dbasestrc *ptr;	// Pointer to data base directory.
  int i, j;
  FILE *filvar;


  ptr = (struct dbasestrc *) database;
  for (i = 0, j = -1; i < ptr->nfiles; i++) {
    if (strcmp (file, ptr->file[i].name) == 0) {
      j = i;
      break;
    }
  }
  if (j == -1) {
    (*error) ("openfile", ENOENT, file);
    return 0;
  }
// Re-open database to get a free file pointer.
  if (!(filvar = fopen (ptr->basename, "rb"))) {
    (*error) ("openfile", ENOENT, ptr->basename);
    return 0;
  }
// Seek to data.
  fseek (filvar, ptr->file[j].fptr, SEEK_SET);

  return filvar;
#endif
}


/*------------------------------------------------------
Function: openfiledirect

Description: In fact this is the same as open the file
	     for read only using fopen.
------------------------------------------------------*/
FILE *openfiledirect (char *file) {
  FILE *filvar;

  if (!(filvar = fopen (file, "rb"))) {
    (*error) ("openfiledirect", ENOENT, file);
    return 0;
  }

  return filvar;
}


/*------------------------------------------------------
Function: closefile

Description:
------------------------------------------------------*/
void closefile (FILE *filvar) {
  fclose (filvar);
}
