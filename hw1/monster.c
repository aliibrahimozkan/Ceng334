#include "message.h"
#include "string.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int man_dist(int playerX, int playerY, int monsterX, int monsterY) {
  int dist = 0;
  dist = abs(playerX - monsterX) + abs(playerY - monsterY);
  return dist;
}
int *calc_new_dist(int playerX, int playerY, int monsterX, int monsterY) {
  int temp[8];
  temp[0] = man_dist(playerX, playerY, monsterX, monsterY - 1);
  temp[1] = man_dist(playerX, playerY, monsterX + 1, monsterY - 1);
  temp[2] = man_dist(playerX, playerY, monsterX + 1, monsterY);
  temp[3] = man_dist(playerX, playerY, monsterX + 1, monsterY + 1);
  temp[4] = man_dist(playerX, playerY, monsterX, monsterY + 1);
  temp[5] = man_dist(playerX, playerY, monsterX - 1, monsterY + 1);
  temp[6] = man_dist(playerX, playerY, monsterX - 1, monsterY);
  temp[7] = man_dist(playerX, playerY, monsterX - 1, monsterY - 1);
  int t = temp[0];
  int val = 0;
  for (int i = 1; i < 8; i++) {
    if (t > temp[i]) {
      t = temp[i];
      val = i;
    }
  }
  static int retVal[2];
  if (val == 0) {
    retVal[0] = monsterX;
    retVal[1] = monsterY - 1;
  } else if (val == 1) {
    retVal[0] = monsterX + 1;
    retVal[1] = monsterY - 1;
  } else if (val == 2) {
    retVal[0] = monsterX + 1;
    retVal[1] = monsterY;
  } else if (val == 3) {
    retVal[0] = monsterX + 1;
    retVal[1] = monsterY + 1;
  } else if (val == 4) {
    retVal[0] = monsterX;
    retVal[1] = monsterY + 1;
  } else if (val == 5) {
    retVal[0] = monsterX - 1;
    retVal[1] = monsterY + 1;
  } else if (val == 6) {
    retVal[0] = monsterX - 1;
    retVal[1] = monsterY;
  } else {
    retVal[0] = monsterX - 1;
    retVal[1] = monsterY - 1;
  }
  return retVal;
}

int maxInt(int x, int y) { return x > y ? x : y; }

int calc_health(int oldHealth, int dam, int def) {
  int new_health;
  new_health = oldHealth - maxInt(0, dam - def);
  return new_health;
}

int main(int argc, char *argv[]) {

  int health = atoi(argv[1]);
  int damage = atoi(argv[2]);
  int defence = atoi(argv[3]);
  int range = atoi(argv[4]);
  printf("%d %d %d %d",health,damage,defence,range);
  monster_message message;
  monster_response resp;
/*
  resp = (monster_response){.mr_type = mr_ready, .mr_content.attack = 0};
  write(1, &resp, sizeof(monster_response));
  int gameOver = 0;
  while (!gameOver) {
    read(0, &message, sizeof(monster_message));
    if (message.game_over == 1) {
      gameOver = 1;
      break;
    }

    health = calc_health(health, message.damage, defence);

    if (health <= 0) {
      resp = (monster_response){.mr_type = mr_dead, .mr_content.attack = 0};
      write(1, &resp, sizeof(monster_response));
      gameOver = 1;
      break;
    }
    if (range >= man_dist(message.player_coordinate.x,
                          message.player_coordinate.y, message.new_position.x,
                          message.new_position.y)) {
      resp.mr_type = mr_attack;
      resp.mr_content.attack = damage;

    } else if (range < man_dist(message.player_coordinate.x,
                                message.player_coordinate.y,
                                message.new_position.x,
                                message.new_position.y)) {
      int *arr;
      resp.mr_type = mr_move;
      resp.mr_content.attack = 0;
      arr = calc_new_dist(message.player_coordinate.x,
                          message.player_coordinate.y, message.new_position.x,
                          message.new_position.y);
      resp.mr_content.move_to.x = arr[0];
      resp.mr_content.move_to.y = arr[1];
    }

    write(1, &resp, sizeof(monster_response));
  }
*/
  return 0;
}