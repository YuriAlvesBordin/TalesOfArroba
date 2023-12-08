#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <math.h>
#include <termios.h>
#include <unistd.h>

//#define _WIN32
#define CLEAR_SCREEN system("clear");
//#define CLEAR_SCREEN printf("\x1b[2J\x1b[H")

#define MAX_BULLETS 5
#define MAX_ENEMIES 100
#define MAP_SIZE 25

char map[MAP_SIZE][MAP_SIZE];
int timeOfDay = 0;

typedef struct {
    int x, y, active, direction, hitFrames;
} Bullet;

typedef struct {
    int prevX, prevY, x, y, active, enemyType, counter;
} Enemy;

typedef struct {
    int x, y, xp, level, life;
} Player;

Player player = {MAP_SIZE/2, MAP_SIZE/2, 0, 1, 3};
Bullet bullets[MAX_BULLETS] = {0};
Enemy enemies[MAX_ENEMIES] = {0};

float noise(int x, int y) {
    int n = x + y * 57;
    n = (n << 13) ^ n;
    return (3 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

float smoothedNoise(int x, int y) {
    float corners = (noise(x - 1, y - 1) + noise(x + 1, y - 1) + noise(x - 1, y + 1) + noise(x + 1, y + 1)) / 16;
    float sides = (noise(x - 1, y) + noise(x + 1, y) + noise(x, y - 1) + noise(x, y + 1)) / 8;
    float center = noise(x, y) / 4;
    return corners + sides + center;
}

int generateHeight(int x, int y, int seed) {
    float value = seed * smoothedNoise(x, y);
    return (int)(value * 10) - 10; // Adjust the range as needed
}

void generateMap(char map[MAP_SIZE][MAP_SIZE], int playerX, int playerY,int seed) {
    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            int worldX = playerX - MAP_SIZE / 2 + j;
            int worldY = playerY - MAP_SIZE / 2 + i;
            
            int height = generateHeight(worldX, worldY, seed);

            if (height > 20 * seed/2) {
                map[i][j] = '#'; // Paredes
            } else if (height >= 14 * seed/2 && height <= 20 * seed/2) {
                map[i][j] = '"'; // Grama
            } else {
                map[i][j] = '~'; // Água
            }
        }
    }
}

void printBoard() {
    CLEAR_SCREEN;

    int seed = 1.5;
    generateMap(map, player.x, player.y, seed);

    printf("XP: [%d/%d] Level: %d\tLife: [%d/%d]\r\n", player.xp, (10 * player.level / 3), player.level, player.life, 3 * player.level);
    printf("Time: %d\r\n", timeOfDay);

    // Adjust colors based on time of day
    const char *colorCode;
    if (timeOfDay == 0) {
        colorCode = "\033[0m"; // Daytime color
    } else {
        colorCode = "\033[2m"; // Nighttime color
    }

    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
            if (i == MAP_SIZE/2 && j == MAP_SIZE/2) {
                if (player.life == 1) {
                    printf("\033[1;31m@ \033[0m");
                } else if (player.life < 3 * player.level) {
                    printf("\033[1;33m@ \033[0m");
                } else {
                    printf("@ ");
                }
            } else {
                int isBullet = 0;
                for (int k = 0; k < MAX_BULLETS; k++) {
                    if (bullets[k].active && bullets[k].x == j && bullets[k].y == i) {
                        printf((bullets[k].hitFrames % 2 == 0) ? "\033[0;31mx \033[0m" : "\033[0;33m+ \033[0m");
                        isBullet = 1;
                        break;
                    }
                }
                if (!isBullet) {
                    int isEnemy = 0;
                        for (int k = 0; k < MAX_ENEMIES; k++) {
                            if (enemies[k].active && enemies[k].x == j && enemies[k].y == i) {
                                // Use a switch statement to handle different characters for each enemy type
                                char enemyChar;
                                if (enemies[k].enemyType == 0) {
                                    enemyChar = 'C'; // Character for enemy type 0
                                    printf("\033[0;35m%c \033[0m", enemyChar);
                                } else if (enemies[k].enemyType == 1) {
                                    enemyChar = 'F'; // Character for enemy type 1
                                    printf("\033[0;35m%c \033[0m", enemyChar);
                                } else if (enemies[k].enemyType == 2) {
                                    enemyChar = 'B'; // Character for enemy type 2
                                    printf("\033[0;35m%c \033[0m", enemyChar);
                                } else if (enemies[k].enemyType == 3) {
                                    enemyChar = '!'; // Character for enemy type 3
                                    printf("\033[0;35m%c \033[0m", enemyChar);
                                }
                                isEnemy = 1;
                                break;
                            }
                        }
                    if (!isEnemy) {
                        // Randomly animate empty spaces between "." and ","
                        const char *colorCode;
                            switch (map[i][j]) {
                                case '^':
                                    colorCode = "\033[0;32m"; // Darker green
                                    break;
                                case '"':
                                    colorCode = "\033[1;32m"; // Lighter green
                                    if (rand() % 5 == 0) {
                                        map[i][j] = '^';
                                    }
                                    break;
                                case '~':
                                    colorCode = "\033[0;34m"; // Blue for water
                                    if (rand() % 10 == 0) {
                                        map[i][j] = '-';
                                    }
                                    break;
                            default:
                                colorCode = "\033[0m"; // Default color
                        }
                        printf("%s%c\033[0m ", colorCode, map[i][j]);
                    }
                }
            }
        }
        printf("\r\n");
    }
}

void updatePlayer() {
    for (int i = 0; i < (3 * player.level / 3); i++) {
        if (enemies[i].active && MAP_SIZE / 2 == enemies[i].x && MAP_SIZE / 2 == enemies[i].y) {
            player.life--;
            enemies[i].active = 0;
        }
    }
                if (player.life <= 0) {
                // Reset player position and attributes
                player.x = MAP_SIZE / 2;
                player.y = MAP_SIZE / 2;
                player.level = 1;
                player.life = 3;

                // Reset bullets
                for (int j = 0; j < MAX_BULLETS; j++) {
                    bullets[j] = (Bullet){0};
                }

                // Reset enemies
                for (int j = 0; j < MAX_ENEMIES; j++) {
                    enemies[j] = (Enemy){0};
                }
                timeOfDay = 0;
            }
}

void updateBullets(char map[MAP_SIZE][MAP_SIZE]) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            
            int nextX = bullets[i].x;
            int nextY = bullets[i].y;

            switch (bullets[i].direction) {
                case 0: nextY -= 1; break;
                case 1: nextX -= 1; break;
                case 2: nextY += 1; break;
                case 3: nextX += 1; break;
            }

            // Animate the bullet movement
            for (int step = 0; step < 2; step++) {
                CLEAR_SCREEN; // Clear the console before printing the updated board
                printBoard();
            }

            // Check if the next position is not a wall
            if (nextX >= 0 && nextX < MAP_SIZE && nextY >= 0 && nextY < MAP_SIZE && map[nextY][nextX] != '#') {
                bullets[i].x = nextX;
                bullets[i].y = nextY;
            } else {
                bullets[i].active = 0;  // Deactivate bullet if it hits a wall
            }

            // Additional logic for handling bullet-player collision
            if (bullets[i].x == MAP_SIZE/2 && bullets[i].y == MAP_SIZE/2) {
                player.life--;
            }

            // Additional logic for handling bullet-enemy collision
            for (int j = 0; j < (3 * player.level / 3); j++) {
                if (enemies[j].active && bullets[i].x == enemies[j].x && bullets[i].y == enemies[j].y) {
                    bullets[i].active = 0;
                    enemies[j].active = 0;
                    player.xp++;

                    if (player.xp >= (10 * player.level / 3)) {
                        player.xp = 0;
                        player.level++;
                        player.life = 3 * player.level;
                    }
                    break;
                }
            }

            bullets[i].hitFrames++;
        }
    }
}

int isBulletPath(int x, int y, Bullet bullets[MAX_BULLETS]) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active && bullets[i].x == x && bullets[i].y == y) {
            return 1;  // Bullet in path
        }
    }
    return 0;  // No bullet in path
}

void spawnEnemy() {
    if (timeOfDay > 18 || timeOfDay < 6) {
        for (int i = 0; i < (3 * player.level / 3); i++) {
            if (!enemies[i].active) {
                // Generate random coordinates for the enemy at the borders of the map
                int side = rand() % 4; // 0: top, 1: right, 2: bottom, 3: left
                int randX, randY;

                switch (side) {
                    case 0: // Top side
                        randX = rand() % MAP_SIZE;
                        randY = 0;
                        break;
                    case 1: // Right side
                        randX = MAP_SIZE - 1;
                        randY = rand() % MAP_SIZE;
                        break;
                    case 2: // Bottom side
                        randX = rand() % MAP_SIZE;
                        randY = MAP_SIZE - 1;
                        break;
                    case 3: // Left side
                        randX = 0;
                        randY = rand() % MAP_SIZE;
                        break;
                }

                enemies[i].x = randX;
                enemies[i].y = randY;
                enemies[i].active = 1;
                enemies[i].enemyType = rand() % 3;
                enemies[i].counter = 0; // Add a counter to control enemy spawning frequency
                break;
            }
        }
    }
}

void updateEnemies() {
    // Introduce a counter to control the movement frequency of enemies
    static int enemyMoveCounter = 0;

    // Only update enemies' positions every few iterations
    if (enemyMoveCounter % 4 == 0) {
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                int playerX = player.x;
                int playerY = player.y;

                // Calculate the direction towards the player
                int deltaX = playerX - enemies[i].x;
                int deltaY = playerY - enemies[i].y;

                // Update enemy position based on behavior type
                switch (enemies[i].enemyType) {
                    case 0:  // Basic behavior (chase player)
                        if (abs(deltaX) > abs(deltaY)) {
                            enemies[i].x += (deltaX > 0 && map[enemies[i].y][enemies[i].x + 1] != '#') ? 1 : (deltaX < 0 && map[enemies[i].y][enemies[i].x - 1] != '#') ? -1 : 0;
                        } else {
                            enemies[i].y += (deltaY > 0 && map[enemies[i].y + 1][enemies[i].x] != '#') ? 1 : (deltaY < 0 && map[enemies[i].y - 1][enemies[i].x] != '#') ? -1 : 0;
                        }
                        break;
                    case 1:  // Flee player
                        if (abs(deltaX) > abs(deltaY)) {
                            enemies[i].x -= (deltaX > 0 && map[enemies[i].y][enemies[i].x - 1] != '#') ? 1 : (deltaX < 0 && map[enemies[i].y][enemies[i].x + 1] != '#') ? -1 : 0;
                        } else {
                            enemies[i].y -= (deltaY > 0 && map[enemies[i].y - 1][enemies[i].x] != '#') ? 1 : (deltaY < 0 && map[enemies[i].y + 1][enemies[i].x] != '#') ? -1 : 0;
                        }
                        break;
                    case 2:  // Flee bullets
                        if (isBulletPath(enemies[i].x, enemies[i].y, bullets)) {
                            enemies[i].x += (deltaX > 0 && map[enemies[i].y][enemies[i].x - 1] != '#') ? -1 : (deltaX < 0 && map[enemies[i].y][enemies[i].x + 1] != '#') ? 1 : 0;
                            enemies[i].y += (deltaY > 0 && map[enemies[i].y - 1][enemies[i].x] != '#') ? -1 : (deltaY < 0 && map[enemies[i].y + 1][enemies[i].x] != '#') ? 1 : 0;
                        } else {
                            // If no bullets, behave like basic chasing
                            if (abs(deltaX) > abs(deltaY)) {
                                enemies[i].x += (deltaX > 0 && map[enemies[i].y][enemies[i].x + 1] != '#') ? 1 : (deltaX < 0 && map[enemies[i].y][enemies[i].x - 1] != '#') ? -1 : 0;
                            } else {
                                enemies[i].y += (deltaY > 0 && map[enemies[i].y + 1][enemies[i].x] != '#') ? 1 : (deltaY < 0 && map[enemies[i].y - 1][enemies[i].x] != '#') ? -1 : 0;
                            }
                        }
                        break;
                    case 3:  // Flee bullets + flee player + get into player
                        // Complex behavior - you can combine the behaviors as needed
                        // For example, flee bullets, flee player, and attempt to get closer to player
                        if (isBulletPath(enemies[i].x, enemies[i].y, bullets)) {
                            enemies[i].x += (deltaX > 0 && map[enemies[i].y][enemies[i].x - 1] != '#') ? -1 : (deltaX < 0 && map[enemies[i].y][enemies[i].x + 1] != '#') ? 1 : 0;
                            enemies[i].y += (deltaY > 0 && map[enemies[i].y - 1][enemies[i].x] != '#') ? -1 : (deltaY < 0 && map[enemies[i].y + 1][enemies[i].x] != '#') ? 1 : 0;
                        } else {
                            enemies[i].x -= (deltaX > 0 && map[enemies[i].y][enemies[i].x + 1] != '#') ? 1 : (deltaX < 0 && map[enemies[i].y][enemies[i].x - 1] != '#') ? -1 : 0;
                            enemies[i].y -= (deltaY > 0 && map[enemies[i].y + 1][enemies[i].x] != '#') ? -1 : (deltaY < 0 && map[enemies[i].y - 1][enemies[i].x] != '#') ? 1 : 0;
                        }
                        break;
                }


                // Update previous position for the next iteration
                enemies[i].prevX = enemies[i].x;
                enemies[i].prevY = enemies[i].y;
            }
        }
    }

    // Increment the counter for the next iteration
    enemyMoveCounter++;
}

void handleInput(char map[MAP_SIZE][MAP_SIZE]) {
    char move;

#ifdef _WIN32
    if (_kbhit()) {
        move = _getch();
    } else {
        move = 0;
    }
#else
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        move = getchar();
    } else {
        move = 0;
    }
#endif

    int newX = player.x;
    int newY = player.y;

    switch (move) {
        case 'w':
            newY = (map[MAP_SIZE / 2 - 1][MAP_SIZE / 2] != '#') ? player.y - 1 : player.y;

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active && map[enemies[i].y + 1][enemies[i].x] != '#' && map[MAP_SIZE / 2 - 1][MAP_SIZE / 2] != '#') {
                    enemies[i].y += 1;  // Move enemies in the opposite direction
                }
            }
            break;
        case 'a':
            newX = (map[MAP_SIZE / 2][MAP_SIZE / 2 - 1] != '#') ? player.x - 1 : player.x;

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active && map[enemies[i].y][enemies[i].x + 1] != '#' && map[MAP_SIZE / 2][MAP_SIZE / 2 - 1] != '#') {
                    enemies[i].x += 1;  // Move enemies in the opposite direction
                }
            }
            break;
        case 's':
            newY = (map[MAP_SIZE / 2 + 1][MAP_SIZE / 2] != '#') ? player.y + 1 : player.y;

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active && map[enemies[i].y - 1][enemies[i].x] != '#' && map[MAP_SIZE / 2 + 1][MAP_SIZE / 2] != '#') {
                    enemies[i].y -= 1;  // Move enemies in the opposite direction
                }
            }
            break;
        case 'd':
            newX = (map[MAP_SIZE / 2][MAP_SIZE / 2 + 1] != '#') ? player.x + 1 : player.x;

            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (enemies[i].active && map[enemies[i].y][enemies[i].x - 1] != '#') {
                    enemies[i].x -= 1;  // Move enemies in the opposite direction
                }
            }
            break;
        case 27:
#ifdef _WIN32
            move = _getch();
#else
            getchar();
            move = getchar();
#endif
            switch (move) {
                case 'A':
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (!bullets[i].active) {
                            bullets[i].x = MAP_SIZE / 2;
                            bullets[i].y = MAP_SIZE / 2;
                            bullets[i].active = 1;
                            bullets[i].direction = 0;
                            bullets[i].hitFrames = 0;
                            break;
                        }
                    }
                    break;
                case 'B':
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (!bullets[i].active) {
                            bullets[i].x = MAP_SIZE / 2;
                            bullets[i].y = MAP_SIZE / 2;
                            bullets[i].active = 1;
                            bullets[i].direction = 2;
                            bullets[i].hitFrames = 0;
                            break;
                        }
                    }
                    break;
                case 'C':
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (!bullets[i].active) {
                            bullets[i].x = MAP_SIZE / 2;
                            bullets[i].y = MAP_SIZE / 2;
                            bullets[i].active = 1;
                            bullets[i].direction = 3;
                            bullets[i].hitFrames = 0;
                            break;
                        }
                    }
                    break;
                case 'D':
                    for (int i = 0; i < MAX_BULLETS; i++) {
                        if (!bullets[i].active) {
                            bullets[i].x = MAP_SIZE / 2;
                            bullets[i].y = MAP_SIZE / 2;
                            bullets[i].active = 1;
                            bullets[i].direction = 1;
                            bullets[i].hitFrames = 0;
                            break;
                        }
                    }
                    break;
                    break;
            }
            break;
    }

    player.x = newX;
    player.y = newY;
}

int main() {
    srand(time(NULL));
    char move;
    clock_t lastInputTime = clock();
    int loopCount = 0;
    const int loopsPerHour = 100;

    do {
        printBoard();
        updateBullets(map);
        updatePlayer();
        spawnEnemy();
        updateEnemies();
        handleInput(map);

#ifndef _WIN32
        usleep(100000);
#endif
        loopCount++;

        // Check if enough loops have passed to represent one hour
        if (loopCount % loopsPerHour == 0) {
            timeOfDay++;
            if (timeOfDay >= 24){
                timeOfDay = 0;
            }
        }

    } while (move != 'q' && move != 'Q');

    return 0;
}
