#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PROGRAM_HEADER "\
extern putchar, getchar\n\
section .bss\n\
\tdata resb 30000\n\
\n\
section .text\n\
\tglobal main\n\
main:\n\
\tmov rbx, data\n\
"
#define PTR_RIGHT "\tinc rbx\n"
#define PTR_LEFT "\tdec rbx\n"
#define ADD "\tinc byte [rbx]\n"
#define SUB "\tdec byte [rbx]\n"
#define OUTPUT "\
\txor rdi, rdi\n\
\tmov dil, [rbx]\n\
\tcall putchar\n\
"
#define INPUT "\
\tcall getchar\n\
\tmov [rbx], al\n\
"
#define OPENING "\
\tcmp [rbx], byte 0\n\
\tjz .c_%zu\n\
.o_%zu:\n\
"
#define CLOSING "\
\tcmp [rbx], byte 0\n\
\tjnz .o_%zu\n\
.c_%zu:\n\
"
#ifdef _WIN32

#define PROGRAM_FOOTER "\
\txor rax, rax\n\
\tret\n\
"
#define NASM "nasm -f win64 -o tmp.o tmp.asm"
#define LINK "ld -e main -lc tmp.o -o %s"

#else

#define PROGRAM_FOOTER "\
\tmov rdi, 0\n\
\tmov rax, 60\n\
\tsyscall\n\
"
#define NASM "nasm -f elf64 -o tmp.o tmp.asm"
#define LINK "ld -e main -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc tmp.o -o %s"

#endif

char *getSrc(char *filePath);
int compile(char *src);
size_t getMatchingOpening(char *src, size_t offset);
size_t getMatchingClosing(char *src, size_t offset);

size_t wrongDepth = 0;

int main (int argc, char **argv)
{
	if (argc < 2)
	{
		printf("No source or output file specified\n");
		return 1;
	} else if (argc < 3)
	{
		printf("No output file specified\n");
		return 2;
	}

	char *src = getSrc(argv[1]);
	if (!src)
	{
		printf("The specified file '%s' couldn't be read\n", argv[1]);
		return 3;
	}

	int compileStatus = compile(src);
	if (compileStatus == 1)
	{
		printf("Number of '[' doesn't match number of ']', %zd\n", wrongDepth);
		return 4;
	}
	else if (compileStatus == 2)
	{
		printf("Couldn't safe to temporary file 'tmp.asm'\n");
		return 5;
	}

	free(src);

	FILE *child = popen(NASM, "r");
	char buf[512];
	while (fgets(buf, 512, child))
	{
		printf("%s", buf);
	}
	pclose(child);

	snprintf(buf, 512, LINK, argv[2]);
	child = popen(buf, "r");
	while (fgets(buf, 512, child))
	{
		printf("%s", buf);
	}
	pclose(child);
}

char *getSrc(char *filePath)
{
	FILE *file;
	file = fopen(filePath, "r");
	if (file)
	{
		fseek(file, 0, SEEK_END);
		size_t file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		char *src = malloc(file_size);

		fread(src, 1, file_size, file);

		fclose(file);
		return src;
	}
	return 0;
}

int compile(char *src)
{
	FILE *file;
	file = fopen("tmp.asm", "w");

	fprintf(file, "%s", PROGRAM_HEADER);

	if (file)
	{
		size_t offset = 0;
		while (src[offset])
		{
			size_t match;
			switch (src[offset])
			{
				case '>':
					fprintf(file, PTR_RIGHT);
					break;
				case '<':
					fprintf(file, PTR_LEFT);
					break;
				case '+':
					fprintf(file, ADD);
					break;
				case '-':
					fprintf(file, SUB);
					break;
				case '.':
					fprintf(file, OUTPUT);
					break;
				case ',':
					fprintf(file, INPUT);
					break;
				case '[':
					match = getMatchingClosing(src, offset);
					if (match == -1)
					{
						fclose(file);
						return 1;
					}
					fprintf(file, OPENING, match, offset);
					break;
				case ']':
					match = getMatchingOpening(src, offset);
					if (match == -1)
					{
						fclose(file);
						return 1;
					}
					fprintf(file, CLOSING, match, offset);
					break;
				default:
					break;
			}
			offset++;
		}

		fprintf(file, PROGRAM_FOOTER);
		fclose(file);
		return 0;
	}
	return 2;
}

size_t getMatchingOpening(char *src, size_t offset)
{
	size_t depth = 0;
	while (offset + 1)
	{
		switch (src[offset])
		{
			case '[':
				depth--;
				if (!depth)
				{
					return offset;
				}
				break;
			case ']':
				depth++;
				break;
			default:
				break;
		}
		offset--;
	}
	wrongDepth = 1;
	return -1;
}


size_t getMatchingClosing(char *src, size_t offset)
{
	size_t depth = 0;
	while (src[offset])
	{
		switch (src[offset])
		{
			case '[':
				depth++;
				break;
			case ']':
				depth--;
				if (!depth)
				{
					return offset;
				}
				break;
			default:
				break;
		}
		offset++;
	}
	wrongDepth = -1;
	return -1;
}
