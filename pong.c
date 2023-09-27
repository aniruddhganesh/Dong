#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>

/*
 * TODO Implement Position detection, ie; detect where ball touches on paddle
 * and accordingly affect the balls direction, instead of just diagonal movement.
 */

 /* For functions REUSED or called by main() like init() */
#define private static

#define WIDTH 640
#define HEIGHT 480

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

enum GameState { SPLASHSCREEN, RUNNING, PAUSE,  PLAYER_WIN, PC_WIN } GameState = SPLASHSCREEN;
enum Player {PC, PLAYER} Player;

static int score_pc = 0;
static int score_player = 0;
        
static SDL_Rect paddles[2] = {
    {.x = 10, .w = 20, .h = 80},
    {.x = WIDTH - 30, .w = 20, .h = 80},
};

static struct ball {
    SDL_Rect rect;
    int dx;
    int dy;
} ball = {.rect.h = 15, .rect.w = 15, .dx = 0, .dy = 0};

private void end_clean(int status)
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
    exit(status);
}

void die(char *msg, const char *err)
{
    fprintf(stderr, "%s %s", msg, err);
    end_clean(EXIT_FAILURE);
}

private void init() 
{
    if(SDL_Init(SDL_INIT_VIDEO)) {
        die("Failed to init", SDL_GetError());
    }

    window = SDL_CreateWindow("Pong", 0, 0, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        die("Failed to get Window", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED  | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        die("Failed to create renderer", SDL_GetError());
    }
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);

    return;
}

void draw_splash_screen() 
{
    SDL_Surface *pong_bmp = SDL_LoadBMP("pong_splash.bmp");
    if (!pong_bmp) {
        die("Cannot draw splashscreen", SDL_GetError());
    }

    SDL_Texture *pong_bmp_tex = SDL_CreateTextureFromSurface(renderer, pong_bmp);
    SDL_Rect dest_rect = {.x = (WIDTH - 100)/2, .y = (HEIGHT - 60)/2, .w = 100, .h = 60 };

    SDL_RenderCopy(renderer, pong_bmp_tex, NULL, &dest_rect);
    return;
}

void reset_game(void)
{
    // Pause gamestate, until player unpauses but only if not from splashscreen
    if (GameState == SPLASHSCREEN) GameState = RUNNING;
    else { GameState = PAUSE; }
    // paddle position
    for (int i = 0; i < 2; i++) {
        paddles[i].y = (HEIGHT - paddles[i].h) / 2;
    }

    // Ball position
    ball.rect.x = (WIDTH - ball.rect.w)/2;
    ball.rect.y = (HEIGHT - ball.rect.h)/2;

    // Initialize Velocity With random number b/w 1 and 4
    ball.dx = (rand() % 2 == 0) ? -1 : 1;
    ball.dy = (rand() % 4 == 0) ? -1 : 1;

    ball.dx *= 2;
    ball.dy *= 2;
    return;
}


void move_player_paddle(int direction)
{
    const int PADDING = 10;
    if (paddles[PLAYER].y - PADDING < 0 && direction < 0) return;
    if (paddles[PLAYER].y > (HEIGHT - paddles[PLAYER].h - PADDING) && direction > 0) return;

    paddles[PLAYER].y += (direction * 5);
}

void move_pc_paddle(int direction)
{
    const int PADDING = 10;
    if (paddles[PC].y - PADDING < 0 && direction < 0) return;
    if (paddles[PC].y > (HEIGHT - paddles[PC].h - PADDING) && direction > 0) return;

    paddles[PC].y += (direction * 5);
}

/* for movement of paddles */
private void handle_keys_game(const uint8_t *keys)
{
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_J]) {
        move_player_paddle(1);
    } else if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_K]) {
        move_player_paddle(-1);
    } else if (keys[SDL_SCANCODE_W]) {
        move_pc_paddle(-1);
    } else if (keys[SDL_SCANCODE_S]) {
        move_pc_paddle(1);
    }
}

/* For pause, play, start, etc */
private void handle_keys_ux(const uint8_t *keys)
{
    if (keys[SDL_SCANCODE_ESCAPE]) {
        GameState = SPLASHSCREEN;
    } else if (keys[SDL_SCANCODE_SPACE]) {
        if (GameState == SPLASHSCREEN) reset_game();
    } else if (keys[SDL_SCANCODE_P]) {
        if (GameState == PAUSE) GameState = RUNNING;
        else GameState = PAUSE;
    }
}

void update_score(enum Player player)
{
    // Reset score
    switch (player) {
        case PC: {
            if (score_pc == 10) { score_pc = 0; GameState = PC_WIN; } 
            break; 
        }
        case PLAYER: 
            if (score_player == 10) {score_player = 0; GameState = PLAYER_WIN;}
            break;
    }
    reset_game();
}

// TODO: Checks if paddle or boundry and bounces ball
int check_and_bounce(void)
{
    const int x = ball.rect.x; /* Sorry no namespaces had to do  */
    const int y = ball.rect.y; /* Single letter Variables        */
    
    // Check loss/win situations
    if (x < 0) {
        update_score(PLAYER);
        return -1;
    }
    else if ((x + ball.rect.w) > WIDTH) {
        update_score(PC);
        return -1;
    } 

    // Top and bottom walls
    if (y < 0 || (y + ball.rect.h) > HEIGHT) {
        ball.dy = -ball.dy;
        return 1;
    }

    /* Paddle (Player) Divide padddle into four parts, extreme ends affect dy more */
    if (x + ball.rect.w > (paddles[PLAYER].x - 5)) {
        if (y > paddles[PLAYER].y && y < (paddles[PLAYER].y + paddles[PLAYER].h)) { // Check within paddle size
            /* Top half */
            if (y > paddles[PLAYER].y + (paddles[PLAYER].h/2)) {
                if (y > paddles[PLAYER].y + (paddles[PLAYER].h/4)) { /* Top quarter */
                    ball.dy += 3;
                } else {
                    ball.dy += 2;
                }
            } else { /* Bottom half */
                if (y > paddles[PLAYER].y + (paddles[PLAYER].h/2 + paddles[PLAYER].h/4)) { /* Bottom Quarter */
                    ball.dy -= 3;
                } else {
                    ball.dy -= 1;
                }
            }

            ball.dx = -ball.dx;
            return 1;
        }
        return 0;
    }

    // Paddle (PC)
    if (ball.rect.x < (paddles[PC].x + paddles[PC].w)) {
        if (ball.rect.y > paddles[PC].y && ball.rect.y < (paddles[PC].y + paddles[PC].h)) {
            ball.dx = -ball.dx;
            return 1;
        }
        return 0;
    }
    return 0;
}

void update_ball_position(void)
{
    check_and_bounce();

    ball.rect.x += ball.dx;
    ball.rect.y += ball.dy;

    return;
}

private void draw_game(void)
{
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    if (GameState != RUNNING)
        goto skip_updates_and_draw_on_pause;
    update_ball_position();

skip_updates_and_draw_on_pause:
    for (int i = 0; i < 2; i++) {
        SDL_RenderDrawRect(renderer, &paddles[i]);
        SDL_RenderFillRect(renderer, &paddles[i]);
    }
    
    SDL_RenderDrawLine(renderer, WIDTH/2, 0, WIDTH/2, HEIGHT);


    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
    {
        SDL_RenderDrawRect(renderer, &ball.rect);
        SDL_RenderFillRect(renderer, &ball.rect);
    }

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
}

int main(void)
{
    bool quit = false;
    SDL_Event e;

    init();
    srand(time(NULL));


    while (!quit) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: quit = true; break;
                case SDL_KEYDOWN: handle_keys_ux(SDL_GetKeyboardState(NULL)); break;
            }
        }
        

        SDL_RenderClear(renderer);
        if (GameState == SPLASHSCREEN) {
            draw_splash_screen();
        } else  {
            handle_keys_game(SDL_GetKeyboardState(NULL));
            draw_game();
        }

        SDL_RenderPresent(renderer);
    }

    end_clean(EXIT_SUCCESS);
    return 0;
}
