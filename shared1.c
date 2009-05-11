#include <string.h>
#include <malloc.h>

#include "nandfs.h"
#include "shared1.h"

typedef struct _content_map_entry
{
	char filename[8];
	u8 sha1_hash[20];
} content_map_entry;

int cmap_entry_count  = 0;
content_map_entry *cmap_entries = NULL;

int shared1_init(struct nandfs *fs)
{
	struct nandfile * cmap = nandfs_open(fs, "/shared1/content.map");
	
	if (cmap->entry.size % sizeof(content_map_entry)) {
		printf("Error: Content.map size not a multiple of %d! (%d)\n", cmap->entry.size % sizeof(content_map_entry), cmap->entry.size);
		return -1;
	}
	cmap_entry_count = cmap->entry.size / sizeof(content_map_entry);
	
	// Let people call init as much as they want
	if (cmap_entries != NULL) {
		free(cmap_entries);
	}
	
	cmap_entries = calloc(cmap_entry_count, sizeof(content_map_entry));
	if (cmap_entries == NULL) {
		printf("Couldn't allocate memory for Content.map buffer (%d)\n", cmap_entry_count * sizeof(content_map_entry));
		return -1;
	}
		
	nandfs_read(cmap_entries, sizeof(content_map_entry) * cmap_entry_count, cmap);
	nandfs_close(cmap);
	return 0;
}

int get_shared1_filename(char * filename, const u8 hash[20])
{
	int i;
	// Sorry, charlie. Not sure how best to do init.
	/*
	if (cmap_entries == NULL) {
		if(__shared1_init()) {
			printf("Failed to init shared1 buffer!");
			return -1;
		}
	} */
	if (cmap_entries == NULL) {
		printf("Shared1 not initialized.\n");
		return -1;
	}
	
	for (i = 0; i < cmap_entry_count; i++) {
		if(!memcmp(cmap_entries[i].sha1_hash, hash, 20)) {
			// Totally faster than sprintf
			strcpy(filename, "/shared1/XXXXXXXX.app");
			memcpy(filename+9, cmap_entries[i].filename, 8);
			return 0;
		}
	}
	printf("No such entry found in content.map\n");
	return 1;
}
