#define _GNU_SOURCE /* TODO: For asprintf */
#include "graph.h"
#include "tup/debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dump_node(FILE *f, struct node *n);

struct node *find_node(const struct graph *g, const tupid_t tupid)
{
	struct node *n;

	/* TODO: Use hash */
	list_for_each_entry(n, &g->node_list, list) {
		if(memcmp(n->tupid, tupid, sizeof(n->tupid)) == 0)
			return n;
	}
	list_for_each_entry(n, &g->plist, list) {
		if(memcmp(n->tupid, tupid, sizeof(n->tupid)) == 0)
			return n;
	}
	return NULL;
}

struct node *create_node(const tupid_t tupid)
{
	struct node *n;

	n = malloc(sizeof *n);
	if(!n) {
		perror("malloc");
		return NULL;
	}
	n->edges = NULL;
	n->type = 0;
	memcpy(n->tupid, tupid, sizeof(n->tupid));
	n->incoming_count = 0;
	return n;
}

void remove_node(struct node *n)
{
	list_del(&n->list);
	if(n->edges) {
		DEBUGP("Warning: Node %.*s still has edges.\n", 8, n->tupid);
	}
	/* TODO: block pool */
	free(n);
}

int create_edge(struct node *n1, struct node *n2)
{
	struct edge *e;

	/* TODO: block pool */
	e = malloc(sizeof *e);
	if(!e) {
		perror("malloc");
		return -1;
	}

	e->dest = n2;

	/* TODO: slist add? */
	e->next = n1->edges;
	n1->edges = e;

	n2->incoming_count++;
	return 0;
}

struct edge *remove_edge(struct edge *e)
{
	struct edge *tmp;
	tmp = e->next;
	e->dest->incoming_count--;
	/* TODO: block pool */
	free(e);
	return tmp;
}

int create_graph(struct graph *g, const tupid_t root)
{
	INIT_LIST_HEAD(&g->node_list);
	INIT_LIST_HEAD(&g->plist);

	g->root = create_node(root);
	if(!g->root)
		return -1;
	list_add(&g->root->list, &g->node_list);
	g->num_nodes = 0;
	return 0;
}

void dump_graph(const struct graph *g, const char *filename)
{
	static int count = 0;
	struct node *n;
	char *realfile;
	FILE *f;

	fprintf(stderr, "Dumping graph '%s'\n", filename);
	if(asprintf(&realfile, filename, count) < 0) {
		perror("asprintf");
		return;
	}
	count++;
	f = fopen(realfile, "w");
	if(!f) {
		perror(realfile);
		return;
	}
	fprintf(f, "digraph G {\n");
	list_for_each_entry(n, &g->node_list, list) {
		dump_node(f, n);
	}
	list_for_each_entry(n, &g->plist, list) {
		dump_node(f, n);
	}
	fprintf(f, "}\n");
	fclose(f);
}

static void dump_node(FILE *f, struct node *n)
{
	struct edge *e;
	int color = 0;
	if(n->type & TUP_CREATE)
		color |= 0x00bb00;
	if(n->type & TUP_DELETE)
		color |= 0xff0000;
	if(n->type & TUP_MODIFY)
		color |= 0x0000ff;
	fprintf(f, "tup%.*s [label=\"%.*s (%i)\",color=\"#%06x\"];\n",
		sizeof(tupid_t), n->tupid,
		8, n->tupid,
		n->incoming_count, color);
	/* TODO: slist_for_each? */
	for(e=n->edges; e; e=e->next) {
		fprintf(f, "tup%.*s -> tup%.*s [dir=back];\n",
			sizeof(tupid_t), e->dest->tupid,
			sizeof(tupid_t), n->tupid);
	}
}
