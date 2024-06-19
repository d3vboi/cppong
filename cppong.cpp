#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <cstring>
#include <random>

const int WIDTH = 40;
const int HEIGHT = 20;

int ballX, ballY;
int ballDirX, ballDirY;
int paddle1Y, paddle2Y;
std::atomic<bool> running(true);
int numPlayers = 1;

std::random_device rd;
std::mt19937 gen(rd());

class Player {
public:
    int score = 0;
};

Player player1;
Player player2;

void setup() {
    ballX = WIDTH / 2;
    ballY = HEIGHT / 2;

    std::uniform_int_distribution<> dirDist(0, 1);
    ballDirX = dirDist(gen) == 0 ? -1 : 1;
    ballDirY = dirDist(gen) == 0 ? -1 : 1;

    paddle1Y = HEIGHT / 2 - 2;
    paddle2Y = HEIGHT / 2 - 2;
}

void draw() {
    std::cout << "\033[2J\033[H"; // Clear console and move the cursor home

    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (x == 0 || x == WIDTH - 1) {
                std::cout << '|'; // Side walls
            } else if (y == 0 || y == HEIGHT - 1) {
                std::cout << '-'; // Top and bottom walls
            } else if (x == ballX && y == ballY) {
                std::cout << 'O'; // Ball
            } else if (x == 2 && y >= paddle1Y && y < paddle1Y + 4) {
                std::cout << '#'; // Paddle 1
            } else if (x == WIDTH - 3 && y >= paddle2Y && y < paddle2Y + 4) {
                std::cout << '#'; // Paddle 2
            } else {
                std::cout << ' '; // Empty space, like my soul
            }
        }
        std::cout << '\n';
    }

    std::cout << "Player 1: " << player1.score << "  Player 2: " << player2.score << '\n';
}

char getChar() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return buf;
}

void input() {
    while (running) {
        char c = getChar();
        switch (c) {
            case 'w': if (paddle1Y > 1) paddle1Y--; break;
            case 's': if (paddle1Y < HEIGHT - 5) paddle1Y++; break;
            case 'i': if (numPlayers == 2 && paddle2Y > 1) paddle2Y--; break;
            case 'k': if (numPlayers == 2 && paddle2Y < HEIGHT - 5) paddle2Y++; break;
            case 'q': running = false; break; // Exit the game
        }
    }
}

void ai(int level) {
    int easyMS = 20;
    int hardMS = 15;

    std::uniform_real_distribution<> randomMoveDist(0.0, 1.0);

    while (running) {
        if (ballDirX == 1 && ballX > WIDTH / 2) {
            switch (level) {
                case 1:
                    // Easy
                    if (randomMoveDist(gen) > 0.15) { // 85% Chance
                        if (ballY > paddle2Y + 2 && paddle2Y < HEIGHT - 5) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(easyMS));
                            paddle2Y++;
                        } else if (ballY < paddle2Y + 2 && paddle2Y > 1) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(easyMS));
                            paddle2Y--;
                        }
                    }
                    break;
                case 2:
                    // Hard
                    if (randomMoveDist(gen) > 0.05) { // 95% Chance
                        if (ballY > paddle2Y + 2 && paddle2Y < HEIGHT - 5) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(hardMS));
                            paddle2Y++;
                        } else if (ballY < paddle2Y + 2 && paddle2Y > 1) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(hardMS));
                            paddle2Y--;
                        }
                    }
                    break;
                case 3:
                    // Impossible
                    if (ballY > paddle2Y + 2 && paddle2Y < HEIGHT - 5) {
                        paddle2Y++;
                    } else if (ballY < paddle2Y + 2 && paddle2Y > 1) {
                        paddle2Y--;
                    }
                    break;
                default:
                    break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
void logic() {
    ballX += ballDirX;
    ballY += ballDirY;

    // Ball collision with top and bottom walls
    if (ballY <= 0 || ballY >= HEIGHT - 1) {
        ballDirY = -ballDirY;
    }

    // Ball collision with paddles
    if (ballX == 3 && ballY >= paddle1Y && ballY < paddle1Y + 4) {
        ballDirX = -ballDirX;
    }

    if (ballX == WIDTH - 4 && ballY >= paddle2Y && ballY < paddle2Y + 4) {
        ballDirX = -ballDirX;
    }

    // Ball goes out of bounds (scores)
    if (ballX <= 0) {
        player2.score++;
        setup(); // Reset ball and paddles
    } else if (ballX >= WIDTH - 1) {
        player1.score++;
        setup();
    }
}

int main(int argc, char* argv[]) {
    int aiLevel = 2;

    // Command line arguments
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--players") == 0 || strcmp(argv[i], "-p") == 0) {
                if (i + 1 < argc) {
                    numPlayers = std::stoi(argv[i + 1]);
                    if (numPlayers < 1 || numPlayers > 2) {
                        std::cerr << "Invalid number of players. Using default (1 player).\n";
                        numPlayers = 1;
                    }
                    ++i; // Next argument
                } else {
                    std::cerr << "Missing argument after --players/-p.\n";
                    return 1;
                }
            } else if (strcmp(argv[i], "--level") == 0 || strcmp(argv[i], "-l") == 0) {
                if (i + 1 < argc) {
                    aiLevel = std::stoi(argv[i + 1]);
                    if (aiLevel < 1 || aiLevel > 3) {
                        std::cerr << "Invalid AI level. Using default (2 - hard).\n";
                        aiLevel = 2;
                    }
                    ++i; // Next argument
                } else {
                    std::cerr << "Missing argument after --level/-l.\n";
                    return 1;
                }
            }
        }
    }

    setup();
    std::thread inputThread(input);
    std::thread aiThread;

    if (numPlayers == 1) {
        aiThread = std::thread(ai, aiLevel);
    }

    while (running) {
        draw();
        logic();
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Delay for game speed
    }

    inputThread.join();
    if (aiThread.joinable()) {
        aiThread.join();
    }
    return 0;
}
