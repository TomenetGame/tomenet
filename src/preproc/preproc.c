/* Custom preprocessor for LUA pkg files by C. Blue
 * Use:
 *   Enable usage of #if..#else..#endif control structures in LUA pkg files
 *   by making use of the C preprocessor.
 * Note:
 *   The LUA file (.pre) will need a #parse statement that includes a header
 *   file containing all #define directives that ought to be used for
 *   evaluating control structures (#if..) within the .pre file.
 *
 *   Local #define directives within the LUA (.pre) file itself will be ignored
 *   for evaluating local control structures!
 *   The reason for this is that 'tolua' actually can and does process
 *   local #define statements already, so they must not be eliminated by
 *   the C preprocessor.
 *
 *   Local #include directives will be processed normally and insert the
 *   according files into the LUA file (.pre) source code.
 *
 *   Local define- and include-statements prefixed with a '$' will not be
 *   evaluated by either the preprocessor or tolua and will instead be passed
 *   down to become actual define- and include-statements in the resulting
 *   w_xxxx.c file!
 */


/* debug mode: Don't remove temporary files */
//#define DEBUG

/* Use custom #parse and #include directives:
   #parse does what #include actually did, while #include is now a statement
   to pseudo-include further lua sources, without actually parsing them with
   the preprocessor - ie it's just for basic text inclusion.
   This allows to include #define lists such as skills required for spells. */
#define CUSTOM_INCLUDE


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
	FILE *f_in, *f_out;
	char tmp_file[160 + 3], line[320 + 1], *line_ptr, line_mod[320 + 1 + 6];
	/* 320 + 1 + 6 -> allow +6 extra marker chars (originally '//', but interferes
	   with additionally included header files. So now a 'non-C' sequence instead,
	   ie a character sequence that doesn't occur in valid C code at line beginning.
	   It must also end on a C comment tag if we want the preprocessor to not remove
	   'obsolete' whitespace. This isn't required, but allows for better comparability
	   if the files are viewed by a human. Current sequence therefore: */
#ifdef CUSTOM_INCLUDE
	int included;//bool
	FILE *f_included;
	char included_file[160 + 3], included_line[320 + 1], *included_file_ptr, *included_line_ptr;
#endif
	char seq[7] = {'1', '=', '/', '0', '/', '/', 0};
	int cpp_opts;

	char *cptr, *in_comment = NULL, *out_comment = NULL, *prev_in_comment;
	int in_comment_block = 0;


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
	f_in = fopen(argv[1], "r");
	if (f_in == NULL) {
		printf("Error: Couldn't open input file to create working copy.\n");
		return -13;
	}

	sprintf(tmp_file, "%s_", argv[1]);
	f_out = fopen(tmp_file, "w");
	if (f_out == NULL) {
		printf("Error: Couldn't create temporary working copy file.\n");
		return -14;
	}

	/* create temporary working copy */
	while (fgets(line, 320, f_in)) {
		/* skip whitespaces at the beginning */
		line_ptr = line;
		while (*line_ptr == ' ' || *line_ptr == '\t') line_ptr++;
		/* copy it */
		fputs(line_ptr, f_out);
	}
	fclose(f_in);
	fclose(f_out);


#ifdef CUSTOM_INCLUDE
	do {
		included = 0;

		/* pseudo-#include other source files recursively until no more #includes are found */
		sprintf(tmp_file, "%s_", argv[1]);
		f_in = fopen(tmp_file, "r");
		if (f_in == NULL) {
			printf("Error: Couldn't open custom-including input file.\n");
			return -11;
		}

		sprintf(tmp_file, "%s__", argv[1]);
		f_out = fopen(tmp_file, "w");
		if (f_out == NULL) {
			printf("Error: Couldn't create custom-including temporary file.\n");
			return -12;
		}

		/* read a line */
		while (fgets(line, 320, f_in)) {
			line_ptr = line;

			/* actually process #include directive in our own, special way, same as all existing #defines:
			   we just add/insert them to the LUA (.pre) source! */
			if (strstr(line_ptr, "#include") != line_ptr) {
				/* copy normal lines without any change */
				fputs(line, f_out);
				continue;
			}

			/* discard line, since WE do the work here, cpp won't get to see this #include at all */
			//hardcore!
			included = 1;

			/* extract file name from #include directive, doesn't matter if in quotations or <..> */
			strcpy(included_file, line_ptr + 8);
			included_file_ptr = included_file;
			/* strip trailing spaces and tabs and newline */
			while (included_file[strlen(included_file) - 1] == ' ' ||
			    included_file[strlen(included_file) - 1] == '\t' ||
			    included_file[strlen(included_file) - 1] == '\n')
				included_file[strlen(included_file) - 1] = '\0';
			/* strip final " or > */
			if (included_file[strlen(included_file) - 1] == '"' ||
			    included_file[strlen(included_file) - 1] == '>')
				included_file[strlen(included_file) - 1] = '\0';
			/* strip leading tabs and spaces */
			while (*included_file_ptr == ' ' ||
			    *included_file_ptr == '\t')
				included_file_ptr++;
			/* strip first " or < */
			if (*included_file_ptr == '"' ||
			    *included_file_ptr == '<')
				included_file_ptr++;

			f_included = fopen(included_file_ptr, "r");
			if (f_included == NULL) {
				printf("Error: Couldn't open included file: %s\n", included_file_ptr);
				return -10;
			}

			fputs("\n", f_out);
			sprintf(tmp_file, "/* ---------------- #include '%s' ---------------- */\n", included_file_ptr);
			fputs(tmp_file, f_out);

			/* copy included file line by line */
			while (fgets(included_line, 320, f_included)) {
				/* skip whitespaces at the beginning */
				included_line_ptr = included_line;
				while (*included_line_ptr == ' ' || *included_line_ptr == '\t') included_line_ptr++;
				/* copy it */
				fputs(included_line_ptr, f_out);
			}

			sprintf(tmp_file, "/* ------------ end of #include '%s'. ------------ */\n", included_file);
			fputs(tmp_file, f_out);
			fputs("\n", f_out);
		}

		fclose(f_in);
		fclose(f_out);

		/* the dest file becomes the src file, for next pass (recursion) */
		sprintf(line, "%s_", argv[1]);
		remove(line);
		sprintf(tmp_file, "%s__", argv[1]);
		rename(tmp_file, line);
	} while (included);
#endif

	/* create temporary file that can be preprocessed */

	/* open file */
	sprintf(tmp_file, "%s_", argv[1]);
	f_in = fopen(tmp_file, "r");
	if (f_in == NULL) {
		printf("Error: Couldn't open input file.\n");
		return -5;
	}

	sprintf(tmp_file, "%s__", argv[1]);
	f_out = fopen(tmp_file, "w");
	if (f_out == NULL) {
		printf("Error: Couldn't create temporary file.\n");
		return -6;
	}

	/* read a line */
	while (fgets(line, 320, f_in)) {
		/* trim newline (at the end) */
		line_ptr = strchr(line, '\n');
		if (line_ptr) *line_ptr = '\0';

		/* skip whitespaces at the beginning (redundant, done in 'working copy') */
		line_ptr = line;
		while (*line_ptr == ' ' || *line_ptr == '\t') line_ptr++;

#ifndef CUSTOM_INCLUDE
		/* forward #parse directives as #include directives to the preprocessor
		   so it can process #if/#else/#endif structures accordingly */
		if (strstr(line_ptr, "#include") == line_ptr) {
			/* keep line as it is, so it will be processed by the C preprocessor */
			sprintf(line_mod, "%s\n", line);
#else
		/* forward #parse directives as #include directives to the preprocessor
		   so it can process #if/#else/#endif structures accordingly */
		if (strstr(line_ptr, "#parse") == line_ptr) {
			/* replace "#parse" by "#include" */
			sprintf(line_mod, "%s\n", line);
			cptr = strstr(line_mod, "#parse");
			strcpy(cptr, "#include");
			strcat(cptr, line_ptr + 6);
			strcat(cptr, "\n");
#endif
#if 0 /* don't include these, way too messy/can't work */
		} else if (strstr(line_ptr, "$#include") == line_ptr) {
			/* turn it into a normal #include so it will be processed by the C preprocessor */
			sprintf(line_mod, "%s\n", line_ptr + 1);
#endif

		/* test for #if / #else / #endif at the beginning
		   of the line (after whitespaces have been trimmed).
		   Any other preprocessor directives could be added. */
		} else if (strstr(line_ptr, "#if") == line_ptr ||
		    strstr(line_ptr, "#else") == line_ptr ||
		    strstr(line_ptr, "#endif") == line_ptr) {
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

	sprintf(tmp_file, "%s___", argv[1]);

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
		/* preprocessor directives that were hidden _inside_ comments
		   because silly tolua will choke on them */
		prev_in_comment = line_mod;
		do {
			if (out_comment) in_comment = out_comment = NULL;

			if (!in_comment) {
				if (in_comment_block) in_comment = line_mod;
				else {
					in_comment = strstr(prev_in_comment, "/*");
					if (in_comment) prev_in_comment = in_comment + 2;
				}
			}
			if (in_comment) {
				if (in_comment_block) out_comment = strstr(line_mod, "*/");
				else out_comment = strstr(in_comment, "*/");

				while ((cptr = strstr(in_comment, "#"))) {
					if (!out_comment || cptr < out_comment) {
						*cptr = ' ';
					} else break;
				}

				if (out_comment) in_comment_block = 0;
			}
		} while (in_comment && out_comment);
		if (in_comment) in_comment_block = 1;

		/* aaand also strip comments that are adjacent, silyl tolua */
		if ((cptr = strstr(line_mod, "*//*"))) {
			cptr[0] = ' ';
			cptr[1] = ' ';
			cptr[2] = ' ';
			cptr[3] = ' ';
		}

		/* gcc 4.8.0 now puts an URL in the top comment, on which tolua
		   chokes, sigh. */
		if ((cptr = strstr(line_mod, "http://www.gnu.org"))) cptr[5] = ':';

		/* on to the actual work.. */


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
