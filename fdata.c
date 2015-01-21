#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pak.h"
#include "fdata.h"

enum
{
	ST_FILE,
	ST_DIR,
	ST_PAK,
};

struct datasource_s
{
	struct datasource_s *next;
	char *path;
	int type;
};

struct filesrc_s
{
	struct datasource_s dsrc;
	int fd;
};

struct dirsrc_s
{
	struct datasource_s dsrc;
};

struct paksrc_s
{
	struct datasource_s dsrc;
	struct pak_s *pak;
};


static struct datasource_s sources = { NULL };


int
Data_AddPath (const char *path)
{
	struct datasource_s *src;

	for (src = sources.next; src != NULL; src = src->next)
	{
		if (strcmp(src->path, path) == 0)
			return 1;
	}

	//...

	return 1;
}


static void
CloseSource (struct datasource_s *src)
{
	if (src->type == ST_FILE)
	{
		struct filesrc_s *fs = (void *)src;
		if (fs->fd != -1)
			close (fs->fd);
		free (fs);
	}
	else if (src->type == ST_DIR)
	{
		free (src);
	}
	else if (src->type == ST_PAK)
	{
		struct paksrc_s *ps = (void *)src;
		Pak_Close (ps->pak);
		free (ps);
	}
	else
	{
		//TODO: die ?
	}
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
		CloseSource (n);
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
Data_Fetch (const char *name, unsigned int *size)
{
	struct datasource_s *src;

	for (src = sources.next; src != NULL; src = src->next)
	{
		if (src->type == ST_FILE)
		{
		}
		else if (src->type == ST_DIR)
		{
		}
		else if (src->type == ST_PAK)
		{
		}
		else
		{
			//TODO: die ?
		}
	}

	return NULL;
}



#if 0
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
	}

	return NULL;
}
#endif
