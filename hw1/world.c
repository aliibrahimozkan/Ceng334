#include "limits.h"
#include "logging.h"
#include "string.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PIPE(fd) socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd)

coordinate *sortingCor(coordinate info[MONSTER_LIMIT], int alive, char *sym) {
  int data, j;
  for (int i = 1; i < alive; i++) {
    data = info[i].x;
    int temp1 = info[i].x;
    int temp2 = info[i].y;
    char temp3 = sym[i];
    j = i - 1;

    while (j >= 0 && (info[j].x) > data) {
      info[j + 1].x = info[j].x;
      info[j + 1].y = info[j].y;
      sym[j + 1] = sym[j];
      j = j - 1;
    }
    info[j + 1].x = temp1;
    info[j + 1].y = temp2;
    sym[j + 1] = temp3;
  }
  for (int i = 1; i < alive; i++) {
    data = info[i].y;
    int temp1 = info[i].x;
    int temp2 = info[i].y;
    char temp3 = sym[i];
    j = i - 1;

    while (j >= 0 && (info[j].y) > data) {
      info[j + 1].x = info[j].x;
      info[j + 1].y = info[j].y;
      sym[j + 1] = sym[j];
      j = j - 1;
    }
    info[j + 1].x = temp1;
    info[j + 1].y = temp2;
    sym[j + 1] = temp3;
  }

  return info;
}

int main() {
  int width_of_the_room, height_of_the_room;
  char *x_position_of_the_door = (char *)malloc(1024);
  char *y_position_of_the_door = (char *)malloc(1024);
  int x_position_of_the_player, y_position_of_the_player;
  char *executable_of_the_player = (char *)malloc(PATH_MAX);
  char *playerArg3 = (char *)malloc(1024);
  char *playerArg4 = (char *)malloc(1024);
  char *playerArg5 = (char *)malloc(1024);
  int number_of_monster;
  scanf("%d", &width_of_the_room);
  scanf("%d", &height_of_the_room);
  scanf("%s", x_position_of_the_door);
  scanf("%s", y_position_of_the_door);
  scanf("%d", &x_position_of_the_player);
  scanf("%d", &y_position_of_the_player);
  scanf("%s", executable_of_the_player);
  scanf("%s", playerArg3);
  scanf("%s", playerArg4);
  scanf("%s", playerArg5);
  scanf("%d", &number_of_monster);

  int xPositionOfMonsters[number_of_monster],
      yPositionOfMonsters[number_of_monster];

  char symbol[MONSTER_LIMIT];
  char sym[MONSTER_LIMIT];

  char *monster_executable = (char *)malloc(PATH_MAX);
  char monster_exec[MONSTER_LIMIT][PATH_MAX];
  char monsterArg1[MONSTER_LIMIT][1024];
  char monsterArg2[MONSTER_LIMIT][1024];
  char monsterArg3[MONSTER_LIMIT][1024];
  char monsterArg4[MONSTER_LIMIT][1024];

  char *temp2 = (char *)malloc(MONSTER_LIMIT);
  char *temp3 = (char *)malloc(1024);
  char *temp4 = (char *)malloc(1024);
  char *temp5 = (char *)malloc(1024);
  char *temp6 = (char *)malloc(1024);

  for (int i = 0; i < number_of_monster; i++) {

    scanf("%s", monster_executable);
    scanf("%s", temp2);
    scanf("%d", &xPositionOfMonsters[i]);
    scanf("%d", &yPositionOfMonsters[i]);
    scanf("%s", temp3);
    scanf("%s", temp4);
    scanf("%s", temp5);
    scanf("%s", temp6);
    strcpy(monster_exec[i], monster_executable);
    strcpy(monsterArg1[i], temp3);
    strcpy(monsterArg2[i], temp4);
    strcpy(monsterArg3[i], temp5);
    strcpy(monsterArg4[i], temp6);
    symbol[i] = *temp2;
  }

  int playerPipe[2];
  int monsterPipe[number_of_monster][2];

  player_response playerResponse;
  player_message playerMessage;

  int *map =
      (int *)malloc(height_of_the_room * width_of_the_room * sizeof(int));

  for (int i = 0; i < height_of_the_room; i++) {
    for (int j = 0; j < width_of_the_room; j++) {
      if (i == 0 || j == 0 || i == height_of_the_room - 1 ||
          j == width_of_the_room - 1) {
        map[i * width_of_the_room + j] = 1;
      } else {
        map[i * width_of_the_room + j] = 0;
      }
    }
  }
  map[y_position_of_the_player * width_of_the_room + x_position_of_the_player] =
      1;

  map_info *mapInfo = malloc(sizeof(struct map_info));
  mapInfo->door.x = atoi(x_position_of_the_door);
  mapInfo->door.y = atoi(y_position_of_the_door);
  mapInfo->map_height = height_of_the_room;
  mapInfo->map_width = width_of_the_room;
  mapInfo->player.x = x_position_of_the_player;
  mapInfo->alive_monster_count = number_of_monster;
  mapInfo->player.y = y_position_of_the_player;
  for (int i = 0; i < number_of_monster; i++) {

    sym[i] = symbol[i];
    mapInfo->monster_coordinates[i].x = xPositionOfMonsters[i];
    mapInfo->monster_coordinates[i].y = yPositionOfMonsters[i];
    map[yPositionOfMonsters[i] * width_of_the_room + xPositionOfMonsters[i]] =
        1;
  }

  int aliveMonster[number_of_monster];

  coordinate *s =
      sortingCor(mapInfo->monster_coordinates, number_of_monster, symbol);
  for (int i = 0; i < number_of_monster; i++) {

    mapInfo->monster_types[i] = symbol[i];
    mapInfo->monster_coordinates[i].x = s[i].x;
    mapInfo->monster_coordinates[i].y = s[i].y;
  }

  playerMessage = (player_message){
      .new_position.x = x_position_of_the_player,
      .new_position.y = y_position_of_the_player,
      .total_damage = 0,
      .alive_monster_count = number_of_monster,
      .game_over = 0,
  };

  for (int i = 0; i < number_of_monster; i++) {
    playerMessage.monster_coordinates[i].x = s[i].x;
    playerMessage.monster_coordinates[i].y = s[i].y;
    aliveMonster[i] = 1;
  }
  monster_response *monsterResponse =
      malloc(sizeof(monster_response) * number_of_monster);
  monster_message *monsterMessage =
      malloc(sizeof(monster_message) * number_of_monster);
  memset(monsterResponse, 0, sizeof(monster_response) * number_of_monster);
  memset(monsterMessage, 0, sizeof(monster_message) * number_of_monster);

  for (int i = 0; i < number_of_monster; i++) {
    monsterMessage[i].game_over = 0;
    monsterMessage[i].player_coordinate.x = x_position_of_the_player;
    monsterMessage[i].player_coordinate.y = y_position_of_the_player;
    monsterMessage[i].damage = 0;
    monsterMessage[i].new_position.x = xPositionOfMonsters[i];
    monsterMessage[i].new_position.y = yPositionOfMonsters[i];
  }

  pid_t childPidPlayer;
  pid_t monsterPidPlayer[number_of_monster];
  int childStatus[number_of_monster];

  int count = 0;
  if (PIPE(playerPipe) < 0) {
    perror("pipe error");
  }
  if ((childPidPlayer = fork()) > 0) {
    close(playerPipe[1]);
  } else {
    close(playerPipe[0]);
    dup2(playerPipe[1], 1);
    dup2(playerPipe[1], 0);
    close(playerPipe[1]);
    execl(executable_of_the_player, executable_of_the_player,
          x_position_of_the_door, y_position_of_the_door, playerArg3,
          playerArg4, playerArg5, (char *)NULL);
  }
  for (int i = 0; i < number_of_monster; i++) {

    if (PIPE(monsterPipe[i]) < 0) {
      perror("pipe error");
    }
    if ((monsterPidPlayer[i] = fork()) == 0) {
      close(monsterPipe[i][0]);
      dup2(monsterPipe[i][1], 1);
      dup2(monsterPipe[i][1], 0);
      close(monsterPipe[i][1]);
      execl(monster_exec[i], monster_exec[i], monsterArg1[i], monsterArg2[i],
            monsterArg3[i], monsterArg4[i], (char *)NULL);
    } else {
      close(monsterPipe[i][1]);
    }
  }

  while (1) {
    read(playerPipe[0], &playerResponse, sizeof(player_response));
    if (playerResponse.pr_type == 0) {
      count++;
      break;
    }
  }

  for (int i = 0; i < number_of_monster; i++) {
    while (1) {
      read(monsterPipe[i][0], &monsterResponse[i], sizeof(monster_response));
      if (monsterResponse[i].mr_type == 0) {
        count++;
        break;
      }
    }
  }
  int gameOver = 0;

  if (count == number_of_monster + 1) {
    print_map(mapInfo);
  }

  game_over_status gos;

  while (!gameOver) {
    playerMessage.game_over = 0;
    write(playerPipe[0], &playerMessage, sizeof(player_message));

    int readMessageLength =
        read(playerPipe[0], &playerResponse, sizeof(player_response));

    if (readMessageLength == 0) {
      gos = go_left;
      break;
    }
    if (playerResponse.pr_type == 1) {
      map[playerMessage.new_position.y * width_of_the_room +
          playerMessage.new_position.x] = 0;
      if ((playerResponse.pr_content.move_to.x ==
           atoi(x_position_of_the_door)) &&
          (playerResponse.pr_content.move_to.y ==
           atoi(y_position_of_the_door))) {
        gos = go_reached;
        break;
      }
      int inRoom = 0;

      if (playerResponse.pr_content.move_to.x > 0) {
        if (playerResponse.pr_content.move_to.x < width_of_the_room - 1) {
          if (playerResponse.pr_content.move_to.y > 0) {
            if (playerResponse.pr_content.move_to.y < height_of_the_room - 1) {
              inRoom = 1;
            }
          }
        }
      }

      if (inRoom) {
        if (map[playerResponse.pr_content.move_to.y * width_of_the_room +
                playerResponse.pr_content.move_to.x] == 0) {
          playerMessage.new_position.x = playerResponse.pr_content.move_to.x;
          playerMessage.new_position.y = playerResponse.pr_content.move_to.y;
        }
      }

      map[playerMessage.new_position.y * width_of_the_room +
          playerMessage.new_position.x] = 1;

    } else if (playerResponse.pr_type == 2) {
      for (int i = 0; i < playerMessage.alive_monster_count; i++) {
        int dam = 0;
        dam = playerResponse.pr_content.attacked[i];
        for (int j = 0; j < number_of_monster; j++) {
          if (aliveMonster[j]) {
            if ((monsterMessage[j].new_position.x ==
                 playerMessage.monster_coordinates[i].x) &&
                (monsterMessage[j].new_position.y ==
                 playerMessage.monster_coordinates[i].y)) {
              monsterMessage[j].damage = dam;
              break;
            }
          }
        }
      }
    } else if (playerResponse.pr_type == 3) {
      gameOver = 1;
      gos = go_died;
      break;
    } else {
      gos = go_left;
      break;
    }

    for (int i = 0; i < number_of_monster; i++) {
      monsterMessage[i].player_coordinate.x = playerMessage.new_position.x;
      monsterMessage[i].player_coordinate.y = playerMessage.new_position.y;

      write(monsterPipe[i][0], &monsterMessage[i], sizeof(monster_message));

      if (aliveMonster[i]) {
        read(monsterPipe[i][0], &monsterResponse[i], sizeof(monster_response));
      }
    }
    coordinate cor[playerMessage.alive_monster_count];
    coordinate cor2[playerMessage.alive_monster_count];
    char symbol2[playerMessage.alive_monster_count];

    int xx = 0;
    for (int j = 0; j < number_of_monster; j++) {
      if (aliveMonster[j]) {
        monsterMessage[j].damage = 0;
        cor[xx].x = monsterMessage[j].new_position.x;
        cor[xx].y = monsterMessage[j].new_position.y;
        symbol2[xx] = sym[j];
        xx++;
      }
    }

    coordinate *ss =
        sortingCor(cor, playerMessage.alive_monster_count, symbol2);
    playerMessage.total_damage = 0;

    int k = 0;
    for (int j = 0; j < playerMessage.alive_monster_count; j++) {
      for (int i = 0; i < number_of_monster; i++) {
        if (aliveMonster[i] == 1) {
          if (ss[j].x == monsterMessage[i].new_position.x &&
              ss[j].y == monsterMessage[i].new_position.y) {

            if (monsterResponse[i].mr_type == 1) {
              map[monsterMessage[i].new_position.y * width_of_the_room +
                  monsterMessage[i].new_position.x] = 0;
              int checkMove = 1;
              int inRoom = 0;
              if (monsterResponse[i].mr_content.move_to.x > 0) {
                if (monsterResponse[i].mr_content.move_to.x <
                    width_of_the_room - 1) {
                  if (monsterResponse[i].mr_content.move_to.y > 0) {
                    if (monsterResponse[i].mr_content.move_to.y <
                        height_of_the_room - 1) {
                      inRoom = 1;
                    }
                  }
                }
              }

              if (inRoom) {
                if (monsterResponse[i].mr_content.move_to.x ==
                        playerMessage.new_position.x &&
                    monsterResponse[i].mr_content.move_to.y ==
                        playerMessage.new_position.y) {
                  checkMove = 0;
                }
                if (map[monsterResponse[i].mr_content.move_to.y *
                            width_of_the_room +
                        monsterResponse[i].mr_content.move_to.x] == 1) {
                  checkMove = 0;
                }
              }
              if (checkMove) {
                monsterMessage[i].new_position.x =
                    monsterResponse[i].mr_content.move_to.x;
                monsterMessage[i].new_position.y =
                    monsterResponse[i].mr_content.move_to.y;
              }
              map[monsterMessage[i].new_position.y * width_of_the_room +
                  monsterMessage[i].new_position.x] = 1;

              cor2[k].x = monsterMessage[i].new_position.x;
              cor2[k].y = monsterMessage[i].new_position.y;
              symbol2[k] = sym[i];
              k++;
            } else if (monsterResponse[i].mr_type == 2) {
              playerMessage.total_damage +=
                  monsterResponse[i].mr_content.attack;
              cor2[k].x = monsterMessage[i].new_position.x;
              cor2[k].y = monsterMessage[i].new_position.y;
              symbol2[k] = sym[i];

              k++;
            } else {
              aliveMonster[i] = 0;
              map[monsterMessage[i].new_position.y * width_of_the_room +
                  monsterMessage[i].new_position.x] = 0;
              close(monsterPipe[i][0]);
              waitpid(monsterPidPlayer[i], &childStatus[i], 0);
            }
          }
        }
      }
    }

    if (k == 0) {
      gos = go_survived;
      playerMessage.game_over = 1;
      write(playerPipe[0], &playerMessage, sizeof(struct player_message));
      break;
    }
    playerMessage.alive_monster_count = k;

    mapInfo->door.x = atoi(x_position_of_the_door);
    mapInfo->door.y = atoi(y_position_of_the_door);
    mapInfo->map_height = height_of_the_room;
    mapInfo->map_width = width_of_the_room;
    mapInfo->player.x = playerMessage.new_position.x;
    mapInfo->alive_monster_count = k;
    mapInfo->player.y = playerMessage.new_position.y;

    coordinate *ss2 = sortingCor(cor2, k, symbol2);
    for (int i = 0; i < k; i++) {

      mapInfo->monster_types[i] = symbol2[i];
      mapInfo->monster_coordinates[i].x = ss2[i].x;
      mapInfo->monster_coordinates[i].y = ss2[i].y;
      symbol[i] = symbol2[i];
    }

    for (int i = 0; i < k; i++) {
      playerMessage.monster_coordinates[i].x = ss2[i].x;
      playerMessage.monster_coordinates[i].y = ss2[i].y;
    }

    print_map(mapInfo);
  }

  if (gos == go_survived) {

    mapInfo->door.x = atoi(x_position_of_the_door);
    mapInfo->door.y = atoi(y_position_of_the_door);
    mapInfo->map_height = height_of_the_room;
    mapInfo->map_width = width_of_the_room;
    mapInfo->player.x = playerMessage.new_position.x;
    mapInfo->alive_monster_count = 0;
    mapInfo->player.y = playerMessage.new_position.y;

    close(playerPipe[0]);

    waitpid(childPidPlayer, &childStatus[0], 0);

  } else if (gos == go_reached) {

    mapInfo->door.x = atoi(x_position_of_the_door);
    mapInfo->door.y = atoi(y_position_of_the_door);
    mapInfo->map_height = height_of_the_room;
    mapInfo->map_width = width_of_the_room;
    mapInfo->player.x = atoi(x_position_of_the_door);
    mapInfo->alive_monster_count = playerMessage.alive_monster_count;
    mapInfo->player.y = atoi(y_position_of_the_door);

    for (int i = 0; i < playerMessage.alive_monster_count; i++) {
      mapInfo->monster_types[i] = symbol[i];
      mapInfo->monster_coordinates[i].x =
          playerMessage.monster_coordinates[i].x;
      mapInfo->monster_coordinates[i].y =
          playerMessage.monster_coordinates[i].y;
    }

    playerMessage.game_over = 1;
    write(playerPipe[0], &playerMessage, sizeof(struct player_message));
    close(playerPipe[0]);

    waitpid(childPidPlayer, &childStatus[0], 0);

  } else if (gos == go_died) {

    mapInfo->door.x = atoi(x_position_of_the_door);
    mapInfo->door.y = atoi(y_position_of_the_door);
    mapInfo->map_height = height_of_the_room;
    mapInfo->map_width = width_of_the_room;
    mapInfo->player.x = playerMessage.new_position.x;
    mapInfo->alive_monster_count = playerMessage.alive_monster_count;
    mapInfo->player.y = playerMessage.new_position.y;

    for (int i = 0; i < playerMessage.alive_monster_count; i++) {
      mapInfo->monster_types[i] = symbol[i];
      mapInfo->monster_coordinates[i].x =
          playerMessage.monster_coordinates[i].x;
      mapInfo->monster_coordinates[i].y =
          playerMessage.monster_coordinates[i].y;
    }
    close(playerPipe[0]);

    waitpid(childPidPlayer, &childStatus[0], 0);

  } else {

    mapInfo->door.x = atoi(x_position_of_the_door);
    mapInfo->door.y = atoi(y_position_of_the_door);
    mapInfo->map_height = height_of_the_room;
    mapInfo->map_width = width_of_the_room;
    mapInfo->player.x = playerMessage.new_position.x;
    mapInfo->alive_monster_count = playerMessage.alive_monster_count;
    mapInfo->player.y = playerMessage.new_position.y;

    for (int i = 0; i < playerMessage.alive_monster_count; i++) {
      mapInfo->monster_types[i] = symbol[i];
      mapInfo->monster_coordinates[i].x =
          playerMessage.monster_coordinates[i].x;
      mapInfo->monster_coordinates[i].y =
          playerMessage.monster_coordinates[i].y;
    }
    close(playerPipe[0]);
    waitpid(childPidPlayer, &childStatus[0], 0);
  }

  for (int j = 0; j < number_of_monster; j++) {
    if (aliveMonster[j]) {
      monsterMessage[j].game_over = 1;
      write(monsterPipe[j][0], &monsterMessage[j], sizeof(monster_message));
      close(monsterPipe[j][0]);
      waitpid(monsterPidPlayer[j], &childStatus[j], 0);
    }
  }

  print_map(mapInfo);
  print_game_over(gos);
  free(monsterMessage);
  free(monsterResponse);
  free(mapInfo);
  free(x_position_of_the_door);
  free(y_position_of_the_door);
  free(map);
  free(temp2);
  free(playerArg3);
  free(playerArg4);
  free(playerArg5);
  free(executable_of_the_player);
  free(monster_executable);
  free(temp3);
  free(temp4);
  free(temp5);
  free(temp6);

  return 0;
}
