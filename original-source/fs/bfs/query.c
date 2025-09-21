#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "bfs.h"
#include "btlib/btree.h"

#include "fs_query.h"
#include "match.h"
#ifdef __BEOS__
#include <Mime.h>  /* for B_MIME_STRING_TYPE */
#endif


/* defines for parsing out tokens */
#define OR         '|'
#define AND        '&'
#define NOTHING    '\0'
#define BANG       '!'
#define LPAREN     '('
#define RPAREN     ')'


/* the order of these is important (it makes certains tests much simpler) */
#define OP_EQUAL          1
#define OP_LT_OR_EQUAL    2
#define OP_GT_OR_EQUAL    3
#define OP_NOT_EQUAL      4
#define OP_LT             5
#define OP_GT             6


#define MAX_STACK            64     /* max complexity of an expression */

#define MAX_VALUE_NAME_LEN   256

union value_type {
	char    str_val[MAX_VALUE_NAME_LEN];   /* == old name of str_val */
	int64   int64_val;
	int32   int32_val;
	float   float_val;
	double  double_val;
};

typedef struct query_info query_info;


typedef struct query_piece
{
	query_info       *qi;  /* ptr back to top (for live queries) */

	char             *index_name;
	bfs_inode        *index;
	char			  need_put;
	char              is_time;
	char              is_pattern;
	char              converted;  /* is the value converted to its real type */
	
	int               op;

	union value_type  value;
} query_piece;



typedef struct node
{
    struct node   *lchild,
                  *rchild;
    query_piece   *qp;
    unsigned char  type;
	unsigned char  flags;
} node;

/* for the type field of the node struct */
#define OR_NODE      1
#define AND_NODE     2
#define FACTOR_NODE  3

/* for the flags field of the node struct */
#define VISITED_LEFT  1
#define VISITED_RIGHT 2


struct query_info
{
	bfs_info        *bfs;   /* need this pointer to be able to get indices */
	lock             lock;

	node            *parse_tree;

	int              stack_depth;
	node            *stack[MAX_STACK];
	
    port_id          port;
    int32            token;
	int32            flags;

	int              counter;      /* for re-syncing with an index */
	vnode_id         last_vnid;
	union value_type last_val;

	/* this is a temp field so we don't have to put it on the stack */
    char             fname_buff[B_FILE_NAME_LENGTH];  

	/* another temp field used in compare_file() so we it's not on the stack */
	union value_type  file_val;
};




node *factor(char **str);
node *and_expr(char **str);
node *or_expr(char **str);
void  statements(void);
void  dump_tree(node *top);
void  apply_demorgans_law(node *top);
void  free_tree(node *top);
int   invert_qp_op(int op);
char *qp_op_to_str(int op);
char *skipwhite(char *str);
int   traverse_leaves(node *top, int (*func)(query_piece *qp, void *arg),
					  void *arg);
int   fill_in_index_ptrs(query_piece *qp, void *arg);
int   put_index_ptrs(query_piece *qp, void *arg);
int   remove_qp_callbacks(query_piece *qp, void *arg);
query_piece *get_query_piece(char *str, char **result);

int   seek_down_to_leaf_node(node *top, node **stack, int stacksize);
int   compare_file_against_tree(node *top, bfs_info *bfs,
								query_info *qi, bfs_inode *bi);
int   live_query_update(int type_of_event, bfs_inode *bi,
						int *old_result, void *arg);


int
bfs_open_query(void *ns, const char *_query, ulong flags,
			   port_id port, long token, void **cookie)
{
	int         ret = 0;
	char       *ptr, *query = NULL, *query_base = NULL;
	bfs_info   *bfs = (bfs_info *)ns;
	query_info *qi = NULL;
	
	/* printf("got query: %s\n", _query); */
	if (_query == NULL)
		return EINVAL;

	if ((flags & B_LIVE_QUERY) && port < 0) {
		printf("live query asked for but no port id given (port %d)\n", port);
		return EINVAL;
	}

	query_base = query = strdup(_query);
	if (query == NULL) {
		return EINVAL;
	}

	qi = (query_info *)calloc(sizeof(query_info), 1);
	if (qi == NULL) {
		return ENOMEM;
	}


	qi->parse_tree = or_expr(&query);
	if (qi->parse_tree == NULL) {
		ret = EINVAL;
		goto clean;
	}

	qi->bfs = bfs;

	qi->port  = port;
	qi->token = token;
	qi->flags = flags;

	if (new_lock(&qi->lock, query) != 0) {
		ret = ENOMEM;
		goto clean;
	}
	
	if ((ret = traverse_leaves(qi->parse_tree, fill_in_index_ptrs, qi)) != 0) {
		printf("taversing the leaves of the query failed.\n");
		goto clean;
	}

	qi->stack_depth = seek_down_to_leaf_node(qi->parse_tree,
											 &qi->stack[0],
											 MAX_STACK);
	if (qi->stack_depth < 0) {
		printf("expression too big\n");
		ret = E2BIG;
		goto clean;
	}

	qi->stack_depth--;   /* so it points to the last element, not after it */


	*cookie = qi;

	free(query_base);

	return 0;

 clean:
	if (query_base)
		free(query_base);
	
	if (qi) {
		if (qi->parse_tree) {
			traverse_leaves(qi->parse_tree, put_index_ptrs, bfs);
			free_tree(qi->parse_tree);
		}
		
		if (qi->lock.s > 0) {
			free_lock(&qi->lock);
		}

		free(qi);
	}

	return ret;
}


int
bfs_free_query_cookie(void *ns, void *node, void *cookie)
{
	bfs_info   *bfs = (bfs_info *)ns;
	query_info *qi = (query_info *)cookie;

	if (qi) {
		if (qi->parse_tree) {
			traverse_leaves(qi->parse_tree, put_index_ptrs, bfs);
			free_tree(qi->parse_tree);
		}

		if (qi->lock.s > 0) {
			free_lock(&qi->lock);
		}

		free(qi);
	}

	return 0;
}


int
bfs_close_query(void *ns, void *cookie)
{
	bfs_info   *bfs = (bfs_info *)ns;
	query_info *qi  = (query_info *)cookie;

	if (qi) {

		LOCK(qi->lock);
		if (qi->parse_tree && (qi->flags & B_LIVE_QUERY) && qi->port > 0) {
			traverse_leaves(qi->parse_tree, remove_qp_callbacks, qi);
		}
		UNLOCK(qi->lock);
	}

	return 0;
}

/*
   this function will determine if a given vnid matches the rest of a
   query.  if it does it will fill in the dirent structure.  if the
   file does match it returns zero, anything else is an error.
*/   
static int
vnid_matches_query(bfs_info *bfs, query_info *qi, vnode_id vnid,
				   struct dirent *de, size_t bufsize)
{
	int        err = 0, j;
	node      *node;
	bfs_inode *bi = NULL;
	
	if (get_vnode(bfs->nsid, vnid, (void **)&bi) != 0)
		return ENOENT;
		
	/* dead men don't ever match (because they wear plaid) */
	if (bi->flags & INODE_DELETED) {
		err = ENOENT;
		goto clean;
	}

	node = qi->stack[qi->stack_depth];
	if (node->type != FACTOR_NODE) {
		printf("vnid_matches_query: top of stack node is bad 0x%x\n", node);
		err = EINVAL;
		goto clean;
	}

	for(j=qi->stack_depth-1; j >= 0; j--) {
		node = qi->stack[j];

		if (node->type == OR_NODE) {
			continue;
		} else if (node->type == AND_NODE) {
			if (qi->stack[j+1] == node->lchild) {
				if (compare_file_against_tree(node->rchild, bfs, qi, bi) == 0){
					err = EBADF;
					goto clean;
				}
			} else if (qi->stack[j+1] == node->rchild) {
				if (compare_file_against_tree(node->lchild, bfs, qi, bi) == 0){
					err = EBADF;
					goto clean;
				}
			}
		} else {
			printf("there shouldn't be a factor node here! (0x%x)\n", node);
			err = EINVAL;
			goto clean;
		}
	}
	

	LOCK(bi->etc->lock);
	if (get_name_attr(bfs, bi, &de->d_name[0], bufsize - sizeof(*de))) {
		UNLOCK(bi->etc->lock);
		err = ENOENT;
		goto clean;
	}

	de->d_ino = inode_addr_to_vnode_id(bi->inode_num);
	de->d_reclen = strlen(de->d_name);
	de->d_dev = bfs->nsid;
#ifdef __BEOS__
	de->d_pdev = bfs->nsid;
	de->d_pino = inode_addr_to_vnode_id(bi->parent);
#endif
	UNLOCK(bi->etc->lock);
	
	put_vnode(bfs->nsid, bi->etc->vnid);
	
	return 0;

 clean:
	if (bi) {
		put_vnode(bfs->nsid, vnid);
	}

	return err;
}



static int
match_int(bfs_info *bfs, query_info *qi, vnode_id *vnid_of_match)
{
	int          err = 0, llen = sizeof(int);
	dr9_off_t    res = BT_ANY_RRN;
	query_piece *qp;
	BT_INDEX    *b;
	bfs_inode   *bi  = NULL;
    int32        diff;
	int32        value;

	if (qi->stack[qi->stack_depth] == NULL)
		return EINVAL;
	
	qp = qi->stack[qi->stack_depth]->qp;
	b = qp->index->etc->btree;
	

	value = qp->value.int32_val;

	if (qi->counter == 0) {  /* then we're just getting started */
		if (qp->op == OP_NOT_EQUAL) {
			qp->index->etc->counter++;
			if (bt_goto(b, BT_BOF) != BT_OK) {
				bt_perror(b, "query: read: goto");
				return ENOTDIR;
			}
			qi->counter = qp->index->etc->counter;
		} else {
			qp->index->etc->counter++;
			err = bt_find(b, (bt_chrp)&value, sizeof(value), &res);

			/* printf("looking for 0x%Lx (%Ld), err %d\n", value, value, err);
			   printf("cky %d cflg %d cpag 0x%Lx dup page 0x%Lx dindex %Ld nduppage %Ld\n",
			   b->cky, b->cflg, b->cpag, b->dup_page, b->dup_index, b->num_dup_pages);
			*/
			qi->counter = qp->index->etc->counter;

			if (err == BT_OK) {
				if (qp->op == OP_LT_OR_EQUAL) {
					dr9_off_t save = res;
					
					qp->index->etc->counter++;
					if (bt_gotolastdup(b, &res) != BT_OK)
						res = save;
					qi->counter = qp->index->etc->counter;

				}

				if (qp->op <= OP_GT_OR_EQUAL) {
					*vnid_of_match = res;
					qi->last_val.int32_val = value;
					qi->last_vnid = res;
					qi->counter   = qp->index->etc->counter;
					return 1;
				}
			} else if (err == BT_NF) {
				if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
					qp->index->etc->counter++;
					bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int32_val,
								llen, &llen, (dr9_off_t *)&res);
					qi->counter = qp->index->etc->counter;
				}
			} else if (err != BT_NF && err != BT_BOF && err != BT_EOF) {
				goto bad_tree;
			}
		}
	}


	if (qi->counter != qp->index->etc->counter) {
		qp->index->etc->counter++;
		err = bt_find(b, (bt_chrp)&qi->last_val.int32_val, sizeof(int32),
					  &qi->last_vnid);

		/*
		   if op is a < or <= we need to do two extra traverse 
		   operations to account for the fact that the bt_find puts 
		   the cursor *after* the item we searched for just above.
		   the first traverse moves us to the item we searched for
		   and the second puts us at the item before that (which is
		   what we're looking for).
		*/

		if (err == BT_OK && (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL)) {
			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int32_val,
						llen, &llen, (dr9_off_t *)&res);

			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int32_val,
						llen, &llen, (dr9_off_t *)&res);
		}

		qi->counter = qp->index->etc->counter;

		if (err == BT_BOF || err == BT_EOF)
			return 0;

		if (err != BT_OK && err != BT_NF) {
			goto bad_tree;
		}
	}


	llen = sizeof(int32);
	qp->index->etc->counter++;

	if (qp->op == OP_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int32_val,
						  llen, &llen, (dr9_off_t *)&res);
	} else if (qp->op == OP_NOT_EQUAL ||
			   qp->op == OP_GT || qp->op == OP_GT_OR_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int32_val,
						  llen, &llen, (dr9_off_t *)&res);
		if (err == BT_EOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
			
		if (qp->op == OP_GT || qp->op == OP_NOT_EQUAL) {
			diff = qi->last_val.int32_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int32_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_EOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				diff = qi->last_val.int32_val - value;

				if (has_signals_pending(NULL)) {
					err = EINTR;
					return err;
				}
			}
		}
	} else if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
		err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int32_val,
						  llen, &llen, (dr9_off_t *)&res);

		if (err == BT_BOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
		
		if (qp->op == OP_LT) {
			diff = qi->last_val.int32_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int32_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_BOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				diff = qi->last_val.int32_val - value;
			}
		}
	} else {
		printf("my oh my.  why are we here in query land?\n");
		return EINVAL;
	}

	qi->counter = qp->index->etc->counter;
	qi->last_vnid = (vnode_id)res;

	if (err == BT_EOF || ((qp->op == OP_LT ||
						   qp->op == OP_LT_OR_EQUAL) && err == BT_BOF)) {
		return 0;
	} else if (err != BT_OK) {
		goto bad_tree;
	}

	err = 0;
	
	if (bt_dtype(b) == BT_INT) {
		diff = qi->last_val.int32_val - value;
	} else if (bt_dtype(b) == BT_UINT) {
		diff = (uint32)qi->last_val.int32_val - (uint32)value;
	} else {
		bfs_die("match_int32: data type for btree 0x%x is scrogged\n", b);
	}
	
	if ((qp->op == OP_EQUAL && diff == 0) ||
		(qp->op == OP_NOT_EQUAL && diff != 0) ||
		(qp->op == OP_LT && diff < 0) ||
		(qp->op == OP_LT_OR_EQUAL && diff <= 0) ||
		(qp->op == OP_GT && diff > 0) ||
		(qp->op == OP_GT_OR_EQUAL && diff >= 0)) {

		*vnid_of_match = res;
		err = 1;   /* not really an error; means we have one match */
	}

	return err;

 bad_tree:
	bt_perror(b, "match_int");
	printf("index tree @ 0x%x is munged?!?\n", qp->index);
	return EINVAL;
}


static int
match_float(bfs_info *bfs, query_info *qi, vnode_id *vnid_of_match)
{
	int          err = 0, llen = sizeof(float);
	dr9_off_t    res = BT_ANY_RRN;
	query_piece *qp;
	BT_INDEX    *b;
	bfs_inode   *bi  = NULL;
    float        diff;
	float        value;

	if (qi->stack[qi->stack_depth] == NULL)
		return EINVAL;
	
	qp = qi->stack[qi->stack_depth]->qp;
	b = qp->index->etc->btree;
	

	value = qp->value.float_val;

	if (qi->counter == 0) {  /* then we're just getting started */
		if (qp->op == OP_NOT_EQUAL) {
			qp->index->etc->counter++;
			if (bt_goto(b, BT_BOF) != BT_OK) {
				bt_perror(b, "query: read: goto");
				return ENOTDIR;
			}
			qi->counter = qp->index->etc->counter;
		} else {
			qp->index->etc->counter++;
			err = bt_find(b, (bt_chrp)&value, sizeof(value), &res);

			/* printf("looking for 0x%Lx (%Ld), err %d\n", value, value, err);
			   printf("cky %d cflg %d cpag 0x%Lx dup page 0x%Lx dindex %Ld nduppage %Ld\n",
			   b->cky, b->cflg, b->cpag, b->dup_page, b->dup_index, b->num_dup_pages);
			*/
			qi->counter = qp->index->etc->counter;

			if (err == BT_OK) {
				if (qp->op == OP_LT_OR_EQUAL) {
					dr9_off_t save = res;
					
					qp->index->etc->counter++;
					if (bt_gotolastdup(b, &res) != BT_OK)
						res = save;
					qi->counter = qp->index->etc->counter;

				}

				if (qp->op <= OP_GT_OR_EQUAL) {
					*vnid_of_match = res;
					qi->last_val.float_val = value;
					qi->last_vnid = res;
					qi->counter   = qp->index->etc->counter;
					return 1;
				}
			} else if (err == BT_NF) {
				if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
					qp->index->etc->counter++;
					bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.float_val,
								llen, &llen, (dr9_off_t *)&res);
					qi->counter = qp->index->etc->counter;
				}
			} else if (err != BT_NF && err != BT_BOF && err != BT_EOF) {
				goto bad_tree;
			}
		}
	}


	if (qi->counter != qp->index->etc->counter) {
		qp->index->etc->counter++;
		err = bt_find(b, (bt_chrp)&qi->last_val.float_val, sizeof(float),
					  &qi->last_vnid);

		/*
		   if op is a < or <= we need to do two extra traverse 
		   operations to account for the fact that the bt_find puts 
		   the cursor *after* the item we searched for just above.
		   the first traverse moves us to the item we searched for
		   and the second puts us at the item before that (which is
		   what we're looking for).
		*/

		if (err == BT_OK && (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL)) {
			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.float_val,
						llen, &llen, (dr9_off_t *)&res);

			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.float_val,
						llen, &llen, (dr9_off_t *)&res);
		}

		qi->counter = qp->index->etc->counter;

		if (err == BT_BOF || err == BT_EOF)
			return 0;

		if (err != BT_OK && err != BT_NF) {
			goto bad_tree;
		}
	}


	llen = sizeof(float);
	qp->index->etc->counter++;

	if (qp->op == OP_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.float_val,
						  llen, &llen, (dr9_off_t *)&res);
	} else if (qp->op == OP_NOT_EQUAL ||
			   qp->op == OP_GT || qp->op == OP_GT_OR_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.float_val,
						  llen, &llen, (dr9_off_t *)&res);
		if (err == BT_EOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
			
		if (qp->op == OP_GT || qp->op == OP_NOT_EQUAL) {
			diff = qi->last_val.float_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.float_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_EOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				diff = qi->last_val.float_val - value;

				if (has_signals_pending(NULL)) {
					err = EINTR;
					return err;
				}
			}
		}
	} else if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
		err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.float_val,
						  llen, &llen, (dr9_off_t *)&res);

		if (err == BT_BOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
		
		if (qp->op == OP_LT) {
			diff = qi->last_val.float_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.float_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_BOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				diff = qi->last_val.float_val - value;
			}
		}
	} else {
		printf("my oh my.  why are we here in query land?\n");
		return EINVAL;
	}

	qi->counter = qp->index->etc->counter;
	qi->last_vnid = (vnode_id)res;

	if (err == BT_EOF || ((qp->op == OP_LT ||
						   qp->op == OP_LT_OR_EQUAL) && err == BT_BOF)) {
		return 0;
	} else if (err != BT_OK) {
		goto bad_tree;
	}

	err = 0;
	
	if (bt_dtype(b) == BT_FLOAT) {
		diff = qi->last_val.float_val - value;
	} else {
		bfs_die("match_float: data type for btree 0x%x is scrogged\n", b);
	}
	
	if ((qp->op == OP_EQUAL && diff == 0) ||
		(qp->op == OP_NOT_EQUAL && diff != 0) ||
		(qp->op == OP_LT && diff < 0) ||
		(qp->op == OP_LT_OR_EQUAL && diff <= 0) ||
		(qp->op == OP_GT && diff > 0) ||
		(qp->op == OP_GT_OR_EQUAL && diff >= 0)) {

		*vnid_of_match = res;
		err = 1;   /* not really an error; means we have one match */
	}

	return err;

 bad_tree:
	bt_perror(b, "match_float");
	printf("index tree @ 0x%x is munged?!?\n", qp->index);
	return EINVAL;
}



static int
match_double(bfs_info *bfs, query_info *qi, vnode_id *vnid_of_match)
{
	int          err = 0, llen = sizeof(double);
	dr9_off_t    res = BT_ANY_RRN;
	query_piece *qp;
	BT_INDEX    *b;
	bfs_inode   *bi  = NULL;
    double       diff;
	double       value;

	if (qi->stack[qi->stack_depth] == NULL)
		return EINVAL;
	
	qp = qi->stack[qi->stack_depth]->qp;
	b = qp->index->etc->btree;
	

	value = qp->value.double_val;

	if (qi->counter == 0) {  /* then we're just getting started */
		if (qp->op == OP_NOT_EQUAL) {
			qp->index->etc->counter++;
			if (bt_goto(b, BT_BOF) != BT_OK) {
				bt_perror(b, "query: read: goto");
				return ENOTDIR;
			}
			qi->counter = qp->index->etc->counter;
		} else {
			qp->index->etc->counter++;
			err = bt_find(b, (bt_chrp)&value, sizeof(value), &res);

			/* printf("looking for 0x%Lx (%Ld), err %d\n", value, value, err);
			   printf("cky %d cflg %d cpag 0x%Lx dup page 0x%Lx dindex %Ld nduppage %Ld\n",
			   b->cky, b->cflg, b->cpag, b->dup_page, b->dup_index, b->num_dup_pages);
			*/
			qi->counter = qp->index->etc->counter;

			if (err == BT_OK) {
				if (qp->op == OP_LT_OR_EQUAL) {
					dr9_off_t save = res;
					
					qp->index->etc->counter++;
					if (bt_gotolastdup(b, &res) != BT_OK)
						res = save;
					qi->counter = qp->index->etc->counter;

				}

				if (qp->op <= OP_GT_OR_EQUAL) {
					*vnid_of_match = res;
					qi->last_val.double_val = value;
					qi->last_vnid = res;
					qi->counter   = qp->index->etc->counter;
					return 1;
				}
			} else if (err == BT_NF) {
				if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
					qp->index->etc->counter++;
					bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.double_val,
								llen, &llen, (dr9_off_t *)&res);
					qi->counter = qp->index->etc->counter;
				}
			} else if (err != BT_NF && err != BT_BOF && err != BT_EOF) {
				goto bad_tree;
			}
		}
	}


	if (qi->counter != qp->index->etc->counter) {
		qp->index->etc->counter++;
		err = bt_find(b, (bt_chrp)&qi->last_val.double_val, sizeof(double),
					  &qi->last_vnid);

		/*
		   if op is a < or <= we need to do two extra traverse 
		   operations to account for the fact that the bt_find puts 
		   the cursor *after* the item we searched for just above.
		   the first traverse moves us to the item we searched for
		   and the second puts us at the item before that (which is
		   what we're looking for).
		*/

		if (err == BT_OK && (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL)) {
			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.double_val,
						llen, &llen, (dr9_off_t *)&res);

			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.double_val,
						llen, &llen, (dr9_off_t *)&res);
		}

		qi->counter = qp->index->etc->counter;

		if (err == BT_BOF || err == BT_EOF)
			return 0;

		if (err != BT_OK && err != BT_NF) {
			goto bad_tree;
		}
	}


	llen = sizeof(double);
	qp->index->etc->counter++;

	if (qp->op == OP_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.double_val,
						  llen, &llen, (dr9_off_t *)&res);
	} else if (qp->op == OP_NOT_EQUAL ||
			   qp->op == OP_GT || qp->op == OP_GT_OR_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.double_val,
						  llen, &llen, (dr9_off_t *)&res);
		if (err == BT_EOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
			
		if (qp->op == OP_GT || qp->op == OP_NOT_EQUAL) {
			diff = qi->last_val.double_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.double_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_EOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				diff = qi->last_val.double_val - value;

				if (has_signals_pending(NULL)) {
					err = EINTR;
					return err;
				}
			}
		}
	} else if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
		err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.double_val,
						  llen, &llen, (dr9_off_t *)&res);

		if (err == BT_BOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
		
		if (qp->op == OP_LT) {
			diff = qi->last_val.double_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.double_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_BOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				diff = qi->last_val.double_val - value;
			}
		}
	} else {
		printf("my oh my.  why are we here in query land?\n");
		return EINVAL;
	}

	qi->counter = qp->index->etc->counter;
	qi->last_vnid = (vnode_id)res;

	if (err == BT_EOF || ((qp->op == OP_LT ||
						   qp->op == OP_LT_OR_EQUAL) && err == BT_BOF)) {
		return 0;
	} else if (err != BT_OK) {
		goto bad_tree;
	}

	err = 0;
	
	if (bt_dtype(b) == BT_DOUBLE) {
		diff = qi->last_val.double_val - value;
	} else {
		bfs_die("match_double: data type for btree 0x%x is scrogged\n", b);
	}
	
	if ((qp->op == OP_EQUAL && diff == 0) ||
		(qp->op == OP_NOT_EQUAL && diff != 0) ||
		(qp->op == OP_LT && diff < 0) ||
		(qp->op == OP_LT_OR_EQUAL && diff <= 0) ||
		(qp->op == OP_GT && diff > 0) ||
		(qp->op == OP_GT_OR_EQUAL && diff >= 0)) {

		*vnid_of_match = res;
		err = 1;   /* not really an error; means we have one match */
	}

	return err;

 bad_tree:
	bt_perror(b, "match_double");
	printf("index tree @ 0x%x is munged?!?\n", qp->index);
	return EINVAL;
}



static int
match_long_long(bfs_info *bfs, query_info *qi, vnode_id *vnid_of_match)
{
	int          err = 0, llen = sizeof(long long);
	dr9_off_t    res = BT_ANY_RRN;
	query_piece *qp;
	BT_INDEX    *b;
	bfs_inode   *bi  = NULL;
	long long    diff;
	unsigned long long  value;

	if (qi->stack[qi->stack_depth] == NULL)
		return EINVAL;
	
	qp = qi->stack[qi->stack_depth]->qp;
	b = qp->index->etc->btree;
	

	value = qp->value.int64_val;

	if (qp->is_time)
		value <<= TIME_SCALE;

	if (qi->counter == 0) {  /* then we're just getting started */
		if (qp->op == OP_NOT_EQUAL) {
			qp->index->etc->counter++;
			if (bt_goto(b, BT_BOF) != BT_OK) {
				bt_perror(b, "query: read: goto");
				return ENOTDIR;
			}
			qi->counter = qp->index->etc->counter;
		} else {
			qp->index->etc->counter++;
			err = bt_find(b, (bt_chrp)&value, sizeof(value), &res);

			/* printf("looking for 0x%Lx (%Ld), err %d\n", value, value, err);
			   printf("cky %d cflg %d cpag 0x%Lx dup page 0x%Lx dindex %Ld nduppage %Ld\n",
			   b->cky, b->cflg, b->cpag, b->dup_page, b->dup_index, b->num_dup_pages);
			*/
			qi->counter = qp->index->etc->counter;

			if (err == BT_OK) {
				if (qp->op == OP_LT_OR_EQUAL) {
					dr9_off_t save = res;
					
					qp->index->etc->counter++;
					if (bt_gotolastdup(b, &res) != BT_OK)
						res = save;
					qi->counter = qp->index->etc->counter;

				}

				if (qp->op <= OP_GT_OR_EQUAL) {
					*vnid_of_match = res;
					qi->last_val.int64_val = value;
					qi->last_vnid = res;
					qi->counter   = qp->index->etc->counter;
					return 1;
				}
			} else if (qp->is_time && err == BT_NF) {  /* then do the fudge */
				qp->index->etc->counter++;
				if (qp->op == OP_LT) {
					err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int64_val,
									  llen, &llen, (dr9_off_t *)&res);

					err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int64_val,
									  llen, &llen, (dr9_off_t *)&res);
					
				} else {
					err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int64_val,
									  llen, &llen, (dr9_off_t *)&res);
				}
				/* printf("trying to get the current entry (err %d val 0x%Lx %Ld)\n", err,
				   qi->last_val.int64_val, qi->last_val.int64_val); */
				qi->counter = qp->index->etc->counter;
				
				if (err == BT_EOF) {
					return 0;
				} else if (err != BT_OK) {
					goto bad_tree;
				}

				diff = (qi->last_val.int64_val & ~TIME_MASK) - value;
				if ((qp->op == OP_EQUAL && diff == 0) ||
					(qp->op == OP_LT && diff < 0) ||
					(qp->op == OP_LT_OR_EQUAL && diff <= 0) ||
					(qp->op == OP_GT && diff > 0) ||
					(qp->op == OP_GT_OR_EQUAL && diff >= 0)) {
					*vnid_of_match = res;
					qi->last_vnid = res;
					return 1;
				}
			} else if (err == BT_NF) {
				if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
					qp->index->etc->counter++;
					bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int64_val,
								llen, &llen, (dr9_off_t *)&res);
					qi->counter = qp->index->etc->counter;
				}
			} else if (err != BT_NF && err != BT_BOF && err != BT_EOF) {
				goto bad_tree;
			}
		}
	}


	if (qi->counter != qp->index->etc->counter) {
		qp->index->etc->counter++;
		err = bt_find(b, (bt_chrp)&qi->last_val.int64_val, sizeof(long long),
					  &qi->last_vnid);

		/*
		   if op is a < or <= we need to do two extra traverse 
		   operations to account for the fact that the bt_find puts 
		   the cursor *after* the item we searched for just above.
		   the first traverse moves us to the item we searched for
		   and the second puts us at the item before that (which is
		   what we're looking for).
		*/

		if (err == BT_OK && (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL)) {
			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int64_val,
						llen, &llen, (dr9_off_t *)&res);

			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int64_val,
						llen, &llen, (dr9_off_t *)&res);
		}

		qi->counter = qp->index->etc->counter;

		if (err == BT_BOF || err == BT_EOF)
			return 0;

		if (err != BT_OK && err != BT_NF) {
			goto bad_tree;
		}
	}


	llen = sizeof(long long);
	qp->index->etc->counter++;

	if (qp->op == OP_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int64_val,
						  llen, &llen, (dr9_off_t *)&res);
	} else if (qp->op == OP_NOT_EQUAL ||
			   qp->op == OP_GT || qp->op == OP_GT_OR_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int64_val,
						  llen, &llen, (dr9_off_t *)&res);
		if (err == BT_EOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
			
		if (qp->op == OP_GT || qp->op == OP_NOT_EQUAL) {
			if (qp->is_time)
				diff = (qi->last_val.int64_val & ~TIME_MASK) - value;
			else
				diff = qi->last_val.int64_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int64_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_EOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				if (qp->is_time)
					diff = (qi->last_val.int64_val & ~TIME_MASK) - value;
				else
					diff = qi->last_val.int64_val - value;

				if (has_signals_pending(NULL)) {
					err = EINTR;
					return err;
				}
			}
		}
	} else if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
		err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int64_val,
						  llen, &llen, (dr9_off_t *)&res);

		if (err == BT_BOF)
			return 0;
		else if (err != BT_OK) {
			goto bad_tree;
		}
		
		if (qp->op == OP_LT) {
			if (qp->is_time)
				diff = (qi->last_val.int64_val & ~TIME_MASK) - value;
			else
				diff = qi->last_val.int64_val - value;

			while (diff == 0) {
				err = bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.int64_val,
								  llen, &llen, (dr9_off_t *)&res);
				if (err == BT_BOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				if (qp->is_time)
					diff = (qi->last_val.int64_val & ~TIME_MASK) - value;
				else
					diff = qi->last_val.int64_val - value;
			}
		}
	} else {
		printf("my oh my.  why are we here in query land?\n");
		return EINVAL;
	}

	qi->counter = qp->index->etc->counter;
	qi->last_vnid = (vnode_id)res;

	if (err == BT_EOF || ((qp->op == OP_LT ||
						   qp->op == OP_LT_OR_EQUAL) && err == BT_BOF)) {
		return 0;
	} else if (err != BT_OK) {
		goto bad_tree;
	}

	err = 0;
	
	if (qp->is_time) {
		diff = (qi->last_val.int64_val & ~TIME_MASK) - value;
	} else if (bt_dtype(b) == BT_LONG_LONG) {
		diff = qi->last_val.int64_val - value;
	} else if (bt_dtype(b) == BT_ULONG_LONG) {
		diff = (uint64)qi->last_val.int64_val - (uint64)value;
	} else {
		bfs_die("match_ll: data type for btree 0x%x is scrogged\n", b);
	}
	
	if ((qp->op == OP_EQUAL && diff == 0) ||
		(qp->op == OP_NOT_EQUAL && diff != 0) ||
		(qp->op == OP_LT && diff < 0) ||
		(qp->op == OP_LT_OR_EQUAL && diff <= 0) ||
		(qp->op == OP_GT && diff > 0) ||
		(qp->op == OP_GT_OR_EQUAL && diff >= 0)) {

		*vnid_of_match = res;
		err = 1;   /* not really an error; means we have one match */
	}

	return err;

 bad_tree:
	bt_perror(b, "match_long_long");
	printf("index tree @ 0x%x is munged?!?\n", qp->index);
	return EINVAL;
}


static int
do_pattern_match_string(bfs_info *bfs, query_info *qi, vnode_id *vnid_of_match)
{
	int          err = 0, llen = 0, got_match;
	dr9_off_t    res;
	BT_INDEX    *b;
	query_piece *qp;
	bfs_inode   *bi  = NULL;
	char        *value;
	
	if (qi->stack[qi->stack_depth] == NULL)
		return EINVAL;
	
	qp = qi->stack[qi->stack_depth]->qp;
	b = qp->index->etc->btree;
	
	value = (char *)qp->value.str_val;

	if (qi->counter == 0) {
		qp->index->etc->counter++;
		if (bt_goto(b, BT_BOF) != BT_OK) {
			bt_perror(b, "query: read: goto");
			return ENOTDIR;
		}

		qi->counter = qp->index->etc->counter;
	}

	if (qi->counter != qp->index->etc->counter) {
		qp->index->etc->counter++;
		err = bt_find(b, (bt_chrp)qi->last_val.str_val, strlen(qi->last_val.str_val),
					  &qi->last_vnid);
		/*
		   if op is a < or <= we need to do two extra traverse 
		   operations to account for the fact that the bt_find puts 
		   the cursor *after* the item we searched for just above.
		   the first traverse moves us to the item we searched for
		   and the second puts us at the item before that (which is
		   what we're looking for).
		*/

		if (err == BT_OK && (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL)) {
			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.str_val,
						sizeof(qi->last_val.str_val),&llen, (dr9_off_t *)&res);

			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.str_val,
						sizeof(qi->last_val.str_val),&llen, (dr9_off_t *)&res);
		}

		if (err != BT_OK && err != BT_NF) {
			goto bad_tree;
		}
		qi->counter = qp->index->etc->counter;
	}


	while(1) {
		qp->index->etc->counter++;
		err = bt_traverse(b, BT_EOF, (bt_chrp)qi->last_val.str_val, 
						  sizeof(qi->last_val.str_val), &llen, (dr9_off_t *)&res);
		qi->counter = qp->index->etc->counter;
		qi->last_vnid = (vnode_id)res;

		if (err == BT_EOF) {
			return 0;
		} else if (err != BT_OK) {
			goto bad_tree;
		}

		qi->last_val.str_val[llen] = '\0';

		got_match = match((uchar *)value, (uchar *)qi->last_val.str_val);

		if ((got_match && qp->op == OP_EQUAL) ||
			(got_match == 0 && qp->op == OP_NOT_EQUAL)) {
			*vnid_of_match = res;
			return 1;
		}

		if (has_signals_pending(NULL)) {
			err = EINTR;
			return err;
		}
	}

	return 0;

 bad_tree:
	bt_perror(b, "pattern_match_string");
	printf("index tree @ 0x%x is munged?!?\n", qp->index);
	return EINVAL;
}


static int
match_string(bfs_info *bfs, query_info *qi, vnode_id *vnid_of_match)
{
	int          err = 0, llen = 0, cmp;
	dr9_off_t    res = BT_ANY_RRN;
	BT_INDEX    *b;
	bfs_inode   *bi  = NULL;
	query_piece *qp;
	char        *value;

	if (qi->stack[qi->stack_depth] == NULL)
		return EINVAL;
	
	qp = qi->stack[qi->stack_depth]->qp;
	b = qp->index->etc->btree;
	

	if (qp->is_pattern) {
		return do_pattern_match_string(bfs, qi, vnid_of_match);
	}

	value = (char *)qp->value.str_val;

	if (qi->counter == 0) {  /* then we're just getting started */
		if (qp->op == OP_NOT_EQUAL) {
			qp->index->etc->counter++;
			if (bt_goto(b, BT_BOF) != BT_OK) {
				bt_perror(b, "query: read: goto");
				return ENOTDIR;
			}
			qi->counter = qp->index->etc->counter;
		} else {
			qp->index->etc->counter++;
			err = bt_find(b, (bt_chrp)value, strlen(value), &res);
			qi->counter = qp->index->etc->counter;

			if (err == BT_OK) {
				if (qp->op == OP_LT_OR_EQUAL) {
					dr9_off_t save = res;
					
					qp->index->etc->counter++;
					if (bt_gotolastdup(b, &res) != BT_OK)
						res = save;
					qi->counter = qp->index->etc->counter;

				}

				if (qp->op <= OP_GT_OR_EQUAL) {
					*vnid_of_match = res;
					strcpy(qi->last_val.str_val, value);
					qi->last_vnid = res;
					return 1;
				}
			} else if (err == BT_NF) {
				if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
					qp->index->etc->counter++;
					bt_traverse(b, BT_EOF, (bt_chrp)&qi->last_val.int64_val,
								llen, &llen, (dr9_off_t *)&res);
					qi->counter = qp->index->etc->counter;
				}
			} else if (err != BT_NF && err != BT_BOF && err != BT_EOF) {
				goto bad_tree;
			}
		}
	}


	if (qi->counter != qp->index->etc->counter) {
		qp->index->etc->counter++;
		err = bt_find(b, (bt_chrp)qi->last_val.str_val, strlen(qi->last_val.str_val),
					  &qi->last_vnid);

		/*
		   if op is a < or <= we need to do two extra traverse 
		   operations to account for the fact that the bt_find puts 
		   the cursor *after* the item we searched for just above.
		   the first traverse moves us to the item we searched for
		   and the second puts us at the item before that (which is
		   what we're looking for).
		*/

		if (err == BT_OK && (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL)) {
			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.str_val,
						sizeof(qi->last_val.str_val),&llen, (dr9_off_t *)&res);

			qp->index->etc->counter++;
			bt_traverse(b, BT_BOF, (bt_chrp)&qi->last_val.str_val,
						sizeof(qi->last_val.str_val),&llen, (dr9_off_t *)&res);
		}

		qi->counter = qp->index->etc->counter;

		if (err == BT_BOF || err == BT_EOF)
			return 0;

		if (err != BT_OK && err != BT_NF) {
			goto bad_tree;
		}
	}

	qp->index->etc->counter++;

	if (qp->op == OP_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)qi->last_val.str_val, 
						  sizeof(qi->last_val.str_val), &llen, (dr9_off_t *)&res);
		
		qi->last_val.str_val[llen] = '\0';
	} else if (qp->op == OP_NOT_EQUAL ||
			   qp->op == OP_GT || qp->op == OP_GT_OR_EQUAL) {
		err = bt_traverse(b, BT_EOF, (bt_chrp)qi->last_val.str_val,
						  sizeof(qi->last_val.str_val), &llen, (dr9_off_t *)&res);

		if (err == BT_EOF) {
			return 0;
		} else if (err != BT_OK) {
			goto bad_tree;
		}
		
		qi->last_val.str_val[llen] = '\0';

		if (qp->op == OP_GT || qp->op == OP_NOT_EQUAL) {
			while (strcmp(qi->last_val.str_val, value) == 0) {
				err = bt_traverse(b, BT_EOF, (bt_chrp)qi->last_val.str_val,
								  sizeof(qi->last_val.str_val), &llen,
								  (dr9_off_t *)&res);
				if (err == BT_EOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				qi->last_val.str_val[llen] = '\0';

				if (has_signals_pending(NULL)) {
					err = EINTR;
					return err;
				}
			}
		}
	} else if (qp->op == OP_LT || qp->op == OP_LT_OR_EQUAL) {
		err = bt_traverse(b, BT_BOF, (bt_chrp)qi->last_val.str_val,
						  sizeof(qi->last_val.str_val), &llen, (dr9_off_t *)&res);

		if (err == BT_BOF) {
			return 0;
		} else if (err != BT_OK) {
			goto bad_tree;
		}
		
		qi->last_val.str_val[llen] = '\0';

		if (qp->op == OP_LT) {
			while (strcmp(qi->last_val.str_val, value) == 0) {
				err = bt_traverse(b, BT_BOF, (bt_chrp)qi->last_val.str_val,
								  sizeof(qi->last_val.str_val), &llen,
								  (dr9_off_t *)&res);
				if (err == BT_BOF)
					return 0;
				else if (err != BT_OK) {
					goto bad_tree;
				}

				qi->last_val.str_val[llen] = '\0';
			}
		}
	} else {
		printf("in the g-string area of match-string.  watch out.\n");
		return EINVAL;
	}

	qi->counter = qp->index->etc->counter;
	qi->last_vnid = (vnode_id)res;

	if (err == BT_EOF || ((qp->op == OP_LT ||
						   qp->op == OP_LT_OR_EQUAL) && err == BT_BOF)) {
		return 0;
	} else if (err != BT_OK) {
		goto bad_tree;
	}

	qi->last_val.str_val[llen] = '\0';

	err = 0;

	cmp = strcmp(qi->last_val.str_val, value);

	if ((qp->op == OP_EQUAL && cmp == 0) ||
		(qp->op == OP_NOT_EQUAL && cmp != 0) ||
		(qp->op == OP_LT && cmp < 0) ||
		(qp->op == OP_LT_OR_EQUAL && cmp <= 0) ||
		(qp->op == OP_GT && cmp > 0) ||
		(qp->op == OP_GT_OR_EQUAL && cmp >= 0)) {

		*vnid_of_match = res;
		err = 1;   /* not really an error; means we have one match */
	}

	return err;
	
 bad_tree:
	bt_perror(b, "match_string");
	printf("index tree @ 0x%x is munged?!?\n", qp->index);
	return EINVAL;
}




int
bfs_read_query(void *ns, void *cookie, long *num,
			   struct dirent *de, size_t bufsize)
{
	int          err = 0, tmp, res;
	node        *node;
	bfs_info    *bfs = (bfs_info *)ns;
	vnode_id     vnid_of_match;
	query_info  *qi  = (query_info *)cookie;
	query_piece *qp;
	BT_INDEX    *b;

	LOCK(qi->lock);

	while (qi->stack_depth >= 0) {
		node = qi->stack[qi->stack_depth];
		if (node == NULL) {
			printf("bad qi (0x%x).  no node at top of stack?\n", qi);
			*num = 0;
			err  = EINVAL;
			break;
		}

		if (node->type != FACTOR_NODE) {
			printf("how bizarre. node @ 0x%x is not a factor node\n", node);
			*num = 0;
			err  = EINVAL;
			break;
		}

		qp = node->qp;
		if (qp->index == NULL) {
			printf("searching on non-indexed fields (%s) not supported yet\n",
				   qp->index_name);
			*num = 0;
			err  = EINVAL;
			break;
		}
	

		b = (BT_INDEX *)qp->index->etc->btree;
		if (b == NULL) {
			printf("null b-tree for index? (0x%x)\n", qp->index);
			*num = 0;
			err  = ENOENT;
			break;
		}

	find_next:
		LOCK(qp->index->etc->lock);

		do {
			if (bt_dtype(b) == BT_STRING) {
				err = match_string(bfs, qi, &vnid_of_match);
			} else if (bt_dtype(b) == BT_INT || bt_dtype(b) == BT_UINT) {
				err = match_int(bfs, qi, &vnid_of_match);
			} else if (bt_dtype(b) == BT_LONG_LONG || bt_dtype(b) == BT_ULONG_LONG) {
				err = match_long_long(bfs, qi, &vnid_of_match);
			} else if (bt_dtype(b) == BT_FLOAT) {
				err = match_float(bfs, qi, &vnid_of_match);
			} else if (bt_dtype(b) == BT_DOUBLE) {
				err = match_double(bfs, qi, &vnid_of_match);
			} else {
				err = EINVAL;
			}
		} while(err == ENOENT);
			
		UNLOCK(qp->index->etc->lock);

		if (has_signals_pending(NULL)) {
			*num = 0;
			err  = EINTR;
			break;
		}

		if (err > 0) {
			if (vnid_matches_query(bfs, qi, vnid_of_match, de, bufsize) != 0)
				goto find_next;

			/* if we get here the file matches the query and we return it */
			*num = 1;
			err  = 0;
			break;
		}

		if (qi->stack[qi->stack_depth]->type == FACTOR_NODE)
			qi->stack_depth--;

		/* pop off all intervening AND nodes */
		while (qi->stack_depth >= 0 &&
			   qi->stack[qi->stack_depth]->type == AND_NODE)
			qi->stack_depth--;

		if (qi->stack_depth < 0) {   /* all done, go home */
			break;
		}

		while(qi->stack_depth >= 0) {
			tmp = qi->stack[qi->stack_depth]->flags & (VISITED_LEFT|VISITED_RIGHT);
			if (tmp == (VISITED_LEFT|VISITED_RIGHT) ||
				qi->stack[qi->stack_depth]->type == AND_NODE)
				qi->stack_depth--;
			else
				break;
		}
		
		if (qi->stack_depth >= 0) {
			res = seek_down_to_leaf_node(qi->stack[qi->stack_depth],
										 &qi->stack[qi->stack_depth],
										 MAX_STACK - qi->stack_depth);
			if (res < 0) {
				printf("2: expression too big (%d %d)\n", MAX_STACK,
					   qi->stack_depth);
				err  = E2BIG;
				*num = 0;
				break;
			}


			if (res + qi->stack_depth >= MAX_STACK) {
				printf("3: expression too big (%d %d)\n", MAX_STACK,
					   res + qi->stack_depth);
				err  = E2BIG;
				*num = 0;
				break;
			}
					
			qi->stack_depth += res;
			qi->stack_depth--;      /* point at the node, not above it */

			qi->counter             = 0;  /* reset index pointers */
			qi->last_vnid           = 0;
			qi->last_val.double_val = 0;
		}

	}

	if (qi->stack_depth < 0) {   /* must be eof, mark it as such */
		*num = 0;
		err  = 0;
	}

	UNLOCK(qi->lock);

	return err;
}



/* first, two little utility functions */
char *
skipwhite(char *str)
{
    if (str == NULL)
        return NULL;
  
    while(*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\f'))
        str++;
    
    return str;
}

#define DOUBLE_QUOTE  '"'
#define SINGLE_QUOTE  '\''
#define BACK_SLASH    '\\'

/*
   this is just a utility function that will strip off quotes and
   handle escaped characters.  the conversion is done in place.
   the return value is the number of characters in the input string
   that we skipped over.
*/   
static int
get_quoted_string(char *str)
{
	int   count = 0;
	char *start = str;
	
	/* skip intervening white space */
	while(*str != '\0' && (*str == ' ' || *str == '\t' || *str == '\n'))
	    str++;

	count += str - start;

	if (*str == '\0')
		return count;

	if (*str == DOUBLE_QUOTE) {
		strcpy(str, str+1);
		count++;

	    while(*str && *str != DOUBLE_QUOTE) {
			if (*str == BACK_SLASH) {
				strcpy(str, str+1);  /* copy everything down */
				count++;
			}

			str++;
			count++;
	    }

		if (*str != '\0') {
			*str++ = '\0';   /* chop the string */
			count++;
		}
	} else if (*str == SINGLE_QUOTE) {  
		strcpy(str, str+1);
		count++;

	    while(*str && *str != SINGLE_QUOTE) {
			if (*str == BACK_SLASH) {
				strcpy(str, str+1);  /* copy everything down */
				count++;
			}
			
			str++;
			count++;
	    }

		if (*str != '\0') {
			*str++ = '\0';   /* chop the string */
			count++;
		}
	} else {
	    start = str;
	    while(*str && *str != ' ' && *str != '\t' && *str != '\n' &&
			  *str != '(' && *str != ')' && *str != '&' && *str != '|' &&
			  *str != '!' && *str != '<' && *str != '>' && *str != '=') {
			if (*str == BACK_SLASH) {
				strcpy(str, str+1);  /* copy everything down */
				count++;
			}
			
			str++;
			count++;
	    }

		if (*str != '\0') {
			*str++ = '\0';   /* chop the string */
			/* don't bump count since whoever called us will want this char */
		}
	}

	return count;
}



node *
or_expr(char **str)
{
    node *lchild, *rchild, *me = NULL, *top = NULL, *tmp;
  
    *str = skipwhite(*str);

    lchild = and_expr(str);
	if (lchild == NULL)
		return NULL;

    *str = skipwhite(*str);
    while(**str == OR) {
		*str = *str + 1;
		if (**str != OR) {
			/* expected another | for an OR operator */
			free_tree(lchild);
			return NULL;
		}
         
        *str = skipwhite(*str + 1);   /* advance over the operator */
       
        rchild = and_expr(str);
		if (rchild == NULL) {
			if (top)
				free_tree(top);
			else
				free_tree(lchild);

			return NULL;
		}

		if (me == NULL) {
			me = (node *)calloc(sizeof(*me), 1);
			if (me == NULL) {
				free_tree(lchild);
				free_tree(rchild);

				return NULL;
			}

			top = me;
			me->lchild = lchild;
		} else {
			tmp = me->rchild;
			me->rchild = (node *)calloc(sizeof(*me), 1);
			if (me->rchild == NULL) {
				free_tree(top); 
				free_tree(rchild);
				
				return NULL;
			}

			me->rchild->lchild = tmp;
			me = me->rchild;
		}

		me->type   = OR_NODE;
		me->rchild = rchild;
			
    }

	if (top == NULL)
		top = lchild;

    return top;
}



node *
and_expr(char **str)
{
	node *me = NULL, *top = NULL, *tmp, *lchild, *rchild;

    lchild = factor(str);
	if (lchild == NULL)
		return NULL;

    *str = skipwhite(*str);

    while(**str == AND) {
		*str = *str + 1;
		if (**str != AND) {
			/* expected another & for an AND operator */
			free_tree(lchild);
			return NULL;
		}
         
        *str = skipwhite(*str + 1);   /* advance over the operator */
       
        rchild = factor(str);
		if (rchild == NULL) {
			if (top)
				free_tree(top);
			else
				free_tree(lchild);

			return NULL;
		}
     
		if (me == NULL) {
			me = (node *)calloc(sizeof(*me), 1);
			if (me == NULL) {
				free_tree(lchild);
				free_tree(rchild);

				return NULL;
			}

			top = me;
			me->lchild = lchild;
		} else {
			tmp = me->rchild;
			me->rchild = (node *)calloc(sizeof(*me), 1);
			if (me->rchild == NULL) {
				free_tree(top);
				free_tree(rchild);
				
				return NULL;
			}

			me->rchild->lchild = tmp;
			me = me->rchild;
		}

		me->type   = AND_NODE;
		me->rchild = rchild;
    }

	if (top == NULL)
		top = lchild;

    return top;
}


node *
factor(char **str)
{
    node *me;
    query_piece *qp;
    char op = NOTHING;


    if (**str == BANG) {  /* our only unary op */
        op = **str;
        *str = skipwhite(*str + 1);
    }

	if (**str == LPAREN) {
        *str = skipwhite(*str + 1);
		
        me = or_expr(str);           /* start over at the top */
		if (me == NULL)
			return NULL;
		
		if (op == BANG) {
			apply_demorgans_law(me);
		}

        if (**str == RPAREN)
            *str = *str + 1;
        else {
			/* mismatched parens */
			free_tree(me);

			return NULL;
		}

        return me;
	} else {
		
        qp = get_query_piece(*str, str);
		if (qp == NULL)
			return NULL;

        *str = skipwhite(*str);

		me = (node *)calloc(sizeof(*me), 1);
		if (me == NULL)
			return NULL;

		me->type   = FACTOR_NODE;
		me->qp     = qp;
		if (op == BANG)
			me->qp->op = invert_qp_op(me->qp->op);
		me->lchild = me->rchild = NULL;

        return me;
    }

  return NULL;
}



query_piece *
get_query_piece(char *str, char **result)
{
	int          ret = 0;
	char        *ptr;
	query_piece *qp = NULL;
	char        *tmpbuff = NULL;
	
	/* printf("starting query %s\n", query); */

	qp = (query_piece *)calloc(sizeof(query_piece), 1);
	if (qp == NULL)
		return NULL;

	tmpbuff = (char *)malloc(B_FILE_NAME_LENGTH);
	if (tmpbuff == NULL) {
		free(qp);
		return NULL;
	}


	/* skip leading white space */
	ptr = str;
	while(*ptr && isspace(*ptr))
		ptr++;
	if (*ptr == '\0') {
		ret = EINVAL;
		goto clean;
	}

	/* now get the index name (a potentially quoted string) */
	strncpy(tmpbuff, ptr, B_FILE_NAME_LENGTH);
	tmpbuff[B_FILE_NAME_LENGTH-1] = '\0';
	ptr += get_quoted_string(tmpbuff);
	if (tmpbuff[0] == '\0' || (qp->index_name = strdup(tmpbuff)) == NULL) {
		ret = EINVAL;
		goto clean;
	}

	/* skip any intervening white space */
	while(*ptr && isspace(*ptr))
		ptr++;

	/* figure out what the operator is */
	if (*ptr == '=') {
		qp->op = OP_EQUAL;
		ptr += 1;
		if (*ptr == '=')   /* accept "==" too */
			ptr += 1;
	} else if (*ptr == '!' && *(ptr+1) == '=') {
		qp->op = OP_NOT_EQUAL;
		ptr += 2;
	} else if (*ptr == '<') {
		ptr += 1;
		if (*ptr == '=') {
			qp->op = OP_LT_OR_EQUAL;
			ptr += 1;
		} else
			qp->op = OP_LT;
	} else if (*ptr == '>') {
		ptr += 1;
		if (*ptr == '=') {
			qp->op = OP_GT_OR_EQUAL;
			ptr += 1;
		} else
			qp->op = OP_GT;
	} else {
		ret = EINVAL;
		goto clean;
	}


	/* skip more white space */
	while(*ptr && isspace(*ptr))
		ptr++;
	if (*ptr == '\0') {
		ret = EINVAL;
		goto clean;
	}

	/* now get the value we're comparing against */
	strncpy(qp->value.str_val, ptr, sizeof(qp->value.str_val));
	qp->value.str_val[sizeof(qp->value.str_val)-1] = '\0';
	ptr += get_quoted_string(qp->value.str_val);
	if (qp->value.str_val[0] == '\0')
		goto clean;


	free(tmpbuff);
	*result = ptr;
	
	return qp;

 clean:
	if (qp) {
		if (qp->index_name)
			free(qp->index_name);
		qp->index_name = NULL;
		
		free(qp);
	}
	if (tmpbuff)
		free(tmpbuff);

	return NULL;
}


int
invert_qp_op(int op)
{
	int inverted_ops[] = {
		0,
		OP_NOT_EQUAL,
		OP_GT,
		OP_LT,
		OP_EQUAL,
		OP_GT_OR_EQUAL,
		OP_LT_OR_EQUAL
	};

	if (op <= 0 || op >= sizeof(inverted_ops)/sizeof(int))
		return 0;

	return inverted_ops[op];
}

char *
qp_op_to_str(int op)
{
	char *strings[] = {
		"bad op",
		"==",
		"<=",
		">=",
		"!=",
		"<",
		">"			
	};

	if (op <= 0 || op >= sizeof(strings)/sizeof(char *))
		return "bad op";

	return strings[op];
}

void
apply_demorgans_law(node *top)
{
	if (top->lchild) {
		apply_demorgans_law(top->lchild);
	}

	if (top->type == OR_NODE)
		top->type = AND_NODE;
	else if (top->type == AND_NODE)
		top->type = OR_NODE;
	else if (top->type == FACTOR_NODE) {
		top->qp->op = invert_qp_op(top->qp->op);
	} else
		printf("we got garbage for node @ 0x%lx (type %d qp 0x%lx)\n",
			   top, top->type, top->qp);

	if (top->rchild) {
		apply_demorgans_law(top->rchild);
	}
}


void
dump_tree(node *top)
{
	if (top->lchild) {
		dump_tree(top->lchild);
	}

	if (top->type == OR_NODE) 
		printf("OR node     @ 0x%x\n", top);
	else if (top->type == AND_NODE)
		printf("AND node    @ 0x%x\n", top);
	else if (top->type == FACTOR_NODE)
		printf("factor node @ 0x%x: value <%s %s %s>\n", top,
			   top->qp->index_name, qp_op_to_str(top->qp->op),
			   top->qp->value.str_val);
	else
		printf("we got garbage for node @ 0x%x (type %d qp 0x%x)\n",
			   top, top->type, top->qp);

	if (top->rchild) {
		dump_tree(top->rchild);
	}
}


void
free_tree(node *top)
{
	if (top->lchild)
		free_tree(top->lchild);

	if (top->rchild)
		free_tree(top->rchild);

	if (top->qp) {
		if (top->qp->index_name)
			free(top->qp->index_name);
		free(top->qp);
	}
	
	top->lchild = top->rchild = NULL;
	top->type = 0xf7;

	free(top);
	
}



int
seek_down_to_leaf_node(node *top, node **stack, int stacksize)
{
	int cur = 0, prefer_left = 1;
	
	/* printf("seek_down: top @ 0x%x, stacksize %d\n", top, stacksize); */

	memset(stack, 0, stacksize * sizeof(node *));

	while(top && cur < stacksize) {
		stack[cur++] = top;

		/*
		   XXXdbg -- this logic for picking the "better" path through
		             a tree could definitely be tuned.
		*/			 

		if (top->rchild && (top->flags & VISITED_RIGHT) == 0 &&
			top->rchild->type == FACTOR_NODE && top->rchild->qp->index) {
			node *lchild = top->lchild, *rchild = top->rchild;

			if (lchild == NULL || lchild->type != FACTOR_NODE ||
				lchild->qp->index == NULL ||
				lchild->qp->index->data.size > rchild->qp->index->data.size ||
				(bt_dtype(rchild->qp->index->etc->btree) != BT_STRING &&
				 bt_dtype(lchild->qp->index->etc->btree) == BT_STRING &&
				 (lchild->qp->is_pattern == 0 ||
				  lchild->qp->index->data.size * 2 >
				  rchild->qp->index->data.size))) {

				/*
				   this complicated piece of logic basically says to
				   only pick the right child if it's not a query on
				   the size index or it is a query on the size index
				   and the op is an equal or the size is greater than
				   64k and the op is > or >=.

				   The general idea is that most filesystems have many
				   many files less than 32k in size and so using size
				   index is really much less than optimal.
				*/   
				if (rchild->qp->index != rchild->qp->qi->bfs->size_index ||
					(rchild->qp->op == OP_EQUAL ||
					 (rchild->qp->value.int64_val >= 32768 &&
					  (rchild->qp->op == OP_GT || rchild->qp->op == OP_GT_OR_EQUAL)))) {
					prefer_left = 0;
				}
			}
		}


		if (top->lchild && prefer_left && (top->flags & VISITED_LEFT) == 0) {
			top->flags |= VISITED_LEFT;
			top = top->lchild;
		} else if ((top->flags & VISITED_RIGHT) == 0) {
			top->flags |= VISITED_RIGHT;
			top = top->rchild;
		} else {
			printf("already visited both children of node 0x%x???\n", top);
			break;
		}
	}

	if (cur >= stacksize)   /* darn. stack is too deep, return an error */
		return -1;
	
	return cur;
}

int
traverse_leaves(node *top, int (*func)(query_piece *qp, void *arg), void *arg)
{
	int ret;
	
	if (top->type == FACTOR_NODE) {
		if (top->qp == NULL)
			bfs_die("query: a factor node with no qp? (0x%x)\n", top);
		
		return func(top->qp, arg);
	}

	if (top->lchild) {
		if ((ret = traverse_leaves(top->lchild, func, arg)) != 0)
			return ret;
	}

	if (top->rchild) {
		if ((ret = traverse_leaves(top->rchild, func, arg)) != 0)
			return ret;
	}

	return 0;
}


int
fill_in_index_ptrs(query_piece *qp, void *arg)
{
	int         ret;
	char       *ptr;
	query_info *qi  = (query_info *)arg;
	bfs_info   *bfs = qi->bfs;
	
	if (qp == NULL) {
		printf("fill_in_index_ptrs called w/null qp?\n");
		return EINVAL;
	}
		
	qp->qi = qi;
	
	ptr = qp->index_name;
	
	if (strcmp("name", ptr) == 0) {
		qp->index = bfs->name_index;
	} else if (strcmp("size", ptr) == 0) {
		qp->index = bfs->size_index;
	} else if (strcmp("created", ptr) == 0) {
		qp->index   = bfs->create_time_index;
		qp->is_time = 1;
	} else if (strcmp("last_modified", ptr) == 0) {
		qp->index   = bfs->last_modified_time_index;
		qp->is_time = 1;
	} else {
		/*
		   XXXdbg -- we should probably be a little nicer here.
		             that is, if the index doesn't exist (ENOENT)
					 we can still survive and run the rest of the
					 query and just do this piece by hand...
		*/			 
		if ((ret = get_index(bfs, ptr, &qp->index)) != 0) {
			qp->index = NULL;    /* make sure it's cleared out */
			if (ret == ENOENT)   /* means the attr name is not indexed */
				ret = 0;
			
			return ret;
		}

		qp->need_put = 1;
	}
	
	if (qp->index == NULL)  /* this can happen on a bfs volume w/no indices */
		return ENOENT;

	if (qp->index->etc == NULL || qp->index->etc->btree == NULL)
		bfs_die("qp @ 0x%x has a bad index (0x%x)\n", qp, qp->index);

	ptr = qp->value.str_val;
	
	if (bt_dtype(qp->index->etc->btree) == BT_INT || 
		bt_dtype(qp->index->etc->btree) == BT_UINT) {
		int ival;

		ival = strtoul(qp->value.str_val, &ptr, 0);
		if (ptr == qp->value.str_val) {  /* hmm, then no conversion happened */
			ret = EINVAL;
			goto clean;
		}

		qp->value.int32_val = ival;

	} else if (bt_dtype(qp->index->etc->btree) == BT_LONG_LONG ||
			   bt_dtype(qp->index->etc->btree) == BT_ULONG_LONG) {
		unsigned long long ull;
		
		ull = strtoull(qp->value.str_val, &ptr, 0);
		if (ptr == qp->value.str_val) {  /* hmm, then no conversion happened */
			ret = EINVAL;
			goto clean;
		}

		qp->value.int64_val = ull;
		
	} else if (bt_dtype(qp->index->etc->btree) == BT_FLOAT) {
		float flt;
		
		if (qp->value.str_val[0] == '0' && qp->value.str_val[1] == 'x') {
			int tmp;

			tmp = strtoul(qp->value.str_val, &ptr, 0);
			flt = *(float *)&tmp;
		} else {
			flt = strtod(qp->value.str_val, &ptr);
		}

		if (ptr == qp->value.str_val) {
			ret = EINVAL;
			goto clean;
		}

		qp->value.float_val = flt;
		
	} else if (bt_dtype(qp->index->etc->btree) == BT_DOUBLE) {
		double dbl;

		if (qp->value.str_val[0] == '0' && qp->value.str_val[1] == 'x') {
			uint64 tmp;

			tmp = strtoull(qp->value.str_val, &ptr, 0);
			dbl = *(double *)&tmp;
		} else {
			dbl = strtod(qp->value.str_val, &ptr);
		}
		
		if (ptr == qp->value.str_val) {
			ret = EINVAL;
			goto clean;
		}

		qp->value.double_val = dbl;
		
	} else if (bt_dtype(qp->index->etc->btree) == BT_STRING) {

		qp->is_pattern = is_pattern((uchar *)qp->value.str_val);

		if (qp->is_pattern && qp->op != OP_EQUAL &&	qp->op != OP_NOT_EQUAL) {
			/* the operators <, >, <=, and >= don't make sense on patterns.*/
			ret = EINVAL;
			goto clean;
		}
	} else {
		printf("index @ 0x%x has a btree with unknown data type %d\n",
			   qp->index, bt_dtype(qp->index->etc->btree));
		ret = EINVAL;
		goto clean;
	}

	qp->converted = 1;  /* we converted the data type to its natural form */

	if (qp->index && (qi->flags & B_LIVE_QUERY) && qi->port > 0)
		add_index_callback(qp->index, live_query_update, qp);


	return 0;

 clean:
	return ret;
}

int
put_index_ptrs(query_piece *qp, void *arg)
{
	bfs_info *bfs = (bfs_info *)arg;

	if (qp == NULL) {
		printf("put_index_ptrs called w/null qp?\n");
		return EINVAL;
	}

	if (qp->index && qp->need_put)
		put_vnode(bfs->nsid, qp->index->etc->vnid);
	qp->index = NULL;
		
	return 0;
}

int
remove_qp_callbacks(query_piece *qp, void *arg)
{
	bfs_info *bfs = (bfs_info *)arg;

	if (qp == NULL) {
		printf("remove_qp_callbacks called w/null qp?\n");
		return EINVAL;
	}

	if (qp->index)
		remove_index_callback(qp->index, qp);
		
	return 0;
}

int
get_attr_value(bfs_info *bfs, bfs_inode *bi, char *name, union value_type *val,
			   int *type)
{
	size_t len;
	int    ret = 0;
	
	if (strcmp("name", name) == 0) {
		if (get_name_attr(bfs, bi, val->str_val, sizeof(val->str_val)) == ENOENT)
			ret = ENOENT;
		*type = B_STRING_TYPE;
	} else if (strcmp("size", name) == 0) {
		val->int64_val = bi->etc->old_size;
		*type = B_INT64_TYPE;
	} else if (strcmp("created", name) == 0) {
		val->int64_val = bi->create_time;
		*type = B_INT64_TYPE;
	} else if (strcmp("last_modified", name) == 0) {
		val->int64_val = bi->etc->old_mtime;
		*type = B_INT64_TYPE;
	} else {
		struct attr_info ai;

		ret = internal_stat_attr(bfs, bi, name, &ai);
		if (ret != ENOENT) {
			*type = ai.type;
		
			len = sizeof(*val);
			ret = internal_read_attr(bfs, bi, name, 0, val, &len, 0);
			if (len >= sizeof(*val))
				len = sizeof(*val) - 1;

			if (ret == 0)
				val->str_val[len] = 0; /* terminate; won't affect others */
		}
	}
	
	return ret;
}


int
convert_value_to_type(int type, union value_type *value)
{
	char *ptr;
	int ret = 0;
	
	ptr = value->str_val;
	
	if (type == B_INT32_TYPE || type == B_UINT32_TYPE || type == B_TIME_TYPE ||
		type == B_SIZE_T_TYPE || type == B_SSIZE_T_TYPE) {
		int ival;

		ival = strtoul(value->str_val, &ptr, 0);
		if (ptr == value->str_val) {  /* hmm, then no conversion happened */
			ret = EINVAL;
			goto clean;
		}

		value->int32_val = ival;

	} else if (type == B_INT64_TYPE || type == B_UINT64_TYPE ||
		type == B_OFF_T_TYPE) {
		unsigned long long ull;
		
		ull = strtoull(value->str_val, &ptr, 0);
		if (ptr == value->str_val) {  /* hmm, then no conversion happened */
			ret = EINVAL;
			goto clean;
		}

		value->int64_val = ull;
		
	} else if (type == B_FLOAT_TYPE) {
		float flt;
		
		if (value->str_val[0] == '0' && value->str_val[1] == 'x') {
			int tmp;

			tmp = strtoul(value->str_val, &ptr, 0);
			flt = *(float *)&tmp;
		} else {
			flt = strtod(value->str_val, &ptr);
		}

		if (ptr == value->str_val) {
			ret = EINVAL;
			goto clean;
		}

		value->float_val = flt;
		
	} else if (type == B_DOUBLE_TYPE) {
		double dbl;

		if (value->str_val[0] == '0' && value->str_val[1] == 'x') {
			uint64 tmp;

			tmp = strtoull(value->str_val, &ptr, 0);
			dbl = *(double *)&tmp;
		} else {
			dbl = strtod(value->str_val, &ptr);
		}
		
		if (ptr == value->str_val) {
			ret = EINVAL;
			goto clean;
		}

		value->double_val = dbl;
		
	} else if (type == B_STRING_TYPE || type == B_MIME_STRING_TYPE) {
		/* nothing to do since it is already a string */;
	} else {
		printf("convert_value_to_type: unknown data type 0x%x (val 0x%x)\n",
			   type, value);
			   
		ret = EINVAL;
		goto clean;
	}

	return 0;

clean:
	return ret;

}



int
compare_file(bfs_info *bfs, query_info *qi, query_piece *qp, bfs_inode *bi)
{
	int               cmp, type = 0, ret;
	BT_INDEX         *b;
	union value_type *file_value = &qi->file_val;

	ret = get_attr_value(bfs, bi, qp->index_name, file_value, &type);
	if (ret != 0) {
#if 0   /* sniff, sniff */
		if (qp->op == OP_NOT_EQUAL && ret == ENOENT)
			return 1;
#endif
		return 0;
	}

	if (qp->converted == 0) {
		if (convert_value_to_type(type, &qp->value) != 0) {
			printf("failed to convert qp @ 0x%x to type 0x%x\n", qp, type);
			return 0;
		}
		
		qp->converted = 1;
	}

	/*
	   XXXdbg - should verify that the attribute is the same type as
	            the type of the query piece
	*/

	if (type == B_STRING_TYPE || type == B_MIME_STRING_TYPE) {
		if (qp->is_pattern)
			cmp = !match((uchar *)qp->value.str_val,
							 (uchar *)file_value->str_val);
		else {
			if (strlen(file_value->str_val) > 0)
				cmp = strcmp(file_value->str_val, qp->value.str_val);
			else if (strlen(qp->value.str_val) > 0)
				cmp = -1;
			else
				cmp = 0;
		}
	} else if (type == B_INT32_TYPE || type == B_TIME_TYPE ||
			   type == B_SSIZE_T_TYPE) {
		cmp = file_value->int32_val - qp->value.int32_val;
	} else if (type == B_UINT32_TYPE || type == B_SIZE_T_TYPE) {
		cmp = (uint32)file_value->int32_val - (uint32)qp->value.int32_val;
	} else if (type == B_INT64_TYPE || type == B_OFF_T_TYPE) {
		int64 bigcmp;
			
		if (qp->is_time) {
			int64 value = qp->value.int64_val << TIME_SCALE;
			
			bigcmp = (file_value->int64_val & ~TIME_MASK) - value;
		} else {
			bigcmp = (uint64)file_value->int64_val - (uint64)qp->value.int64_val;
		}

		if (bigcmp == 0)
			cmp = 0;
		else if (bigcmp < 0)
			cmp = -1;
		else
			cmp = 1;
	} else if (type == B_UINT64_TYPE) {
		int64 bigcmp;
			
		bigcmp = (uint64)file_value->int64_val - (uint64)qp->value.int64_val;

		if (bigcmp == 0)
			cmp = 0;
		else if (bigcmp < 0)
			cmp = -1;
		else
			cmp = 1;
	} else if (type == B_FLOAT_TYPE) {
		cmp = file_value->float_val - qp->value.float_val;
	} else if (type == B_DOUBLE_TYPE) {
		cmp = file_value->double_val - qp->value.double_val;
	} else {
		/*
		   XXXdbg -- here we just punt since we don't know the data type 
		*/			 
		if (strlen(file_value->str_val) > 0)
			cmp = memcmp(file_value, &qp->value, strlen(file_value->str_val));
		else if (strlen(qp->value.str_val) > 0)
			cmp = -1;
		else
			cmp = 0;
	}

	if ((qp->op == OP_EQUAL && cmp == 0) ||
		(qp->op == OP_NOT_EQUAL && cmp != 0) ||
		(qp->op == OP_LT && cmp < 0) ||
		(qp->op == OP_LT_OR_EQUAL && cmp <= 0) ||
		(qp->op == OP_GT && cmp > 0) ||
		(qp->op == OP_GT_OR_EQUAL && cmp >= 0)) {
		return 1;
	}
	
	return 0;
}

int
compare_file_against_tree(node *top, bfs_info *bfs,
						  query_info *qi, bfs_inode *bi)
{
	int ret;
	
	if (top->type == FACTOR_NODE) {
		return compare_file(bfs, qi, top->qp, bi);
	}

	if (top->type == OR_NODE) {
		if ((ret = compare_file_against_tree(top->lchild, bfs, qi, bi)))
			return ret;

		return compare_file_against_tree(top->rchild, bfs, qi, bi);
	} else if (top->type == AND_NODE) {
		ret = compare_file_against_tree(top->lchild, bfs, qi, bi);
		if (ret == 0)
			return ret;

		return compare_file_against_tree(top->rchild, bfs, qi, bi);
	} else {
		printf("node @ 0x%x looks trashed\n", top);
		return 0;
	}

	return 0;
}



int
live_query_update(int type_of_event, bfs_inode *bi, int *old_result, void *arg)
{
	int          res, op;
	query_piece *qp  = (query_piece *)arg;
	query_info  *qi  = qp->qi;
	bfs_info    *bfs = qi->bfs;
	char        *name = &qi->fname_buff[0];

	res = compare_file_against_tree(qi->parse_tree, qi->bfs, qi, bi);
	
	if (res && (type_of_event == NEW_FILE || type_of_event == DELETE_FILE)) {
		if (type_of_event == NEW_FILE)
			op = B_ENTRY_CREATED;
		else
			op = B_ENTRY_REMOVED;

		get_name_attr(bfs, bi, &name[0], sizeof(qi->fname_buff));
		send_notification(qi->port, qi->token, B_QUERY_UPDATE, op,
						  qi->bfs->nsid,
						  qi->bfs->nsid,
						  inode_addr_to_vnode_id(bi->parent),
						  0,
						  inode_addr_to_vnode_id(bi->inode_num),
						  name);
	}

	return 0;
}
