#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE (8192)

struct node {
	struct room *room;
	struct node *next;
};

struct room {
	char name[100];
	int north;
	struct room *n_room;
	int south;
	struct room *s_room;
	int west;
	struct room *w_room;
	int east;
	struct room *e_room;
};

enum command { NORTH, SOUTH, WEST, EAST, QUIT, UNDEFINED };

struct node *rooms = NULL;
struct room *curr_room = NULL;

void load_data(int argc, char** argv);
void load_room_data(FILE *file);
void load_path_data(FILE *file);
void create_room(char *name);
void create_path(char *name_a, char *name_b, char *direction);
struct room *get_room(char *name);
void run();
void start();
void print_room();
void act(enum command);
void quit();
int parse_command(char *command);
struct room *get_first_room();

int main(int argc, char** argv) {

	load_data(argc, argv);

	run();

	return 0;
}


/* ************************************************** *
 * LOAD DATA
 * ************************************************** */

void load_data(int argc, char** argv) {
	if (argc < 2) {
		printf("No Level File Specified\n");
		exit(1);
	}

	FILE *file = fopen(argv[1], "rb");
	if (!file) {
		printf("Cannot Find Level File\n");
		exit(1);
	}

	load_room_data(file);
	load_path_data(file);
}

void load_room_data(FILE *file) {
	char *buffer = (char *)calloc(MAX_LINE, sizeof(char));
	char *name;

	if (!fgets(buffer, sizeof(char) * MAX_LINE, file)) {
		exit(1);
	}

	name = strtok(buffer, " ");
	name[strcspn(name, "\n")] = 0;
	create_room(name);

	while ((name = strtok(NULL, " ")) != NULL) {
		name[strcspn(name, "\n")] = 0;
		create_room(name);
	}

	free(buffer);
}

void load_path_data(FILE *file) {
	char *buffer = (char *)calloc(MAX_LINE, sizeof(char));
	char *room_a;
	char *room_b;
	char *direction;

	while (fgets(buffer, sizeof(char) * MAX_LINE, file)) {
		room_a = strtok(buffer, " > ");
		direction = strtok(NULL, " > ");
		room_b = strtok(NULL, " > ");
		room_b[strcspn(room_b, "\n")] = 0;

		create_path(room_a, room_b, direction);
	}

	free(buffer);
}

void create_room(char *name) {
	struct room *new_room = (struct room *)malloc(sizeof(struct room));
	memset(new_room, 0, sizeof(struct room));
	strcpy(new_room->name, name);

	struct node *new_node = (struct node *)malloc(sizeof(struct node));
	new_node->room = new_room;
	new_node->next = rooms;
	rooms = new_node;
}

void create_path(char *name_a, char *name_b, char *direction) {
	struct room *room_a = get_room(name_a);
	struct room *room_b = get_room(name_b);

	switch (parse_command(direction)) {
		case NORTH:
			room_a->north = 1;
			room_a->n_room = room_b;
			break;
		case SOUTH:
			room_a->south = 1;
			room_a->s_room = room_b;
			break;
		case WEST:
			room_a->west = 1;
			room_a->w_room = room_b;
			break;
		case EAST:
			room_a->east = 1;
			room_a->e_room = room_b;
	}
}

struct room *get_room(char *name) {
	struct node *curr = rooms;
	while (curr) {
		if (!strcmp(curr->room->name, name)) {
			return curr->room;
		}
		curr = curr->next;
	}
	return NULL;
}

/* ************************************************** *
 * Run
 * ************************************************** */

void run() {
	start();

	char *command = (char *)malloc(sizeof(char) * MAX_LINE);
	while (scanf("%s", command)) {
		command = strtok(command, "\n");
		act(parse_command(command));
	}

	free(command);
	return;
}

void start() {
	curr_room = get_first_room();
	print_room();
}

struct room *get_first_room() {
	struct node *curr = rooms;
	while (curr->next) {
		curr = curr->next;
	}
	return curr->room;
}

void print_room() {
	printf("\n%s\n", curr_room->name);

	if (curr_room->north) {
		printf(" ---N--- \n");
	} else {
		printf(" ------- \n");
	}

	printf("|       |\n");
	printf("|       |\n");

	if (curr_room->west) {
		printf("W       ");
	} else {
		printf("|       ");
	}

	if (curr_room->east) {
		printf("E\n");
	} else {
		printf("|\n");
	}

	printf("|       |\n");
	printf("|       |\n");

	if (curr_room->south) {
		printf(" ---S--- \n\n");
	} else {
		printf(" ------- \n\n");
	}
}

void act(enum command command) {
	switch (command) {
		case NORTH:
			if (!curr_room->north) { printf("No Path This Way\n"); }
			curr_room = curr_room->north ? curr_room->n_room : curr_room;
			break;
		case SOUTH:
			if (!curr_room->south) { printf("No Path This Way\n"); }
			curr_room = curr_room->south ? curr_room->s_room : curr_room;
			break;
		case WEST:
			if (!curr_room->west) { printf("No Path This Way\n"); }
			curr_room = curr_room->west ? curr_room->w_room : curr_room;
			break;
		case EAST:
			if (!curr_room->east) { printf("No Path This Way\n"); }
			curr_room = curr_room->east ? curr_room->e_room : curr_room;
			break;
		case QUIT:
			quit();
			break;
		case UNDEFINED:
			printf("What?\n");
	}
	print_room();
}

void quit() {
	struct node *curr = rooms;
	struct node *tmp = NULL;

	while (curr) {
		tmp = curr;
		curr = curr->next;
		free(tmp->room);
		free(tmp);
	}
	exit(0);
}

int parse_command(char *command) {
	if (!strcmp(command, "NORTH")) {
		return NORTH;
	}
	if (!strcmp(command, "SOUTH")) {
		return SOUTH;
	}
	if (!strcmp(command, "WEST")) {
		return WEST;
	}
	if (!strcmp(command, "EAST")) {
		return EAST;
	}
	if (!strcmp(command, "QUIT")) {
		return QUIT;
	}
	return UNDEFINED;
}
