#ifndef _POLAR_PROP_H
#define _POLAR_PROP_H

/* 
 * To support polar coordinates for complex values, we create temporary
 * properties to hold the converted values.
 */
typedef struct polar_property {
	char *name;
	char *item;
	PROPERTYADDR addr;
	double mag;
	double arg;
	double ang;
	struct polar_property * next;
} polar_property_t;

static polar_property_t * polar_property_head = NULL;

static polar_property_t * find_polar_property(OBJECT *obj, char *item) {
	polar_property_t *runner = polar_property_head;
	while (NULL != runner) {
		if (strcmp(obj->name, runner->name) == 0) {
			if (strcmp(item, runner->item) == 0) {
				break;
			}
		}
		runner = runner->next;
	}
	/* if item not found, create new and prepend */
	if (NULL == runner) {
		runner = malloc(sizeof(polar_property_t));
		runner->name = strdup(obj->name);
		runner->item = strdup(item);
		runner->next = polar_property_head;
		polar_property_head = runner;
	}
	return runner;
}

static polar_property_t * get_polar_property(OBJECT *obj, char *item, PROPERTY *prop) {
	polar_property_t *polar = find_polar_property(obj, item);
	polar->addr = GETADDR(obj, prop);
	return polar;
}

static void update_polar_properties() {
	polar_property_t *runner = polar_property_head;
	while (NULL != runner) {
		complex rect = *((complex*)runner->addr);
		runner->mag = complex_get_mag(rect);
		runner->arg = complex_get_arg(rect);
		runner->ang = runner->arg * 180/PI;
		runner = runner->next;
	}
}

static void clear_polar_properties() {
	polar_property_t *runner = polar_property_head;
	while (NULL != runner) {
		polar_property_t *deleteme = runner;
		runner = runner->next;
		free(deleteme->name);
		free(deleteme->item);
		free(deleteme);
	}
}

#define POLARADDR(O,A) ((void*)((char*)((A))-(char*)((O)+1)))

#endif
