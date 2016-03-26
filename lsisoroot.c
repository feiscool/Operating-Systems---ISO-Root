#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 2048

static char *read_sector(FILE *file, char *buffer);
static void printn(const char *string, int len);
static int lsisoroot(FILE *file);
static void ls(FILE *file, int32_t root_dir_size);

int main(int argc, char **argv)
{
	FILE *file;
	int result;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: lsisoroot filename.iso\n");
		return EXIT_FAILURE;
	}

	if ((file = fopen(argv[1], "rb")) == NULL)
	{
		perror(argv[1]);
		return EXIT_FAILURE;
	}

	result = lsisoroot(file);

	if (fclose(file))
	{
		perror("Error closing file");
		return EXIT_FAILURE;
	}

	return result;
}

/* read_sector reads SECTOR_SIZE bytes from file into buffer */
static char *read_sector(FILE *file, char *buffer)
{
	memset(buffer, 0, SECTOR_SIZE);
	if (fread(buffer, SECTOR_SIZE, 1, file) == 0)
	{
		perror("Error reading file");
		exit(EXIT_FAILURE);
	}
	return buffer;
}

/* printn prints the first len characters of string */
static void printn(const char *string, int len)
{
	int i;
	for (i = 0; i < len; i++)
		printf("%c", string[i]);
}

/* lsisoroot displays the files in the root directory of the given ISO image */
static int lsisoroot(FILE *file)
{
	char sector[SECTOR_SIZE];
	int32_t root_dir_loc;
	long root_dir_offset;
	int32_t root_dir_size;

	/* Skip past system area */
	if (fseek(file, 32768, SEEK_SET))
	{
		perror("fseek");
		return EXIT_FAILURE;
	}

	/* Verify that the first volume descriptor is a primary vol desc */
	read_sector(file, sector);
	if (sector[0] != 1)
	{
		fprintf(stderr, "Error: first volume descriptor is not "
		                "a primary volume descriptor\n");
		exit(EXIT_FAILURE);
	}

	/* Display the volume ID */
	printf("Volume ID is ");
	printn(sector+40, 32);
	printf("\n");

	/* Read the location from the root directory entry */
	root_dir_loc = *(int32_t *)(sector+158);

	/* Read the size of the root directory */
	root_dir_size = *(int32_t *)(sector+166);

	fprintf(stderr, "Root directory is at location %xh = byte offset %d\n",
	                root_dir_loc, root_dir_loc*SECTOR_SIZE);
	fprintf(stderr, "Root directory size is %xh = %u bytes\n",
	                root_dir_size, root_dir_size);

	/* Seek to the root directory */
	root_dir_offset = root_dir_loc * SECTOR_SIZE;
	if (fseek(file, root_dir_offset, SEEK_SET))
	{
		perror("fseek");
		return EXIT_FAILURE;
	}

	/* Read the root directory entries */
	ls(file, root_dir_size);

	return EXIT_SUCCESS;
}

/* ls reads and displays directory entries */
static void ls(FILE *file, int32_t root_dir_size) {	

	// Local variables. Note that the sector array can be complicated, due
	// the fact that it starts at 0, yet the entry at index 0 might be many
	// bytes into the ISO image. number_of_sectors is calculated for convenience,
	// it's simply the size of the root directory (in bytes) divided by the sector
	// size (also in bytes)
	char sector[SECTOR_SIZE];
	int32_t entry_length;
	int32_t entry_start = 0;			
	int32_t entry_location;
	int32_t letters_to_print;
	int sector_number = 1;
	int number_of_sectors = root_dir_size / SECTOR_SIZE;
	int i = 0;	

	// Read in the first sector. Note that, due to the read(), the file
	// is seeked to the start of the next sector by the time that the
	// operation is complete.  So, the next read() 
	// performed will read in the next sector automatically
	read_sector(file, sector);

	while (1 == 1) {

		// The length of the directory entry is the first byte of the entry
		entry_length = sector[entry_start];	

		// If the entry is of zero length and there are no more sectors to read,
		// then the function is done
		if (entry_length == 0 && (sector_number == number_of_sectors)) {
			break;
		}

		// If the entry is of zero length and there are more sectors to read,
		// then read in the next sector. 
		else if (entry_length == 0 && (sector_number < number_of_sectors)) {

			read_sector(file, sector);
			
			// The entry_start variable is set to zero to indicate that a 
			// new sector has been read in to the sector array and the 
			// function should start examining elements at the start of the
			// array. The sector_number is incremented to indicate that the 
                	// next sector has been read in                                                

			entry_start = 0;
			sector_number++;	
		}
	
		// Otherwise, proceed as usual - print the required information about the
		// directory entry.  
		else {
	
			// The entry_location, which is the offset to be printed out
			entry_location = *(int32_t *)(sector + entry_start + 2);		
 			
			// The first two entries in the root directory are references to itself. The first
			// is the current directory (.), which is root, and the second is the parent 
			// directory (..), which is also root! To avoid printing garbage, as the name
			// of the root directory is not printable, a special case is undertaken twice
			if (i < 2) {			
				printf("  Root directory is at byte offset %d\n", entry_location * SECTOR_SIZE);	
				i++;
			}	

			// Once past the first two entries, calculate how many leters are to be printed out for
			// the filename. The reason for this is because the file name is of variable length.
			// Since the file name is the last field in the entry, and the name starts 33 bytes 
			// into the entry, the length of the file name can be easily calculated. printn() is
			// called to print out the name. The offset of the entry is then printed			
			else {
				letters_to_print = entry_length - 33;
				printn(sector + entry_start + 33, letters_to_print);
				printf(" is at byte offset %d\n", entry_location * SECTOR_SIZE);
			}	

			// Once the current entry has been printed out, skip to the next entry, which is simply
			// the combination of the where the current entry started and where the current entry ended.
			// This works because the next entry is always directly after the current entry	
			entry_start = entry_start + entry_length;		
		}
	}
}
