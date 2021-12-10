/* virtual file system */

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* command	action
 * -------	------
 *  print	print current working directory and all descendants
 *  mkdir	directory create
 *  rm		delete directory or file
 *  mkfil	file create
 *  exit    quit from program
 */

 /*--------------------------------------------------------------------------------*/

int debug = 0;	// extra output; 1 = on, 0 = off

/*--------------------------------------------------------------------------------*/

int do_print(char* name, char* size);
int do_mkdir(char* name, char* size);
int do_rm(char* name, char* size);
int do_mkfil(char* name, char* size);
int do_exit(char* name, char* size);

struct action {
	char* cmd;					
	int (*action)(char* name, char* size);	// pointer to function
} table[] = {
	{ "print", do_print },
	{ "mkdir", do_mkdir },
	{ "rm"	 , do_rm	},
	{ "mkfil", do_mkfil },
	{ "exit" , do_exit  },
	{ NULL, NULL }	
};

#define LINESIZE			128
#define MAX_STRING_LENGTH	100


typedef struct item_type {
	char name[MAX_STRING_LENGTH];				//Name of file or dir
	char size[MAX_STRING_LENGTH];				//Size of file	
	bool dir_type;								//true if directory, false if file
	struct item_type* next;
	struct item_type* prev;
	struct ptritems_type* ptritems;
} item_type;

typedef struct ptritems_type {
	item_type* item;
	struct ptritems_type* next;
	struct ptritems_type* prev;
} ptritems_type;

item_type* tail = NULL;
item_type* head = NULL;
int level;

item_type* add_item();
ptritems_type* add_ptritems(ptritems_type** ptritems);
item_type* find_name(char* name);
ptritems_type* find_name_ptritems(char* name, ptritems_type* head);
void delete_item(item_type* item);
void delete_links(char* name);
void print_folder(item_type* folder);
void rm_folder(item_type* folder);


void parse(char* buf, int* argc, char* argv[]);
int parse_dir(char* buf, char* name);
int parse_name(char* buf, char* name);


int main(int argc, char* argv[])
{
	(void)argc;
	(void)*argv;
	char in[LINESIZE];
	char* cmd, * fnm, * fsz;
	char dummy[] = "";

	printf("%s", "command	action\n");
	printf("%s", "* ---------------------\n");
	printf("%s", "* print /folder/folder\n");
	printf("%s", "* mkdir /folder/folder\n");
	printf("%s", "* rm	/folder/folder/item\n");
	printf("%s", "* mkfil /folder/file\n");
	printf("%s", "* exit\n");
	printf("%s", "Cannot create an object in a non-existent folder!\n");
	int n;
	char* a[LINESIZE];

	while (fgets(in, LINESIZE, stdin) != NULL)
	{
		// parse in
		parse(in, &n, a);

		cmd = (n > 0) ? a[0] : dummy;
		fnm = (n > 1) ? a[1] : dummy;
		fsz = (n > 2) ? a[2] : dummy;

		if (debug) printf(":%s:%s:%s:\n", cmd, fnm, fsz);

		if (n == 0) continue;

		int found = 0;

		for (struct action* ptr = table; ptr->cmd != NULL; ptr++)
		{
			if (strcmp(ptr->cmd, cmd) == 0)
			{
				found = 1;

				int ret = (ptr->action)(fnm, fsz);
				if (ret == -1)
				{
					printf("  %s %s %s: failed\n", cmd, fnm, fsz);
				}
				break;
			}
		}
		if (!found) { printf("command not found: %s\n", cmd); }
	}

	return 0;
}


int do_mkdir(char* name, char* size)
{
	char top_level_name[MAX_STRING_LENGTH];
	item_type* top_level_folder;
	ptritems_type* ptritems;
	int flg;

	item_type *folder = add_item();
	if (folder == NULL)
	{
		printf("Error\n");
		return 0;
	}

	flg = parse_dir(top_level_name, name);

	// null level
	if (flg == 1)
	{
		if ((find_name(name)) != NULL)
		{
			printf("%s: the object already exists '%s'\n", "mkdir", name);
			return 0;
		}
	}

	strcpy(folder->name, (const char*)name);
	folder->dir_type = true;
	folder->ptritems = NULL;


	if (flg == -1)
	{
		delete_item(folder);
		printf("%s: incorrect command format '%s'\n", "mkdir", name);
		return 0;
	}
	// level == 0
	else if (flg == 1) return 0;

	if (((top_level_folder = find_name(top_level_name)) == NULL) ||
		!top_level_folder->dir_type)
		{
			delete_item(folder);
			printf("%s: cannot find '%s': folder not exists\n", "mkdir", top_level_name);
			return 0;
		}

	if ((find_name_ptritems(name, top_level_folder->ptritems)) != NULL)
	{
		delete_item(folder);
		printf("%s: the object already exists '%s'\n", "mkdir", name);
		return 0;
	}

	ptritems = add_ptritems(&top_level_folder->ptritems);
	if (ptritems != NULL)
	{
		ptritems->item = folder;
	}

	return 0;
}

int do_mkfil(char* name, char* size)
{
	char top_level_name[MAX_STRING_LENGTH];
	item_type* top_level_folder;
	ptritems_type* ptritems;
	bool flg;

	item_type* folder = add_item();
	if (folder == NULL)
	{
		printf("Error\n");
		return 0;
	}

	strcpy(folder->name, (const char*)name);
	strcpy(folder->size, (const char*)size);
	folder->dir_type = false;
	folder->ptritems = NULL;


	flg = parse_dir(top_level_name, name);

	if (flg == -1 || flg == 1)
	{
		delete_item(folder);
		printf("%s: incorrect command format '%s'\n", "mkfil", name);
		return 0;
	}

	if (((top_level_folder = find_name(top_level_name)) == NULL) ||
		!top_level_folder->dir_type)
		{
		delete_item(folder);
		printf("%s: cannot find '%s': folder not exists\n", "mkfil", top_level_name);
		return 0;
	}

	if ((find_name_ptritems(name, top_level_folder->ptritems)) != NULL)
	{
		delete_item(folder);
		printf("%s: the object already exists '%s'\n", "mkdir", name);
		return 0;
	}

	ptritems = add_ptritems(&top_level_folder->ptritems);
	if (ptritems != NULL)
	{
		ptritems->item = folder;
	}

	if (debug) printf(":%s:\n", folder->name);

	return 0;
}


int do_print(char* name, char* size)
{
	item_type* folder;

	if ((folder = find_name(name)) == NULL)
	{
		printf("%s: cannot find '%s': item not exists\n", "print", name);
		return 0;
	}

	level = 0;
	print_folder(folder);

	return 0;
}


void print_folder(item_type* folder)
{
	ptritems_type* ptr;
	char name[MAX_STRING_LENGTH];

	if (parse_name(name, folder->name) == -1)
	{
		return;
	}

	for (int i = 0; i < level; i++)
		printf(" ");
	printf("%s\n", name);

	ptr = folder->ptritems;
	level++;
	while (ptr != NULL)
	{
		print_folder(ptr->item);
		ptr = ptr->next;
	}

	level--;

	return;
}


int do_rm(char* name, char* size)
{
	item_type* folder;

	if ((folder = find_name(name)) == NULL)
	{
		printf("%s: cannot find directory '%s': object not exists\n", "rm", name);
		return 0;
	}

	rm_folder(folder);

	delete_links(folder->name);
	delete_item(folder);

	return 0;
}

void rm_folder(item_type* folder)
{
	ptritems_type* ptr, *tmp;

	ptr = folder->ptritems;
	while (ptr != NULL)
	{
		rm_folder(ptr->item);
		tmp = ptr->next;
		delete_item(ptr->item);
		ptr = tmp;
	}

	return;
}


item_type* add_item()
{
	item_type *tmp;

	item_type* item = malloc(sizeof(item_type));
	if (item == NULL)
	{
		printf("Error\n");
		return NULL;
	}

	if (head == NULL)
	{
		head = item;
		tail = item;
		item->next = NULL;
		item->prev = NULL;
	}
	else
	{
		tmp = tail;
		tmp->next = item;

		tail = item;
		item->next = NULL;
		item->prev = tmp;
	}

	return item;
}

ptritems_type* add_ptritems(ptritems_type** head)
{
	ptritems_type *tmp;

	ptritems_type* ptritems = malloc(sizeof(ptritems_type));
	if (ptritems == NULL)
	{
		printf("Error\n");
		return NULL;
	}

	if (*head == NULL)
	{
		*head = ptritems;
		ptritems->next = NULL;
		ptritems->prev = NULL;
	}
	else
	{
		tmp = *head;
		while (tmp->next != NULL) 
			tmp = tmp->next;

		tmp->next = ptritems;
		ptritems->prev = tmp;
		ptritems->next = NULL;
	}

	return ptritems;
}

void delete_item(item_type* item)
{
	ptritems_type* tmp, *ttmp;

	if (item == head)
	{
		head = item->next;				// first item
		if (head) head->prev = NULL;
		else tail = NULL;				// the only one item
	}
	else
	{
		item->prev->next = item->next;
		if (item->next) item->next->prev = item->prev;
		else tail = item->prev;			// last item
	}

	// delete ptritems
	tmp = item->ptritems;
	if (item->ptritems != NULL)
	{
		ttmp = tmp->next;
		free(tmp);
		tmp = ttmp;
	}

	free(item);
}

void delete_links(char* name)
{
	char top_level_name[MAX_STRING_LENGTH];
	item_type* folder;
	ptritems_type *ptritem, *head;
	bool flg;

	flg = parse_dir(top_level_name, name);
	if ((flg == -1) || (flg == 1))
	{
		return;
	}

	if ((folder = find_name(top_level_name)) == NULL)
	{
		printf("Error\n");
		return;
	}

	if ((ptritem = find_name_ptritems(name, folder->ptritems)) == NULL)
	{
		printf("Error\n");
		return;
	}

	// 
	head = folder->ptritems;
	if (ptritem == head)
	{
		head = ptritem->next;				// first item
		if (head) head->prev = NULL;
	}
	else
	{
		ptritem->prev->next = ptritem->next;
		if (ptritem->next) ptritem->next->prev = ptritem->prev;
	}

	folder->ptritems = head;
	free(ptritem);
}

item_type* find_name(char* name)
{
	item_type* folder = head;

	while (folder != NULL)
	{
		if (strcmp(folder->name, name) == 0)
		{
			return folder;
		}

		folder = folder->next;
	}

	return NULL;
}

ptritems_type* find_name_ptritems(char* name, ptritems_type* head)
{
	ptritems_type* ptritem = head;

	while (ptritem != NULL)
	{
		if (strcmp(ptritem->item->name, name) == 0)
		{
			return ptritem;
		}

		ptritem = ptritem->next;
	}

	return NULL;
}

int parse_dir(char* buf, char* name)
{
	char* delim, * tmp;

	*buf = '\0';

	// not fount '/'
	if ((delim = strrchr((const char*)name, '/')) == NULL)
	{
		return -1;
	}

	//  null level folder
	if (delim == name)
	{
		return 1;
	}

	tmp = name;

	while (tmp != delim)
	{
		*buf++ = *tmp++;
	}

	*buf = '\0';
	return 0;
}

int parse_name(char* buf, char* name)
{
	char* delim, * tmp;    

	if ((delim = strrchr((const char*)name, '/')) == NULL)
	{
		return -1;
	}

	++delim;
	tmp = delim;

	while (*tmp != '\0')
	{
		*buf++ = *tmp++;
	}

	*buf = '\0';

	return 0;
}

void parse(char* buf, int* argc, char* argv[])
{
	char* delim;          
	int count = 0;        

	char whsp[] = " \t\n\v\f\r";          

	while (1)                             
	{
		buf += strspn(buf, whsp);         
		delim = strpbrk(buf, whsp);       
		if (delim == NULL)                
		{
			break;
		}
		argv[count++] = buf;              
		*delim = '\0';                    
		buf = delim + 1;                  
	}
	argv[count] = NULL;

	*argc = count;

	return;
}


int do_exit(char* name, char* size)
{
	(void)*name;
	(void)*size;

	item_type* tmp = head;
	item_type* ttmp;

	while (tmp != NULL)
	{
		ttmp = tmp->next;
		delete_item(tmp);
		tmp = ttmp;
	}

	if (debug) printf("\t[%s]::Exiting\n", __func__);
	exit(0);
	return 0;
}