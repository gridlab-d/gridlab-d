#ifndef _POLAR_PROP_H
#define _POLAR_PROP_H

/* 
 * To support polar coordinates for complex values, we create temporary
 * properties to hold the converted values.
 */
typedef struct polar_property {
	char *item;
	double mag;
	double arg;
	double ang;
	struct polar_property * next;
} polar_property_t;

static polar_property_t * polar_property_head = NULL;

static polar_property_t * find_polar_property(char *item) {
	polar_property_t *runner = polar_property_head;
	while (NULL != runner) {
		if (strcmp(item, runner->item) == 0) {
			break;
		}
		runner = runner->next;
	}
	/* if item not found, create new and prepend */
	if (NULL == runner) {
		runner = malloc(sizeof(polar_property_t));
		runner->item = strdup(item);
		runner->next = polar_property_head;
		polar_property_head = runner;
	}
	return runner;
}

static polar_property_t * get_polar_property(char *item, PROPERTYADDR addr) {
	polar_property_t *polar = find_polar_property(item);
	complex rect = *((complex*)addr);
	polar->mag = complex_get_mag(rect);
	polar->arg = complex_get_arg(rect);
	polar->ang = polar->arg * 180/PI;
	return polar;
}

static void clear_polar_properties() {
	polar_property_t *runner = polar_property_head;
	while (NULL != runner) {
		polar_property_t *deleteme = runner;
		runner = runner->next;
		free(deleteme->item);
		free(deleteme);
	}
}

#define POLARADDR(O,A) ((void*)((char*)((A))-(char*)((O)+1)))

#endif
