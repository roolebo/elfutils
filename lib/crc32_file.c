#include "system.h"
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

int
crc32_file (int fd, uint32_t *resp)
{
  unsigned char buffer[1024 * 8];
  uint32_t crc = 0;
  off_t off = 0;
  ssize_t count;

  struct stat st;
  if (fstat (fd, &st) == 0)
    {
      /* Try mapping in the file data.  */
      size_t mapsize = st.st_size;
      void *mapped = mmap (NULL, mapsize, PROT_READ, MAP_PRIVATE, fd, 0);
      if (mapped == MAP_FAILED && errno == ENOMEM)
	{
	  const size_t pagesize = sysconf (_SC_PAGE_SIZE);
	  mapsize = ((mapsize / 2) + pagesize - 1) & -pagesize;
	  while (mapsize >= pagesize
		 && (mapped = mmap (NULL, mapsize, PROT_READ, MAP_PRIVATE,
				    fd, 0)) == MAP_FAILED && errno == ENOMEM)
	    mapsize /= 2;
	}
      if (mapped != MAP_FAILED)
	{
	  do
	    {
	      if (st.st_size <= (off_t) mapsize)
		{
		  *resp = crc32 (crc, mapped, st.st_size);
		  munmap (mapped, mapsize);
		  return 0;
		}
	      crc = crc32 (crc, mapped, mapsize);
	      off += mapsize;
	      st.st_size -= mapsize;
	    } while (mmap (mapped, mapsize, PROT_READ, MAP_FIXED|MAP_PRIVATE,
			   fd, off) == mapped);
	  munmap (mapped, mapsize);
	}
    }

  while ((count = TEMP_FAILURE_RETRY (pread (fd, buffer, sizeof buffer,
					     off))) > 0)
    {
      off += count;
      crc = crc32 (crc, buffer, count);
    }

  *resp = crc;

  return count == 0 ? 0 : -1;
}
