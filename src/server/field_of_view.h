/* src/server/field_of_view.h
 *
 * Generate view of map as visible to player.
 */



#ifndef FIELD_OF_VIEW_H
#define FIELD_OF_VIEW_H



/* Return map cells sequence as visible to the player, with invisible cells as
 * whitespace. Super-impose over visible map cells map objects positioned there.
 */
extern char * build_visible_map();



#endif
