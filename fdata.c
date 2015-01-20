#include <stdlib.h>

#include "pak.h"
#include "fdata.h"

struct datasource_s
{
	struct datasource_s *next;
	int type;
};

struct filesrc_s
{
	struct datasource_s dsrc;
	char *path;
};

struct dirsrc_s
{
	struct datasource_s dsrc;
	char *path;
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
}


void
Data_RemovePath (const char *path)
{
}


void
Data_CloseAll (void)
{
	while (sources.next != NULL)
	{
		struct datasource_s *s = sources.next;
		sources.next = sources.next->next;
	}
}


void *
Data_Free (void *dat)
{
	if (dat != NULL)
	{
		free (dat);
		dat = NULL;
	}
	return dat;
}


void *
Data_Fetch (const char *name, unsigned int *size)
{
	//...
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
