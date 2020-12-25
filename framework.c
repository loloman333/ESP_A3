#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "framework.h"

#define FRAMEWORK_GETLINE_BUFSIZE 16 * sizeof(char)
#define FRAMEWORK_COORD_TO_INDEX(width, row, col) (width * row + col)

// ----------------------------------------------------------------------------
char* pipeToChar(uint8_t pipe)
{
  switch (pipe & 0xAAu)
  {
    case 0xAAu:
      return "╬";
    case 0x2Au:
      return "╦";
    case 0x8Au:
      return "╠";
    case 0xA2u:
      return "╩";
    case 0xA8u:
      return "╣";
    case 0x88u:
      return "║";
    case 0x22u:
      return "═";
    case 0xA0u:
      return "╝";
    case 0x28u:
      return "╗";
    case 0x0Au:
      return "╔";
    case 0x82u:
      return "╚";
    case 0x0u:
      return "█";
    default:
      return "▞"; // invalid value
  }
}

// ----------------------------------------------------------------------------
char* specialPipeToChar(uint8_t pipe)
{
  switch (pipe & 0xAAu)
  {
    case 0x80u:
      return "╨";
    case 0x20u:
      return "╡";
    case 0x08u:
      return "╥";
    case 0x02u:
      return "╞";
    default:
      return "▞"; // invalid value
  }
}

// ----------------------------------------------------------------------------
uint8_t getNumberOfDigits(uint8_t number)
{
  if (number == 0)
  {
    return number;
  }
  return 1 + getNumberOfDigits(number / 10);
}

// ----------------------------------------------------------------------------
uint8_t power(uint8_t base, uint8_t exponent)
{
  if (exponent == 0)
  {
    return 1;
  }
  else if (exponent == 1)
  {
    return base;
  }
  else
  {
    return base * power(base, --exponent);
  }
}

// ----------------------------------------------------------------------------
void printMap(uint8_t** map, uint8_t width, uint8_t height, uint8_t start[2], uint8_t dest[2])
{
  uint8_t num_digits_row = getNumberOfDigits(height);
  uint8_t num_digits_col = getNumberOfDigits(width);

  printf("\n");

  // print column header
  for (uint8_t i = 0; i < num_digits_col; ++i)
  {
    for (uint8_t j = 0; j < num_digits_row; ++j)
    {
      printf(" ");
    }
    printf("│");
    for (uint8_t j = 1; j <= width; ++j)
    {
      uint8_t digit = j / power(10, (num_digits_col - i - 1)) % 10;
      printf("%u", digit);
    }
    printf("\n");
  }

  // print horizontal seperator
  for (uint8_t i = 0; i < num_digits_row; ++i)
  {
    printf("─");
  }
  printf("┼");
  for (uint8_t i = 0; i < width; ++i)
  {
    printf("─");
  }
  printf("\n");

  // print row header and map
  for (uint8_t row = 0; row < height; ++row)
  {
    printf("%0*u│", num_digits_row, row + 1);
    for (uint8_t col = 0; col < width; ++col)
    {
      if ((row == start[0] && col == start[1]) || (row == dest[0] && col == dest[1]))
      {
        printf("%s", specialPipeToChar(map[row][col]));
      }
      else
      {
        printf("%s", pipeToChar(map[row][col]));
      }
    }
    printf("\n");
  }
  printf("\n");
}

// ----------------------------------------------------------------------------
bool arePipesConnectedM(uint8_t** map, int8_t* path, uint8_t width, uint8_t height, uint8_t coord[2], uint8_t val)
{
  if (path[FRAMEWORK_COORD_TO_INDEX(width, coord[0], coord[1])] == -1)
  {
    return true;
  }
  path[FRAMEWORK_COORD_TO_INDEX(width, coord[0], coord[1])] = val;

  for (uint8_t dir = 0; dir < 4; ++dir)
  {
    if (!((dir == 0 && coord[0] == 0) || (dir == 1 && coord[1] == 0)
      || (dir == 2 && coord[0] >= height - 1) || (dir == 3 && coord[1] >= width - 1))
      && (map[coord[0]][coord[1]] & (0x1u << 2*(3 - dir))))
    {
      uint8_t new[2];
      new[0] = (dir % 2 == 0) ? coord[0] + (dir - 1) : coord[0];
      new[1] = (dir % 2 == 1) ? coord[1] + (dir - 2) : coord[1];
      if (path[FRAMEWORK_COORD_TO_INDEX(width, new[0], new[1])] <= 0
        && arePipesConnectedM(map, path, width, height, new, val + 1))
      {
        return true;
      }
    }
  }

  return false;
}

// ----------------------------------------------------------------------------
bool arePipesConnected(uint8_t** map, uint8_t width, uint8_t height, uint8_t start[2], uint8_t dest[2])
{
  //return arePipesConnectedR(map, width, height, start, dest, start);
  
  int8_t* path = (int8_t*) calloc(width * height, sizeof(int8_t));
  path[FRAMEWORK_COORD_TO_INDEX(width, dest[0], dest[1])] = -1;
  bool is_conn = arePipesConnectedM(map, path, width, height, start, 1);
  free(path);
  return is_conn;
}

// ----------------------------------------------------------------------------
char* getLine()
{
  size_t bufsize = 0;
  bool has_newline = false;
  char* line = NULL;
  char* line_end = NULL;

  clearerr(stdin);
  while(!(feof(stdin) || has_newline))
  {
    bufsize += FRAMEWORK_GETLINE_BUFSIZE;
    char* line_tmp = (char*) realloc(line, bufsize);
    if (line_tmp == NULL)
    {
      free(line);
      return NULL;
    }
    if (line_end == NULL)
    {
      line_end = line_tmp;
      *line_end = '\0';
    }
    else
    {
      line_end = line_end - line + line_tmp; // recalculate pointer
    }
    line = line_tmp;

    fgets(line_end, FRAMEWORK_GETLINE_BUFSIZE, stdin);
    if ((line_tmp = strchr(line_end, '\n')) != NULL)
    {
      has_newline = true;
      *line_tmp = '\0';
    }
    line_end += FRAMEWORK_GETLINE_BUFSIZE - sizeof(char);
  }

  if (feof(stdin))
  {
    free(line);
    return (char*) EOF;
  }

  return line;
}

// ----------------------------------------------------------------------------
bool parseCommandRotate(size_t* dir, uint8_t* row, uint8_t* col)
{
    // parse direction
    char *token = strtok(NULL, " \t\n");
    for (size_t i = 0; token != NULL && token[i] != '\0'; ++i)
    {
      token[i] = tolower(token[i]);
    }
    if (token != NULL && strcmp("left", token) == 0)
    {
      *dir = 1;
    }
    else if (token != NULL && strcmp("right", token) == 0)
    {
      *dir = 3;
    }
    else
    {
      return false;
    }

    // parse row and column
    token = strtok(NULL, " \t\n");
    if (token != NULL)
    {
      char** token_end = &token;
      int num = strtol(token, token_end, 10);
      if (num < 1 || strchr(" \t\n", **token_end) == NULL)
      {
        return false;
      }
      *row = (uint8_t) num;
      
      token = strtok(NULL, " \t\n");
      if (token != NULL)
      {
        num = strtol(token, token_end, 10);
        if (num < 1 || strchr(" \t\n", **token_end) == NULL)
        {
          return false;
        }
        *col = (uint8_t) num;

        // check for additional parameters
        token = strtok(NULL, " \t\n");
        if (token == NULL)
        {
          return true;
        }
      }
    }
    return false;
}

// ----------------------------------------------------------------------------
char* parseCommand(char* line, Command* cmd, size_t* dir, uint8_t* row, uint8_t* col)
{
  char* token = strtok(line, " \t\n");
  for (size_t i = 0; token != NULL && token[i] != '\0'; ++i)
  {
    token[i] = tolower(token[i]);
  }

  if (token == NULL)
  {
    *cmd = (size_t) NONE;
  }
  else if (strcmp("rotate", token) == 0)
  {
    bool success = parseCommandRotate(dir, row, col);
    *cmd = (size_t) ROTATE;
    if (!success)
    {
      return (char*) 1;
    }
  }
  else if (strcmp("help", token) == 0)
  {
    *cmd = (size_t) HELP;
  }
  else if (strcmp("quit", token) == 0)
  {
    *cmd = (size_t) QUIT;
  }
  else if (strcmp("restart", token) == 0)
  {
    *cmd = (size_t) RESTART;
  }
  else // unknown command
  {
    return token;
  }
  
  return NULL;
}

