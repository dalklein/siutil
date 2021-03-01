
#include "libyasdi.h"
#include "libyasdimaster.h"
#include <ctype.h>
#include <sys/types.h>

//WChanType = 0x040f and bChanIndex = 0 denotes all parameter channels
//WChanType = 0x090f and bChanIndex = 0 denotes all spot value channels
#define CHANTYPE_PARM 0x040f
#define CHANTYPE_SPOT 0x090f
#define CHANTYPE_ALL 0xffff

#define MAX_CHANHANDLES	2048

#ifdef DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif

extern BOOL TSecurity_SetNewAccessLevel(char * user, char * passwd);

int dolist(int handle, int type, char *chanName) {
	DWORD ChanHandles[MAX_CHANHANDLES];
	char valtext[32],name[32];
	int r,i,count;

	dprintf("handle: %d, type: %x, chanName: %s\n", handle, type, chanName);

	/* If name specified, list possible options for chan */
	if (chanName) {
		int chan;
		char valtext[32];

		chan = FindChannelName(handle, chanName);
		dprintf("chan: %d\n", chan);
		if (chan < 1) {
			printf("error: channel name not found\n");
			return 1;
		}
		count = GetChannelStatTextCnt(chan);
		dprintf("count: %d\n", count);
		if (!count) {
			printf("channel has no option names\n");
			return 0;
		}
		for(i=0; i < count; i++) {
			r = GetChannelStatText(chan, i, valtext, sizeof(valtext)-1);
			dprintf("r: %d\n", r);
			if (r < 0) {
				printf("error: unable to get channel option\n");
				return 1;
			}
			dprintf("i: %d, text: %s\n", i, valtext);
			printf("%s\n",valtext);
		}
	} else {
		memset(valtext,0,sizeof(valtext));
		r = GetChannelHandles(handle, ChanHandles, MAX_CHANHANDLES, type, 0);
		printf("r: %d\n", r);
		if (r < 0) return 1;
		count = r;
		for(i=0; i < count; i++) {
			r = GetChannelName(ChanHandles[i], name, sizeof(name)-1);
			if (r < 0) {
				printf("list: error getting channel name for channel %d!\n",i);
				return 1;
			}
			printf("%d: %s\n", i, name);
		}
	}
	return 0;
}

int getparm(int handle, char *name) {
	int chan,r;
	char valtext[32];
	double cval;

	dprintf("name: %s\n", name);

	chan = FindChannelName(handle, name);
	dprintf("chan: %d\n", chan);
	if (chan < 1) {
		printf("error: channel name not found\n");
		return 1;
	}

	memset(valtext,0,sizeof(valtext));
	r = GetChannelValue(chan,handle,&cval,valtext,sizeof(valtext)-1,99);
	dprintf("r: %d\n", r);
	if (r < 0) {
		printf("error: unable to get current value\n");
		return 1;
	}
	dprintf("cval: %f, valtext: %s\n", cval, valtext);

	dprintf("%d: %s (%f)\n", chan, valtext, cval);
	if (strlen(valtext))
		printf("%s\n",valtext);
	else
		printf("%f\n",cval);
	return 0;
}

int setparm(int handle, char *name, char *sval) {
	int i,chan,count,r,found;
	char valtext[32];
	double cval,dval;

	dprintf("name: %s, sval: %s\n", name, sval);

	chan = FindChannelName(handle, name);
	dprintf("chan: %d\n", chan);
	if (chan < 0) {
		printf("error: param not found\n");
		return 1;
	}

	memset(valtext,0,sizeof(valtext));
	r = GetChannelValue(chan,handle,&cval,valtext,sizeof(valtext)-1,99);
	dprintf("r: %d\n", r);
	if (r < 0) {
		printf("error: unable to get current value\n");
		return 1;
	}
	dprintf("cval: %f, valtext: %s\n", cval, valtext);

	count = GetChannelStatTextCnt(chan);
	dprintf("count: %d\n", count);
	if (count) {
		dprintf("current value: %s\n", valtext);
		if (strcmp(sval,valtext)==0) return 0;
		found = 0;
		for(i=0; i < count; i++) {
			r = GetChannelStatText(chan, i, valtext, sizeof(valtext)-1);
			dprintf("r: %d\n", r);
			if (r < 0) {
				printf("error: unable to get channel option\n");
				return 1;
			}
			dprintf("i: %d, text: %s\n", i, valtext);
			if (strcmp(valtext,sval)==0) {
				dval = i;
				found = 1;
				break;
			}
		}
		dprintf("found: %d\n", found);
		if (!found) {
			printf("error: option not found\n");
			return 1;
		}
	} else {
		dval = atof(sval);
		dprintf("cval: %f, dval: %f\n", cval, dval);
		if (dval == cval) return 0;
	}
	r = SetChannelValue(chan,handle,dval);
	dprintf("r: %d\n", r);
	if (r < 0) {
		printf("error: unable to set value: %f\n",dval);
		return 1;
	}
	return 0;
}

static int getmyhomedir(char *dest, int destlen) {
	FILE *fp;
	char line[256],*p,*s;
	long uid;
	int len,id,i;
//	struct passwd *pw;

	uid = getuid();
	fp = fopen("/etc/passwd","r");
	if (!fp) {
		perror("fopen /etc/passwd");
		return 1;
	}
	while(fgets(line,sizeof(line)-1,fp)) {
		len = strlen(line);
		if (!len) continue;
		while(len > 1 && isspace(line[len-1])) len--;
		dprintf("line[%d]: %d\n", len, line[len]);
		line[len] = 0;
		dprintf("line: %s\n", line);
		p = line;
		for(i=0; i < 2; i++) {
			p = strchr(p+1,':');
			if (!p) goto getmyhomedir_error;
			dprintf("p(%d): %s\n", i, p);
		}
		s = p+1;
		p = strchr(s,':');
		if (p) *p = 0;
		dprintf("s: %s\n", s);
		id = atoi(s);
		if (id == uid) {
			for(i=0; i < 2; i++) {
				p = strchr(p+1,':');
				if (!p) goto getmyhomedir_error;
				dprintf("p(%d): %s\n", i, p);
			}
			s = p+1;
			p = strchr(s,':');
			if (p) *p = 0;
			dprintf("s: %s\n", s);
			dest[0] = 0;
			strncat(dest,s,destlen);
			len = strlen(dest);
			if (dest[len] != '/') strcat(dest,"/");
			fclose(fp);
			return 0;
		}
	}
getmyhomedir_error:
	fclose(fp);
	return 1;
}

char *find_config_file(char *name) {
	static char temp[1024];

	if (access(name,R_OK)==0) return name;
	if (!getmyhomedir(temp,sizeof(temp)-1)) {
		strcat(temp,"etc/");
		strcat(temp,name);
		dprintf("temp: %s\n", temp);
		if (access(temp,R_OK)==0) return temp;
	}

	sprintf(temp,"/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	sprintf(temp,"/usr/local/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	sprintf(temp,"/opt/mybmm/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	return 0;
}

void usage(char *myname) {
	printf("usage: %s [-alps] -n chan | ChanName [value]\n",myname);
	return;
}

int main(int argc, char **argv) {
	int opt,all,list,param,action,type;
	int i,driverCount,found,r,done;
	char *progName,*configFile,name[32],device_type[32];
	DWORD drivers[5],devCount,count,deviceHandles[1];

	progName = argv[0];

	all = list = param = 0;
	while ((opt=getopt(argc, argv, "alph")) != -1) {
		switch (opt) {
		case 'a':
			all = 1;
			break;
		case 'l':
			list = 1;
			break;
		case 'p':
			param = 1;
			break;
		default:
		case 'h':
			usage(progName);
			exit(0);
			break;
		}
	}
	dprintf("all: %d, list: %d, param: %d\n", all, list, param);

	if (all && !list) printf("info: all flag (-a) only applies to list (-l), ignored.\n");

	argc -= optind;
	argv += optind;

	for(i=0; i < argc; i++) dprintf("arg[%d]: %s\n",i,argv[i]);

	dprintf("argc: %d\n", argc);
	if (argc < 1 && list == 0) {
		usage(progName);
		return 1;
	}
	if (list) {
		action = 0;
		if (all) {
			type = CHANTYPE_ALL;
		} else if (param) {
			type = CHANTYPE_PARM;
		} else {
			type = CHANTYPE_SPOT;
		}
	} else if (argc < 2) {
		action = 1;
	} else {
		action = 2;
	}

	configFile = find_config_file("yasdi.ini");
	if (!configFile) {
		printf("error: unable to find yasdi.ini\n");
		return 1;
	}
	dprintf("configFile: %s\n", configFile);
	yasdiMasterInitialize(configFile,&count);

	driverCount = yasdiMasterGetDriver(drivers, 5);
	dprintf("driverCount: %d\n",driverCount);
	if (driverCount < 0 || driverCount > 5) {
		printf("error initializing drivers\n");
		return 1;
	}

	found=0;
	for(i=0; i < count; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		if (yasdiSetDriverOnline(drivers[i])) {
			dprintf("Driver: %s: Online\n", name);
			found=1;
		} else {
			printf("Driver: %s: FAILED\n",name);
		}
	}
	if (!found) {
		printf("error: no drivers online\n");
		return 1;
	}

	done = 0;
	while(!done) {
		r = DoStartDeviceDetection(1, TRUE);
		dprintf("r: %d\n", r);
		switch(r) {
		case YE_OK:
			done = 1;
			break;
		case YE_NOT_ALL_DEVS_FOUND:
			sleep(1);
			break;
		case YE_DEV_DETECT_IN_PROGRESS:
			printf("error: another detection is in progress\n");
			goto error;
			break;
		default:
			printf("error: %d\n", r);
			goto error;
			break;
		}
	}

	TSecurity_SetNewAccessLevel(0,0);

	devCount = GetDeviceHandles(deviceHandles, 1);
	dprintf("deviceHandles[0]: %d\n", deviceHandles[0]);
	for(i=0; i < devCount; i++) {
		GetDeviceType(deviceHandles[i], device_type, sizeof(device_type)-1);
		dprintf("%d: device_type: %s\n", i, device_type);
	}

	dprintf("action: %d\n", action);
	switch(action) {
	case 0:
		r = dolist(deviceHandles[0], type, (argc ? argv[0] : 0));
		break;
	case 1:
		r = getparm(deviceHandles[0], argv[0]);
		break;
	case 2:
		r = setparm(deviceHandles[0], argv[0], argv[1]);
		break;
	}

error:
	for(i=0; i < count; i++) {
		yasdiGetDriverName(drivers[i], name, sizeof(name));
		yasdiSetDriverOffline(drivers[i]);
		dprintf("Driver: %s: Offline\n", name);
	}
	yasdiMasterShutdown();
	return r;
}
