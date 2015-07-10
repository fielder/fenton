#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "pak.h"
#include "fdata.h"

struct datasource_s
{
	struct datasource_s *next;
	struct pak_s *pak;
	char path[0];
};


static struct datasource_s sources = { NULL };


int
Data_AddPath (const char *path)
{
	struct datasource_s *src;
	struct pak_s *pak;

	/* see if already loaded */
	for (src = sources.next; src != NULL; src = src->next)
	{
		if (strcmp(src->path, path) == 0)
			return 1;
	}

	if ((pak = Pak_Open(path)) == NULL)
		return 0;

	src = malloc(sizeof(*src) + strlen(path) + 1);
	src->next = sources.next;
	sources.next = src;
	strcpy (src->path, path);
	src->pak = pak;

	return 1;
}


void
Data_RemovePath (const char *path)
{
	struct datasource_s *p, *n;

	for (	p = &sources, n = sources.next;
		n != NULL && strcmp(n->path, path) != 0;
		p = p->next, n = n->next ) {}

	if (n != NULL)
	{
		p->next = n->next;

		if (n->pak != NULL)
			Pak_Close (n->pak);
		free (n);
	}
}


void
Data_CloseAll (void)
{
	while (sources.next != NULL)
		Data_RemovePath (sources.next->path);
}


void *
Data_Free (void *dat)
{
	if (dat != NULL)
		free (dat);
	return NULL;
}


void *
Data_Fetch (const char *name, int *size)
{
	struct datasource_s *src;

	for (src = sources.next; src != NULL; src = src->next)
	{
		void *ret = Pak_ReadEntry (src->pak, name, size);
		if (ret != NULL)
			return ret;
	}

	/* not found */

	return NULL;
}


void *
Data_ReadFile (const char *path, int *size)
{
	void *ret = NULL;
	int fd;

	if ((fd = open(path, O_RDONLY)) != -1)
	{
		off_t sz = lseek (fd, 0, SEEK_END);
		if (sz != -1 && sz < (1024 * 1024 * 1024))
		{
			if ((ret = malloc(sz + 1)) != NULL)
			{
				if (lseek(fd, 0, SEEK_SET) == -1 || read(fd, ret, sz) != sz)
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

	return ret;
}


int
Data_IsDir (const char *path, int *isdir)
{
	struct stat sb;
	if (stat(path, &sb) < 0)
		return 0;
	*isdir = S_ISDIR(sb.st_mode);
	return 1;
}
