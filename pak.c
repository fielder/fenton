#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pak.h"

struct pakfile_s
{
	char name[56];
	unsigned int filepos;
	unsigned int filelen;
};

struct pak_s
{
	char *path;
	int fd;
	unsigned int num_files;
	struct pakfile_s files[0];
};


static unsigned int
GetUInt (const void *ptr)
{
	const unsigned char *b = ptr;
	return (b[0] << 0) | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
}


struct pak_s *
Pak_Open (const char *path)
{
	int fd, i, j;
	char hdr[12];
	unsigned int dirofs, dirlen;
	struct pak_s *pak;

	if ((fd = open(path, O_RDONLY)) == -1)
	{
		return NULL;
	}

	if (read(fd, hdr, sizeof(hdr)) != sizeof(hdr))
	{
		close (fd);
		return NULL;
	}

	if (strncmp(hdr, "PACK", 4) != 0)
	{
		close (fd);
		return NULL;
	}

	dirofs = GetUInt (hdr + 4);
	dirlen = GetUInt (hdr + 8);

	pak = malloc (sizeof(*pak) + dirlen + strlen(path) + 1);

	pak->fd = fd;
	pak->num_files = dirlen / sizeof(struct pakfile_s);
	pak->path = (char *)pak + sizeof(*pak) + dirlen;
	strcpy (pak->path, path);

	lseek (pak->fd, dirofs, SEEK_SET);
	if (read(pak->fd, pak->files, dirlen) != dirlen)
	{
		close (pak->fd);
		free (pak);
		return NULL;
	}

	for (i = 0; i < pak->num_files; i++)
	{
		for (j = 0; j < 56 && pak->files[i].name[j] != '\0'; j++) {}
		for (; j < 56; j++)
			pak->files[i].name[j] = '\0';
		pak->files[i].filepos = GetUInt (&pak->files[i].filepos);
		pak->files[i].filelen = GetUInt (&pak->files[i].filelen);
	}

	return pak;
}


void *
Pak_Close (struct pak_s *pak)
{
	if (pak->fd != -1)
		close (pak->fd);
	free (pak);
	return NULL;
}


void *
Pak_ReadEntry (struct pak_s *pak, const char *name, unsigned int *size)
{
	const struct pakfile_s *pf;
	int i;

	for (	i = 0, pf = pak->files;
		i < pak->num_files && strcmp(pf->name, name) != 0;
		i++, pf++) {}

	if (i == pak->num_files)
		return NULL;

	void *dat = malloc (pf->filelen + 1);
	if (lseek(pak->fd, pf->filepos, SEEK_SET) == -1 || read(pak->fd, dat, pf->filelen) != pf->filelen)
	{
		free (dat);
		return NULL;
	}
	((char *)dat)[pf->filelen] = '\0';
	if (size != NULL)
		*size = pf->filelen;
	return dat;
}


void *
Pak_FreeEntry (void *dat)
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
		struct pak_s *p;
		if ((p = Pak_Open(argv[i])) == NULL)
		{
			fprintf (stderr, "error: unable to open \"%s\"\n", argv[i]);
		}
		else
		{
			int j;
			unsigned int bytesused = 0;
			printf ("offset  size  name\n");
			for (j = 0; j < p->num_files; j++)
			{
				printf ("%8d %8d %s\n", p->files[j].filepos, p->files[j].filelen, p->files[j].name);
				bytesused += p->files[j].filelen;
			}
			printf ("%d bytes in %d entries\n", bytesused, p->num_files);
			p = Pak_Close (p);
		}
	}

	return 0;
}

#endif
