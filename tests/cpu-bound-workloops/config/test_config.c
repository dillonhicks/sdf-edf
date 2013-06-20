#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include "test_config.h"

static thread_info* traverse_config(xmlNode *node, int *count);
static thread_info* get_new_thread_info();
static void set_thread_info(thread_info *ti, xmlNode *node);

//Public:

thread_info* get_test_config(char *filename, int *count)
{
	//stores the parsed document tree
	xmlDoc *doc = NULL;

	//the root node is the head of a document
	xmlNode *root = NULL;

	thread_info *ti_head = NULL;

	/* initialize the library and check for ABI mismatches
	 * between how it was compiled and the actual shared lib
	 * imported
	 */
	LIBXML_TEST_VERSION

	doc = xmlReadFile(filename, NULL, 0);
	*count = 0;
	
	if(doc == NULL)
	{
		printf("error: failed to parse configuration file %s\n", filename);
	}
	else
	{
		//we need the root element from the document tree
		root = xmlDocGetRootElement(doc);

		//if we traverse the root node, we won't get any of our <thread/> items, so we traverse
		//the children instead
		ti_head = traverse_config(root->children, count);
	}

	//free resources libxml2 has allocated throughout the course here
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return ti_head;
}

void free_test_config(thread_info *head)
{
	thread_info *cur = NULL, *next = NULL;
	for(cur = head; cur; cur = next)
	{
		next = cur->next;
		free(cur);
	}
}

//Private:

static thread_info* traverse_config(xmlNode *node, int *count)
{
	thread_info *head = NULL, *ti_cur = NULL;
	xmlNode *cur = NULL;
	for(cur = node; cur; cur = cur->next)
	{
		//for each xmlNode off the root element, if it's a <thread/> element we process it
		if(cur->type == XML_ELEMENT_NODE && xmlStrEqual(cur->name, (const xmlChar*) "thread"))
		{

			//linked list handling code
			if(head == NULL)
			{
				head = get_new_thread_info();
				ti_cur = head;
			}
			else
			{
				ti_cur->next = get_new_thread_info();
				ti_cur = ti_cur->next;
			}

			set_thread_info(ti_cur, cur);
			(*count)++;
		} 
	}
	return head;
}

static thread_info* get_new_thread_info()
{
	thread_info *ti = (thread_info*)malloc(sizeof(thread_info));
	ti->id = 0;
	strcpy(ti->name, "None");
	ti->intervals = 0;
	ti->loop_count = 0;

	ti->next = NULL;

	return ti;
}

static void set_thread_info(thread_info* ti, xmlNode* node)
{
	//check the xmlNode to see if it has all the attributes we're looking for
	//if available, set the proper elements in the struct
	
	//note that xmlGetProp returns an xmlChar* which needs to be free'd with
	//xmlFree()
	if(xmlHasProp(node, (const xmlChar*) "id"))
	{
		xmlChar *t_id = xmlGetProp(node, (const xmlChar*) "id");
		ti->id = atoi(t_id);
		xmlFree(t_id);
	}

	if(xmlHasProp(node, (const xmlChar*) "name"))
	{
		xmlChar *t_name = xmlGetProp(node, (const xmlChar*) "name");
		strcpy(ti->name, t_name);
		xmlFree(t_name);
	}
	else
	{
		//threadName = "T-threadID" if available
	}

	if(xmlHasProp(node, (const xmlChar*) "intervals"))
	{
		xmlChar *t_intervals = xmlGetProp(node, (const xmlChar*) "intervals");
		ti->intervals = atoi(t_intervals);
		xmlFree(t_intervals);
	}

	if(xmlHasProp(node, (const xmlChar*) "loop_count"))
	{
		xmlChar *work_count = xmlGetProp(node, (const xmlChar*) "loop_count");
		ti->loop_count = atoi(work_count);
		xmlFree(work_count);
	} 
}
