#include <stdbool.h>
#include <stdint.h>

#define USAGE_APPLICATION     "Usage: ./a3 CONFIG_FILE\n"
#define ERROR_OPEN_FILE       "Error: Cannot open file: %s\n"
#define ERROR_INVALID_FILE    "Error: Invalid file: %s\n"
#define ERROR_OUT_OF_MEMORY   "Error: Out of memory\n"
#define ERROR_UNKNOWN_COMMAND "Error: Unknown command: %s\n"
#define USAGE_COMMAND_ROTATE  "Usage: rotate ( left | right ) ROW COLUMN\n"
#define ERROR_ROTATE_INVALID  "Error: Rotating start- or end-pipe is not allowed\n"
#define ERROR_NAME_ALPHABETIC "Error: Invalid name. Only alphabetic letters allowed\n"
#define ERROR_NAME_LENGTH     "Error: Invalid name. Name must be exactly 3 letters long\n"

#define INPUT_PROMPT "%u > "
#define INPUT_NAME   "Please enter 3-letter name: "

#define HELP_TEXT "Commands:\n" \
                  " - rotate <DIRECTION> <ROW> <COLUMN>\n" \
                  "    <DIRECTION> is either `left` or `right`.\n\n" \
                  " - help\n" \
                  "    Prints this help text.\n\n" \
                  " - quit\n" \
                  "    Terminates the game.\n\n" \
                  " - restart\n" \
                  "    Restarts the game.\n"

#define INFO_PUZZLE_SOLVED  "Puzzle solved!\n"
#define INFO_SCORE          "Score: %u\n"
#define INFO_BEAT_HIGHSCORE "Beat Highscore!\n"
#define INFO_HIGHSCORE_HEADER "Highscore:\n"
#define INFO_HIGHSCORE_ENTRY  "   %s %u\n"

typedef enum _Command_
{
  NONE,   // empty or only whitespace
  ROTATE,
  HELP,
  QUIT,
  RESTART
} Command;


// ----------------------------------------------------------------------------
// Prints the game map
//
// @param map     the game map
// @param width   the maps width
// @param height  the maps height
// @param start   row and column of start pipe
// @param dest    row and column of dest pipe
//
void printMap(uint8_t** map, uint8_t width, uint8_t height, uint8_t start[2], uint8_t dest[2]);

// ----------------------------------------------------------------------------
// Checks if start- and dest-pipe are connected
//
// @param map     the game map
// @param width   the maps width
// @param height  the maps height
// @param start   row and column of start pipe
// @param dest    row and column of dest pipe
// @return        true if connected, otherwise false
//
bool arePipesConnected(uint8_t** map, uint8_t width, uint8_t height, uint8_t start[2], uint8_t dest[2]);

// ----------------------------------------------------------------------------
// reads a line (i.e., until newline is found) from stdin
//
// This function allocates memory to parse arbitrarily long strings, the return
// value of this function must thus be `free`d when not needed anymore.
//
// Returns NULL, if out of memory
// Returns EOF, if hits end of file
//
// @return  the null-terminated line, with newline stripped
char* getLine();

// ----------------------------------------------------------------------------
// Parses the command and its arguments from the string <line>
//
// Saves the parsed values to applicable parameters.
// <cmd> is set to NONE when nothing or only whitespace is entered.
//
// Returns 1, if <cmd> is ROTATE and ...
//  - <dir> is neither "left" or "right"
//  - <row> or <col> are not an integer greater than 0
//  - there are too few/many arguments
//
// @param line  the string to parse
// @param cmd   the (well-known) command
// @param dir   the direction, if <cmd> is ROTATE (see README.md#datentypen)
// @param row   the row, if <cmd> is ROTATE
// @param col   the column, if <cmd> is ROTATE
// @return      NULL on success; 1 on invalid arguments; command token on unknown command
//
char* parseCommand(char* line, Command* cmd, size_t* dir, uint8_t* row, uint8_t* col);
