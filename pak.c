#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "pak.h"

struct pakfile_s
{
	char name[56];
	unsigned int filepos;
	unsigned int filelen;
};

struct pak_s
{
	struct pak_s *next;
	char *path;
	int fd;
	unsigned int num_files;
	struct pakfile_s files[0];
};

static struct pak_s paks = { NULL };


static unsigned int
getUInt (const void *ptr)
{
	const unsigned char *bytes = ptr;
	return	((unsigned int)bytes[0] << 0) | ((unsigned int)bytes[1] << 8) |
		((unsigned int)bytes[2] << 16) | ((unsigned int)bytes[3] << 24);
}


int
Pak_AddFile (const char *path)
{
	struct pak_s *pak;
	int fd, i, j;
	char hdr[12];
	unsigned dirofs, dirlen;

	for (pak = paks.next; pak != NULL; pak = pak->next)
	{
		if (strcmp(pak->path, path) == 0)
			return 1;
	}

	if ((fd = open(path, O_RDONLY)) == -1)
		return 0;

	if (read(fd, hdr, sizeof(hdr)) != sizeof(hdr))
	{
		close (fd);
		return 0;
	}

	if (strncmp(hdr, "PACK", 4) != 0)
	{
		close (fd);
		return 0;
	}

	dirofs = getUInt (hdr + 4);
	dirlen = getUInt (hdr + 8);

	pak = malloc (sizeof(*pak) + dirlen + strlen(path) + 1);

	pak->next = paks.next;
	paks.next = pak;
	pak->fd = fd;
	pak->num_files = dirlen / sizeof(struct pakfile_s);
	pak->path = (char *)pak + sizeof(*pak) + dirlen;
	strcpy (pak->path, path);

	lseek (pak->fd, dirofs, SEEK_SET);
	if (read(pak->fd, pak->files, dirlen) != dirlen)
	{
		Pak_CloseFile (path);
		return 0;
	}

	for (i = 0; i < pak->num_files; i++)
	{
		for (j = 0; j < 56 && pak->files[i].name[j] != '\0'; j++) {}
		for (; j < 56; j++)
			pak->files[i].name[j] = '\0';
		pak->files[i].filepos = getUInt (&pak->files[i].filepos);
		pak->files[i].filelen = getUInt (&pak->files[i].filelen);
	}

	return 1;
}


void
Pak_CloseFile (const char *path)
{
	struct pak_s *pak;

	for (	pak = &paks;
		pak->next != NULL && strcmp(pak->next->path, path) != 0;
		pak = pak->next) {}

	if (pak->next != NULL)
	{
		struct pak_s *n = pak->next;
		pak->next = pak->next->next;
		if (n->fd != -1)
			close (n->fd);
		free (n);
	}
}


void
Pak_CloseAll (void)
{
	while (paks.next != NULL)
		Pak_CloseFile (paks.next->path);
}


void *
Pak_Read (const char *name, unsigned int *size)
{
	const struct pak_s *pak;

	/* try a file first */
	{
		void *ret = NULL;
		int fd = open (name, O_RDONLY);
		if (fd != -1)
		{
			int sz = lseek (fd, 0, SEEK_END);
			if (sz != -1 && sz < (1024 * 1024 * 1024))
			{
				if ((ret = malloc(sz + 1)) != NULL)
				{
					lseek (fd, 0, SEEK_SET);
					if (read(fd, ret, sz) != sz)
					{
						free (ret);
						ret = NULL;
					}
					else
					{
						((char *)ret)[sz] = '\0';
						if (size != NULL)
							*size = sz;
					}
				}
			}
			close (fd);
			fd = -1;
		}
		if (ret != NULL)
			return ret;
	}

	for (pak = paks.next; pak != NULL; pak = pak->next)
	{
		const struct pakfile_s *pf;
		int i;
		for (	i = 0, pf = pak->files;
			i < pak->num_files && strcmp(pf->name, name) != 0;
			i++, pf++) {}
		if (i < pak->num_files)
		{
			void *dat = malloc (pf->filelen + 1);
			lseek (pak->fd, pf->filepos, SEEK_SET);
			if (read(pak->fd, dat, pf->filelen) != pf->filelen)
			{
				free (dat);
				return NULL;
			}
			((char *)dat)[pf->filelen] = '\0';
			if (size != NULL)
				*size = pf->filelen;
			return dat;
		}
	}

	return NULL;
}


void *
Pak_Free (void *dat)
{
	if (dat != NULL)
	{
		free (dat);
		dat = NULL;
	}
	return dat;
}


#if 0

#include <stdio.h>

int
main (int argc, const char **argv)
{
	int i;

	for (i = 1; i < argc; i++)
	{
		if (!Pak_AddFile(argv[i]))
		{
			fprintf (stderr, "error: unable to open \"%s\"\n", argv[i]);
		}
		else
		{
			int j;
			unsigned int bytesused = 0;
			printf ("offset  size  name\n");
			for (j = 0; j < paks.next->num_files; j++)
			{
				printf ("%8d %8d %s\n", paks.next->files[j].filepos, paks.next->files[j].filelen, paks.next->files[j].name);
				bytesused += paks.next->files[j].filelen;
			}
			printf ("%d bytes in %d entries\n", bytesused, paks.next->num_files);
			Pak_CloseAll ();
		}
	}

	return 0;
}

#endif
