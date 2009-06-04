#include "work_queue.h"

#include "copy_stream.h"
#include "memory_info.h"
#include "disk_info.h"
#include "fast_popen.h"
#include "link.h"
#include "debug.h"
#include "stringtools.h"
#include "load_average.h"
#include "domain_name_cache.h"
#include "getopt.h"
#include "full_io.h"

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

static void show_version(const char *cmd)
{
	printf("%s version %d.%d.%d built by %s@%s on %s at %s\n", cmd, CCTOOLS_VERSION_MAJOR, CCTOOLS_VERSION_MINOR, CCTOOLS_VERSION_MICRO, BUILD_USER, BUILD_HOST, __DATE__, __TIME__);
}

static void show_help(const char *cmd)
{
	printf("Use: %s <masterhost> <port>\n", cmd);
	printf("where options are:\n");
	printf(" -d <subsystem> Enable debugging for this subsystem\n");
	printf(" -t <time>      Abort after this amount of idle time. (default=1h)\n");
	printf(" -o <file>      Send debugging to this file.\n");
	printf(" -v             Show version string\n");
	printf(" -w <size>      Set TCP window size.\n");
	printf(" -h             Show this help screen\n");
}

int main( int argc, char *argv[] )
{
	const char *host;
	int port;
	struct link *master=0;
	char addr[LINK_ADDRESS_MAX];
	UINT64_T memory_avail, memory_total;
	UINT64_T disk_avail, disk_total;
	int ncpus;
	char c;
	int timeout=3600;
	int idle_abort_timeout=3600;
	time_t idle_abort_time;
	char hostname[DOMAIN_NAME_MAX];
	int w;

	ncpus = load_average_get_cpus();
	memory_info_get(&memory_avail,&memory_total);
	disk_info_get(".",&disk_avail,&disk_total);
	
	debug_config(argv[0]);

	while((c = getopt(argc, argv, "d:t:o:w:vi")) != (char) -1) {
		switch (c) {
		case 'd':
			debug_flags_set(optarg);
			break;
		case 't':
			idle_abort_timeout = string_time_parse(optarg);
			break;
		case 'o':
			debug_config_file(optarg);
			break;
		case 'v':
			show_version(argv[0]);
			return 0;
		case 'w':
			w = string_metric_parse(optarg);
			link_window_set(w,w);
			break;
		case 'h':
		default:
			show_help(argv[0]);
			return 1;
		}
	}

	if((argc-optind)!=2) {
		show_help(argv[0]);
		return 1;
	}

	host = argv[optind];
	port = atoi(argv[optind+1]);

	if(getenv("_CONDOR_SCRATCH_DIR")) {
		chdir(getenv("_CONDOR_SCRATCH_DIR"));
	} else {
		char tempdir[WORK_QUEUE_LINE_MAX];
		sprintf(tempdir,"/tmp/worker-%d-%d",getuid(),getpid());
		mkdir(tempdir,0700);
		chdir(tempdir);
	}

	if(!domain_name_cache_lookup(host,addr)) {
		printf("couldn't lookup address of host %s\n",host);
		return 1;
	}

	domain_name_cache_guess(hostname);

	idle_abort_time = time(0) + idle_abort_timeout;

	while(1) {
		char line[WORK_QUEUE_LINE_MAX];
		int result, length, mode, fd;
		char filename[WORK_QUEUE_LINE_MAX];
		char *buffer;
		FILE *stream;

		if(time(0)>idle_abort_time) break;

		if(!master) {
			master = link_connect(addr,port,time(0)+timeout);
			if(!master) {
				sleep(5);
				continue;
			}
		}

		link_tune(master,LINK_TUNE_INTERACTIVE);
		sprintf(line,"ready %s %d %llu %llu %llu %llu\n",hostname,ncpus,memory_avail,memory_total,disk_avail,disk_total);
		link_write(master,line,strlen(line),time(0)+timeout);

		if(link_readline(master,line,sizeof(line),time(0)+timeout)) {
			debug(D_DEBUG,"%s",line);
			if(sscanf(line,"work %d",&length)) {
				buffer = malloc(length+1);
				link_read(master,buffer,length,time(0)+timeout);
				buffer[length] = 0;
				debug(D_DEBUG,"%s",buffer);
				stream = fast_popen(buffer);
				free(buffer);
				if(stream) {
					length = copy_stream_to_buffer(stream,&buffer);
					if(length<0) length = 0;
					result = fast_pclose(stream);
				} else {
					length = 0;
					result = -1;
					buffer = 0;
				}
				sprintf(line,"result %d %d\n",result,length);
				debug(D_DEBUG,line);
				link_write(master,line,strlen(line),time(0)+timeout);
				link_write(master,buffer,length,time(0)+timeout);
				if(buffer) free(buffer);
			} else if(sscanf(line,"put %s %d %o",filename,&length,&mode)==3) {
				if(strchr(filename,'/')) goto recover;

				fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,mode);
				if(fd<0) goto recover;

				int actual = link_stream_to_fd(master,fd,length,time(0)+timeout);
				close(fd);
				if(actual!=length) goto recover;

			} else if(sscanf(line,"get %s",filename)==1) {
				fd = open(filename,O_RDONLY,0);
				if(fd>=0) {
					struct stat info;
					fstat(fd,&info);
					sprintf(line,"%d\n",(int)info.st_size);
					link_write(master,line,strlen(line),time(0)+timeout);
					int actual = link_stream_from_fd(master,fd,length,time(0)+timeout);
					close(fd);
					if(actual!=length) goto recover;
				} else {
					sprintf(line,"-1\n");
					link_write(master,line,strlen(line),time(0)+timeout);
				}					
			} else if(!strcmp(line,"exit")) {
				exit(0);
			} else {
				link_write(master,"error\n",6,time(0)+timeout);
			}

			idle_abort_time = time(0) + idle_abort_timeout;

		} else {
			recover:
			link_close(master);
			master = 0;
			sleep(5);
		}
	}

	return 0;
}
