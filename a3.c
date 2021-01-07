//-----------------------------------------------------------------------------
// a3.c
//
// ESPipes
// 
// A variation of the "Pipes-Minigames". Goal of the game is to connect the 
// start and the end pipe by rotating the pipes in between. 
// The game keeps track of the board and the highscores with config files, 
// which need to be read from and written to.
//
// Group: 12
//
// Author: 12007661
//-----------------------------------------------------------------------------
//

// TODO
// Write Highscore back in Config File

//----------
// Includes
//----------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "framework.h"

//----------
// Defines
//----------

#define MAGIC_NUMBER "ESPipes"
#define PLACEHOLDER_NAME "---"
#define DO_RESTART 2
#define HIGHSCORE_NAME_LENGTH 3

#define MAX_UNIT8_T 0xFF
#define MIN_UNIT8_T 0x00
#define FILTER_RIGHT 0x03
#define FILTER_LEFT 0xC0
#define SWITCH 0x40

//----------
// Typedefs
//----------

typedef enum _ReturnValue_
{
  SUCCESS,
  WRONG_PARAMETER,
  CANNOT_OPEN_FILE,
  INVALID_FILE_FORMAT,
  OUT_OF_MEMORY
} ReturnValue;

typedef struct _Board_
{
  uint8_t** map;
  uint8_t map_width;
  uint8_t map_height;
  uint8_t start[2];
  uint8_t end[2];
} Board;

typedef struct _HighscoreEntry_
{
  uint8_t score;
  char name[4];
} HighscoreEntry;

typedef struct _Highscore_
{
  uint8_t count;
  HighscoreEntry* entries;
} Highscore;

typedef enum _Direction_
{
  TOP,
  LEFT,
  BOTTOM,
  RIGHT
} Direction;

//---------------------
// Forward Definitions
//---------------------

// Loading
ReturnValue loadGame(Board** game_board, Highscore** highscore_list, char* file_name, char** error_context);
FILE* openConfigFile(char* file_name, ReturnValue* error_code);
ReturnValue loadConfigFile(Board** game_board, Highscore** highscore_list, FILE* file);
void loadHighscoreList(Highscore* highscore_list, FILE* file, ReturnValue* error_code);
void loadGameBoard(Board* game_board, FILE* file, ReturnValue* error_code);

// Game Logic
ReturnValue runGame(Board* game_board, int* score, char* restart);
Command getInput(char round, uint8_t* row, uint8_t* col, Direction* dir);
char runCommand(Command command, Board* game_board, uint8_t row, uint8_t col, Direction dir, char* stop);
char rotatePipe(Board* game_board, uint8_t row, uint8_t col, Direction dir);
void setConnectedBits(Board* game_board, uint8_t row, uint8_t col);
void setConnectedBitInDirection(Board* game_board, uint8_t row, uint8_t col, Direction dir);
char checkConnection(Board* game_board, uint8_t row, uint8_t col, Direction dir);

// Highscore
ReturnValue handleScore(Highscore* highscore_list, int score, char* file_name);
ReturnValue writeHighscore(Highscore* highscore_list, char* file_name);
char doesScoreBeatHighscore(Highscore* highscore_list, int score);
char* beatHighscore();
void printHighscore(Highscore* highscore_list);

// Helper Functions
void moveCoordiantesInDirection(uint8_t* row, uint8_t* col, Direction dir);
char areCoordinatesOnBoard(Board* game_board, uint8_t row, uint8_t col);
Direction getOppositeDirection(Direction dir);
char isPipeOpenInDirection(uint8_t pipe, Direction dir);

// Tidying Up
void freeResources(Board* game_board, Highscore* highscore_list);
int exitApplication(ReturnValue error_code, char* error_context);

//-----------------------------------------------------------------------------
///
/// The main program
/// 
/// Checks if the amount of parameters is correct and calls
/// the other functions for loading and running the game
/// and for handling the score and highscores.
/// Frees the alloced ressoures on exit.
///
/// @param argc count of the parameters
/// @param argv list of the parameters
///
/// @return 1 - 4 based on the error that occured; 0 on success
//
int main(int argc, char** argv)
{
  if (argc != 2)
  {
    return exitApplication(WRONG_PARAMETER, NULL);
  }

  ReturnValue error_code = SUCCESS;
  char* error_context = NULL;
  Board* game_board = NULL;
  Highscore* highscore_list = NULL;
  char restart = false;
  int score = 0;

  do 
  {
    if (restart)
    {
      restart = false;
      freeResources(game_board, highscore_list);
    }

    error_code = loadGame(&game_board, &highscore_list, argv[1], &error_context);
    if (error_code != NONE)
    {
      break;
    }
    
    error_code = runGame(game_board, &score, &restart);
  }
  while (restart);

  if (error_code == SUCCESS && score != 0)
  {
    error_code = handleScore(highscore_list, score, argv[1]);
  }

  freeResources(game_board, highscore_list);
  return exitApplication(error_code, error_context);
}

//-----------------------------------------------------------------------------
/// 
/// Loads the important variables for the game by setting "game_board"
/// and "highscore_list" accord to a config file.
/// 
/// @param gameboard A pointer to a pointer to the Board instance 
/// @param highscore_list A pointer to a pointer to the Highscore instance 
/// @param file_name A string with the path to the config file
/// @param error_context A pointer to a string that contains infomation if an error occured
///
/// @return 1 - 4 based on the error that occured; 0 on success
//
ReturnValue loadGame(Board** game_board, Highscore** highscore_list, char* file_name, char** error_context)
{
  ReturnValue error_code = SUCCESS;

  FILE* file = openConfigFile(file_name, &error_code);
  if (error_code != SUCCESS)
  {
    *error_context = file_name;
    return error_code;
  }

  error_code = loadConfigFile(game_board, highscore_list, file);

  return error_code;
}

//-----------------------------------------------------------------------------
/// 
/// Attemps to open a config file and checks if it is formated correctly
/// 
/// @param file_name A string with the path to the config file
/// @param error_code gets set to 1 - 4 based on the error that occured or 0 on success
///
/// @return A pointer to the opened file, NULL on error
//
FILE* openConfigFile(char* file_name, ReturnValue* error_code)
{
  FILE* file = fopen(file_name, "rb+");

  if (file == NULL)
  {
    *error_code = CANNOT_OPEN_FILE;
    return NULL;
  }

  char firstBytes[8];
  fgets(firstBytes, 8, file);

  if (strcmp(firstBytes, MAGIC_NUMBER))
  {
    fclose(file);
    *error_code = INVALID_FILE_FORMAT;
    return NULL;
  }

  return file;
}

//-----------------------------------------------------------------------------
/// 
/// Loads a Config File and writes contents to parameters
///
/// @param gameboard A pointer to a pointer to the Board instance 
/// @param highscore_list A pointer to a pointer to the Highscore instance 
/// @param file A file pointer to the config file
///
/// @return 1 - 4 based on the error that occured; 0 on success
//
ReturnValue loadConfigFile(Board** game_board, Highscore** highscore_list, FILE* file)
{
  *game_board = malloc(sizeof(Board));
  *highscore_list = malloc(sizeof(Highscore));

  ReturnValue error_code = SUCCESS;

  if (game_board == NULL || highscore_list == NULL)
  {
    error_code = OUT_OF_MEMORY;

    if (game_board != NULL)
    {
      free(game_board);
    }
    if (highscore_list != NULL)
    {
      free(highscore_list);
    }
    return error_code;
  }

  // Read fix-sized part of config
  fread(&((*game_board)->map_width), 1, 1, file);
  fread(&((*game_board)->map_height), 1, 1, file);
  fread(&((*game_board)->start), 1, 2, file);
  fread(&((*game_board)->end), 1, 2, file);
  fread(&((*highscore_list)->count), 1, 1, file);

  // Read variable-sized part of config
  loadHighscoreList(*highscore_list, file, &error_code);
  loadGameBoard(*game_board, file, &error_code);

  fclose(file);
  return error_code;
}

//-----------------------------------------------------------------------------
/// 
/// Loads the highscore list form a config file
///
/// @param highscore_list A pointer to the Highscore instance 
/// @param file A file pointer to the config file
/// @param error_code error_code based on the error that occured
//
void loadHighscoreList(Highscore* highscore_list, FILE* file, ReturnValue* error_code)
{
  highscore_list->entries = malloc(sizeof(HighscoreEntry) * highscore_list->count);
  if (highscore_list->entries == NULL)
  {
    *error_code = OUT_OF_MEMORY;
    return;
  }

  for (int i = 0; i < highscore_list->count; i++)
  {
    fread(&((highscore_list->entries[i]).score), 1, 1, file);
    fread(&((highscore_list->entries[i]).name), 3, 1, file);
    highscore_list->entries[i].name[3] = '\0';
  }
}

//-----------------------------------------------------------------------------
/// 
/// Loads the highscore list form a config file
/// 
/// @param game_board A pointer to the Board instance
/// @param file A file pointer to the config file
/// @param error_code error_code based on the error that occured
//
void loadGameBoard(Board* game_board, FILE* file, ReturnValue* error_code)
{
  game_board->map = malloc(sizeof(uint8_t*) * game_board->map_height);
  if (game_board->map == NULL)
  {
    *error_code = OUT_OF_MEMORY;
    return;
  }

  for (int row_index = 0; row_index < game_board->map_height; row_index++)
  {
    game_board->map[row_index] = malloc(sizeof(uint8_t) * game_board->map_width);
    if (game_board->map[row_index] == NULL)
    {
      *error_code = OUT_OF_MEMORY;
      return;
    }

    for (int col_index = 0; col_index < game_board->map_width; col_index++)
    {    
      fread(&(game_board->map[row_index][col_index]), 1, 1, file);
    }
  }
}

//-----------------------------------------------------------------------------
/// 
/// Runs the game by printing the map, asking for user input
/// and executing the commands available to the user
/// 
/// @param game_board A pointer to the Board instance
/// @param score A pointer to an integer variable - will be filled with the score
/// @param restart a char that can be interpreted as true/false - true if the game should be restarted
///
/// @return 1 - 4 based on the error that occured; 0 on success
//
ReturnValue runGame(Board* game_board, int* score, char* restart)
{
  Command command = 0;
  Direction dir = 0;
  uint8_t row = 0;
  uint8_t col = 0;
  char stop = false;
  char round = 1;
  char skipPrinting = 0;

  while(!stop)
  {
    if (skipPrinting)
    {
      skipPrinting = false;
    }
    else
    {
      printMap(
        game_board->map, 
        game_board->map_width, 
        game_board->map_height, 
        game_board->start, 
        game_board->end
      );
    } 

    command = getInput(round, &row, &col, &dir);
    if (command == NONE)
    {
      return OUT_OF_MEMORY;
    }

    if (runCommand(command, game_board, row, col, dir, &stop))
    {
      round++;
    }
    else
    {
      skipPrinting = true;
    }
    
    if (stop == DO_RESTART)
    {
      *restart = true;
      return SUCCESS;
    }

    if (!stop)
    {
      stop = arePipesConnected(
        game_board->map, 
        game_board->map_width, 
        game_board->map_height, 
        game_board->start, 
        game_board->end
      );
    }  
  }

  if (command != QUIT)
  {
    printMap(
      game_board->map, 
      game_board->map_width, 
      game_board->map_height, 
      game_board->start, 
      game_board->end
    );
    *score = round - 1;
  }

  return SUCCESS;
}

//-----------------------------------------------------------------------------
/// 
/// Prompts the user for an input, checks if the input is
/// a valid command and saves the information to parameters.
/// 
/// @param round The current round number
/// @param row A pointer to the row - Will be set if command = rotate
/// @param col A pointer to the column - Will be set if command = rotate
/// @param dir A pointer to the direction - Will be set if command = rotate
///
/// @return Command that corresponds to user input; NONE if out of memory
//
Command getInput(char round, uint8_t* row, uint8_t* col, Direction* dir)
{
  char* input;

  printf(INPUT_PROMPT, round);
  input = getLine();
  if (input == NULL)
  {
    return NONE;
  } 
  else if (input == (char*) EOF)
  {
    return QUIT;
  }

  Command command = NONE;
  char* ret = parseCommand(input, &command, (size_t*)dir, row, col);

  (*row)--;
  (*col)--;

  if (ret == NULL)
  {
    if (command != NONE)
    {
      free(input);
      return command;
    }
  } 
  else if(ret == (char*) 1)
  {
    printf(USAGE_COMMAND_ROTATE);
  } 
  else 
  {
    printf(ERROR_UNKNOWN_COMMAND, ret);
  }

  free(input);
  return getInput(round, row, col, dir);
}

//-----------------------------------------------------------------------------
/// 
/// Executes a single command with ceritain parameters
/// 
/// @param command The command to execute
/// @param game_board A pointer to a Board instance
/// @param row If command = rotate: the row index
/// @param col If command = rotate: the column index
/// @param dir If command = rotate: the direction to rotate
/// @param stop A pointer to a character - will be set to "true" if game should stop
///
/// @return true if successfull; false otherwise
//
char runCommand(Command command, Board* game_board, uint8_t row, uint8_t col, Direction dir, char* stop)
{
  switch (command)
  {
  case QUIT:
    *stop = true;
    break;

  case HELP:
    printf(HELP_TEXT);
    break;

  case RESTART:
    *stop = DO_RESTART;
    break;

  case ROTATE:
    return rotatePipe(game_board, row, col, dir);
    break;
  
  default:
    break;
  }

  return true;
}

//-----------------------------------------------------------------------------
/// 
/// Rotates a pipe at certain coordinates in certain direction
/// 
/// @param game_board A pointer to a Board instance
/// @param row the row index
/// @param col the column index
/// @param dir the direction to rotate in
///
/// @return true if successfull; false otherwise
//
char rotatePipe(Board* game_board, uint8_t row, uint8_t col, Direction dir)
{
  if (! areCoordinatesOnBoard(game_board, row, col))
  {
    printf(USAGE_COMMAND_ROTATE);
    return false;
  }
  

  if ((row == game_board->start[0] && col == game_board->start[1])
    || (row == game_board->end[0] && col == game_board->end[1]))
  {
    printf(ERROR_ROTATE_INVALID);
    return false;
  }

  uint8_t new_pipe = game_board->map[row][col];

  if (dir == LEFT)
  {
    new_pipe = ((new_pipe & FILTER_RIGHT) << 6) | (new_pipe >> 2);
  } 
  else if (dir == RIGHT)
  {
    new_pipe = ((new_pipe & FILTER_LEFT) >> 6) | (new_pipe << 2);
  } 

  game_board->map[row][col] = new_pipe;

  setConnectedBits(game_board, row, col);

  return true;
}

//-----------------------------------------------------------------------------
/// 
/// Updates the connected bits of a pipe and its neighbors
/// 
/// @param game_board A pointer to a Board instance
/// @param row the row index
/// @param col the column index
//
void setConnectedBits(Board* game_board, uint8_t row, uint8_t col)
{
  for (Direction dir = TOP; (int) dir <= RIGHT; dir++)
  {  
    setConnectedBitInDirection(game_board, row, col, dir);

    uint8_t new_row = row;
    uint8_t new_col = col;
    moveCoordiantesInDirection(&new_row, &new_col, dir);
    if (areCoordinatesOnBoard(game_board, new_row, new_col))
    {
      setConnectedBitInDirection(game_board, new_row, new_col, getOppositeDirection(dir));
    }
  }
}

//-----------------------------------------------------------------------------
/// 
/// Updates the connected bit of a pipe in one direction
///
/// @param game_board A pointer to a Board instance
/// @param row the row index
/// @param col the column index
/// @param dir the direction of the bit to set
//
void setConnectedBitInDirection(Board* game_board, uint8_t row, uint8_t col, Direction dir)
{
  uint8_t new_pipe = game_board->map[row][col];
 
  char connected;
  if (isPipeOpenInDirection(game_board->map[row][col], dir))
  {
    connected = checkConnection(game_board, row, col, dir);
  }
  else
  {
    connected = false;
  }    

  char shift_value = (2 * dir);

  if (connected)
  {  
    new_pipe = new_pipe | (SWITCH >> shift_value);
  }
  else
  {
    new_pipe = new_pipe & (~(SWITCH >> shift_value));
  }

  game_board->map[row][col] = new_pipe; 

}

//-----------------------------------------------------------------------------
/// 
/// Checks if a pipe should connect in one direction
/// 
/// @param game_board A pointer to a Board instance
/// @param row the row index
/// @param col the column index
/// @param dir the direction to check
///
/// @return a char that can be interpreted as true/false
//
char checkConnection(Board* game_board, uint8_t row, uint8_t col, Direction dir)
{

  moveCoordiantesInDirection(&row, &col, dir);

  if (! areCoordinatesOnBoard(game_board, row, col))
  {
    return false;
  }

  Direction opp_dir = getOppositeDirection(dir);
  if (isPipeOpenInDirection(game_board->map[row][col], opp_dir))
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
/// 
/// Takes a score as parameter, checks if it breaks a highscore 
/// and eventually asks for a name and updates the highscores
/// 
/// @param highscore_list A pointer to the Highscore instance
/// @param score the new score
///
/// @return 1 - 4 based on the error that occured; 0 on success
//
ReturnValue handleScore(Highscore* highscore_list, int score, char* file_name)
{
  printf(INFO_PUZZLE_SOLVED);
  printf(INFO_SCORE, score);

  ReturnValue error_code = SUCCESS;

  if (doesScoreBeatHighscore(highscore_list, score))
  {
    char* name = beatHighscore();
    HighscoreEntry new_entry;
    new_entry.score = score;
    strcpy(new_entry.name, name);
    free(name);

    for (int i = 0; i < highscore_list->count; i++)
    {
      int entry_score = highscore_list->entries[i].score;

      if (entry_score == 0)
      {
        highscore_list->entries[i] = new_entry;
        break;
      }
      else if (entry_score > new_entry.score)
      {
        HighscoreEntry tmp = highscore_list->entries[i];
        highscore_list->entries[i] = new_entry;
        new_entry = tmp;
      }
    }

    error_code = writeHighscore(highscore_list, file_name);
  }

  printHighscore(highscore_list);   
  return error_code;
}

//-----------------------------------------------------------------------------
/// 
/// Writes the highscore back to the config file
/// 
/// @param highscore_list A pointer to the Highscore instance
/// @param file_name path to config file
///
/// @return 1 - 4 based on the error that occured; 0 on success
//
ReturnValue writeHighscore(Highscore* highscore_list, char* file_name)
{
  ReturnValue error_code = SUCCESS;
  FILE* file = openConfigFile(file_name, &error_code);

  if (error_code != SUCCESS)
  {
    return error_code;
  }

  fseek(file, 14, SEEK_SET); //TODO 15?

  for (int i = 0; i < highscore_list->count; i++)
  {
    fwrite(&((highscore_list->entries[i]).score), 1, 1, file);
    fwrite(&((highscore_list->entries[i]).name), 3, 1, file);
  }

  fclose(file);
  return SUCCESS;
}

//-----------------------------------------------------------------------------
/// 
/// Checks if a certain score beats a highscore int the list
/// 
/// @param highscore_list the list to check in
/// @param score the score to use to check
///
/// @return a char that can be interpreted as true/false
//
char doesScoreBeatHighscore(Highscore* highscore_list, int score)
{
  for (int i = 0; i < highscore_list->count; i++)
  {
    int entry_score = highscore_list->entries[i].score; 
    if (entry_score == 0 || score < entry_score)
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
/// 
/// Prints the information that a highscore was beat to stdout
/// Then asks the user for a 3-letter name
///
/// @return A string containing the user-name
//
char* beatHighscore()
{
  printf(INFO_BEAT_HIGHSCORE);
  printf(INPUT_NAME);

  char* name;
  char name_valid = false;

  while (!name_valid)
  {
    name = getLine();

    if (name == NULL)
    {
      printf(ERROR_OUT_OF_MEMORY);
      exit(OUT_OF_MEMORY);
    } 
    else if (name == (char*) EOF)
    {
      continue;
    }
    else if (strlen(name) == HIGHSCORE_NAME_LENGTH)
    {
      name_valid = true;
      for (int i = 0; i < HIGHSCORE_NAME_LENGTH; i++)
      {
        name[i] = toupper(name[i]);
        if (name[i] > 90 || name[i] < 65)  
        {
          name_valid = false;
          break;
        }
      }
    }
    if (!name_valid)
    {
      free(name);
    }
  }

  return name;
}

//-----------------------------------------------------------------------------
/// 
/// Prints to list of highscores to stdout
///
/// @param highscore_list pointer to the Highscore instance to print 
//
void printHighscore(Highscore* highscore_list)
{
  printf(INFO_HIGHSCORE_HEADER);

  for (int i = 0; i < highscore_list->count; i++)
  {
    int score = highscore_list->entries[i].score;
    if (score == 0)
    {
      printf(INFO_HIGHSCORE_ENTRY, PLACEHOLDER_NAME, score);
    } 
    else 
    {
      printf(INFO_HIGHSCORE_ENTRY, highscore_list->entries[i].name, score);
    }
  }
}

//-----------------------------------------------------------------------------
/// 
/// Takes two coordinates as parameters and changes to according to a direction
///
/// @param row a pointer to the row index
/// @param col a pointer to the column index
/// @param dir the direction to change coordinates to
//
void moveCoordiantesInDirection(uint8_t* row, uint8_t* col, Direction dir)
{
  switch (dir)
  {
  case TOP:
    (*row)--;
    break;
  case LEFT:
    (*col)--;;
    break;
  case BOTTOM:
    (*row)++;
    break;
  case RIGHT:
    (*col)++;
    break;
  }
}

//-----------------------------------------------------------------------------
/// 
/// Checks if two coordinates are in bounds of the map
/// 
/// @param game_board the game board to check in
/// @param row the row index
/// @param col the column index
//
/// @return a char that can be interpreted as true/false
//
char areCoordinatesOnBoard(Board* game_board, uint8_t row, uint8_t col)
{
  char res = (
    row > game_board->map_height - 1 || 
    col > game_board->map_width - 1 ||
    row < 0 || col < 0);

  return !res;
}

//-----------------------------------------------------------------------------
/// 
/// Takes a direction as parameter and return the opposite
///
/// @param the direction tho get the opposite from 
///
/// @return the opposite direction
//
Direction getOppositeDirection(Direction dir)
{
  for (unsigned i = 2; i > 0; i--)
  {
    dir--;
    if ((int) dir < TOP)
    {
      dir = RIGHT;
    }
  }
  return dir;
}

//-----------------------------------------------------------------------------
/// 
/// Checks if a pipe in open in one direction
///
/// @param pipe the pipe that should be checked
/// @param dir the direction to check for
///
/// @return a char that can be interpreted as true/false
//
char isPipeOpenInDirection(uint8_t pipe, Direction dir)
{
  uint8_t checker = 0x80;
  
  return pipe & (checker >> (2 * dir));
}

//-----------------------------------------------------------------------------
/// 
/// Frees the ressources that were alloced for the game
/// 
/// @param game_board A pointer to the Board instance taht should be freed
/// @param highscore_list A pointer to the Highscore instance taht should be freed
//
void freeResources(Board* game_board, Highscore* highscore_list)
{
  if (highscore_list != NULL)
  {
    free(highscore_list->entries);
    free(highscore_list);
  }

  if (game_board != NULL)
  {
    if (game_board->map != NULL)
    {
      for (int i = 0; i < game_board->map_height; i++)
      {
        free(game_board->map[i]);
      }
      free(game_board->map);
    }
    free(game_board);
  }  
}

//-----------------------------------------------------------------------------
/// 
/// Prints an error message based on an error code and returns it
/// 
/// @param error_code the error_code describing the message
/// @param error_context context for printing the error message
///
/// @return 1 - 4 based on the error code
//
int exitApplication(ReturnValue error_code, char* error_context)
{
  switch (error_code)
  {
  case WRONG_PARAMETER:
    printf(USAGE_APPLICATION);
    break;
  case CANNOT_OPEN_FILE:
    printf(ERROR_OPEN_FILE, error_context);
    break;
  case INVALID_FILE_FORMAT:
    printf(ERROR_INVALID_FILE, error_context);
    break;
  case OUT_OF_MEMORY:
    printf(ERROR_OUT_OF_MEMORY);
    break;  
  default:
    break;
  }
  return error_code;
}
