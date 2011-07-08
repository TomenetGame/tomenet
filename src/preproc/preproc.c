/* Custom preprocessor for LUA pkg files by C. Blue
 * Use: Enable usage of #if..#else..#endif control structures in LUA pkg files
 *      by making use of the C preprocessor.
 * Note: The LUA file will need an #include statement that includes a header
 *       file containing all #define directives that ought to be used.
 *       Local #define directives within the LUA file itself will be ignored!
 *       The reason for this is that 'tolua' actually can and does process
 *       local #define statements already, so they must not be eliminated by
 *       the C preprocessor.
 */


/* debug mode: Don't remove temporary files */
//#define DEBUG


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	FILE *f_in, *f_out;
	char tmp_file[160 + 3], line[320 + 1], *line_p, line_mod[320 + 1 + 6];
	/* 320 + 1 + 6 -> allow +6 extra marker chars (originally '//', but interferes
	   with additionally included header files. So now a 'non-C' sequence instead,
	   ie a character sequence that doesn't occur in valid C code at line beginning.
	   It must also end on a C comment tag if we want the preprocessor to not remove
	   'obsolete' whitespace. This isn't required, but allows for better comparability
	   if the files are viewed by a human. Current sequence therefore: */
	char seq[7] = {'1', '=', '/', '0', '/', '/', 0};
	int cpp_opts;


	/* check command-line arguments */

	if (argc < 4) {
		printf("Usage: preproc <input file> <output file> <name of C preprocessor> [<C preprocessor options>..]\n");
		printf("       C preprocessor options should usually (cpp) be: -C -P\n");
		return -1;
	}
	if (strlen(argv[1]) == 0) { /* paranoia */
		printf("Error: No input filename specified.\n");
		return -2;
	}
	if (strlen(argv[1]) > 160) {
		printf("Error: Input filename must not be longer than 160 characters.\n");
		return -3;
	}
	if (strlen(argv[2]) == 0) { /* paranoia */
		printf("Error: No output filename specified.\n");
		return -4;
	}


	/* open file */

	sprintf(tmp_file, "%s_", argv[1]);

	f_in = fopen(argv[1], "r");
	if (f_in == NULL) {
		printf("Error: Couldn't open input file.\n");
		return -5;
	}

	f_out = fopen(tmp_file, "w");
	if (f_out == NULL) {
		printf("Error: Couldn't create temporary file.\n");
		return -6;
	}


	/* create temporary file that can be preprocessed */

	/* read a line */
	while (fgets(line, 320, f_in)) {
		/* trim newline (at the end) */
		line_p = strchr(line, '\n');
		if (line_p) *line_p = '\0';

		/* skip whitespaces at the beginning */
		line_p = line;
		while (*line_p == ' ' || *line_p == '\t') line_p++;

		/* forward #include directives to the preprocessor so it
		   can process #if/#else/#endif structures accordingly */
		if (strstr(line_p, "#include") == line_p) {
			/* keep line as it is, so it will be processed by the C preprocessor */
			sprintf(line_mod, "%s\n", line);
#if 0 /* don't include these, way too messy/can't work */
		} else if (strstr(line_p, "$#include") == line_p) {
			/* turn it into a normal #include so it will be processed by the C preprocessor */
			sprintf(line_mod, "%s\n", line_p + 1);
#endif

		/* test for #if / #else / #endif at the beginning
		   of the line (after whitespaces have been trimmed).
		   Any other preprocessor directives could be added. */
		} else if (strstr(line_p, "#if") == line_p ||
		    strstr(line_p, "#else") == line_p ||
		    strstr(line_p, "#endif") == line_p) {
			/* keep line as it is, so it will get treated by the C preprocessor */
			sprintf(line_mod, "%s\n", line);

		/* normal line - protect it from being changed by the C preprocessor */
		} else {
			/* add the marker that indicates a line that is not to be touched */
			sprintf(line_mod, "%s%s\n", seq, line);
		}

		/* write modified line to temporary file */
		fputs(line_mod, f_out);
	}

	fclose(f_out);
	fclose(f_in);


	/* call C preprocessor on temporary file */

	sprintf(line, "%s ", argv[3]);
	for (cpp_opts = 5; cpp_opts <= argc; cpp_opts++) {
		strcat(line, argv[cpp_opts - 1]);
		strcat(line, " ");
	}
	strcat(line, tmp_file);
	strcat(line, " ");
#if 0 /* This works if we just invoke 'cpp', but it doesn't if the system doesn't \
         have a 'cpp' command and we need to fallback to 'gcc', because gcc \
         doesn't take another file name parm in the same syntax as cpp. */
	strcat(line, tmp_file);
	strcat(line, "_");
#else /* So instead we'll just grab the stdout output, which works fine with both */
	strcat(line, "> ");
	strcat(line, tmp_file);
	strcat(line, "_");
#endif

	if (system(line) == -1) {
		printf("Error: Couldn't execute C preprocessor.\n");
		return -7;
	}

#ifndef DEBUG
	remove(tmp_file);
#endif


	/* clean up preprocessed file to generate output file */

	sprintf(tmp_file, "%s__", argv[1]);

	f_in = fopen(tmp_file, "r");
	if (f_in == NULL) {
		printf("Error: Couldn't open preprocessed temporary file.\n");
		return -8;
	}

	f_out = fopen(argv[2], "w");
	if (f_out == NULL) {
		printf("Error: Couldn't create output file.\n");
		return -9;
	}

	/* read a line */
	while (fgets(line_mod, 320 + 2, f_in)) {
		/* strip prefixed marker sequence again and write line to output file */
		if (line_mod[0] == seq[0] &&
		    line_mod[1] == seq[1] &&
		    line_mod[2] == seq[2] &&
		    line_mod[3] == seq[3] &&
		    line_mod[4] == seq[4] &&
		    line_mod[5] == seq[5])
			fputs(line_mod + 6, f_out);
		/* lines that were eliminated by the preprocessor don't need any treatment */
		else fputs(line_mod, f_out);
	}

	fclose(f_out);
	fclose(f_in);

#ifndef DEBUG
	remove(tmp_file);
#endif


	/* all done */

	return 0;
}
