/* TomeNET updater (cross-platform) for client and audio packs. - C. Blue */

#include <stdio.h>
#include <string.h>

#include <stdlib.h> /* system() call */

#ifdef WINDOWS
 #include <windows.h>
 #include <winreg.h>	/* remote control of installed 7-zip via registry approach */
 #if 1
  #include <process.h>	/* use _spawnl() instead of normal system() (WINE bug/Win inconsistency even maybe) */
 #else
  #include <wchar.h>	/* use _wspawnl() instead of normal system() (WINE bug/Win inconsistency even maybe) */
 #endif
 #define MAX_KEY_LENGTH 255
 #define MAX_VALUE_NAME 16383

// #include <gtk-2.0/gtk.h>
 #include <gtk/gtk.h>

//redundant?
// #include <glib/gerror.h>
// #include <gdk-pixbuf/gdk-pixbuf.h>

 char path_7z[1024], path_7z_quoted[1024];

 #include <unistd.h> /* for chdir() */
#else
 #include <gtk/gtk.h>
 #include <sys/stat.h> /* for mkdir() */
#endif


//#define WINDOWS


#ifndef WINDOWS
static short int posix_terminal = -1;//5

unsigned char is_linux = 1;
#ifdef __x86_64__
unsigned char is_64bit = 1;
#else
unsigned char is_64bit = 0;
#endif
#endif

GtkWidget *top_window;

/* use URL2FILE instead of wget? */
//#define USE_URL2FILE


GdkPixbuf *create_pixbuf(const gchar * filename) {
	GdkPixbuf *pixbuf;
	GError *error = NULL;

	pixbuf = gdk_pixbuf_new_from_file(filename, &error);
	if (!pixbuf) {
		fprintf(stderr, "%s\n", error->message);
		g_error_free(error);
	}
	return pixbuf;
}

void show_error(gpointer window) {
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(GTK_WINDOW(window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_ERROR,
	    GTK_BUTTONS_OK,
#ifdef WINDOWS
	    "You must first install 7-zip from www.7-zip.org !");
#else
	    "You must install 7-zip-GUI (7zG) first! Package name is 'p7zip'.");
#endif
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void show_error_broken(gpointer window) {
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new(GTK_WINDOW(window),
	    GTK_DIALOG_DESTROY_WITH_PARENT,
	    GTK_MESSAGE_ERROR,
	    GTK_BUTTONS_OK,
	    "The download link seems to be invalid or deleted!\n" \
	    "Please notify us about this error message and make\n" \
	    "sure it was not just an internet connection problem.");
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

//void install_client(GtkWidget *widget, gpointer label) {
void install_client(GtkButton *button, gpointer label) {
	gtk_button_set_label(button, "..please wait..");
	while(gtk_events_pending())
	    gtk_main_iteration();

#ifdef WINDOWS
	int res;

	remove("TomeNET-latest-install.exe");
 #if 0 /* winbash is dysfunctional, no alternatives available ----------------------------------------- */
  #if 0 /* problem: 7z doesn't support stripping a path level */ 
	/* Download */
	_spawnl(_P_WAIT, "updater/dl_win32/winbash/wget.exe", "wget.exe", "http://www.tomenet.eu/downloads/TomeNET-latest-client.zip", NULL); /* supposed to work on WINE, yet crashes if not exit(0)ing next oO */
	/* Extract */
	_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", "TomeNET-latest-client.zip", NULL);
  #else /* workaround: we just use the installer instead */
	/* Download */
	_spawnl(_P_WAIT, "updater/dl_win32/winbash/wget.exe", "wget.exe", "http://www.tomenet.eu/downloads/TomeNET-latest-install.exe", NULL); /* supposed to work on WINE, yet crashes if not exit(0)ing next oO */
	/* Extract */
	_spawnl(_P_WAIT, "TomeNET-latest-install.exe", "TomeNET-latest-install.exe", NULL);
  #endif
 #else /* just use wget and silly html processing for now, ew ----------------------------------------- */
	/* Download */
  #ifndef USE_URL2FILE
	res = _spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "--dot-style=mega", "http://www.tomenet.eu/downloads/TomeNET-latest-install.exe", NULL);
	if (res != 0) show_error_broken(top_window);
	else
  #else
	res = _spawnl(_P_WAIT, "updater\\URL2FILE.EXE", "URL2FILE.EXE", "http://www.tomenet.eu/downloads/TomeNET-latest-install.exe", "TomeNET-latest-install.exe", NULL);
	if (res != 0) show_error_broken(top_window);
	else
  #endif
	/* Extract */
	_spawnl(_P_WAIT, "TomeNET-latest-install.exe", "TomeNET-latest-install.exe", NULL);
 #endif
#else
	char cwd_path[1024], out_val[1024];
	void *cwd;
	const char *clname;
	FILE *fp;


	if (is_linux) {
		/* Linux */
		if (is_64bit)
			clname = "tomenet-latest-linux-amd64.tar.bz2";
		else
			clname = "tomenet-latest-linux.tar.bz2";
	} else {
		/* OSX */
		if (is_64bit)
			clname = "TomeNET-latest-client-OSX-amd64.tar.bz2";
		else
			clname = "TomeNET-latest-client-OSX-x86.tar.bz2";
	}

	/* open a new terminal window to process the download in it, for showing the user what's going on */
	/* get own app path and prepare path'ed batch file */
	cwd = getcwd(out_val, sizeof(out_val));
	if (!cwd) return;//paranoia?
	strcpy(cwd_path, out_val);
	strcat(cwd_path, "/updater/_update_cl.sh");
	fp = fopen(cwd_path, "w");
	if (!fp) return;//paranoia?
	fprintf(fp, "cd %s\n", out_val);
	fprintf(fp, "rm %s\n", clname);
	fprintf(fp, "rm updater/result.tmp\n");
	fprintf(fp, "wget http://www.tomenet.eu/downloads/%s\n", clname);
	fprintf(fp, "RETVAL=$?\n");
	fprintf(fp, "echo $RETVAL > updater/result.tmp\n");
	fprintf(fp, "tar xvjf %s --strip-components 1\n", clname);
	fprintf(fp, "echo\n");
	fprintf(fp, "read -p \"Press ENTER to finish.\"\n");
	fclose(fp);
	sprintf(out_val, "chmod +x %s", cwd_path);
	system(out_val);
	/* execute the batch file */
	switch (posix_terminal) {
	case 0: sprintf(out_val, "xfce4-terminal --disable-server -x %s", cwd_path); break;
	case 1: sprintf(out_val, "konsole -e /bin/sh %s", cwd_path); break;
	case 2: sprintf(out_val, "gnome-terminal -e %s", cwd_path); break;
	case 3: sprintf(out_val, "xterm -e %s", cwd_path); break;
	case 4: sprintf(out_val, "bash -c %s", cwd_path); break;
	case 5: sprintf(out_val, "sh -c %s", cwd_path); break;
	}
	/* download + extract */
	system(out_val);

	fp = fopen("updater/result.tmp", "r");
	if (fp) {//~paranoia?
		long res;

		fgets(out_val, 20, fp);
		res = atoi(out_val);
		if (res != 0) show_error_broken(top_window);
		fclose(fp);
	}
#endif

	gtk_button_set_label(button, "Install/update game client");
	while(gtk_events_pending())
	    gtk_main_iteration();
}

void install_sound(GtkButton *button, gpointer label) {
	gtk_button_set_label(button, "..please wait..");
	while(gtk_events_pending())
	    gtk_main_iteration();

	//path_build(path, 1024, ANGBAND_DIR_XTRA, "sound");
#ifdef WINDOWS
	char path[1024], out_val[1024];
	FILE *fp;

	remove("TomeNET-soundpack.7z");

	/* download */
 #if 0 /* winbash is dysfunctional, no alternatives available ----------------------------------------- */
  #if 0
	//_spawnl(_P_WAIT, "wget.exe", "wget.exe", "http://download814.mediafire.com/wzihil4gthxg/issv5sdv7kv3odq/TomeNET-soundpack.7z", NULL); /* supposed to work on WINE, yet crashes if not exit(0)ing next oO */
	//_spawnl(_P_WAIT, "cmd.exe", "cmd.exe", "/c", "_dl_win32.bat", NULL);
	_spawnl(_P_WAIT, "updater/_dl_sp_win32.bat", "updater/_dl_sp_win32.bat", NULL);
  #else /* do it without helper bat files */
	chdir("updater/dl_win32/winbash");
	//_spawnl(_P_WAIT, "updater/dl_win32/winbash/bash.exe", "updater/dl_win32/winbash/bash.exe", "-c", "\"./download.sh http://www.mediafire.com/?issv5sdv7kv3odq\"", NULL);
	//_spawnl(_P_WAIT, "cmd.exe", "cmd.exe", "/c", "start", "bash.exe", "bash.exe", "-c", "\"./download.sh http://www.mediafire.com/?issv5sdv7kv3odq\"", NULL);
	_spawnl(_P_WAIT, "bash.exe", "bash.exe", "-c", "\"./download.sh http://www.mediafire.com/?issv5sdv7kv3odq\"", NULL);
   #if 0 /* no need to use the shell for this.. */
	//_spawnl(_P_WAIT, "bash.exe", "bash.exe", "-c", "\"./mv *.7z ../../..\"", NULL);
   #else /* ..better use the ansi c function: */
	rename("TomeNET-soundpack.7z", "../../../TomeNET-soundpack.7z");
   #endif
	chdir("../../..");
  #endif

printf("mhhhHDHHAHFH---\n");
exit(0);
 #else /* just use wget and silly html processing for now, ew ----------------------------------------- */
  #ifndef USE_URL2FILE
	//_spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "-O", "temp.html", "--dot-style=mega", "http://www.mediafire.com/download/issv5sdv7kv3odq/TomeNET-soundpack.7z", NULL);
	_spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "-O", "temp.html", "--dot-style=mega", "http://www.mediafire.com/download/issv5sdv7kv3odq/TomeNET-soundpack.7z", NULL);
  #else
	_spawnl(_P_WAIT, "updater\\URL2FILE.EXE", "URL2FILE.EXE", "http://www.mediafire.com/download/issv5sdv7kv3odq/TomeNET-soundpack.7z", "temp.html", NULL);
  #endif
	fp = fopen("temp.html", "r");
	out_val[0] = out_val[1] = 0;
	path[0] = 0;
	while (!feof(fp)) {
		fread(out_val + 5, sizeof(char), 1000, fp);
		out_val[1005] = 0;
		if (strstr(out_val, "kNO = ")) {
			/* extract URL */
			strncpy(path, strstr(out_val, "kNO = ") + 7, 1000);
			*(strstr(path, "\"")) = 0;
			break;
		}
		/* wrap around (n-1) chars regarding our search string */
		out_val[0] = out_val[1000];//k
		out_val[1] = out_val[1001];//N
		out_val[2] = out_val[1002];//O
		out_val[3] = out_val[1003];// 
		out_val[4] = out_val[1004];// =
	}
	fclose(fp);
	/* no valid dl link? We probably got a captcha request -_- */
	if (!path[0]) {
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(GTK_WINDOW(top_window),
		    GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_ERROR,
		    GTK_BUTTONS_CLOSE,
		    "Error - the download server received too many requests\n" \
		    "and responded with a captcha request which this\n" \
		    "downloader currently does not support, sorry!\n \nPlease just try again later.");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
 
		gtk_button_set_label(button, "Install/update sound pack");
		while(gtk_events_pending())
		    gtk_main_iteration();
		return;
	}
	remove("temp.html");
  #ifndef USE_URL2FILE
	_spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "--dot-style=mega", path, NULL);
  #else
	_spawnl(_P_WAIT, "updater\\URL2FILE.EXE", "URL2FILE.EXE", path, "TomeNET-soundpack.7z", NULL);
  #endif
 #endif
	/* extract */
	_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", "TomeNET-soundpack.7z", NULL);
	strcpy(path, ".\\lib\\xtra\\sound");
	sprintf(out_val, "xcopy /I /E /Q /Y /H sound %s", path);
	system(out_val);
	system("rmdir /S /Q sound");
#else /* assume posix */
	char cwd_path[1024], out_val[1024];
	void *cwd;
	FILE *fp;

	/* download */
	/* get own app path and prepare path'ed batch file */
	cwd = getcwd(out_val, sizeof(out_val));
	if (!cwd) return;//paranoia?
	strcpy(cwd_path, out_val);
	strcat(cwd_path, "/updater/_update_sp.sh");
	fp = fopen(cwd_path, "w");
	if (!fp) return;//paranoia?
	/* download */
	fprintf(fp, "cd %s/updater\n", out_val);
	fprintf(fp, "rm result.tmp\n");
	fprintf(fp, "./_dl_sp_posix.sh\n");
	/* move to correct location and extract */
	fprintf(fp, "mv TomeNET-soundpack.7z ..\n");
	fprintf(fp, "mkdir -p %s/lib/xtra\n", out_val); /* in case someone deleted his whole lib folder */
	fprintf(fp, "cd %s/lib/xtra\n", out_val);
	fprintf(fp, "7zG x -y ../../TomeNET-soundpack.7z\n");
	/* done, wait for confirmation */
	fprintf(fp, "echo\n");
	fprintf(fp, "read -p \"Press ENTER to finish.\"\n");
	fclose(fp);
	/* make the batch file executable */
	sprintf(out_val, "chmod +x %s", cwd_path);
	system(out_val);
	/* execute the batch file */
	switch (posix_terminal) {
	case 0: sprintf(out_val, "xfce4-terminal --disable-server -x %s", cwd_path); break;
	case 1: sprintf(out_val, "konsole -e %s", cwd_path); break;
	case 2: sprintf(out_val, "gnome-terminal -e %s", cwd_path); break;
	case 3: sprintf(out_val, "xterm -e %s", cwd_path); break;
	case 4: sprintf(out_val, "bash -c %s", cwd_path); break;
	case 5: sprintf(out_val, "sh -c %s", cwd_path); break;
	}
	/* download + extract */
	system(out_val);

	fp = fopen("updater/result.tmp", "r");
	if (fp) {//~paranoia?
		long res;

		fgets(out_val, 20, fp);
		res = atoi(out_val);
		if (res != 0) show_error_broken(top_window);
		fclose(fp);
	}
#endif

	gtk_button_set_label(button, "Install/update sound pack");
	while(gtk_events_pending())
	    gtk_main_iteration();
}

void install_music(GtkButton *button, gpointer label) {
	gtk_button_set_label(button, "..please wait..");
	while(gtk_events_pending())
	    gtk_main_iteration();

	//path_build(path, 1024, ANGBAND_DIR_XTRA, "music");
#ifdef WINDOWS
	char path[1024], out_val[1024];
	FILE *fp;

	remove("TomeNET-musicpack.7z");
	/* download */
 #if 0 /* winbash is dysfunctional, no alternatives available ----------------------------------------- */
  #if 0
	//_spawnl(_P_WAIT, "wget.exe", "wget.exe", "http://download1140.mediafire.com/352dj7foeneg/3j87kp3fgzpqrqn/TomeNET-musicpack.7z", NULL); /* supposed to work on WINE, yet crashes if not exit(0)ing next oO */
	//_spawnl(_P_WAIT, "cmd.exe", "cmd.exe", "/c", "_dl_win32.bat", NULL);
	_spawnl(_P_WAIT, "updater/_dl_mp_win32.bat", "updater/_dl_mp_win32.bat", NULL);
  #else /* do it without helper bat files */
	chdir("updater/dl_win32/winbash");
	//_spawnl(_P_WAIT, "updater/dl_win32/winbash/bash.exe", "updater/dl_win32/winbash/bash.exe", "-c", "\"./download.sh http://www.mediafire.com/?3j87kp3fgzpqrqn\"", NULL);
	_spawnl(_P_WAIT, "bash.exe", "bash.exe", "-c", "\"./download.sh http://www.mediafire.com/?3j87kp3fgzpqrqn\"", NULL);
	_spawnl(_P_WAIT, "bash.exe", "bash.exe", "-c", "\"./mv *.7z ../../..\"", NULL);
	chdir("../../..");
  #endif
 #else /* just use wget and silly html processing for now, ew ----------------------------------------- */
  #ifndef USE_URL2FILE
	_spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "-O", "temp.html", "--dot-style=mega", "http://www.mediafire.com/download/3j87kp3fgzpqrqn/TomeNET-musicpack.7z", NULL);
  #else
	_spawnl(_P_WAIT, "updater\\URL2FILE.EXE", "URL2FILE.EXE", "http://www.mediafire.com/download/3j87kp3fgzpqrqn/TomeNET-musicpack.7z", "temp.html", NULL);
  #endif
 	fp = fopen("temp.html", "r");
	out_val[0] = out_val[1] = 0;
	path[0] = 0;
	while (!feof(fp)) {
		fread(out_val + 5, sizeof(char), 1000, fp);
		out_val[1005] = 0;
		if (strstr(out_val, "kNO = ")) {
			/* extract URL */
			strncpy(path, strstr(out_val, "kNO = ") + 7, 1000);
			*(strstr(path, "\"")) = 0;
			break;
		}
		/* wrap around (n-1) chars regarding our search string */
		out_val[0] = out_val[1000];//k
		out_val[1] = out_val[1001];//N
		out_val[2] = out_val[1002];//O
		out_val[3] = out_val[1003];// 
		out_val[4] = out_val[1004];// =
	}
	fclose(fp);
	/* no valid dl link? We probably got a captcha request -_- */
	if (!path[0]) {
		GtkWidget *dialog;
		dialog = gtk_message_dialog_new(GTK_WINDOW(top_window),
		    GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_ERROR,
		    GTK_BUTTONS_CLOSE,
		    "Error - the download server received too many requests\n" \
		    "and responded with a captcha request which this\n" \
		    "downloader currently does not support, sorry!\n \nPlease just try again later.");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
 
		gtk_button_set_label(button, "Install/update music pack");
		while(gtk_events_pending())
		    gtk_main_iteration();
		return;
	}
	remove("temp.html");
  #ifndef USE_URL2FILE
	_spawnl(_P_WAIT, "updater\\wget.exe", "wget.exe", "--dot-style=mega", path, NULL);
  #else
	_spawnl(_P_WAIT, "updater\\URL2FILE.EXE", "URL2FILE.EXE", path, "TomeNET-musicpack.7z", NULL);
  #endif
 #endif
	/* extract */
	_spawnl(_P_WAIT, path_7z, path_7z_quoted, "x", "TomeNET-musicpack.7z", NULL);
	strcpy(path, ".\\lib\\xtra\\music");
	sprintf(out_val, "xcopy /I /E /Q /Y /H music %s", path);
	system(out_val);
	system("rmdir /S /Q music");
#else /* assume posix */
	char cwd_path[1024], out_val[1024];
	void *cwd;
	FILE *fp;

	/* download */
	/* get own app path and prepare path'ed batch file */
	cwd = getcwd(out_val, sizeof(out_val));
	if (!cwd) return;//paranoia?
	strcpy(cwd_path, out_val);
	strcat(cwd_path, "/updater/_update_mp.sh");
	fp = fopen(cwd_path, "w");
	if (!fp) return;//paranoia?
	/* download */
	fprintf(fp, "cd %s/updater\n", out_val);
	fprintf(fp, "rm result.tmp\n");
	fprintf(fp, "./_dl_mp_posix.sh\n");
	/* move to correct location and extract */
	fprintf(fp, "mv TomeNET-musicpack.7z ..\n");
	fprintf(fp, "mkdir -p %s/lib/xtra\n", out_val); /* in case someone deleted his whole lib folder */
	fprintf(fp, "cd %s/lib/xtra\n", out_val);
	fprintf(fp, "7zG x -y ../../TomeNET-musicpack.7z\n");
	/* done, wait for confirmation */
	fprintf(fp, "echo\n");
	fprintf(fp, "read -p \"Press ENTER to finish.\"\n");
	fclose(fp);
	/* make the batch file executable */
	sprintf(out_val, "chmod +x %s", cwd_path);
	system(out_val);
	/* execute the batch file */
	switch (posix_terminal) {
	case 0: sprintf(out_val, "xfce4-terminal --disable-server -x %s", cwd_path); break;
	case 1: sprintf(out_val, "konsole -e %s", cwd_path); break;
	case 2: sprintf(out_val, "gnome-terminal -e %s", cwd_path); break;
	case 3: sprintf(out_val, "xterm -e %s", cwd_path); break;
	case 4: sprintf(out_val, "bash -c %s", cwd_path); break;
	case 5: sprintf(out_val, "sh -c %s", cwd_path); break;
	}
	/* download + extract */
	system(out_val);

	fp = fopen("updater/result.tmp", "r");
	if (fp) {//~paranoia?
		long res;

		fgets(out_val, 20, fp);
		res = atoi(out_val);
		if (res != 0) show_error_broken(top_window);
		fclose(fp);
	}
#endif

	gtk_button_set_label(button, "Install/update music pack");
	while(gtk_events_pending())
	    gtk_main_iteration();
}

int main(int argc, char *argv[]) {
	FILE *fp;
#ifdef WINDOWS /* use windows registry to locate 7-zip */
	HKEY hTestKey;
	LPBYTE path_7z_p = (LPBYTE)path_7z;
 #if 0
	int path_7z_size = 1023;
	LPDWORD path_7z_size_p = (LPDWORD)&path_7z_size;
 #else
	DWORD path_7z_size = 1023;
	LPDWORD path_7z_size_p = &path_7z_size;
 #endif
 	unsigned long path_7z_type = REG_SZ;
#else
	char buf[1024];
#endif

	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *insclient;
	GtkWidget *inssfx;
	GtkWidget *insmus;

	gtk_init(&argc, &argv);
	top_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(top_window), "TomeNET Updater v1.0");
	gtk_window_set_default_size(GTK_WINDOW(top_window), 320, 220);
	gtk_window_set_position(GTK_WINDOW(top_window), GTK_WIN_POS_CENTER);
	//gtk_window_set_icon(GTK_WINDOW(window), create_pixbuf("updater/tomenet4.png"));
	//gtk_window_set_icon(GTK_WINDOW(top_window), create_pixbuf("updater/TomeNET-Updater.png"));
	gtk_window_set_icon(GTK_WINDOW(top_window), create_pixbuf("updater/TomeNET-Updater.ico"));

	frame = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(top_window), frame);

	label = gtk_label_new("  Make sure to quit and close the\ngame before running this updater!");
	gtk_fixed_put(GTK_FIXED(frame), label, 20, 15); 

	insclient = gtk_button_new_with_label("Install/update game client");
	gtk_widget_set_size_request(insclient, 255, 35);
	gtk_fixed_put(GTK_FIXED(frame), insclient, 30, 70);

	inssfx = gtk_button_new_with_label("Install/update sound pack");
	gtk_widget_set_size_request(inssfx, 255, 35);
	gtk_fixed_put(GTK_FIXED(frame), inssfx, 30, 120);

	insmus = gtk_button_new_with_label("Install/update music pack");
	gtk_widget_set_size_request(insmus, 255, 35);
	gtk_fixed_put(GTK_FIXED(frame), insmus, 30, 170);

	gtk_widget_show_all(top_window);

	g_signal_connect(insclient, "clicked", G_CALLBACK(install_client), NULL);
	g_signal_connect(inssfx, "clicked", G_CALLBACK(install_sound), NULL);
	g_signal_connect(insmus, "clicked", G_CALLBACK(install_music), NULL);

	g_signal_connect_swapped(G_OBJECT(top_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);


	/* check whether 7-zip is installed, otherwise
	   give error message about it and terminate */

#ifdef WINDOWS /* use windows registry to locate 7-zip */
	/* check registry for 7zip (note that for example WinRAR could cause problems with 7z files) */
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\7-Zip\\"), 0, KEY_READ, &hTestKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hTestKey, "Path", NULL, &path_7z_type, path_7z_p, path_7z_size_p) == ERROR_SUCCESS) {
			path_7z[path_7z_size] = '\0';
		} else {
			// odd case
			RegCloseKey(hTestKey);
			//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
			show_error(top_window);
			return -1;
		}
	} else {
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\7-Zip\\"), 0, KEY_READ | 0x0200, &hTestKey) == ERROR_SUCCESS) {//KEY_WOW64_32KEY (0x0200)
			if (RegQueryValueEx(hTestKey, "Path", NULL, &path_7z_type, path_7z_p, path_7z_size_p) == ERROR_SUCCESS) {
				path_7z[path_7z_size] = '\0';
			} else {
				// odd case
				RegCloseKey(hTestKey);
				//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
				show_error(top_window);
				return -1;
			}
		} else {
			/* This case should work on 64-bit Windows (w/ 32-bit client) */
			if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\7-Zip\\"), 0, KEY_READ | 0x0100, &hTestKey) == ERROR_SUCCESS) {//KEY_WOW64_64KEY (0x0100)
				if (RegQueryValueEx(hTestKey, "Path", NULL, &path_7z_type, path_7z_p, path_7z_size_p) == ERROR_SUCCESS) {
					path_7z[path_7z_size] = '\0';
				} else {
					// odd case
					RegCloseKey(hTestKey);
					//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
					show_error(top_window);
					return -1;
				}
			} else {
				//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
				show_error(top_window);
				return -1;
			}
		}
	}
	RegCloseKey(hTestKey);
	/* enclose full path in quotes, to handle possible spaces */
	strcpy(path_7z_quoted, "\"");
	strcat(path_7z_quoted, path_7z);
	strcat(path_7z_quoted, "\\7zG.exe\"");
	strcat(path_7z, "\\7zG.exe");

	/* do the same tests once more as for posix clients */
	fp = fopen("tmp.file", "w");
	fprintf(fp, "mh");
	fclose(fp);

	_spawnl(_P_WAIT, path_7z, path_7z_quoted, "a", "tmp.7z", "tmp.file", NULL); /* supposed to work on WINE, yet crashes if not exit(0)ing next oO */
	remove("tmp.file");

	if (!(fp = fopen("tmp.7z", "r"))) { /* paranoia? */
		//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
		show_error(top_window);
		return -1;
	} else if (fgetc(fp) == EOF) { /* normal */
		fclose(fp);
		//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
		show_error(top_window);
		return -1;
	}
#else /* assume posix */
 #if 0	/* command-line 7z */
	system("7z > tmp.7z");
 #else	/* GUI 7z (for password prompts) */
	fp = fopen("tmp.file", "w");
	fclose(fp);
	system("7zG a tmp.7z tmp.file");
	remove("tmp.file");
 #endif
        if (!(fp = fopen("tmp.7z", "r"))) { /* paranoia? */
		//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
		show_error(top_window);//, "7-zip GUI not found ('7zG'). Install it first. (Package name is 'p7zip'.)");
		return -1;
	} else if (fgetc(fp) == EOF) { /* normal */
		fclose(fp);
		//MessageBox(NULL, "You must first install 7-zip from www.7-zip.org !", "Error", MB_OK);
		show_error(top_window);//, "7-zip GUI not found ('7zG'). Install it first. (Package name is 'p7zip'.)");
		return -1;
	}
#endif
	fclose(fp);
	remove("tmp.7z");

#ifndef WINDOWS
	/* find out about Linux vs OS X */
	system("uname -s > arch.tmp");
	fp = fopen("arch.tmp", "r");
	fgets(buf, 64, fp);
	fclose(fp);
	remove("arch.tmp");
	if (strncmp(buf, "Linux", 5)) is_linux = 0; //usually 'Darwin' for OS X

	/* find out about 32 vs 64 bit */
	system("uname -m > arch.tmp");
	fp = fopen("arch.tmp", "r");
	fgets(buf, 64, fp);
	fclose(fp);
	remove("arch.tmp");
	if (strncmp(buf, "x86_64", 6)) is_64bit = 0;

	/* find out which posix terminal we have available */
	if (posix_terminal == -1) {/* no pre-defined favourite? */
		if (system("xfce4-terminal -x echo") == 0) posix_terminal = 0;
		else if (system("konsole -e echo") == 0) posix_terminal = 1;
		else if (system("gnome-terminal -e echo") == 0) posix_terminal = 2;
		else if (system("xterm -e echo") == 0) posix_terminal = 3;
		else if (system("bash -c echo") == 0) posix_terminal = 4;
		else if (system("sh -c echo") == 0) posix_terminal = 5;
	}
#endif

	gtk_main();

	return 0;
}
