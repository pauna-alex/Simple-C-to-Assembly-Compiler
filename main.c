#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINES 100
#define MAX_LINE_LENGTH 256

int elements_after_eq(char *text)
{
	char *p = strchr(text, '=');
	p++;

	int count = 0;
	char *token = strtok(p, " ");

	while (token)
	{
		count++;
		token = strtok(NULL, " ");
	}

	return count;
}

void format_variable(char *var)
{
	int is_num = 1;
	for (int i = 0; var[i] != '\0'; i++)
	{
		if (!isdigit(var[i]))
		{
			is_num = 0;
			break;
		}
	}

	if (!is_num)
	{
		int len = strlen(var);
		char formatted[len + 3];
		formatted[0] = 'e';
		strcpy(formatted + 1, var);
		formatted[len + 1] = 'x';
		formatted[len + 2] = '\0';
		strcpy(var, formatted);
	}
}

int isinteger(const char *str)
{
	if (*str == '-' || *str == '+')
		str++;

	if (*str == '\0')
		return 0;

	while (*str)
	{
		if (!isdigit(*str))
			return 0;
		str++;
	}
	return 1;
}

void three_elm_case(char first, char *semn, char *third)
{
	format_variable(third);

	if (strcmp(semn, "+") == 0)
	{
		printf("ADD e%cx, %s\n", first, third);
	} else if (strcmp(semn, "-") == 0)
	{
		printf("SUB e%cx, %s\n", first, third);
	} else if (strcmp(semn, "*") == 0)
	{
		if (first == 'a') {
			printf("MUL %s\n", third);
		} else {
			printf("MOV eax, e%cx\n", first);
			printf("MUL %s\n", third);
			printf("MOV e%cx, eax\n", first);
		}
	} else if (strcmp(semn, "&") == 0)
	{
		printf("AND e%cx, %s\n", first, third);
	} else if (strcmp(semn, "|") == 0)
	{
		printf("OR e%cx, %s\n", first, third);
	} else if (strcmp(semn, "^") == 0)
	{
		printf("XOR e%cx, %s\n", first, third);
	} else if (strcmp(semn, "<<") == 0)
	{
		printf("SHL e%cx, %s\n", first, third);
	} else if (strcmp(semn, ">>") == 0)
	{
		printf("SHR e%cx, %s\n", first, third);
	} else if (strcmp(semn, "/") == 0)
	{
		if (strcmp(third, "0") == 0) {
			printf("Error\n");
		} else {
			printf("MOV eax, e%cx\n", first);
			printf("DIV %s\n", third);
			printf("MOV e%cx, eax\n", first);
		}
	}
}

void translate_to_assembly(char *line)
{
	char line_copy[MAX_LINE_LENGTH];
	strcpy(line_copy, line);
	int nr = elements_after_eq(line_copy);
	strcpy(line_copy, line);
	if (nr == 1) {
		char first, second[4];
		if (sscanf(line_copy, " %c = %[^;] ;", &first, second) != 2)
			printf("error");
		if (isinteger(second) == 1)
			printf("MOV e%cx, %s\n", first, second);
		else
			printf("MOV e%cx, e%sx\n", first, second);
	} else {
		char first, second[10], semn[4], third[10];
		if (sscanf(line_copy, " %c = %s %s %[^;] ;", &first, second,
				   semn, third) != 4)
			printf("error");
		three_elm_case(first, semn, third);
	}
}

void read_block(char ***commands, int *line_count, char *first_line)
{
	char line[MAX_LINE_LENGTH];
	int brace_count = 0;

	*commands = NULL;
	*line_count = 0;

	*commands = realloc(*commands, (*line_count + 1) * sizeof(char *));
	(*commands)[*line_count] = malloc(strlen(first_line) + 1);
	strcpy((*commands)[*line_count], first_line);
	(*line_count)++;

	if (strchr(first_line, '{'))
		brace_count++;

	while (fgets(line, sizeof(line), stdin))
	{
		*commands = realloc(*commands, (*line_count + 1) * sizeof(char *));
		(*commands)[*line_count] = malloc(strlen(line) + 1);
		strcpy((*commands)[*line_count], line);
		(*line_count)++;

		if (strchr(line, '{'))
			brace_count++;
		if (strchr(line, '}'))
			brace_count--;

		if (brace_count == 0)
			break;
	}
}

void free_block(char ***commands, int *line_count)
{
	for (int i = 0; i < *line_count; i++)
	{
		free((*commands)[i]);
	}
	free(*commands);
	*commands = NULL;
	*line_count = 0;
}

void generate_jump(const char *op)
{
	if (strcmp(op, "==") == 0)
		printf("JNE end_label\n");
	else if (strcmp(op, "!=") == 0)
		printf("JE end_label\n");
	else if (strcmp(op, "<") == 0)
		printf("JGE end_label\n");
	else if (strcmp(op, "<=") == 0)
		printf("JG end_label\n");
	else if (strcmp(op, ">") == 0)
		printf("JLE end_label\n");
	else if (strcmp(op, ">=") == 0)
		printf("JL end_label\n");
}

void case_if(char **commands)
{
	char expr[MAX_LINE_LENGTH];
	if (sscanf(commands[0], "if ( %[^)] ) {", expr) != 1)
		printf("error");
	char var1[10], op[4], var2[10];
	if (sscanf(expr, "%s %s %s", var1, op, var2) != 3)
		printf("error");
	format_variable(var2);
	printf("CMP e%sx, %s\n", var1, var2);
	generate_jump(op);
	int i = 1;
	while (strncmp(commands[i], "}", 1) != 0)
	{
		translate_to_assembly(commands[i]);
		i++;
	}
	printf("end_label:\n");
}

void case_while(char **commands)
{
	char expr[MAX_LINE_LENGTH];
	if (sscanf(commands[0], "while ( %[^)] ) {", expr) != 1)
		printf("error");
	char var1[10], op[4], var2[10];
	if (sscanf(expr, "%s %s %s", var1, op, var2) != 3)
		printf("error");
	format_variable(var2);
	printf("start_loop:\n");
	printf("CMP e%sx, %s\n", var1, var2);
	generate_jump(op);
	int i = 1;
	while (strncmp(commands[i], "}", 1) != 0)
	{
		translate_to_assembly(commands[i]);
		i++;
	}
	printf("JMP start_loop\n");
	printf("end_label:\n");
}

void case_for(char **commands)
{
	char expr[MAX_LINE_LENGTH];
	char var1[10], op[4], var2[10];
	if (sscanf(commands[0], "for ( %*[^;]; %[^;]; %*[^)] )", expr) != 1)
		printf("error");
	if (sscanf(expr, "%s %s %s", var1, op, var2) != 3)
		printf("error");
	printf("MOV e%sx, 0\n", var1);
	printf("start_loop:\n");
	printf("CMP e%sx, %s\n", var1, var2);
	generate_jump(op);
	int i = 1;
	while (strncmp(commands[i], "}", 1) != 0)
	{
		translate_to_assembly(commands[i]);
		i++;
	}
	printf("ADD e%sx, 1\n", var1);
	printf("JMP start_loop\n");
	printf("end_loop:\n");
}

void translate_to_assembly_block(char **commands, char pre[])
{
	if (strncmp(pre, "if", 2) == 0)
		case_if(commands);
	if (strncmp(pre, "wh", 2) == 0)
		case_while(commands);
	if (strncmp(pre, "fo", 2) == 0)
		case_for(commands);
}

int main(void)
{
	char line[MAX_LINE_LENGTH];

	while (fgets(line, sizeof(line), stdin))
	{
		char pre[3];
		strncpy(pre, line, 2);
		if (strncmp(pre, "if", 2) == 0 || strncmp(pre, "wh", 2) == 0 ||
			strncmp(pre, "for", 2) == 0) {
			char **commands = NULL;
			int line_count = 0;

			read_block(&commands, &line_count, line);

			translate_to_assembly_block(commands, pre);

			free_block(&commands, &line_count);
		} else {
			translate_to_assembly(line);
		}
	}

	return 0;
}
