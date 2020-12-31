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
FILE* openConfigFile(char* file_name, ReturnValue* error_code);
void loadConfigFile(Board** game_board, Highscore** highscore_list, FILE* file, ReturnValue* error_code);
void loadHighscoreList(Highscore* highscore_list, FILE* file, ReturnValue* error_code);
void loadGameBoard(Board* game_board, FILE* file, ReturnValue* error_code);

Command getInput(uint8_t* row, uint8_t* col, Direction* dir);
void runCommand(Command command, uint8_t row, uint8_t col, Direction dir, char* stop);

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

  FILE* file = openConfigFile(argv[1], &error_code);
  if (error_code != SUCCESS)
  {
    return exitApplication(error_code, argv[1]);
  }

  Board* game_board = NULL;
  Highscore* highscore_list = NULL;
  loadConfigFile(&game_board, &highscore_list, file, &error_code);
  if (error_code != SUCCESS)
  {
    freeResources(game_board, highscore_list);
    return exitApplication(error_code, NULL);
  }

  Command command = 0;
  Direction dir = 0;
  uint8_t row = 0;
  uint8_t col = 0;

  char stop = 0;

  while(!stop)
  {
    printMap(
      game_board->map, 
      game_board->map_width, 
      game_board->map_height, 
      game_board->start, 
      game_board->end
    );

    command = getInput(&row, &col, &dir);
    if (command == NONE)
    {
      error_code = OUT_OF_MEMORY;
      break;
    }

    runCommand(command, row, col, dir, &stop);

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

  freeResources(game_board, highscore_list);
  return exitApplication(error_code, NULL);
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
  *game_board = malloc(sizeof(Board));         //TODO: malloc in main (?)
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

  /* TODO: Remove
  printf("Width: %d\n", game_board->map_width);
  printf("Height: %d\n", game_board->map_height);
  printf("# of Highscores: %d\n", highscore_list->count);

  printf("Start Row: %d\n", game_board->start[0]);
  printf("Start Col: %d\n", game_board->start[1]);
  */

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

      //TODO remove
      //printf("%s\n", pipeToChar(game_board->map[row_index][col_index]));
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
Command getInput(uint8_t* row, uint8_t* col, Direction* dir)
{
  static unsigned round = 1;
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

  return getInput(row, col, dir);
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
//
void runCommand(Command command, uint8_t row, uint8_t col, Direction dir, char* stop)
{
  row = 0;
  col = 0;
  dir = 0;

  switch (command)
  {
  case QUIT:
    *stop = 1;
    break;

  case HELP:
    printf(HELP_TEXT);
    break;

  case RESTART:
    *stop = 1;
    break;

  case ROTATE:
    *stop = 1;
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
void freeResources(Board* game_board, Highscore* highscore_list)
{
  free(highscore_list->entries);
  free(highscore_list);

  for (int i = 0; i < game_board->map_height; i++)
  {
    free(game_board->map[i]);
  }
  free(game_board->map);
  free(game_board);
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
