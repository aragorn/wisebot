/* $Id$ */
#ifndef APACHE_UTIL_CFGTREE_H
#define APACHE_UTIL_CFGTREE_H

/**
 * @package Config Tree Package
 */

typedef struct ap_directive_t ap_directive_t;

/**
 * Structure used to build the config tree.  The config tree only stores
 * the directives that will be active in the running server.  Directives
 * that contain other directions, such as <Directory ...> cause a sub-level
 * to be created, where the included directives are stored.  The closing
 * directive (</Directory>) is not stored in the tree.
 */
struct ap_directive_t {
    /** The current directive */
    const char *directive;
    /** The arguments for the current directive, stored as a space 
     *  separated list */
    const char *args;
    /** The next directive node in the tree
     *  @defvar ap_directive_t *next */
    struct ap_directive_t *next;
    /** The first child node of this directive 
     *  @defvar ap_directive_t *first_child */
    struct ap_directive_t *first_child;
    /** The parent node of this directive 
     *  @defvar ap_directive_t *parent */
    struct ap_directive_t *parent;

    /** directive's module can store add'l data here */
    void *data;

    /* ### these may go away in the future, but are needed for now */
    /** The name of the file this directive was found in */
    const char *filename;
    /** The line number the directive was on */
    int line_num;
};


/**
 * Add a node to the configuration tree.
 * @param parent The current parent node.  If the added node is a first_child,
                 then this is changed to the current node
 * @param current The current node
 * @param toadd The node to add to the tree
 * @param child Is the node to add a child node
 * @return the added node
 */
ap_directive_t *add_directive_node(ap_directive_t **parent, 
		ap_directive_t *current, ap_directive_t *toadd, int child);

#endif
