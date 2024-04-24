/*
    James McCracken
    16/07/2023
    A game of ping pong for linux consoles
*/

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <sys/ioctl.h>
#include <SDL2/SDL_mixer.h>
#include <ncurses.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <ratio>
#include <string>
#include <thread>
#include <vector>

class Console
{
    public:
    Console() {}

    void hideCursor()
    {
        print("\033[?25l");
    }

    void showCursor()
    {
        print("\033[?25h");
    }

    void moveCursor(int line, int column)
    {
        print("\033[" + std::to_string(line) + ";" + std::to_string(column) + "H");
    }

    void clearScreen()
    {
        print("\033[2J");
    }

    void print(std::string&& str)
    {
        std::cout << str << std::flush;
    }
};

class Ball
{
    public:
    Ball() {}

    int line { 1 };
    int lastLine { line };
    int column { 1 };
    int lastColumn { column };
    float lineSpeed { 1 };
    float columnSpeed { 1 };
};

class Game
{
    public:
    Ball ball {};
    int minRow { 1 };
    int minCol { 1 };
    int maxRow;
    int maxCol;
    bool ping;

    Game() 
    {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        maxRow = w.ws_row;
        maxCol = w.ws_col;
    }

    void update()
    {
        ball.lastLine = ball.line;
        ball.lastColumn = ball.column;

        ball.line += ball.lineSpeed;
        ball.column += ball.columnSpeed;
        
        bool topOrBottom = ball.line == maxRow || ball.line == minRow;
        ball.lineSpeed = topOrBottom ? ball.lineSpeed*-1 : ball.lineSpeed;

        bool leftOrRight = ball.column == maxCol || ball.column == minCol;
        ball.columnSpeed = leftOrRight ? ball.columnSpeed*-1 : ball.columnSpeed;

        ping = topOrBottom || leftOrRight;
    }
};

class Render
{
    private:
    Console& console;

    public:
    Render(Console& _console) : console(_console) {}

    void draw(const Ball& ball)
    {
        console.moveCursor(ball.lastLine, ball.lastColumn);
        console.print(" ");
        console.moveCursor(ball.line, ball.column);
        console.print("O");  
    }
};

class Frame
{
    public:
    Frame() {}

    void limit()
    {
        constexpr long MS_PER_FRAME { 100 };
        std::this_thread::sleep_for(std::chrono::milliseconds(MS_PER_FRAME));
    }
};

enum class ProgramState { Continue, Stop };

class Input
{
    public:
    Input() {}

    void setup()
    {
        initscr();
        noecho();
        cbreak();
        nodelay(stdscr, TRUE);
    }

    void clean()
    {
        endwin();
    }

    ProgramState handle(Game& game)
    {
        
        int c = getch();

        if (c == 27)
            return ProgramState::Stop;            

        return ProgramState::Continue;
    }
};

class Audio
{
    private:
    std::vector<Mix_Chunk*> pings{};

    public:
    Audio()
    {
        int err = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096);
        if (err) throw err;
        pings.push_back(Mix_LoadWAV("ping1.wav"));
        pings.push_back(Mix_LoadWAV("ping2.wav"));
        pings.push_back(Mix_LoadWAV("ping3.wav"));
        std::srand(std::time(nullptr));
    }

    ~Audio()
    {
        Mix_CloseAudio();
        for (auto ping : pings)
            Mix_FreeChunk(ping);
    }

    void Play()
    {
        int pingIndex = std::rand() % pings.size();
        if (!pings[pingIndex]) return;
        Mix_PlayChannel(-1, pings[pingIndex], 0);   
    }
};

int main()
{
    Console console {};
    Game game {};
    Render render { console };
    Frame frame {};
    Input input {};
    Audio audio {};

    console.hideCursor();
    console.clearScreen();
    input.setup();

    while(1)
    {
        frame.limit();

        if (input.handle(game) == ProgramState::Stop)
            break;

        game.update();

        if (game.ping)
            audio.Play();

        render.draw(game.ball);
    }

    console.showCursor();
    input.clean();
}