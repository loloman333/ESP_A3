//-----------------------------------------------------------------------------
// a3.c
//
// ESPipes
// TODO
//
// Group: 12
//
// Author: 12007661
//-----------------------------------------------------------------------------
//

// Includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "framework.h"

// Defines
#define MAGIC_NUMBER "ESPipes"
#define DO_RESTART 2

// Typedefs
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
  char name[3];
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

// Forward Definitions     TODO: order these the same way as below

ReturnValue loadGame(Board** game_board, Highscore** highscore_list, char* file_name, char** error_context);
FILE* openConfigFile(char* file_name, ReturnValue* error_code);
void loadConfigFile(Board** game_board, Highscore** highscore_list, FILE* file, ReturnValue* error_code);
void loadHighscoreList(Highscore* highscore_list, FILE* file, ReturnValue* error_code);
void loadGameBoard(Board* game_board, FILE* file, ReturnValue* error_code);

ReturnValue runGame(Board* game_board, char* restart);
Command getInput(char round, uint8_t* row, uint8_t* col, Direction* dir);
void runCommand(Command command, Board* game_board, uint8_t row, uint8_t col, Direction dir, char* stop);
void rotatePipe(Board* game_board, uint8_t row, uint8_t col, Direction dir);

void freeResources(Board* game_board, Highscore* highscore_list);
int exitApplication(ReturnValue error_code, char* error_context);

//-----------------------------------------------------------------------------
///
/// The main program
/// 
/// TODO
///
/// @return always zero
//
int main(int argc, char** argv) //TODO: char** argv == char * argv[]  ???
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

  do 
  {
    error_code = loadGame(&game_board, &highscore_list, argv[1], &error_context);
    if (error_code != NONE)
    {
      break;
    }
    
    error_code = runGame(game_board, &restart);
  }
  while (restart);

  freeResources(game_board, highscore_list);
  return exitApplication(error_code, error_context);
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
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

  loadConfigFile(game_board, highscore_list, file, &error_code);

  return error_code;
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
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
  fgets(firstBytes, 8, file); //TODO: Return value interesting?

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
/// TODO
/// 
///
/// @return TODO
//
void loadConfigFile(Board** game_board, Highscore** highscore_list, FILE* file, ReturnValue* error_code)
{
  *game_board = malloc(sizeof(Board));
  *highscore_list = malloc(sizeof(Highscore));

  if (game_board == NULL || highscore_list == NULL)
  {
    *error_code = OUT_OF_MEMORY;

    if (game_board != NULL)
    {
      free(game_board);
    }
    if (highscore_list != NULL)
    {
      free(highscore_list);
    }
    return;
  }

  // Read fix-sized part of config
  fread(&((*game_board)->map_width), 1, 1, file);
  fread(&((*game_board)->map_height), 1, 1, file);
  fread(&((*game_board)->start), 1, 2, file);
  fread(&((*game_board)->end), 1, 2, file);
  fread(&((*highscore_list)->count), 1, 1, file);

  // Read variable-sized part of config
  loadHighscoreList(*highscore_list, file, error_code);
  loadGameBoard(*game_board, file, error_code);

  fclose(file);
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
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
  }
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
//
ReturnValue runGame(Board* game_board, char* restart)
{
  Command command = 0;
  Direction dir = 0;
  uint8_t row = 0;
  uint8_t col = 0;
  char stop = false;
  char round = 1;

  while(!stop)
  {
    printMap(
      game_board->map, 
      game_board->map_width, 
      game_board->map_height, 
      game_board->start, 
      game_board->end
    );

    command = getInput(round, &row, &col, &dir);
    if (command == NONE)
    {
      return OUT_OF_MEMORY;
    }

    runCommand(command, game_board, row, col, dir, &stop);
    round++;

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

  return SUCCESS;
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
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
    round++;
    return QUIT;
  }

  Command command = NONE;
  char* ret;

  ret = parseCommand(input, &command, (size_t*)dir, row, col);
  free(input);

  if (ret == NULL)
  {
    if (command != NONE)
    {
      round++;
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

  return getInput(round, row, col, dir);
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
//
void runCommand(Command command, Board* game_board, uint8_t row, uint8_t col, Direction dir, char* stop)
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
    rotatePipe(game_board, row, col, dir);
    break;
  
  default:
    break;
  }
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
//
void rotatePipe(Board* game_board, uint8_t row, uint8_t col, Direction dir)
{
  if (row > game_board->map_height || col > game_board->map_width)
  {
    printf(USAGE_COMMAND_ROTATE);
    return;
  }

  if ((row - 1 == game_board->start[0] && col - 1 == game_board->start[1])
    || (row - 1 == game_board->end[0] && col - 1 == game_board->end[1]))
  {
    printf(ERROR_ROTATE_INVALID);
    return;
  }

  uint8_t new_pipe = game_board->map[row - 1][col - 1];

  if (dir == LEFT)
  {
    uint8_t right = 0x03;
    new_pipe = ((new_pipe & right) << 6) | (new_pipe >> 2);
  } 
  else if (dir == RIGHT)
  {

    uint8_t left = 0xC0;
    new_pipe = ((new_pipe & left) >> 6) | (new_pipe << 2);
  } 

  //STOPPED HERE 
  //TODO: set connected bits

  game_board->map[row - 1][col - 1] = new_pipe;
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
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
