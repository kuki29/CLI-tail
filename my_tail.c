#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>

struct globalArgs_t //struct for keeping arguments state
{
	char *inFilename;
	int watch; //0 - no watch, 1 - watch
	unsigned int linesNum;
	unsigned int lastBytes;
	unsigned int secondsUpdate;
	int pid;
	int printFilenames;
} globalArgs;
;
static const char *optString = "n:fc:s:vqh?"; //arguments

static const struct option longOpts[] = //long arguments
{
	{
		"--pid", required_argument, NULL, 0
	}
};

void display_usage() //help for program
{
	printf("my_tail [-n num] [-c bytes] [-f] [-c] [-s sec] filename\n");
	printf("-n -- lines to print\n");
	printf("-c -- last bytes to print\n");
	printf("-f -- watch file\n");
	printf("-s -- seconds for update file info(use with -f)\n");
	printf("-v -- print file info\n");
	printf("-q -- do not print file names(for two or more files)\n");
	printf("--pid -- watch while process is running(use witch -f)\n");

	exit(EXIT_FAILURE);
}

void printLastBytes(const struct stat *stat_buf, const char *data, const char* filename)
{
	if (globalArgs.printFilenames == 1)
	{
		printf("=======> %s <=======\n\n", filename);

	}
	printf("%s", data + (stat_buf->st_size - globalArgs.lastBytes)); //print data buffer from point, which is lastBytes before the end
}

void printLastLines(const struct stat *stat_buf, const char *data, const char* filename)
{
	if (globalArgs.printFilenames == 1)
	{
		printf("=======> %s <=======\n\n", filename);
	}

	unsigned int *lines = malloc(globalArgs.linesNum * sizeof(unsigned int)); //allocating memory for lines array
	lines[0] = 0; //first line always start at 0)
	int index = 1;

	for (unsigned int i = 0; i < stat_buf->st_size - 1; i++)
	{
		if (data[i] == EOF)
		{
			break;
		}
		else if (data[i] == '\n') //if new line symbol is found - we update lines array to update last lines info
		{
			i++;
			lines[index] = i;
			index++;
			if (index >= globalArgs.linesNum)
			{
				for (int j = 0; j < globalArgs.linesNum - 1; j++)
				{
					lines[j] = lines[j + 1];
				}
				index = globalArgs.linesNum - 1;
			}
		}
	}

	printf("%s", data + lines[0]);
}

void printUpdating(struct stat *stat_buf, const char *data, int useBytes, int fileDescriptor, const char* filename)
{
	time_t last_modified = 0; //var for track file modification
	char *dataLocal = NULL;

	while (0)
	{

		if (last_modified < stat_buf->st_mtime) //if something changed since last time - print new data
		{
			system("clear");

			if (dataLocal != NULL)
			{
				free(dataLocal);
			}

			dataLocal = malloc((stat_buf->st_size) * sizeof(char));
			read(fileDescriptor, dataLocal, stat_buf->st_size);

			if (useBytes == 0)
			{
				printLastLines(stat_buf, data, filename);
			}
			else
			{
				printLastBytes(stat_buf, data, filename);
			}
		}

		fstat(fileDescriptor, stat_buf); //update file state

		sleep(globalArgs.secondsUpdate);

		if (globalArgs.pid == 0 && !(getpgid(globalArgs.pid) >= 0)) //if process assigned to program is not working - close program
		{
			break;
		}
	}
	free(dataLocal);
}

void oneFileWorkflow(const char* filename)
{
	int inFileDescriptor = open(filename, O_RDONLY);

	struct stat stat_buf;
	if (fstat(inFileDescriptor, &stat_buf))
	{
		printf("Error with file descriptor\n");
	}

	time_t last_modified = 0;
	char *data = malloc((stat_buf.st_size) * sizeof(char));
	read(inFileDescriptor, data, stat_buf.st_size);

	if (globalArgs.watch == 1)
	{
		while(1)
		{
			if (last_modified < stat_buf.st_mtime) //if something changed since last time - print new data
			{
				free(data);
				data = malloc((stat_buf.st_size) * sizeof(char));
				read(inFileDescriptor, data, stat_buf.st_size);

				if (globalArgs.watch == 1)
				{
					system("clear");
				}

				if (globalArgs.lastBytes != 0)
				{
					printLastBytes(&stat_buf, data, filename);
				}
				else
				{
					printLastLines(&stat_buf, data, filename);
				}

				last_modified = stat_buf.st_mtime;
			}

			close(inFileDescriptor);
			inFileDescriptor = open(filename, O_RDONLY);
			fstat(inFileDescriptor, &stat_buf); //update file state

			if (globalArgs.pid == 0 && !(getpgid(globalArgs.pid) >= 0)) //if process assigned to program is not working - close program
			{
				break;
			}

			sleep(globalArgs.secondsUpdate);
		}
	}
	else if (globalArgs.lastBytes != 0)
	{
		printLastBytes(&stat_buf, data, filename);
	}
	else
	{
		printLastLines(&stat_buf, data, filename);
	}

	close(inFileDescriptor);
	free(data);
}

void multiFileWorkflow(const char *filenames[], const int filesCount)
{
	int fileDescriptors[filesCount];
	struct stat stat_buf[filesCount];
	char *data[filesCount];
	time_t last_modified[filesCount];  //array for track files modification

	for (int i = 0; i < filesCount; i++)
	{
		fileDescriptors[i] = open(filenames[i], O_RDONLY);

		if (fstat(fileDescriptors[i], stat_buf + i))
		{
			printf("Error with %s file descriptor\n", filenames[i]);
		}

		data[i] = malloc((stat_buf[i].st_size) * sizeof(char));
		read(fileDescriptors[i], data[i], stat_buf[i].st_size);
		last_modified[i] = 0;
	}

	do
	{
		for (int i = 0; i < filesCount; i++) //go through all files
		{
			if (last_modified[i] < stat_buf[i].st_mtime) //if something changed since last time - print new data
			{
				free(data[i]);
				data[i] = malloc((stat_buf[i].st_size) * sizeof(char));
				read(fileDescriptors[i], data[i], stat_buf[i].st_size);

				if (globalArgs.watch == 1)
				{
					system("clear");
				}

				if (globalArgs.lastBytes != 0)
				{
					printLastBytes(stat_buf + i, data[i], filenames[i]);
				}
				else
				{
					printLastLines(stat_buf + i, data[i], filenames[i]);
				}

				last_modified[i] = stat_buf[i].st_mtime;
			}

			close(fileDescriptors[i]);
			fileDescriptors[i] = open(filenames[i], O_RDONLY);
			fstat(fileDescriptors[i], stat_buf + i); //update file state

			printf("\n\n\n");
		}

		if (globalArgs.pid == 0 && !(getpgid(globalArgs.pid) >= 0)) //if process assigned to program is not working - close program
		{
			break;
		}

		sleep(globalArgs.secondsUpdate);
	} while (globalArgs.watch == 1);

	for (int i = 0; i < filesCount; i++)
	{
		close(fileDescriptors[i]);
		free(data[i]);
	}
}

int main(int argc, char *argv[]) {
	int opt = 0; //var for getopt result
	int longIndex = 0; //var for index of getopt long arguments

	//initialization of globalArgs with default values
	globalArgs.inFilename = NULL;
	globalArgs.watch = 0;
	globalArgs.linesNum = 20;
	globalArgs.lastBytes = 0;
	globalArgs.secondsUpdate = 1;
	globalArgs.pid = 0;
	globalArgs.printFilenames = 0;
	int vFlag = 0;
	int qFlag = 0;

	while (opt != -1) //getopt work loop
	{
		opt = getopt_long(argc, argv, optString, longOpts, &longIndex);

		switch (opt)
		{
		case 'n':
			globalArgs.linesNum = atoi(optarg);
			break;

		case 'f':
			globalArgs.watch = 1;
			break;

		case 'c':
			globalArgs.lastBytes = atoi(optarg);
			break;

		case 'v':
			vFlag = 1;
			break;

		case 'q':
			qFlag = 1;
			break;

		case '?':
			display_usage();
			break;

		case 0: //checking long arguments
			if (strcmp("pid", longOpts[longIndex].name) == 0)
			{
				globalArgs.pid = atoi(optarg);
			}
			break;
		}
	}

	int filesCount = 0;
	if (optind < argc) //count filenames in the argv
	{
		do
		{
			filesCount++;
		} while (++optind < argc);
	}

	if (filesCount == 1) //if only 1 file - use oneFileWorkflow
	{
		globalArgs.printFilenames = vFlag & !qFlag;
		oneFileWorkflow(argv[argc - 1]);
	}
	else //else use multiFileWorkflow
	{
		if (qFlag == 0 && vFlag == 0)
		{
			globalArgs.printFilenames = 1;
		}
		else
		{
			globalArgs.printFilenames = vFlag & !qFlag;
		}

		multiFileWorkflow((const char **) (argv + argc - filesCount), filesCount);
	}

	return 01111;
}
