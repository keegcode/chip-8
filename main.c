#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


uint8_t ram[4096];

uint8_t sprites[80] = {
  0xF0,0x90,0x90,0x90,0xF0,
  0x20,0x60,0x20,0x20,0x70,
  0xF0,0x10,0xF0,0x80,0xF0,
  0xF0,0x10,0xF0,0x10,0xF0,
  0x90,0x90,0xF0,0x10,0x10,
  0xF0,0x80,0xF0,0x10,0xF0,
  0xF0,0x80,0xF0,0x90,0xF0,
  0xF0,0x10,0x20,0x40,0x40,
  0xF0,0x90,0xF0,0x90,0xF0,
  0xF0,0x90,0xF0,0x10,0xF0,
  0xF0,0x90,0xF0,0x90,0x90,
  0xE0,0x90,0xE0,0x90,0xE0,
  0xF0,0x80,0x80,0x80,0xF0,
  0xE0,0x90,0x90,0x90,0xE0,
  0xF0,0x80,0xF0,0x80,0xF0,
  0xF0,0x80,0xF0,0x80,0x80
};

uint8_t V[16];

uint16_t stack[16];

uint16_t I = 0;

uint8_t sound = 0;
uint8_t delay = 0;

uint16_t PC = 512;
uint8_t SP = 0;

typedef struct {
  SDL_DisplayMode displayMode;
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_Texture* texture;
  uint16_t h, w;
  uint8_t pixels[];
} Display;

Display* Display_Create();
void Display_Clear(Display* display);
void Display_Render(Display* display);
void Display_Destroy(Display* display);

int main(int argc, char *argv[]) {
  Display* display = Display_Create();

  if (!display) {
    const char* error = SDL_GetError();
    fprintf(stderr, "failed to create a display: %s", error);
    Display_Destroy(display);
    return -1;
  }

  memcpy(ram, sprites, sizeof(sprites));

  FILE* file = fopen(argv[1], "r");
  uint16_t code = PC;

  int a;
  while ((a = fgetc(file)) != EOF) {
    ram[code] = a;
    code++;
  }

  fclose(file);

  bool isRunning = true;

  while (isRunning) {
    uint16_t instr = (ram[PC] << 8) | ram[PC + 1];
    PC += 2;
    
    uint16_t addr = instr & 0x0FFF;
    uint8_t kk = instr & 0x00FF;
    uint8_t n = instr & 0x000F;

    uint8_t x = (instr >> 8) & 0x000F; 
    uint8_t y = (instr >> 4) & 0x000F;

    uint8_t opcode = (instr >> 12) & 0x000F; 

    //printf("OP: %d, INSTR: %04x \n", opcode, instr);

    switch (opcode) {
      case 0x0:
        switch (instr) {
          //CLS
          case 0x00E0: 
            Display_Clear(display);
            break;
          //RET
          case 0x00EE:
            PC = stack[SP];
            SP -= 1;
            break;
          //SYS
          default:
            PC = addr;
            break;
        };
        break;
      //JP
      case 0x1:
        PC = addr;
        break;
      //CALL
      case 0x2:
        SP += 1;
        stack[SP] = PC; 
        PC = addr;
        break;
      //SE Vx, byte
      case 0x3:
        if (V[x] == kk) {
          PC += 2;
        }
        break;
      //SNE Vx, byte
      case 0x4:
        if (V[x] != kk) {
          PC += 2;
        }
        break;
      //SE Vx, Vy
      case 0x5:
        if (V[x] == V[y]) {
          PC += 2;
        }
        break;
      //LD Vx, byte
      case 0x6:
        V[x] = kk;
        break;
      //ADD Vx, byte
      case 0x7:
        V[x] += kk;
        break;
      case 0x8:
        switch (n) {
          //LD Vx,  Vy
          case 0x0:
            V[x] = V[y];
            break;
          //OR Vx, Vy
          case 0x1:
            V[x] |= V[y];
            break;
          //AND Vx, Vy
          case 0x2:
            V[x] &= V[y];
            break;
          //XOR Vx, Vy
          case 0x3:
            V[x] ^= V[y];
            break;
          //ADD Vx, Vy
          case 0x4:
            //printf("ADD V[x]: %d, V[y]: %d, V[F]: %d \n", V[x], V[y], V[0xF]);
            V[0xF] = (uint16_t) V[x] + V[y] > UINT8_MAX;
            V[x] += V[y];
            //printf("ADD V[x]: %d, V[y]: %d, V[F]: %d \n", V[x], V[y], V[0xF]);
            break;
          //SUB Vx, Vy
          case 0x5:
            //printf("SUB V[x]: %d, V[y]: %d, V[F]: %d \n", V[x], V[y], V[0xF]);
            V[x] -= V[y];
            V[0xF] = V[x] > V[y];
            //printf("SUB V[x]: %d, V[y]: %d, V[F]: %d \n", V[x], V[y], V[0xF]);
            break;
          //SHR Vx
          case 0x6:
            //printf("SHR V[x]: %d, V[F]: %d \n", V[x], V[0xF]);
            V[0xF] = V[x] & 0x01; 
            V[x] >>= 1; 
            //printf("SHR V[x]: %d, V[F]: %d \n", V[x], V[0xF]);
            break;
          //SUBN Vx, Vy
          case 0x7:
            //printf("SUBN V[x]: %d, V[y]: %d, V[F]: %d \n", V[x], V[y], V[0xF]);
            V[x] = V[y] - V[x];
            V[0xF] = V[y] > V[x];
            //printf("SUBN V[x]: %d, V[y]: %d, V[F]: %d \n", V[x], V[y], V[0xF]);
            break;
          //SHL Vx 
          case 0xE:
            //printf("SHL V[x]: %d, V[F]: %d \n", V[x], V[0xF]);
            V[0xF] = (V[x] >> 7) & 0x01; 
            V[x] <<= 1; 
            //printf("SHL V[x]: %d, V[F]: %d \n", V[x], V[0xF]);
            break;
          default:
            break;
        }
      //SNE Vx, Vy
      case 0x9:
        if (V[x] != V[y]) {
          PC += 2;
        }
        break;
      //LD I, addr
      case 0xA:
        I = addr;
        break;
      //JP V0, addr
      case 0xB:
        PC = addr + V[0];
        break;
      //RND Vx, byte
      case 0xC:
        V[x] = (uint8_t) rand() & kk;
        break;
      //DRW Vx, Vy, n
      case 0xD:
        V[0xF] = 0;
        for (uint8_t h = 0; h < n; h++) {
          uint8_t sprite = ram[(I + h)];
          uint8_t posY = (V[y] + h) % 32;
          for (uint8_t w = 0; w < 8; w++) {
            uint8_t posX = (V[x] + w) % 64;
            uint8_t bit = (sprite >> (7 - w)) & 0x01;
            uint8_t* pixel = display->pixels + (posX + posY * 64);
            uint8_t xor = bit ^ (*pixel & 0xFF);
            if (!xor) {
              V[0xF] = 1;
              *pixel = 0;
            } else {
              *pixel = 0xFF;
            } 
          }
        }
        break;
      case 0xE:
      case 0xF:
        switch (kk) {
          case 0x07:
          case 0x0A:
          case 0x15:
          case 0x18:
          case 0x1E:
            I += V[x];
            break;
          case 0x29:
          case 0x33:
            ram[I] = (V[x] / 100);
            ram[I + 1] = (V[x] / 10) % 10;
            ram[I + 2] = V[x] % 10;
            break;
          case 0x55:
            for (uint8_t i = 0; i <= x; i++) {
              ram[I + i] = V[i];
            }
            break;
          case 0x65:
            for (uint8_t i = 0; i <= x; i++) {
              V[i] = ram[I + i];
            }
            break;
        }
        break;
      default:
        break;
    }

    if (PC >= code) {
      break;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        isRunning = false;
        break;
      }
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
        isRunning = false;
        break;
      }
    }
    
    Display_Render(display);
    SDL_Delay(10);
  }
  
  Display_Destroy(display);

  return 0;
}



Display* Display_Create() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "failed to init sdl");
    return NULL;
  }

  SDL_DisplayMode displayMode;
  
  if (SDL_GetDesktopDisplayMode(0, &displayMode)) {
    fprintf(stderr, "failed to get display mode");
    return NULL;
  }
  
  Display* display = malloc(sizeof(Display) + 64 * 32);

  display->h = displayMode.h / 3;
  display->w = displayMode.w / 3;

  display->window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display->w, display->h, 0);
  
  if (!display->window) {
    fprintf(stderr, "failed to create a window");
    return NULL;
  }

  display->renderer = SDL_CreateRenderer(display->window, -1, SDL_RENDERER_PRESENTVSYNC);

  if (!display->renderer) {
    fprintf(stderr, "failed to create a renderer");
    return NULL;
  }
  
  display->texture = SDL_CreateTexture(display->renderer, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING, 64, 32);

  if (!display->texture) {
    fprintf(stderr, "failed to create a texture");
    return NULL;
  }

  return display;
}

void Display_Clear(Display* display) {
  memset(display->pixels, 0, 32 * 64);
}

void Display_Render(Display* display) {
  SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 0);
  SDL_RenderClear(display->renderer);
  SDL_UpdateTexture(display->texture, NULL, display->pixels, 64 * sizeof(uint8_t));
  SDL_RenderCopy(display->renderer, display->texture, NULL, NULL);
  SDL_RenderPresent(display->renderer);
}

void Display_Destroy(Display* display) {
  if (display->texture) {
    SDL_DestroyTexture(display->texture);
  }

  if (display->renderer) {
    SDL_DestroyRenderer(display->renderer);
  }

  if (display->window) {
    SDL_DestroyWindow(display->window);
  }

  SDL_Quit();
  free(display);
}
