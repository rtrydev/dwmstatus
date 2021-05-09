/*
 * Copy me if you can.
 * by 20h
 */

#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tzargentina = "America/Buenos_Aires";
char *tzutc = "UTC";
char *tzberlin = "Europe/Berlin";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL)
		return smprintf("");

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		return smprintf("");
	}

	return smprintf("%s", buf);
}

char* checklang(char* input){
        char pl[12] = "xkb:pl::pol";
        char jp[8] = "mozc-jp";
        for(int i=0; i<sizeof(pl)-1; i++){
                if(input[i] != pl[i]) break;
                if(i == sizeof(pl)-2) return "PL";
        }
        for(int i=0; i<sizeof(jp)-1; i++){
                if(input[i] != jp[i]) break;
                if(i == sizeof(jp)-2) return "あ";
        }
        return "?";
}

char* getinput(){
        FILE *fp;
        fp = popen("ibus engine", "r");
        char buffor[20];
        char* read = malloc(sizeof(char) * sizeof(buffor));
        while(fgets(buffor, sizeof(buffor), fp)!=NULL){
                sprintf(read, "%s", buffor);
        }
        pclose(fp);
        char* lang = smprintf("%s", checklang(read));
        free(read);
        return lang;
}


char *
getcpuutil(void) {
    long double a[4], b[4], loadavg;
    FILE *fp;
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &a[0], &a[1], &a[2], &a[3]);
    fclose(fp);
    sleep(1);
    fp = fopen("/proc/stat", "r");
    fscanf(fp, "%*s %Lf %Lf %Lf %Lf", &b[0], &b[1], &b[2], &b[3]);
    fclose(fp);
    loadavg = ((b[0] + b[1] + b[2]) - (a[0] + a[1] + a[2])) / ((b[0] + b[1] + b[2] + b[3]) - (a[0] + a[1] + a[2] + a[3])) * 100;
    return smprintf("%.0Lf", loadavg);
}

char *
getcputemp(void) {
    int temperature;
    FILE *fp;

    fp = fopen("/sys/class/hwmon/hwmon0/temp1_input", "r");
    fscanf(fp, "%d", &temperature);
    fclose(fp);

    return smprintf("%d", temperature / 1000);
}

int
getVolume()
{
	FILE *fp;
	fp = popen("pulsemixer --get-volume | sed 's/ .*//'", "r");
	char buffor[5];
	char *read = malloc(sizeof(char) * sizeof(buffor));
	while(fgets(buffor, sizeof(buffor), fp) != NULL){
		sprintf(read,"%s", buffor);
	}
	pclose(fp);
	int result = atoi(read);
	free(read);

	return result;
}

int
getMicrophoneVolume()
{
        FILE *fp;
        fp = popen("pulsemixer --id source-1 --get-volume | sed 's/ .*//'", "r");
        char buffor[5];
        char *read = malloc(sizeof(char) * sizeof(buffor));
        while(fgets(buffor, sizeof(buffor), fp) != NULL){
                sprintf(read,"%s", buffor);
        }
        pclose(fp);
        int result = atoi(read);
        free(read);

        return result;
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

int
main(void)
{
	char *status;
	char *tmbln;
	char *cpuutil;
	char *cputemp;
	char *input;
	int volume;
	int volumeMic;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(1)) {
		cputemp = getcputemp();
		cpuutil = getcpuutil();
		volume = getVolume();
		volumeMic = getMicrophoneVolume();
		input = getinput();
		tmbln = mktimes("%d %b %Y %H:%M", tzberlin);
		status = smprintf("\ufb19  %s%%  \uf2c8 %s°C  \uf812  %s  \uf028  %d%%  \uf130 %d%%  \uf5f5  %s "
				,cpuutil,cputemp,input,volume,volumeMic,tmbln);
		setstatus(status);

		free(cputemp);
		free(input);
		free(cpuutil);
		free(tmbln);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

