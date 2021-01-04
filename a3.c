//-----------------------------------------------------------------------------
// a3.c
//
// ESPipes
// 
// 
//
// Group: 12
//
// Author: 12007661
//-----------------------------------------------------------------------------
//

// TODO
// Write Highscore back in Config File

// Includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "framework.h"

// Defines
#define MAGIC_NUMBER "ESPipes"
#define DO_RESTART 2
#define HIGHSCORE_NAME_LENGTH 3

#define MAX_UNIT8_T 0xFF
#define MIN_UNIT8_T 0x00
#define FILTER_RIGHT 0x03
#define FILTER_LEFT 0xC0
#define SWITCH 0x40

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

// Forward Definitions     TODO: order these the same way as below
ReturnValue loadGame(Board** game_board, Highscore** highscore_list, char* file_name, char** error_context);
FILE* openConfigFile(char* file_name, ReturnValue* error_code);
void loadConfigFile(Board** game_board, Highscore** highscore_list, FILE* file, ReturnValue* error_code);
void loadHighscoreList(Highscore* highscore_list, FILE* file, ReturnValue* error_code);
void loadGameBoard(Board* game_board, FILE* file, ReturnValue* error_code);

ReturnValue runGame(Board* game_board, int* score, char* restart);
Command getInput(char round, uint8_t* row, uint8_t* col, Direction* dir);
char runCommand(Command command, Board* game_board, uint8_t row, uint8_t col, Direction dir, char* stop);
char rotatePipe(Board* game_board, uint8_t row, uint8_t col, Direction dir);
void setConnectedBits(Board* game_board, uint8_t row, uint8_t col);
void setConnectedBitInDirection(Board* game_board, uint8_t row, uint8_t col, Direction dir);
char checkConnection(Board* game_board, uint8_t row, uint8_t col, Direction dir);

void handleScore(Highscore* highscore_list, int score);
char doesScoreBeatHighscore(Highscore* highscore_list, int score);
char* beatHighscore();
void printHighscore(Highscore* highscore_list);

void moveCoordiantesInDirection(uint8_t* row, uint8_t* col, Direction dir);
char areCoordinatesOnBoard(Board* game_board, uint8_t row, uint8_t col);
Direction getOppositeDirection(Direction dir);
char isPipeOpenInDirection(uint8_t pipe, Direction dir);

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
    handleScore(highscore_list, score);
  }

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
    highscore_list->entries[i].name[3] = '\0';
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
//
void handleScore(Highscore* highscore_list, int score)
{
  printf(INFO_PUZZLE_SOLVED);
  printf(INFO_SCORE, score);

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
  }

  printHighscore(highscore_list);   
}

//-----------------------------------------------------------------------------
/// 
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
//
void printHighscore(Highscore* highscore_list)
{
  printf(INFO_HIGHSCORE_HEADER);

  for (int i = 0; i < highscore_list->count; i++)
  {
    int score = highscore_list->entries[i].score;
    if (score == 0)
    {
      printf(INFO_HIGHSCORE_ENTRY, "---", score);
    } 
    else 
    {
      printf(INFO_HIGHSCORE_ENTRY, highscore_list->entries[i].name, score);
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
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
/// TODO
/// 
///
/// @return TODO
//
char isPipeOpenInDirection(uint8_t pipe, Direction dir)
{
  uint8_t checker = 0x80;
  
  return pipe & (checker >> (2 * dir));
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
