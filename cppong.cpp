#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <cstring>

const int WIDTH = 40;
const int HEIGHT = 20;

int ballX, ballY;
int ballDirX, ballDirY;
int paddle1Y, paddle2Y;
std::atomic<bool> running(true);
int numPlayers = 1;

bool wPressed = false, sPressed = false, iPressed = false, kPressed = false;

class Player {
public:
    int score = 0;
};

Player player1;
Player player2;

void setup() {
    ballX = WIDTH / 2;
    ballY = HEIGHT / 2;
    ballDirX = -1;
    ballDirY = -1;
    paddle1Y = HEIGHT / 2 - 2;
    paddle2Y = HEIGHT / 2 - 2;
}

void draw() {
    std::cout << "\033[2J\033[H"; // Clear the console and move the cursor to home position

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
                std::cout << ' '; // Empty space
            }
        }
        std::cout << '\n';
    }

    std::cout << "Player 1: " << player1.score << "  Player 2: " << player2.score << '\n';
}

void setNonBlockingInput(bool enable) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (enable) {
        tty.c_lflag &= ~(ICANON | ECHO);
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 0;
    } else {
        tty.c_lflag |= (ICANON | ECHO);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void input() {
    setNonBlockingInput(true);
    while (running) {
        struct pollfd fds[1];
        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;
        int ret = poll(fds, 1, 0);
        if (ret > 0) {
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                if (c == 'w') wPressed = true;
                if (c == 's') sPressed = true;
                if (c == 'i') iPressed = true;
                if (c == 'k') kPressed = true;
                if (c == 'q') running = false;
            }
        }
        if (!ret || (ret && fds[0].revents & POLLHUP)) {
            if (wPressed && paddle1Y > 1) paddle1Y--;
            if (sPressed && paddle1Y < HEIGHT - 5) paddle1Y++;
            if (iPressed && paddle2Y > 1) paddle2Y--;
            if (kPressed && paddle2Y < HEIGHT - 5) paddle2Y++;
            wPressed = sPressed = iPressed = kPressed = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    setNonBlockingInput(false);
}

void ai() {
    while (running) {
        if (ballDirX == 1 && ballX > WIDTH / 2) {
            if (ballY > paddle2Y + 2 && paddle2Y < HEIGHT - 5) {
                paddle2Y++;
            } else if (ballY < paddle2Y + 2 && paddle2Y > 1) {
                paddle2Y--;
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
    if (argc > 1) {
        if ((strcmp(argv[1], "--players") == 0 || strcmp(argv[1], "-p") == 0) && argc > 2) {
            numPlayers = std::stoi(argv[2]);
            if (numPlayers < 1 || numPlayers > 2) {
                std::cerr << "Invalid number of players. Using default (1 player).\n";
                numPlayers = 1;
            }
        }
    }

    setup();
    std::thread inputThread(input);
    std::thread aiThread;

    if (numPlayers == 1) {
        aiThread = std::thread(ai);
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